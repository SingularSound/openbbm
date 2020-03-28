#include "soundManager.h"
#include "mixer.h"
#include "math.h"
#include "pragmapack.h"
#ifdef am335x
#include "songManager.h"
#include "bootloader.h"
#include "timer.h"
#include "songPlayer.h"
#include "sdCardUtils.h"
#include "mixer.h"
#include "ff.h"
#include <stdlib.h>
#include <string.h>
#include "fileHeader.h"
#include "hw_gpio_v2.h"
#include "hw_types.h"
#include "ff.h"
#include "hsmmcsd.h"
#include "mmcsd_proto.h"
#include "interrupt.h"
#include "drumsetFile.h"
#include "string.h"
#else
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#endif

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

#ifdef am335x
extern int dynacoParallelTask(void);
#endif
/*****************************************************************************
 **                        INTERNAL TYPEDEF
 *****************************************************************************/
#ifdef am335x


typedef enum {
    FULL,
    LOWER_HALF,
    UPPER_HALF
}MEMORY_TYPE;

typedef enum {
    UNABLE_TO_ALLOCATE,
    FREE,
    ACTIVE,
    ERROR
} MALLOC_RESULT_t;
#else 

typedef enum {
    FREE,
    ACTIVE,
}MALLOC_RESULT_t;
#endif

PACK typedef struct {
    char name[16];          // FIle name of the special effect
    MALLOC_RESULT_t status; // Status of the memory for the effect
    unsigned short bps;     // Bits per sample
    unsigned short nChannel;// Number of channel (mono/stereo)
    unsigned int nSample;   // NUmber of sample contain in the special effect
    unsigned char* addr;
} PACKED Effect_t;


#ifdef am335x

PACK typedef struct {
    Instrument_t inst[MIDIPARSER_NUMBER_OF_INSTRUMENTS];
    MALLOC_RESULT_t status[MIDIPARSER_NUMBER_OF_INSTRUMENTS];
    unsigned char ChokeChan[MIDIPARSER_NUMBER_OF_CHOKE];\
    unsigned char * wPtr;
} PACKED DrumsetStruct_t;

char CurDrumpath[PATH_SIZE];
DRM_LOADING_STATUS DrumsetLoadStatus = DRM_WAITING;
//unsigned int DrumsetLoadStatus2 = 0;

#else

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

#endif


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
#ifdef am335x
/* Initialisation flag of the file */
static unsigned int SoundManagerInitFlag = 0;

/* Drumset Variable */
static DrumsetStruct_t Drumset1;
static DrumsetStruct_t Drumset2;
static DrumsetStruct_t *OldDrumset;
static DrumsetStruct_t *CurDrumset;

static char CurrName[NAME_SIZE];
static char NextName[NAME_SIZE];
static char tmpCurrName[NAME_SIZE];
static char tmpNextName[NAME_SIZE];

char RequestPath[PATH_SIZE];
static unsigned int LoadDrumsetFlag = 0;

// While be change by the soungPlayer that in an interruption
static volatile unsigned int LoadSpecialEffectFlag = 0;

/* Drumset file structure */
#pragma DATA_ALIGN(DrumsetFil, SOC_CACHELINE_SIZE);
static FIL DrumsetFil;

/* Special effect file structure */
#pragma DATA_ALIGN(EffectFil, SOC_CACHELINE_SIZE);
static FIL EffectFil;

/* Special section for all drumset data. The linkder will allocate a 100MB block ok the ram */
#pragma DATA_ALIGN(DataPtr, SOC_CACHELINE_SIZE);
static volatile unsigned char DataPtr[MEMORY_SIZE] __attribute__ ((section (".noinit")));

static MEMORY_TYPE MemType = FULL;

/* Special Effect variables */
static Effect_t SpecialEffect[3];
static Effect_t* CurrEffectPtr = NULL;
static unsigned int CurrEffectIndex;
#else 
/* Drumset Variable */
static DrumsetStruct_t Drumset;
#    if (defined(__x86_64__) || defined(_M_X64))
static DrumsetStruct64_t Drumset64;
#    endif
#endif
/*****************************************************************************
 **                     INTERNAL FUNCTION PROTOTYPE
 *****************************************************************************/
#ifdef am335x
static int AllocateInstrument(FIL *fil, DrumsetStruct_t *drumset, unsigned int instNumber);
static void loadSpecialEffect(void);
static int LoadEffectSound(char* name, Effect_t *effect);
static int loadDrumsetTask(void);
#endif
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


#ifdef am335x
int SoundManager_init(void) {
    unsigned int *tempBufferPtr;

    unsigned int i,j;
    // Invalidate all the drumset channels
    for (i = 0; i < MIDIPARSER_NUMBER_OF_INSTRUMENTS; i++) {
        Drumset1.status[i] = FREE;
        Drumset2.status[i] = FREE;
    }

    // Reset all the choke channel
    for (i = 0; i < MIDIPARSER_NUMBER_OF_CHOKE; i++) {
        Drumset1.ChokeChan[i] = 0u;
        Drumset2.ChokeChan[i] = 0u;
    }

    // Set the current and old drumset
    OldDrumset = &Drumset2;
    CurDrumset = &Drumset1;

    // Special effect initialisation
    for (i = 0; i < 3; i++) {
        SpecialEffect[i].nSample = 0;
        SpecialEffect[i].name[0] = (char)0;
        SpecialEffect[i].status = FREE;
        tempBufferPtr = (unsigned int*) memalign(MEMORY_ALIGMENT_SIZE,
                SPECIAL_EFECT_SIZE);
        if (tempBufferPtr == NULL ) {
            return -1;
        }
        SpecialEffect[i].addr = (unsigned char*) tempBufferPtr;
    }

    strcpy(CurrName, "");
    strcpy(NextName, "");
    strcpy(RequestPath, "");

    float top_db;
    float req_db;
    float dif_db;

    fillGainTable();


    // set the soundManager as initialised
    SoundManagerInitFlag = 1;

    // Return zero for successful initialisation
    return 0;
}

void SoundManager_task(void) {

    unsigned char status;

    // Special Effect loading manager
    if (LoadSpecialEffectFlag == 1) {

        status = IntDisable();
        // Copy the temp name of the special effect into the current special effect
        strcpy(CurrName, tmpCurrName);
        strcpy(NextName, tmpNextName);
        LoadSpecialEffectFlag = 0;
        IntEnable(status);

        loadSpecialEffect();
    }

    if (LoadDrumsetFlag) {
        loadDrumsetTask();
    }
}

void SoundManager_RequestSpecialEffectLoading(char* currName, char* nextName) {

    unsigned char status = IntDisable();

    // Copy the request name into temp arrays in a Protected wav
    strncpy(tmpCurrName, currName, NAME_SIZE - 1);
    tmpCurrName[NAME_SIZE - 1] = '\0';
    strncpy(tmpNextName, nextName, NAME_SIZE - 1);
    tmpNextName[NAME_SIZE - 1] = '\0';
    LoadSpecialEffectFlag = 1;
    IntEnable(status);
}

void loadSpecialEffect(void) {

    unsigned int nextIndex;
    unsigned int tmpCurrIndex;

    tmpCurrIndex = (CurrEffectIndex + 1) % 3;
    nextIndex = (CurrEffectIndex + 2) % 3;

    // If the new special effect is null
    if (0 == strcmp(CurrName, "")) {
        CurrEffectPtr = NULL;
    } else {

        if (0 == strcmp(CurrName, SpecialEffect[tmpCurrIndex].name)) {

            if (SpecialEffect[tmpCurrIndex].status == ACTIVE) {
                CurrEffectPtr = &SpecialEffect[tmpCurrIndex];
            } else {
                if (ACTIVE == LoadEffectSound(CurrName, &SpecialEffect[tmpCurrIndex])) {
                    CurrEffectPtr = &SpecialEffect[tmpCurrIndex];
                } else {
                    CurrEffectPtr = NULL;
                }
            }
        } else {
            // if the name of the effect loaded is not the same a the new current one
            if (ACTIVE == LoadEffectSound(CurrName, &SpecialEffect[tmpCurrIndex])) {
                CurrEffectPtr = &SpecialEffect[tmpCurrIndex];
            } else {
                CurrEffectPtr = NULL;
            }
        }
    }

    // Loading of the next special effect
    // if the next special effect name is null
    if (0 == strcmp(NextName, "")) {
        // dont load anything
    } else {
        LoadEffectSound(NextName, &SpecialEffect[nextIndex]);
    }
    CurrEffectIndex = tmpCurrIndex;
}

int SoundManager_RequestloadDrumset(char *path, unsigned int force_reload) {
    int different = strcmp(path,CurDrumpath);

    if ((different != 0) || (force_reload == 1) || (DrumsetLoadStatus ==  DRM_ERROR) || ( DrumsetLoadStatus == DRM_WAITING)){
        strcpy(RequestPath, path);
        LoadDrumsetFlag = 1;
    }

    return 0;
}



void SoundManager_freeDrumset(void) {

    unsigned char status = IntDisable();
    strcpy(CurDrumpath, "");
    strcpy(RequestPath, "");
    LoadDrumsetFlag = 0;
    gStartTime = Timer_GlobalMsTime_getTime();
    DrumsetLoadStatus = DRM_WAITING;
    IntEnable(status);

    gTotalDrumsetSize = 0;
    gCompletedSize = 0;
    gShortTotalDrumsetSize = 0;
}


static int loadDrumsetTask(void) {

    unsigned char presentNotes[MIDIPARSER_NUMBER_OF_INSTRUMENTS] = {0};
    unsigned char nextNotes[MIDIPARSER_NUMBER_OF_INSTRUMENTS];
    unsigned char status;
    int count;
    unsigned int index;
    unsigned int totalSize;
    unsigned int i;

    // Copy the resquest flag from the GUI to the local variable
    status = IntDisable();
    strcpy(CurDrumpath, RequestPath);
    LoadDrumsetFlag = 0;
    gStartTime = Timer_GlobalMsTime_getTime();
    DrumsetLoadStatus = DRM_LOADING;
    IntEnable(status);



    DRUMSETFILE_HeaderStruct header;
    UINT b;
    DrumsetStruct_t *tmpDrs;

    totalSize = FileSize(CurDrumpath);

    if (totalSize == 0){
        DrumsetLoadStatus = DRM_ERROR; // 1 == error
        return -1;
    }

    // Open the file
    if (f_open(&DrumsetFil, CurDrumpath, FA_READ) != FR_OK) {
        f_close(&DrumsetFil);
        DrumsetLoadStatus = DRM_ERROR; // 1 == error
        return -1;
    }

    // read header
    if (FR_OK != f_read(&DrumsetFil, &header, sizeof(header), &b)) {
        f_close(&DrumsetFil);
        DrumsetLoadStatus = DRM_ERROR; // 1 == error
        return -1;
    }

    if (b != sizeof(header)) {
        f_close(&DrumsetFil);
        DrumsetLoadStatus = DRM_ERROR; // 1 == error
        return -1;
    }

    // Verify version of the file
    if (header.version > 1) {
        DrumsetLoadStatus = DRM_ERROR; // 1 == error
        return -1;
    }

    // Read the Drumset Structure (into the current drumset)
    if (FR_OK != f_read(&DrumsetFil, OldDrumset->inst, MIDIPARSER_NUMBER_OF_INSTRUMENTS * sizeof(Instrument_t), &b)) {
        f_close(&DrumsetFil);
        DrumsetLoadStatus = DRM_ERROR; // 1 == error
        return -1;
    }

    gTotalDrumsetSize = 0;
    gCompletedSize = 0;
    gShortTotalDrumsetSize = 0;


    // Swap the drumsets
    tmpDrs = OldDrumset;
    OldDrumset = CurDrumset;
    CurDrumset = tmpDrs;

    // The "new" drumset must be free before beginning
    for (i = 0; i < MIDIPARSER_NUMBER_OF_INSTRUMENTS; i++) {

        CurDrumset->status[i] = FREE;
        mixer_removeSoundWithAddress(CurDrumset->inst[i].vel[0].addr,CurDrumset->inst[i].dataSize);

        // Add each velocity size
        gTotalDrumsetSize += CurDrumset->inst[i].dataSize;
    }


    if (gTotalDrumsetSize > BIG_FILE_THRESHOLD){
        for (i = 0 ; i < MIDIPARSER_NUMBER_OF_INSTRUMENTS; i++){
            OldDrumset->status[i] = FREE;
            mixer_removeSoundWithNote(i);
        }
        MemType = FULL;
        CurDrumset->wPtr = (unsigned char *)DataPtr + LOWER_HALF_OFFSET;
    } else {

        // If the previous drumset was a big drumset
        if (MemType == FULL){
            for (i = 0 ; i < MIDIPARSER_NUMBER_OF_INSTRUMENTS; i++){
                OldDrumset->status[i] = FREE;
                mixer_removeSoundWithNote(i);
            }
            CurDrumset->wPtr = (unsigned char *)DataPtr + LOWER_HALF_OFFSET;
        }

        // If current drumset is on lower Half
        if (MemType == LOWER_HALF){
            CurDrumset->wPtr = (unsigned char *)DataPtr + UPPER_HALF_OFFSET;
            MemType = UPPER_HALF;
        } else {
            CurDrumset->wPtr = (unsigned char *)DataPtr + LOWER_HALF_OFFSET;
            MemType  = LOWER_HALF;
        }
    }


    //Get the next note values
    count = SongPlayer_getNextNoteValue(nextNotes, presentNotes);
    index = count;
    for (i = 0; i < MIDIPARSER_NUMBER_OF_INSTRUMENTS; i++){
        if (presentNotes[i]){
            gShortTotalDrumsetSize += CurDrumset->inst[i].dataSize;
        } else {
            nextNotes[index++] = i;

        }
    }

    for (i = 0; i < MIDIPARSER_NUMBER_OF_INSTRUMENTS; i++) {

        unsigned int index = nextNotes[i];

        if (CurDrumset->status[index]== FREE){

            if (CurDrumset->inst[index].nVel != 0) {

                switch (AllocateInstrument(&DrumsetFil, CurDrumset, index)) {

                case ACTIVE:
                    break;
                case UNABLE_TO_ALLOCATE:
                case FREE:
                case ERROR:
                    // break the loop and ask for a new drumset loading
                    LoadDrumsetFlag = 1;
                    break;
                default:
                    break;
                }
            }
        }

        OldDrumset->status[index] = FREE;
        // If a new drumset is requested before the drumset is completly loaded
        if (LoadDrumsetFlag){
            f_close(&DrumsetFil);
            // Remove any sounds that could be skipped in the previous loop
            for (i = 0; i < MIDIPARSER_NUMBER_OF_INSTRUMENTS; i++) {
                OldDrumset->status[i] = FREE;
            }
            DrumsetLoadStatus = DRM_WAITING;
            return RETURN_FAILURE;
        }
        // Main parallel task
        if (!dynacoParallelTask()){
            f_close(&DrumsetFil);

            // Remove any sounds that could be skipped in the previous loop
            for (i = 0; i < MIDIPARSER_NUMBER_OF_INSTRUMENTS; i++) {
                OldDrumset->status[i] = FREE;
            }
            DrumsetLoadStatus = DRM_WAITING;
            return RETURN_FAILURE;
        }
    }

    f_close(&DrumsetFil);

    // Remove any sounds that could be skipped in the previous loop
    for (i = 0; i < MIDIPARSER_NUMBER_OF_INSTRUMENTS; i++) {
        OldDrumset->status[i] = FREE;
    }

    DrumsetLoadStatus = DRM_LOADED;

    gStopTime = Timer_GlobalMsTime_getTime();
    return RETURN_SUCCESS;
}


/**
 * @brief   This function tries to allocate a new memory space in the ram. It discards oldest sounds first
 * @param   fil : file structure pointer
 *          inst: Intrument to save to wav
 * @retval  0 == succes, 1 == error
 */
static int AllocateInstrument(FIL *fil, DrumsetStruct_t *drumset,
        unsigned int instNumber) {

    unsigned int size = drumset->inst[instNumber].dataSize;
    Instrument_t *inst;
    UINT b;
    unsigned int byte2Read;
    unsigned int byteRead = 0;
    unsigned int i;
    unsigned int initialOffset;



    inst = &drumset->inst[instNumber];

    /* Jump to the right place in the file */
    if (FR_OK != f_lseek(fil, (DWORD) inst->vel[0].addr)) {
        return ERROR;
    }

    /* Read all the wavs of the instrument */
    while (byteRead < size) {

        byte2Read = size - byteRead;
        if (byte2Read > F_READ_CHUNK_SIZE) {
            byte2Read = F_READ_CHUNK_SIZE;
        }

        if (FR_OK != f_read(fil, drumset->wPtr + byteRead, (WORD) byte2Read, &b)) {
            return ERROR;
        }
        byteRead += b;
        gCompletedSize += b;

        if (!b){
            return ERROR;
        }
        if (!dynacoParallelTask()){
            break;
        }
    }

    /* Adjust offsets of the instrument */
    initialOffset = (unsigned int) inst->vel[0].addr;

    for (i = 0; i < inst->nVel; i++) {
        inst->vel[i].addr = ((unsigned int) drumset->wPtr + (unsigned int) (inst->vel[i].addr) - initialOffset);
    }

    // Adjust the volume if == 0 <-- to support old drumset... TODO should be removed in the future
    if (inst->volume == 0) {
        inst->volume = DEFAULT_VOLUME;
    } else if (inst->volume > DEFAULT_VOLUME) {
        inst->volume = DEFAULT_VOLUME;
    }

    drumset->status[instNumber] = ACTIVE;
    drumset->wPtr +=byteRead;

    // Align the offset on the next SOC_CACHELINE
    if ((unsigned int)drumset->wPtr % SOC_CACHELINE_SIZE){
        drumset->wPtr += SOC_CACHELINE_SIZE - ((unsigned int)drumset->wPtr % SOC_CACHELINE_SIZE);
    }

    return ACTIVE;

}

static int LoadEffectSound(char* name, Effect_t *effect) {
    WavHeader_t *wavHeader;
    Chunk_t *chunkPtr;

    char path[24];
    unsigned int byteRead;
    unsigned int fileSize;
    unsigned int dataOffset;
    UINT b;

    // Remove completely effect from the RAM and the mixer
    if (effect->status == ACTIVE) {
        effect->status = FREE;
        mixer_removeSoundWithAddress((unsigned int)effect->addr, 1);
    }

    // Create the path to the wav file
    strcpy(path, "0:EFFECTS/");
    strcat(path, name);

    // Check for the file size
    fileSize = FileSize(path);
    if (fileSize > SPECIAL_EFECT_SIZE) {
        return FREE;
    }

    if (fileSize == 0) {
        return FREE;
    }

    // Open the file
    if (FR_OK != f_open(&EffectFil, path, FA_READ)) {
        f_close(&EffectFil);
        return FREE;
    }

    // Read all the file
    byteRead = 0;
    do {
        if (FR_OK!= f_read(&EffectFil, effect->addr + byteRead, F_READ_CHUNK_SIZE, &b)) {
            f_close(&EffectFil);
            return FREE;
        }
        byteRead += b;
        dynacoParallelTask();
    } while (b != 0);
    f_close(&EffectFil);

    // Verify the size of the read
    if (fileSize != byteRead)
        return -1;

    if (fileSize >= sizeof(WavHeader_t)) {
        wavHeader = (WavHeader_t *) effect->addr;
    } else {
        return FREE;
    }

    // Header format verification
    if (wavHeader->audio_format != 1)
        return FREE;
    if (wavHeader->bits_per_sample != 16) {
        if (wavHeader->bits_per_sample != 24) {
            return FREE;
        }
    }
    if (wavHeader->sample_rate != SAMPLING_RATE)
        return FREE;

    // Read the chunk descriptor until we found the data block
    chunkPtr = (Chunk_t *) ((unsigned int) (effect->addr) + 20
            + wavHeader->subchunk1.size);

    // Scan until id == DATA
    while (strncmp(chunkPtr->id, "data", 4)) {
        chunkPtr =
                (Chunk_t *) ((((unsigned int) chunkPtr) + chunkPtr->size) + 8);
    }
    dataOffset = ((unsigned int) chunkPtr) + 8 - (unsigned int) effect->addr;

    // If there is wav data to read
    if (chunkPtr->size > 0) {
        strcpy(effect->name, name);
        effect->addr = (unsigned char *) (effect->addr + dataOffset);

        // Check number of channel (support only Mono / Stereo)
        if (wavHeader->num_channels == 0 || wavHeader->num_channels > 2) return FREE;
        effect->nChannel = wavHeader->num_channels;


        // Bit rate check
        if (wavHeader->bits_per_sample == 16) {
            effect->nSample = chunkPtr->size / 2;
            effect->bps = wavHeader->bits_per_sample;
        } else if (wavHeader->bits_per_sample == 24) {
            effect->nSample = chunkPtr->size / 3;
            effect->bps = wavHeader->bits_per_sample;
        } else {
            return FREE;
        }
        effect->status = ACTIVE;
        return ACTIVE;
    } else {
        return FREE;
    }
}
#else

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



    // Complete for all the intrumsnets
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


#endif




#ifdef am335x
void SoundManager_playSpecialEffect(unsigned char vel) {
    unsigned char status = IntDisable();

    unsigned int volume = vel * gLinearGainFactor;

    if (CurrEffectPtr != NULL ) {
        switch (CurrEffectPtr->bps) {
        case 16:
            if (CurrEffectPtr->nChannel == 2){
                mixer_addPCM16Stereo((unsigned int) CurrEffectPtr->addr,
                        CurrEffectPtr->nSample,
                        volume,
                        0,
                        0,
                        MIDIPARSER_NUMBER_OF_INSTRUMENTS,
                        0,
                        0,
                        0);
            } else {
                mixer_addPCM16Mono((unsigned int) CurrEffectPtr->addr,
                        CurrEffectPtr->nSample,
                        volume,
                        0,
                        0,
                        MIDIPARSER_NUMBER_OF_INSTRUMENTS,
                        0,
                        0,
                        0);
            }
            break;

        case 24:
            if (CurrEffectPtr->nChannel == 2){
                mixer_addPCM24Stereo((unsigned int) CurrEffectPtr->addr,
                        CurrEffectPtr->nSample,
                        volume,
                        0,
                        0,
                        MIDIPARSER_NUMBER_OF_INSTRUMENTS,
                        0,
                        0,
                        0);
            } else {
                mixer_addPCM24Mono((unsigned int) CurrEffectPtr->addr, CurrEffectPtr->nSample,
                        volume,
                        0,
                        0,
                        MIDIPARSER_NUMBER_OF_INSTRUMENTS,
                        0,
                        0,
                        0);
            }
            break;
        }
    }
    IntEnable(status);
}
#else
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
#endif

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
        float delay_seconde, float ratio, unsigned int partID) {

    unsigned int fillChokeGroup;
    unsigned int fillChokeDelay_nsample;
    unsigned int volume;
    unsigned int groupVelocity = 0;
    int high_index = 0;
    int low_index = 0;
    int delta;
    DrumsetStruct_t *drum = NULL;
    unsigned int nDelay = ((unsigned int) (delay_seconde * SAMPLING_RATE));

    // If there is no sound for the note
#ifndef am335x
#if (defined(__x86_64__) || defined(_M_X64))
    DrumsetStruct64_t *drum64 = &Drumset64;
#endif
    if (Drumset.status[note] == FREE) return;
    drum = &Drumset;
#else
    if (!SoundManagerInitFlag)
        return;
    // Check for the most recent drumset and the oldest drum set for a valid sound for the note
    if (CurDrumset->status[note] != FREE) {

        drum = CurDrumset;

    } else if (OldDrumset->status[note] != FREE) {
        drum = OldDrumset;
    } else {
        // No sound to play
        return;
    }



#endif

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
            if(mixer_shouldNoteBeExcluded(fillChokeGroup,partID)) return;

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
