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
#include "soundManager.h"
#include "mixer.h"
#include "math.h"
#include "pragmapack.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>


#ifdef __cplusplus
extern "C" {
#endif



/*****************************************************************************
 **                         INTERNAL MACROS
 *****************************************************************************/

#define DEFAULT_VOLUME              (100u)

#define DRUMSET_ALLOCATED_MEMORY    (100 * 1024 * 1024 + 32)    // 100 Mo
#define DRUMSET_BIG_SIZE_THRESHOLD  (80  * 1024 * 1024)         // 80 M0
#define MAX_DRUMSET_ALLOC_INSTR     (256)

#define SPECIAL_EFECT_SIZE          (2* 3u * 15u * SAMPLING_RATE) // (7 MO / 3)
#define NUMBER_OF_SPECIAL_EFFECT    (3u)


#define ALLOC_TABLE_LENGTH          (2 * MIDIPARSER_NUMBER_OF_INSTRUMENTS)
#define INITIAL_OFFSET              (0u)

#define RETURN_SUCCESS              (0)
#define RETURN_FAILURE              (-1)

#define F_READ_CHUNK_SIZE           (32768)
#define PATH_SIZE                   (32)
#define NAME_SIZE                   (16)
#define MEMORY_ALIGMENT_SIZE        (512)

#define SAMPLING_RATE				(44100)
#define NEXT_NOTE_SEEK_NUMBER       (128)



#define MEMORY_SIZE                 (101 * 1024 * 1024)
#define BIG_FILE_THRESHOLD          (50 * 1024 * 1024)
#define LOWER_HALF_OFFSET           (0)
#define UPPER_HALF_OFFSET           (MEMORY_SIZE / 2)

/*****************************************************************************
 **                        INTERNAL CONSTANCE
 *****************************************************************************/
static const unsigned int Divider[3] = { 4, 8, 16 };
static const unsigned long gLinearGainFactor = 10000;
static unsigned int gGain[128][128];

unsigned long long gStartTime;
unsigned long long gStopTime;
int gTotalDrumsetSize = 0;
int gShortTotalDrumsetSize = 0;
int gCompletedSize = 0;

/*****************************************************************************
 **                        INTERNAL TYPEDEF
 *****************************************************************************/
typedef enum {
    FREE,
    ACTIVE,
}MALLOC_RESULT_t;

PACK typedef struct {
    char name[16];          // FIle name of the special effect
    MALLOC_RESULT_t status; // Status of the memory for the effect
    unsigned short bps;     // Bits per sample
    unsigned short nChannel;// Number of channel (mono/stereo)
    unsigned int nSample;   // NUmber of sample contain in the special effect
    unsigned char* addr;
} PACKED Effect_t;

typedef struct {
    Instrument_t *inst;
    MALLOC_RESULT_t status[MIDIPARSER_NUMBER_OF_INSTRUMENTS];
    unsigned char ChokeChan[MIDIPARSER_NUMBER_OF_CHOKE];
} DrumsetStruct_t;


#if (defined(__x86_64__) || defined(_M_X64))
// Need to keep address in a separate structure since it takes 8 bytes
typedef struct {
    Instrument64_t inst[MIDIPARSER_NUMBER_OF_INSTRUMENTS];
} DrumsetStruct64_t;
#endif

PACK typedef struct HeaderStruct {
    char     fileType[4];
    uint8_t  version;
    uint8_t  revision;
    uint16_t build;
    uint32_t fileCRC;
} PACKED DRUMSETFILE_HeaderStruct;
Effect_t EffectTable[32];


PACK typedef struct chunk {
    char id[4];
    int size;
} PACKED Chunk_t;

PACK typedef struct header {
    Chunk_t chunk;
    char format[4];
    Chunk_t subchunk1;
    short int audio_format;
    short int num_channels;
    int sample_rate;
    int byte_rate;
    short int block_align;
    short int bits_per_sample;
    Chunk_t subchunk2;
} PACKED WavHeader_t;

/*****************************************************************************
 **                     INTERNAL GLOBAL VARIABLE
 *****************************************************************************/
/* Drumset Variable */
static DrumsetStruct_t Drumset;
#    if (defined(__x86_64__) || defined(_M_X64))
static DrumsetStruct64_t Drumset64;
#    endif

/*****************************************************************************
 **                     INTERNAL FUNCTION PROTOTYPE
 *****************************************************************************/

/*****************************************************************************
 **                         FUNCTION DEFINITION
 *****************************************************************************/


static void fillGainTable(){
    float top_db;
    float req_db;
    float dif_db;
    int i,j;
    // i = top velocity of the group
    // j = requested veolicity IN the group ( <= )
    for (i = 1; i < 128 ; i++){
        top_db = -20.0 * log10f(127.0/(float)i);
        for (j = 1; j <= i; j++){

            // Calulate the requested gain and the difference
            req_db =  -20.0f * log10f(127.0/(float)j);
            dif_db = req_db - top_db;

            // Convert dB into fractionnal gain
            gGain[i][j] = (unsigned int)(10000.0f * powf(10.0f,dif_db / 20.0f));
        }
    }
}

void SoundManager_init(void){
    unsigned int i;

    // Invalidate all the drumset channels
    for (i = 0; i < MIDIPARSER_NUMBER_OF_INSTRUMENTS; i++){
        Drumset.status[i] = FREE;
    }
    // Reset all the choke channel
    for (i = 0; i < MIDIPARSER_NUMBER_OF_CHOKE; i++){
        Drumset.ChokeChan[i] = 0u;
    }

    for (i = 0; i < 32 ; i++){
        EffectTable[i].status = FREE;
    }

    fillGainTable();
}

void SoundManager_LoadEffect(char* file, uint32_t part){
    WavHeader_t *wavHeader;
#if !(defined(__x86_64__) || defined(_M_X64))
    uint32_t dataOffset;
#else
    uint64_t dataOffset;
#endif
    Chunk_t *chunkPtr;

    if (file == NULL) {
        EffectTable[part].status = FREE;
        return;
    }


    wavHeader = (WavHeader_t *) file;


    // Header format verification
    if (wavHeader->audio_format != 1) return;
    if (wavHeader->bits_per_sample != 16){
        if (wavHeader->bits_per_sample != 24){
            return;
        }
    }
    if (wavHeader->sample_rate != 44100) return;

    // Read the chunk descriptor until we found the data block
#if !(defined(__x86_64__) || defined(_M_X64))
    chunkPtr = (Chunk_t *) ((unsigned int)(file) + 20 + wavHeader->subchunk1.size);
#else
    chunkPtr = (Chunk_t *) ((uint64_t)(file) + 20 + wavHeader->subchunk1.size);
#endif
    while(strncmp(chunkPtr->id,"data",4)){
#if !(defined(__x86_64__) || defined(_M_X64))
        chunkPtr = (Chunk_t *) ((((unsigned int) chunkPtr) + chunkPtr->size) + 8);
#else
        chunkPtr = (Chunk_t *) ((((uint64_t) chunkPtr) + chunkPtr->size) + 8);
#endif
    }
#if !(defined(__x86_64__) || defined(_M_X64))
    dataOffset = ((unsigned int) chunkPtr) + 8;
#else
    dataOffset = ((uint64_t) chunkPtr) + 8;
#endif

    // If there is wav data to read
    if (chunkPtr->size > 0){
        EffectTable[part].addr = (unsigned char *) dataOffset;

        // Bit rate check
        if (wavHeader->bits_per_sample == 16){
            EffectTable[part].nSample = chunkPtr->size / 2;
            EffectTable[part].bps = wavHeader->bits_per_sample;
        } else if (wavHeader->bits_per_sample == 24){
            EffectTable[part].nSample = chunkPtr->size / 3;
            EffectTable[part].bps = wavHeader->bits_per_sample;
        } else {
            return;
        }

        if (wavHeader->num_channels == 1) {
            EffectTable[part].nChannel = 1;
        } else if (wavHeader->num_channels == 2) {
            EffectTable[part].nChannel = 2;
        } else {
            return;
        }

        EffectTable[part].status = ACTIVE;
    }
}


void SoundManager_LoadDrumset(char* file, uint32_t size)
{
    unsigned int i, j;

    (void)size; // remove warning

    // TODO make sure old sound stop playing
    for (i = 0; i < MIDIPARSER_NUMBER_OF_INSTRUMENTS; i++){
        Drumset.status[i] = FREE;
    }

    // Set the address of the instruments array
    Drumset.inst = (Instrument_t*)(file + sizeof(DRUMSETFILE_HeaderStruct));



    // Complete for all the instruments
    for (i = 0; i < MIDIPARSER_NUMBER_OF_INSTRUMENTS; i++){
        if (Drumset.inst[i].nVel){
            if (Drumset.inst[i].volume == 0)Drumset.inst[i].volume = 100;
            if (Drumset.inst[i].volume > 100) Drumset.inst[i].volume = 100;

            for (j=0; j < Drumset.inst[i].nVel; j++){
#if !(defined(__x86_64__) || defined(_M_X64))
                Drumset.inst[i].vel[j].addr += (unsigned int)file;
#else
                // Need to keep address in a separate structure since it takes 8 bytes
                Drumset64.inst[i].vel[j].addr = (uint64_t)file + Drumset.inst[i].vel[j].offset;
#endif
            }
            Drumset.status[i] = ACTIVE;
        }
    }
}


void SoundManager_playSpecialEffect(unsigned char vel, uint32_t part){
    unsigned int volume = vel * gLinearGainFactor;
    if (part >= 32) return;

    if (EffectTable[part].status == ACTIVE){
        switch (EffectTable[part].bps){
        case 16:
            if (EffectTable[part].nChannel == 2){
#if !(defined(__x86_64__) || defined(_M_X64))
                mixer_addPCM16Stereo((unsigned int)EffectTable[part].addr,
#else
                mixer_addPCM16Stereo((uint64_t)EffectTable[part].addr,
#endif
                        EffectTable[part].nSample,
                        volume,
                        0,
                        0,
                        128,
                        0,
                        0,
                        0);
            } else if (EffectTable[part].nChannel == 1){
#if !(defined(__x86_64__) || defined(_M_X64))
                mixer_addPCM16Mono((unsigned int)EffectTable[part].addr,
#else
                mixer_addPCM16Mono((uint64_t)EffectTable[part].addr,
#endif

                        EffectTable[part].nSample,
                        volume,
                        0,
                        0,
                        128,
                        0,
                        0,
                        0);
            }
            break;

        case 24:
            if (EffectTable[part].nChannel == 2){
#if !(defined(__x86_64__) || defined(_M_X64))
                mixer_addPCM24Stereo((unsigned int)EffectTable[part].addr,
#else
                mixer_addPCM24Stereo((uint64_t)EffectTable[part].addr,
#endif
                        EffectTable[part].nSample,
                        volume,
                        0,
                        0,
                        128,
                        0,
                        0,
                        0);
            } else if (EffectTable[part].nChannel == 1){
#if !(defined(__x86_64__) || defined(_M_X64))
                mixer_addPCM24Mono((unsigned int)EffectTable[part].addr,
#else
                mixer_addPCM24Mono((uint64_t)EffectTable[part].addr,
#endif
                        EffectTable[part].nSample,
                        volume,
                        0,
                        0,
                        128,
                        0,
                        0,
                        0);
            }
            break;
        default :
            break;
        }
    }
}


/**
 *  \brief
 *
 *  \param Midi note of the instrument
 *  \param Velocity of the note to be played
 *  \param delay in second before the note should be played
 *  \param ratio to convert ticks into time for the fill choke group
 *  \param fill choke group flag that determine is the 2 parts of one on the other
 *
 *
 *
 */
void SoundManager_playDrumsetNote(unsigned char note, unsigned char velocity,
        float delay_seconde, float ratio, unsigned int partID, int pickUp) {

    unsigned int fillChokeGroup;
    unsigned int fillChokeDelay_nsample;
    unsigned int volume;
    unsigned int groupVelocity = 0;
    int high_index = 0;
    int low_index = 0;
    int delta;
    DrumsetStruct_t *drum = NULL;
    unsigned int nDelay = (pickUp == 0)?((unsigned int) (delay_seconde * SAMPLING_RATE)):0;

    // If there is no sound for the note

#if (defined(__x86_64__) || defined(_M_X64))
    DrumsetStruct64_t *drum64 = &Drumset64;
#endif
    if (Drumset.status[note] == FREE) return;
    drum = &Drumset;

    if (velocity < 1 && drum->inst[note].nonPercussion>0) {
        // Choke note when velocity is zero, and non percussion
        mixer_chokeNote(note);
    } else {
        // Start the iteration with the end of the velocity array
        high_index = drum->inst[note].nVel - 1;


        // Loop until the velocity is lower or equal to the requested velocity
        while (drum->inst[note].vel[high_index].vel > velocity) {
            high_index--;
        }

        if (high_index < drum->inst[note].nVel - 1){
            volume = gGain[drum->inst[note].vel[high_index + 1].vel - 1][velocity] * drum->inst[note].volume;
        } else {
            volume = gGain[127][velocity] * drum->inst[note].volume;
        }

        // Save the index and the top velocity of the group
        low_index = high_index;
        groupVelocity = drum->inst[note].vel[high_index].vel;

        // Look up every that have the same velocity of the group
        while (groupVelocity == drum->inst[note].vel[low_index].vel) {
            low_index--;
            if (low_index < 0)
                break;
        }

        // Number of possible velocity
        delta = high_index - low_index;

        // Get a random index to add some variations in the sounds of the player
        high_index = 1 + low_index + (rand() % delta);

        // If a choke group is activated for the instrument
        if (drum->inst[note].chokeGroup) {

            // If the last note in the choke channel is different of the current note
            if (drum->ChokeChan[drum->inst[note].chokeGroup] != note) {
                // Choke the channel
                mixer_chokeChannel(drum->inst[note].chokeGroup, nDelay);
                // Save the new note in the choke channel array
                drum->ChokeChan[drum->inst[note].chokeGroup] = note;
            }
        }

        // Get the fill choke group from the current note to be played
        fillChokeGroup = drum->inst[note].fillChokeGroup;

        if (fillChokeGroup != 0){
            // Fill choke note excluder
            if(mixer_shouldNoteBeExcluded(fillChokeGroup,partID) && delay_seconde != 0.0) return;

            // calculate the delay in sample
            if (drum->inst[note].fillChokeDelay > 2) drum->inst[note].fillChokeDelay = 2;
            fillChokeDelay_nsample = (SAMPLING_RATE * (480 / (Divider[drum->inst[note].fillChokeDelay]))) * ratio;
        } else {
            fillChokeDelay_nsample = 0;
        }


        // Play the sound (support Mono/Stero 16 & 24 bits)
        switch (drum->inst[note].vel[high_index].bps) {
        case 16:
            // if channel is stereo
            if (drum->inst[note].vel[high_index].nChannel == 2) {
#if !(defined(__x86_64__) || defined(_M_X64))
                mixer_addPCM16Stereo(drum->inst[note].vel[high_index].addr,
#else
                mixer_addPCM16Stereo(drum64->inst[note].vel[high_index].addr,
#endif
                        drum->inst[note].vel[high_index].nSample,
                        volume,
                        nDelay,
                        drum->inst[note].chokeGroup,
                        note,
                        fillChokeGroup,
                        fillChokeDelay_nsample,
                        partID);
            } else {
#if !(defined(__x86_64__) || defined(_M_X64))
                mixer_addPCM16Mono(drum->inst[note].vel[high_index].addr,
#else
                mixer_addPCM16Mono(drum64->inst[note].vel[high_index].addr,
#endif
                        drum->inst[note].vel[high_index].nSample,
                        volume,
                        nDelay,
                        drum->inst[note].chokeGroup,
                        note,
                        fillChokeGroup,
                        fillChokeDelay_nsample,
                        partID);
            }
            break;

        case 24:
            // if channel is stereo
            if (drum->inst[note].vel[high_index].nChannel == 2) {
#if !(defined(__x86_64__) || defined(_M_X64))
                mixer_addPCM24Stereo(drum->inst[note].vel[high_index].addr,
#else
                mixer_addPCM24Stereo(drum64->inst[note].vel[high_index].addr,
#endif
                        drum->inst[note].vel[high_index].nSample,
                        volume,
                        nDelay,
                        drum->inst[note].chokeGroup,
                        note,
                        fillChokeGroup,
                        fillChokeDelay_nsample,
                        partID);
            } else {
#if !(defined(__x86_64__) || defined(_M_X64))
                mixer_addPCM24Mono(drum->inst[note].vel[high_index].addr,
#else
                mixer_addPCM24Mono(drum64->inst[note].vel[high_index].addr,
#endif
                        drum->inst[note].vel[high_index].nSample,
                        volume,
                        nDelay,
                        drum->inst[note].chokeGroup,
                        note,
                        fillChokeGroup,
                        fillChokeDelay_nsample,
                        partID);
            }

            break;
        default:
            break;
        }

        // Polyphony manager
        mixer_polyphonyRemove(note, drum->inst[note].poly, nDelay);
    }

}


#ifdef __cplusplus
}
#endif
