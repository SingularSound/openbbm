/**
*	Song.h
*	Songs and song part structures
*/
#ifndef _SONG_H_
#define _SONG_H_


#include <stdint.h>

#include "midiParser.h"
#include "pragmapack.h"

//#ifdef __cplusplus
//extern "C" {
//#endif


#define MIN_BPM  (40)
#define MAX_BPM  (300)
#define MAX_DRUM_FILLS  (8)
#define MAX_SONG_PARTS  (32)
#define MAX_EFFECT_NAME  (16)
#define MAX_DRM_NAME  (16)

PACK typedef struct SongPartStruct {
    uint32_t nDrumFill;
    int32_t bpmDelta;
    uint16_t repeatFlag;  // 0 = no, 1 = yes
    uint16_t shuffleFlag; // 0 = no, 1 = yes
    uint32_t timeSigNum;
    uint32_t timeSigDen;
    uint32_t tickPerBar;

#ifndef USE_MIDI_TRACK_PTR // Defined in firmware only
    int32_t mainLoopIndex;
    int32_t transFillIndex;
    int32_t drumFillIndex[MAX_DRUM_FILLS]; // static size
#else
    MIDIPARSER_MidiTrack * trackPtr;
    MIDIPARSER_MidiTrack * tranFillPtr;
    MIDIPARSER_MidiTrack * drumFillPtr[MAX_DRUM_FILLS]; // static size
#endif
    int8_t effectName[MAX_EFFECT_NAME]; // static size

    uint32_t effectVolume; // 100 = no volume, 0-200 are % of original sound

    uint32_t loopCount;
    uint32_t reserved2;
    uint32_t reserved3;

#ifdef __cplusplus
    SongPartStruct(uint32_t repeatFlag = 1):
       nDrumFill(0),
       bpmDelta(0),
       repeatFlag(repeatFlag),
       shuffleFlag(0),
       timeSigNum(4),
       timeSigDen(4),
       tickPerBar(4*480),
       effectVolume(100),
       loopCount(0),
       reserved2(0),
       reserved3(0)
    {
       mainLoopIndex = -1;
       transFillIndex = -1;
       for (int i = 0; i<MAX_DRUM_FILLS; i++){
          drumFillIndex[i] = -1;
       }
       for (int i = 0; i<MAX_EFFECT_NAME; i++){
          effectName[i] = 0;
       }

    }
#endif
} PACKED SONG_SongPartStruct;



PACK typedef struct SongStruct{
    uint32_t loopSong;
    uint32_t bpm;
    uint32_t nPart;

    uint32_t reserved1;

    int8_t defaultDrmName[MAX_DRM_NAME]; // static size

    SONG_SongPartStruct intro;
    SONG_SongPartStruct outro;
    SONG_SongPartStruct part[MAX_SONG_PARTS]; // static

#ifdef __cplusplus
    SongStruct(uint32_t bpm = 120):
       loopSong(0),
       bpm(bpm),
       nPart(1),
       reserved1(0)
    {
       for (int i = 0; i<MAX_DRM_NAME; i++){
          defaultDrmName[i] = 0;
       }
    }
#endif
} PACKED SONG_SongStruct;



#endif // _SONG_H_
