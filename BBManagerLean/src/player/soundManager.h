#include <stdint.h>
#include <stdio.h>

#ifndef SOUNDMANAGER_H
#define SOUNDMANAGER_H

#include "pragmapack.h"

#ifdef __cplusplus
extern "C" {
#endif



#define MIDIPARSER_NUMBER_OF_INSTRUMENTS       (128)
#define MIDIPARSER_MAX_NUMBER_VELOCITY         (16)
#define MIDIPARSER_NUMBER_OF_CHOKE             (16)



// value is mapped to a file
PACK typedef struct {
    unsigned short bps;      // Bits per sample
    unsigned short nChannel; // Number of channel (mono/stereo)
    unsigned int fs;         // Sampling frequency
    unsigned int vel;        // Lower bound velocity
    unsigned int nSample;      // Number of sample in the wav
    unsigned int data0;      // Reserved
    unsigned int data1;      // Reserved
#if !(defined(__x86_64__) || defined(_M_X64))
    unsigned int addr;       // Starting address of the wav in the file/ram
#else
    unsigned int offset;     // Offset from start of file
#endif
} PACKED Vel_t;


#if (defined(__x86_64__) || defined(_M_X64))
typedef struct {
    // Need to keep address in a separate structure since it takes 8 bytes
    uint64_t addr;       // Starting address of the wav in ram
} Vel64_t;
#endif

PACK typedef struct {
    unsigned short chokeGroup;    // 0 = Disable 1 @ 15 = choke group
    unsigned short poly;
    unsigned int nVel;
    unsigned int dataSize;
    unsigned char volume;
    unsigned char fillChokeGroup;
    unsigned char fillChokeDelay;
    unsigned char nonPercussion;
    unsigned int data1;         // Reserved
    Vel_t vel[MIDIPARSER_MAX_NUMBER_VELOCITY];
} PACKED Instrument_t;

#if (defined(__x86_64__) || defined(_M_X64))
// Need to keep address in a separate structure since it takes 8 bytes
typedef struct {
    Vel64_t vel[MIDIPARSER_MAX_NUMBER_VELOCITY];
} Instrument64_t;
#endif


extern void SoundManager_init(void);
extern void SoundManager_LoadDrumset(char* file, uint32_t size);
extern void SoundManager_playDrumsetNote(unsigned char note, unsigned char velocity, float delay_seconde,float ratio, unsigned int isExclusive, int pickUp);
extern void SoundManager_playSpecialEffect(unsigned char vel, uint32_t part);
extern void SoundManager_LoadEffect(char* file, uint32_t part);
extern char* SongPlayer_getSoundEffectName(uint32_t part);


#ifdef __cplusplus
}
#endif

#endif // SOUNDMANAGER_H

