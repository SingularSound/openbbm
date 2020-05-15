/*
  	This software and the content provided for use with it is Copyright © 2014-2020 Singular Sound 
 	BeatBuddy Manager is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2 as published by
    the Free Software Foundation.
    
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
    
    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/
#include "mixer.h"
#include "settings.h"
#include "pragmapack.h"
#include <string.h>
#include <math.h>
#include <time.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>



#ifdef __cplusplus
extern "C" {
#endif


/******************************************************************************
 **                         INTERNAL MACROS
 ******************************************************************************/

#define RELEASE_TIME_MAX            250
//#define RELEASE_TIME_MS             30
#define RELEASE_GAIN_LENGTH         ((RELEASE_TIME_MAX * 44100) / 1000)
//#define RELEASE_GAIN_LENGTH         (2)

#define MASTER_DIVIDER               (100000LL * 100LL * 100LL)





#define MIXER_MAX_CHANNEL_ARRAY      (64u) // Number max of stereo channel

// MIXER_BUFFER_LENGTH_SAMPLES DEFINED in .h so that it can be accessed by player.cpp
#    define MIXER_BUFFER_LENGTH          (70560u)//(MIXER_BUFFER_LENGTH_SAMPLES)
#define BUFFER_LENGTH               (MIXER_BUFFER_LENGTH)


#define MIXER_TIME_SAMPLE_US_RATIO  (1.0f/(1000000.0f/44100.0f))

#define MIN_GAIN                    (-50.0f)
#define MAX_GAIN                    (4.0f)


#define FIXED_POINT_OFF             (1000)
#define MAX_VALUE                   (8388607)
#define MIN_VALUE                   (-8388607)
#ifndef true
#define true 1
#endif

#ifndef false
#define false 0
#endif

#ifndef NULL
#define NULL ((void*) 0)
#endif

#define GET_SAMPLE_VALUE(c)         ((int64_t)(c->velocity)) * ((int64_t)((((*(int32_t*) & c->add[c->byteIndex]) & (c->dataMask)) << c->leftShift) >> c->rightShift))
#define NEXT_RIGHT_SAMPLE(c)        (c->byteIndex += c->offsetStereo)
#define NEXT_LEFT_SAMPLE(c)         (c->byteIndex += c->offsetNext)

/******************************************************************************
                          INTERNAL TYPEDEF
 ******************************************************************************/

typedef struct {
    unsigned char * add;         // Starting address of the audio samples
    unsigned int offsetSample;   // Number of byte to skip for a frame ( L + R )
    unsigned int offsetStereo;   // Number of offset bytes to apply betwwen left and right channel
    unsigned int offsetNext;     // Numver of offser bytes to apply afeter the right channel
    unsigned int leftShift;      // Number of byte to move the sound so that the MSB reache the MSB of an integer
    unsigned int rightShift;     // RIght shift to replace the sample as a 24 bits
    int dataMask;                // Mask to apply to the array of sound to retreive a unique sample left or right

    signed int nByte;            // Length of the array of the sound
    signed int byteIndex;        // Current Play index of the sample

    signed int velocity;              // Gain given to the track
    unsigned int timeiD;              // Unique ID given to the track (choke option)
    unsigned int chokeGroup;          // choke group of the sample
    unsigned int noteID;              // Note id of the sound
    unsigned int fillChokeGroup;      // Ending Fills choke group
    int fillChokeDelay;               // Fill choke delay
    unsigned int fillChokePartId;     // Group associated to the fill (part of the song)
    unsigned int release_position;
    unsigned int release_delay;
} MIXER_channel_t;



/******************************************************************************
 **                     INTERNAL GLOBAL VARIABLE 
 ********************************************** *******************************/

static MIXER_channel_t Channel[MIXER_MAX_CHANNEL_ARRAY];

static unsigned int UniqueId = 0;

static signed long long R_Buffer[MIXER_BUFFER_LENGTH];
static signed long long L_Buffer[MIXER_BUFFER_LENGTH];
static volatile unsigned int numEmptyValues;

static unsigned long counter = 180;

static int ReleaseCoeff[RELEASE_GAIN_LENGTH];
static int ReleaseLength = RELEASE_GAIN_LENGTH;

static float g_level;

/******************************************************************************
 **                     INTERNAL FUNCTION PROTOTYPE
 ******************************************************************************/

char IntDisable(void){ return 0; }
void IntEnable(char status){(void) status;}

static void calculateReleaseTimeCoeff(int *array, int length);
static void settingsCallback(SETTINGS_main_key_enum key, int value);
static inline void quickRelease(MIXER_channel_t *channel);

/******************************************************************************
 **              FUNCTION DEFINITIONS
 ******************************************************************************/

int gLeftFreq = 0;
int gRightFreq = 0;

void mixer_setLeftFreq(unsigned int freq){
    gLeftFreq = freq;
}

void mixer_setRightFreq(unsigned int freq){
    gRightFreq = freq;
}

/*
 * \brief Init the mixer object by freeing all the channels
 */
void mixer_init(void)
{
    unsigned int i;
    //Free all channel (active state is defined by byteIndex < nByte)
    for( i = 0; i < MIXER_MAX_CHANNEL_ARRAY; i++ ) {
        Channel[i].nByte = 0u;
        Channel[i].timeiD = 0u;
        Channel[i].byteIndex = 0u;
    }




    // Initialise the mixer circular buffer
    numEmptyValues = MIXER_BUFFER_LENGTH;

    calculateReleaseTimeCoeff(ReleaseCoeff,100);
}


static void calculateReleaseTimeCoeff(int *array, int length){
    int i;
    for (i = 0 ; i < length; i++){
        array[i] = (1000.0f  * (length - i)) / length;
    }
}

/**
 * \brief  This function tries to add activate a ADD/Activate a channel in the mixer,
 *          remove the oldest if no empty channel was found \n
 *
 * \param  startAddress   Start address of the channel.\n
 * \param  length         Length of the audio sample .\n
 * \param  vol            Volume of the track : 0 @ 200;
 *
 * \ return the unique ID of the sample
 **/
#if !(defined(__x86_64__) || defined(_M_X64))
unsigned int mixer_addPCM16Stereo(unsigned int startAddress,
#else
unsigned int mixer_addPCM16Stereo(uint64_t  startAddress,
#endif
        unsigned int nSample,
        unsigned int vol,
        unsigned int nDelay,
        unsigned int chokeGroup,
        unsigned int noteID,
        unsigned int fillChokeGroup,
        unsigned int fillChokeDelay,
        unsigned int fillChokeId)
{
    unsigned char status = IntDisable();
    unsigned int i;
    unsigned int oldestIndex = 0;

    //Find the first empty channel and initialize it
    //Search also for the oldest sample (e.g. smallest unique ID)
    for( i = 0; i < MIXER_MAX_CHANNEL_ARRAY; i++ ) {


        if( Channel[i].byteIndex >= Channel[i].nByte ) {

            Channel[i].add = (unsigned char*) startAddress;
            Channel[i].nByte = nSample * 2;
            Channel[i].byteIndex = 0 - (4 * nDelay);
            Channel[i].velocity = vol;
            Channel[i].timeiD = ++UniqueId;
            Channel[i].chokeGroup =  chokeGroup;
            Channel[i].offsetSample = 4;
            Channel[i].offsetStereo = 2;
            Channel[i].offsetNext = 2;
            Channel[i].leftShift = 16;
            Channel[i].rightShift = 8;
            Channel[i].dataMask = 0x0000FFFF;
            Channel[i].noteID = noteID;
            Channel[i].fillChokeGroup = fillChokeGroup;
            Channel[i].fillChokeDelay = fillChokeDelay * 4; // Convert value delay in sample to number of byte
            Channel[i].fillChokePartId = fillChokeId;
            Channel[i].release_position = 0;
            Channel[i].release_delay = 0;
            IntEnable(status);
            return UniqueId;
        } else {

            /* Look for the smaller */
            if ( Channel[i].timeiD < Channel[oldestIndex].timeiD ){
                oldestIndex = i;
            }
        }
    }

    /* Replace the oldest with the new if no channel were found */
    quickRelease(&Channel[oldestIndex]);
    Channel[oldestIndex].add = (unsigned char*)startAddress;
    Channel[oldestIndex].nByte = nSample * 2;
    Channel[oldestIndex].byteIndex = 0 - (4 * nDelay);
    Channel[oldestIndex].velocity = vol;
    Channel[oldestIndex].timeiD = ++UniqueId;
    Channel[oldestIndex].chokeGroup = chokeGroup;
    Channel[oldestIndex].offsetSample = 4;
    Channel[oldestIndex].offsetStereo = 2;
    Channel[oldestIndex].offsetNext = 2;
    Channel[oldestIndex].leftShift = 16;
    Channel[oldestIndex].rightShift = 8;
    Channel[oldestIndex].dataMask = 0x0000FFFF;
    Channel[oldestIndex].noteID = noteID;
    Channel[oldestIndex].fillChokeGroup = fillChokeGroup;
    Channel[oldestIndex].fillChokeDelay = fillChokeDelay * 4;
    Channel[oldestIndex].fillChokePartId = fillChokeId;
    Channel[oldestIndex].release_position = 0;
    Channel[oldestIndex].release_delay = 0;

    IntEnable(status);
    return UniqueId;
}




/**
 * \brief  This function tries to add activate a ADD/Activate a channel in the mixer,
 *          remove the oldest if no empty channel was found \n
 *
 * \param  startAddress   Start address of the channel.\n
 * \param  length         Length of the audio sample .\n
 * \param  vol            Volume of the track : 0 @ 200;
 *
 * \ return the unique ID of the sample
 **/
#if !(defined(__x86_64__) || defined(_M_X64))
unsigned int mixer_addPCM16Mono(unsigned int startAddress,
#else
unsigned int mixer_addPCM16Mono(uint64_t  startAddress,
#endif
        unsigned int nSample,
        unsigned int vol,
        unsigned int nDelay,
        unsigned int chokeGroup,
        unsigned int noteID,
        unsigned int fillChokeGroup,
        unsigned int fillChokeDelay,
        unsigned int fillChokeId)
{
    unsigned int i;
    unsigned int oldestIndex;
    unsigned char status = IntDisable();
    // Get the first sound to initialise the minimum seeker */
    oldestIndex =  0;

    //Find the first empty channel and initialize it
    //Search also for the oldest sample (e.g. smallest unique ID)
    for( i = 0; i < MIXER_MAX_CHANNEL_ARRAY; i++ ) {


        if( Channel[i].byteIndex >= Channel[i].nByte ) {

            Channel[i].add = (unsigned char*) startAddress;
            Channel[i].nByte = nSample * 2;
            Channel[i].byteIndex = 0 - (2 * nDelay);
            Channel[i].velocity = vol;
            Channel[i].timeiD = ++UniqueId;
            Channel[i].chokeGroup =  chokeGroup;
            Channel[i].offsetSample = 2;
            Channel[i].offsetStereo = 0;
            Channel[i].offsetNext = 2;
            Channel[i].leftShift = 16;
            Channel[i].rightShift = 8;
            Channel[i].dataMask = 0x0000FFFF;
            Channel[i].noteID = noteID;
            Channel[i].fillChokeGroup = fillChokeGroup;
            Channel[i].fillChokeDelay = fillChokeDelay * 2;
            Channel[i].fillChokePartId = fillChokeId;
            Channel[i].release_position = 0;
            Channel[i].release_delay = 0;
            IntEnable(status);
            return UniqueId;
        } else {

            /* Look for the smaller */
            if ( Channel[i].timeiD < Channel[oldestIndex].timeiD ){
                oldestIndex = i;
            }
        }
    }

    /* Replace the oldest with the new if no channel were found */
    quickRelease(&Channel[oldestIndex]);
    Channel[oldestIndex].add = (unsigned char*)startAddress;
    Channel[oldestIndex].nByte = nSample * 2;
    Channel[oldestIndex].byteIndex = 0 - (2 * nDelay);
    Channel[oldestIndex].velocity = vol;
    Channel[oldestIndex].timeiD = ++UniqueId;
    Channel[oldestIndex].chokeGroup = chokeGroup;
    Channel[oldestIndex].offsetSample = 2;
    Channel[oldestIndex].offsetStereo = 0;
    Channel[oldestIndex].offsetNext = 2;
    Channel[oldestIndex].leftShift = 16;
    Channel[oldestIndex].rightShift = 8;
    Channel[oldestIndex].dataMask = 0x0000FFFF;
    Channel[oldestIndex].noteID = noteID;
    Channel[oldestIndex].fillChokeGroup = fillChokeGroup;
    Channel[oldestIndex].fillChokeDelay = fillChokeDelay * 2;
    Channel[oldestIndex].fillChokePartId = fillChokeId;
    Channel[oldestIndex].release_position = 0;
    Channel[oldestIndex].release_delay = 0;

    IntEnable(status);
    return UniqueId;
}


/**
 * \brief  This function tries to add activate a ADD/Activate a channel in the mixer,
 *          remove the oldest if no empty channel was found \n
 *
 * \param  startAddress   Start address of the channel .\n
 * \param  nSample         Number of sample (sum of 24bit sample) .\n
 * \param  vol            Volume of the track : 0 @ 200;
 *
 * \ return the unique ID of the sample
 **/
 
#if !(defined(__x86_64__) || defined(_M_X64))
unsigned int mixer_addPCM24Stereo(unsigned int  startAddress,
#else
unsigned int mixer_addPCM24Stereo(uint64_t  startAddress,
#endif
        unsigned int nSample,
        unsigned int vol,
        unsigned int nDelay,
        unsigned int chokeGroup,
        unsigned int noteID,
        unsigned int fillChokeGroup,
        unsigned int fillChokeDelay,
        unsigned int fillChokeId){


    unsigned int i;
    unsigned int oldestIndex;
    unsigned char status = IntDisable();


    // Get the first sound to initialise the minimum seeker */
    oldestIndex =  0;

    //Find the first empty channel and initialize it
    //Search also for the oldest sample (e.g. smallest unique ID)
    for( i = 0; i < MIXER_MAX_CHANNEL_ARRAY; i++ ) {


        if( Channel[i].byteIndex >= Channel[i].nByte ) {

            Channel[i].add = (unsigned char*) startAddress;
            Channel[i].nByte = nSample * 3;
            Channel[i].byteIndex = 0 - (6 * nDelay);
            Channel[i].velocity = vol;
            Channel[i].timeiD = ++UniqueId;
            Channel[i].chokeGroup =  chokeGroup;
            Channel[i].offsetSample = 6;
            Channel[i].offsetStereo = 3;
            Channel[i].offsetNext = 3;
            Channel[i].leftShift = 8;
            Channel[i].rightShift = 8;
            Channel[i].dataMask = 0x00FFFFFF;
            Channel[i].noteID = noteID;
            Channel[i].fillChokeGroup = fillChokeGroup;
            Channel[i].fillChokeDelay = fillChokeDelay * 6;
            Channel[i].fillChokePartId = fillChokeId;
            Channel[i].release_position = 0;
            Channel[i].release_delay = 0;
            IntEnable(status);
            return UniqueId;
        } else {

            /* Look for the smaller */
            if ( Channel[i].timeiD < Channel[oldestIndex].timeiD ){
                oldestIndex = i;
            }
        }
    }
    /* Replace the oldest with the new if no channel were found */
    quickRelease(&Channel[oldestIndex]);
    Channel[oldestIndex].add = (unsigned char*) startAddress;
    Channel[oldestIndex].nByte = nSample * 3;
    Channel[oldestIndex].byteIndex = 0 - (6 * nDelay);
    Channel[oldestIndex].velocity = vol;
    Channel[oldestIndex].timeiD = ++UniqueId;
    Channel[oldestIndex].chokeGroup = chokeGroup;
    Channel[oldestIndex].offsetSample = 6;
    Channel[oldestIndex].offsetStereo = 3;
    Channel[oldestIndex].offsetNext = 3;
    Channel[oldestIndex].leftShift = 8;
    Channel[oldestIndex].rightShift = 8;
    Channel[oldestIndex].dataMask = 0x00FFFFFF;
    Channel[oldestIndex].noteID = noteID;
    Channel[oldestIndex].fillChokeGroup = fillChokeGroup;
    Channel[oldestIndex].fillChokeDelay = fillChokeDelay * 6;
    Channel[oldestIndex].fillChokePartId = fillChokeId;
    Channel[oldestIndex].release_position = 0;
    Channel[oldestIndex].release_delay = 0;

    IntEnable(status);
    return UniqueId;
}



/**
 * \brief  This function tries to add activate a ADD/Activate a channel in the mixer,
 *          remove the oldest if no empty channel was found \n
 *
 * \param  startAddress   Start address of the channel .\n
 * \param  nSample         Number of sample (sum of 24bit sample) .\n
 * \param  vol            Volume of the track : 0 @ 200;
 *
 * \return the unique ID of the sample
 **/
#if !(defined(__x86_64__) || defined(_M_X64))
unsigned int mixer_addPCM24Mono(unsigned int startAddress,
#else
unsigned int mixer_addPCM24Mono(uint64_t  startAddress,
#endif
        unsigned int nSample,
        unsigned int vol,
        unsigned int nDelay,
        unsigned int chokeGroup,
        unsigned int noteID,
        unsigned int fillChokeGroup,
        unsigned int fillChokeDelay,
        unsigned int fillChokeId)
{

    unsigned int i;
    unsigned int oldestIndex;
    unsigned char status = IntDisable();


    // Get the first sound to initialise the minimum seeker */
    oldestIndex =  0;

    //Find the first empty channel and initialize it
    //Search also for the oldest sample (e.g. smallest unique ID)
    for( i = 0; i < MIXER_MAX_CHANNEL_ARRAY; i++ ) {


        if( Channel[i].byteIndex >= Channel[i].nByte ) {

            Channel[i].add = (unsigned char*) startAddress;
            Channel[i].nByte = nSample * 3;
            Channel[i].byteIndex = 0 - (3 * nDelay);
            Channel[i].velocity = vol;
            Channel[i].timeiD = ++UniqueId;
            Channel[i].chokeGroup =  chokeGroup;
            Channel[i].offsetSample = 3;
            Channel[i].offsetStereo = 0;
            Channel[i].offsetNext = 3;
            Channel[i].leftShift = 8;
            Channel[i].rightShift = 8;
            Channel[i].dataMask = 0x00FFFFFF;
            Channel[i].noteID = noteID;
            Channel[i].fillChokeGroup = fillChokeGroup;
            Channel[i].fillChokeDelay = fillChokeDelay * 3;
            Channel[i].fillChokePartId = fillChokeId;
            Channel[i].release_position = 0;
            Channel[i].release_delay = 0;
            IntEnable(status);
            return UniqueId;
        } else {

            /* Look for the smaller */
            if ( Channel[i].timeiD < Channel[oldestIndex].timeiD ){
                oldestIndex = i;
            }
        }
    }
    /* Replace the oldest with the new if no channel were found */
    quickRelease(&Channel[oldestIndex]);
    Channel[oldestIndex].add = (unsigned char*)startAddress;
    Channel[oldestIndex].nByte = nSample * 3;
    Channel[oldestIndex].byteIndex = 0 - (3 * nDelay);
    Channel[oldestIndex].velocity = vol;
    Channel[oldestIndex].timeiD = ++UniqueId;
    Channel[oldestIndex].chokeGroup = chokeGroup;
    Channel[oldestIndex].offsetSample = 3;
    Channel[oldestIndex].offsetStereo = 0;
    Channel[oldestIndex].offsetNext = 3;
    Channel[oldestIndex].leftShift = 8;
    Channel[oldestIndex].rightShift = 8;
    Channel[oldestIndex].dataMask = 0x00FFFFFF;
    Channel[oldestIndex].noteID = noteID;
    Channel[oldestIndex].fillChokeGroup = fillChokeGroup;
    Channel[oldestIndex].fillChokeDelay = fillChokeDelay * 3;
    Channel[oldestIndex].fillChokePartId = fillChokeId;
    Channel[oldestIndex].release_position = 0;
    Channel[oldestIndex].release_delay = 0;

    IntEnable(status);
    return UniqueId;
}


/**
 * \brief  This function removes every sound of the same choke group in the mixer \n
 *
 * \param  chokeGroup   Choke group number.\n
 * \param  nDelay       Delay in sample before the sound should be choked .\n
 *
 * \ return the unique ID of the sample
 **/
void mixer_chokeChannel(unsigned int chokeGroup,unsigned int nDelay)
{
    unsigned int i, tmp;
    unsigned char status = IntDisable();

    for (i=0; i<MIXER_MAX_CHANNEL_ARRAY; i++){

        // Adjust the delay to stop the sound at the right time
        if (Channel[i].chokeGroup == chokeGroup){

            tmp = Channel[i].byteIndex + (nDelay * (Channel[i].offsetSample));

            if (tmp <= Channel[i].nByte && Channel[i].release_position == 0){
                //Channel[i].release_ = tmp;
                Channel[i].release_delay = nDelay;
                Channel[i].release_position = 1;
            }
        }
    }
    IntEnable(status);
}

void mixer_chokeNote(uint32_t note) {
    uint32_t i;
    uint8_t status = IntDisable();

    for (i = 0; i < MIXER_MAX_CHANNEL_ARRAY; i++) {
        if (Channel[i].noteID == note) {

            if (Channel[i].byteIndex <= Channel[i].nByte && Channel[i].release_position == 0){
                Channel[i].release_delay = 0;
                Channel[i].release_position = 1;
                // TODO add nDelay just like mixer_chokeChannel
            }
        }
    }

    IntEnable(status);
}


// Polyphony Manager
void mixer_polyphonyRemove(unsigned int note, unsigned int nLimit, unsigned nDelay){
    unsigned int i;
    unsigned int oldestId = 0xFFFFFFFFu;
    unsigned int oldestIndex = 0;
    unsigned int activeCnt = 0;
    unsigned int tmp;


    unsigned char status = IntDisable();

    // A polyphoniy of zero if unlimited power!
    if (nLimit){
        // For all the channel in the mixer
        for (i=0; i<MIXER_MAX_CHANNEL_ARRAY; i++){

            // Scan for any active channel with the corresponding note
            if (Channel[i].noteID == note){
                if (Channel[i].byteIndex < Channel[i].nByte){
                    activeCnt++;
                    if (Channel[i].timeiD < oldestId){
                        oldestId = Channel[i].timeiD;
                        oldestIndex = i;
                    }
                }

            }
        }

        if (activeCnt > nLimit){

            tmp = Channel[oldestIndex].byteIndex + (nDelay * (Channel[oldestIndex].offsetSample));

            if (tmp <= Channel[oldestIndex].nByte && Channel[i].release_position == 0){

                Channel[oldestIndex].release_delay = nDelay;
                Channel[oldestIndex].release_position = 1;
            }
        }
    }


    IntEnable(status);
}


unsigned int mixer_shouldNoteBeExcluded(unsigned int fillChokeGroup, unsigned int fillChokeId){

    unsigned int i;
    unsigned char status = IntDisable();
    // Scan the mixer channel array
    for (i = 0 ; i < MIXER_MAX_CHANNEL_ARRAY; i++){

        // if channel is active
        if (Channel[i].byteIndex < Channel[i].nByte){

            // if same fill choke group note
            if (Channel[i].fillChokeGroup == fillChokeGroup){

                // if not the same choke id
                if (Channel[i].fillChokePartId != fillChokeId){

                    // if the fill choke delay is not finished
                    if (Channel[i].byteIndex < Channel[i].fillChokeDelay){
                        IntEnable(status);
                        return 1;
                    }
                }
            }
        }
    }
    IntEnable(status);
    return 0;
}



/**
 * \brief  This function removes every sound oin the mixer\n
 * \ return void
 **/
void mixer_removeAll(void)
{
    unsigned char i;
    unsigned char status = IntDisable();

    for( i = 0; i < MIXER_MAX_CHANNEL_ARRAY; i++ ) {
        Channel[i].nByte = 0;
    }

    /* Restart the unique id */
    UniqueId = 0;
    IntEnable(status);
}



void mixer_removeSoundWithNote(unsigned int note){
    int i;
    unsigned char status = IntDisable();

    for(i = 0 ; i < MIXER_MAX_CHANNEL_ARRAY; i++){
        // Of note of the instrument is the requested note, remove the instrument
        if (Channel[i].noteID == note){
            quickRelease(&Channel[i]);
            Channel[i].nByte = 0;
        }
    }

    IntEnable(status);
}

/**
 *  \brief This function will remove from the player any channel taht have the same starting address
 */
#if !(defined(__x86_64__) || defined(_M_X64))
void mixer_removeSoundWithAddress(unsigned int addr, unsigned int range){
    unsigned int numOfRemovedChan = 0;
    unsigned int lowerAddress = (unsigned int) addr;
    unsigned int upperAddress = ((unsigned int) addr) + range;
    unsigned int i;

    unsigned char status = IntDisable();

    // For all the channel of the mixer
    for (i = 0; i < MIXER_MAX_CHANNEL_ARRAY; i++){
        if (((unsigned int)Channel[i].add >= lowerAddress) && ((unsigned int)Channel[i].add < upperAddress)) {
            quickRelease(&Channel[i]);
            // This action wil remove the sound from the player.
            Channel[i].nByte = 0;
            numOfRemovedChan++;

        }
    }
    IntEnable(status);
}
#else
void mixer_removeSoundWithAddress(uint64_t addr, unsigned int range){
    unsigned int numOfRemovedChan = 0;
    uint64_t lowerAddress = (uint64_t) addr;
    uint64_t upperAddress = ((uint64_t) addr) + range;
    unsigned int i;

    unsigned char status = IntDisable();

    // For all the channel of the mixer
    for (i = 0; i < MIXER_MAX_CHANNEL_ARRAY; i++){
        if (((uint64_t)Channel[i].add >= lowerAddress) && ((uint64_t)Channel[i].add < upperAddress)) {
            quickRelease(&Channel[i]);
            // This action wil remove the sound from the player.
            Channel[i].nByte = 0;
            numOfRemovedChan++;
        }
    }
    IntEnable(status);
}
#endif

/**
 * @brief mixer_setOutputLevel
 *       Set the output level of the data stream. The multiplication is made at the end of the audio processing
 *       after the hard clipping, si it's important to always use values lower than 1
 * @param level Level from 0 to 1
 */
void mixer_setOutputLevel(float level){
    g_level = level;
}

/**
 * @brief mixer_getOutputLevel
 * @return The output level of the mixer (range from 0 - 1)
 */
float mixer_getOutputLevel(void){
    return g_level;
}



#define PI  (3.1415926535897932384626433832795)


static unsigned int tt = 0;
/* Function handler of the EMPTY DMA
 */
void mixer_ReadOutputStream(signed short * buff, unsigned int length)
{

    // NOTE: length is in absolute sample count (regardless of stereo/mono)

    unsigned int k;
    unsigned int i;
    MIXER_channel_t *chanPtr;
    long tmp;

    memset(R_Buffer,0,sizeof(R_Buffer));
    memset(L_Buffer,0,sizeof(L_Buffer));

    unsigned char status = IntDisable();

    /* For production only  ( the inversion is normal) */
    if (gLeftFreq | gRightFreq){
        for ( i = 0; i < MIXER_BUFFER_LENGTH; i++ ) {
            if (gLeftFreq){
                R_Buffer[i] =   (signed long long) (800000000.0 * cos( 2.0 * PI *  gLeftFreq *(double)tt/44100.0));
            } else {
                R_Buffer[i] = 0;
            }
            if (gRightFreq){
                L_Buffer[i] =   (signed long long) (800000000.0 * cos( 2.0 * PI * gRightFreq *(double)tt/44100.0));
            } else {
                L_Buffer[i] = 0;
            }
            tt = (tt + 1) % 44100;
        }
    } else {
        /* For each channel present in the mixer */
        for ( i = 0; i < MIXER_MAX_CHANNEL_ARRAY; i++ ) {
            chanPtr = &Channel[i];
            k = 0;

            while(k<(length/2)){
                if (chanPtr->byteIndex < chanPtr->nByte){
                    if (chanPtr->byteIndex >= 0){

                        /* The calculation id based on 4 step:
                         * #1 - Read 4 byte ( they can be unaligned since the increment between sample is not  always 4)
                         * #2 - Type cast those byte into an integer
                         * #3 - Mask the integer to get the important values
                         * #4 - Bit shift left and bit shift right for sign extension. Typecast the result into a 64 bits
                         * #5 - Multiply the result with the gain
						 * #6 - Multiply with the release gain value
                         */
						 
                        L_Buffer[k]  +=  (int64_t)ReleaseCoeff[chanPtr->release_position] * (int64_t)GET_SAMPLE_VALUE(chanPtr);
                        NEXT_RIGHT_SAMPLE(chanPtr);

                        R_Buffer[k]  +=  (int64_t)ReleaseCoeff[chanPtr->release_position] * (int64_t)GET_SAMPLE_VALUE(chanPtr);
                        NEXT_LEFT_SAMPLE(chanPtr);

                        // Decrement release delay if it is set
                        if (chanPtr->release_position) {
                            if (chanPtr->release_delay){
                                chanPtr->release_delay--;
                            } else {
                                chanPtr->release_position++;
                                if (chanPtr->release_position >= ReleaseLength){
                                    chanPtr->nByte = 0;
                                }
                            }
                        }

                    } else {
                        chanPtr->byteIndex += chanPtr->offsetSample;
                    }
                    k++;
                } else {
                    break;
                }
            }
        }
    }

    k = 0;
    i = 0;

    while(k < length){
        // LEFT HARD CLIP
        tmp = (L_Buffer[i]/MASTER_DIVIDER);

        tmp = tmp > MAX_VALUE ? MAX_VALUE : tmp;
        tmp = tmp < MIN_VALUE ? MIN_VALUE : tmp;
        buff[k++] = ((int)(g_level * (float)tmp)) >> 8;

        // RIGHT HARD CLIP
        tmp = (R_Buffer[i++]/MASTER_DIVIDER);

        tmp = tmp > MAX_VALUE ? MAX_VALUE : tmp;
        tmp = tmp < MIN_VALUE ? MIN_VALUE : tmp;
        buff[k++] = ((int)(g_level * (float)tmp)) >> 8;
    }
    IntEnable(status);

}

static inline void quickRelease(MIXER_channel_t *chanPtr) {

    int k = 0;

    while(k<(BUFFER_LENGTH/2)){
        if (chanPtr->byteIndex < chanPtr->nByte){
            if (chanPtr->byteIndex >= 0){

                /* The calculation id based on 4 step:
                 * #1 - Read 4 byte ( they can be unaligned since the increment between sample is not  always 4)
                 * #2 - Type cast those byte into an integer
                 * #3 - Mask the integer to get the important values
                 * #4 - Bit shift left and bit shift right for sign extension. Typecast the result into a 64 bits
                 * #5 - Multiply the result with the gain
                 * #6 - Multiply with the release gain value
                 */
                L_Buffer[k]  +=  ((int64_t)((BUFFER_LENGTH/2) - k)) * (int64_t)ReleaseCoeff[chanPtr->release_position] * (int64_t) GET_SAMPLE_VALUE(chanPtr) / (int64_t)(BUFFER_LENGTH/2);
                NEXT_RIGHT_SAMPLE(chanPtr);

                R_Buffer[k]  +=  ((int64_t)((BUFFER_LENGTH/2) - k)) * (int64_t)ReleaseCoeff[chanPtr->release_position] * (int64_t) GET_SAMPLE_VALUE(chanPtr) / (int64_t)(BUFFER_LENGTH/2);
                NEXT_LEFT_SAMPLE(chanPtr);


                // Decrement release delay if it is set
                if (!chanPtr->release_delay){
                    chanPtr->release_delay--;
                } else {

                    // when the release delay equal 0, if the release position exist, advance in the release array
                    if (chanPtr->release_position){
                        chanPtr->release_position++;
                        if (chanPtr->release_position >= ReleaseLength){
                            chanPtr->nByte = 0;
                        }
                    }
                }
            } else {
                chanPtr->byteIndex += chanPtr->offsetSample;
            }
            k++;
        } else {
            break;
        }
    }
}


#ifdef __cplusplus
}
#endif
