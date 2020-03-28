#ifndef SONGFILE_H
#define SONGFILE_H

#include <stdint.h>

#include "song.h"
#include "midiParser.h"
#include "pragmapack.h"
#include "version.h"
#include "autopilot.h"

#ifdef __cplusplus
#include <array>
#endif



#define SONGFILE_MAX_PARTS_CNT (MAX_SONG_PARTS)
#define SONGFILE_MAX_INTRO_CNT (1u)
#define SONGFILE_MAX_OUTRO_CNT (1u)
#define SONGFILE_MAX_DRUM_LOOP_PER_PART_CNT (1u)
#define SONGFILE_MAX_DRUM_FILL_PER_PART_CNT (MAX_DRUM_FILLS)
#define SONGFILE_MAX_TRANS_FILL_PER_PART_CNT (1u)

// should be 10
#define SONGFILE_MAX_TRACKS_PER_PART_CNT (\
   SONGFILE_MAX_DRUM_LOOP_PER_PART_CNT + \
   SONGFILE_MAX_DRUM_FILL_PER_PART_CNT + \
   SONGFILE_MAX_TRANS_FILL_PER_PART_CNT)

// should be 322
#define SONGFILE_MAX_TRACKS_PER_SONG (\
   SONGFILE_MAX_INTRO_CNT + \
   SONGFILE_MAX_OUTRO_CNT + \
   32 * SONGFILE_MAX_TRACKS_PER_PART_CNT)

#define SONGFILE_MAX_TRACK_META_SIZE (256u)

// should be 659456 or 644k
#define SONGFILE_TRACK_DATA_BUFFER_SIZE (\
   SONGFILE_MAX_TRACKS_PER_SONG * \
   SONGFILE_MAX_TRACK_DATA_SIZE)

// should be 82432 or 80.5k
#define SONGFILE_TRACK_META_BUFFER_SIZE (\
   SONGFILE_MAX_TRACKS_PER_SONG * \
   SONGFILE_MAX_TRACK_META_SIZE)

#define SONGFILE_FLAG_SONG_FILE_INVALID_MASK    (1 << 0)
#define SONGFILE_FLAG_SONG_FILE_INVALID_SHIFT   (0)

// corresponds to 15s @ 24 bit stereo
#define MAX_ACCENT_HIT_FILE_SIZE 3969000



/**
*   SONGFILE Version #0 :
*       Revision 0 : Original Format and features
*       Revision 1 : Added reserved and padding to 128 bit align. Added effect Volume
*       Revision 2 : Continually loop flag
*   SONGFILE Version #1 :
*       Revision 0 : Pre-Parsed midi
*       Revision 1 : Validity flag
*       Revision 2 : Default Drm as a crc int
*   SONGFILE Version #2 :
*       Revision 0 : Early adopter version
*                    - Shuffled structure order in order no to be readable by old pedal fw
*                    - Default Drm Changed to 16 bits string
*                    - Added padding (for reserved)
*                    - Added Build info
*                    - Unified player and song building headers
*       Revision 1 : Some added features
*                    - added loopCount for autopilot
*   SONGFILE Version #3  - New Versions:
*       Revision 0 : AutoPilot Mode Addition
*                   -Added Autopilot File Structure
*                   -Added New Place Holders for future Versions
*
*/

#define BACK_COMP_SONGFILE_VERSION  2u
#define BACK_COMP_SONGFILE_REVISION 0u
#define BACK_COMP_SONGFILE_BUILD    VER_BUILDVERSION


#define ACTUAL_SONGFILE_VERSION  3u
#define ACTUAL_SONGFILE_REVISION 0u
#define ACTUAL_SONGFILE_BUILD    VER_BUILDVERSION

#define ACCEPTABLE_REVISIONS { 0u, 1u }

PACK typedef struct HeaderStruct {
   char     fileType[4];
   uint8_t  backCompVersion;
   uint8_t  backCompRevision;
   uint16_t backCompBuild;
   uint32_t crc;

   // Flags definition
   // Bit  0    : song file invalid
   // Bits 1..7 : reserved
   uint8_t  flags;
   uint8_t  reserved1;
   uint16_t reserved2;

   uint8_t  actualVersion;
   uint8_t  actualRevision;
   uint16_t actualBuild;

   uint32_t reserved4;
   uint32_t reserved5;
   uint32_t reserved6;

#ifdef __cplusplus
   HeaderStruct():
      backCompVersion(  BACK_COMP_SONGFILE_VERSION),
      backCompRevision( BACK_COMP_SONGFILE_REVISION),
      backCompBuild(    BACK_COMP_SONGFILE_BUILD),
      crc(0),
      flags(0),
      reserved1(0),
      reserved2(0),
      actualVersion(    ACTUAL_SONGFILE_VERSION),
      actualRevision(   ACTUAL_SONGFILE_REVISION),
      actualBuild(      ACTUAL_SONGFILE_BUILD),
      reserved4(0),
      reserved5(0),
      reserved6(0)
   {
      fileType[0] = 'B';
      fileType[1] = 'B';
      fileType[2] = 'S';
      fileType[3] = 'F';
   }
#endif
} PACKED SONGFILE_HeaderStruct;

#define SONGFILE_META_SIZE (256u)

PACK typedef struct {
   char fileName[8]; // .WAV is implicit
} PACKED SONGFILE_EffectItemStruct;

PACK typedef struct TrackIndexingItemStruct {
   uint32_t dataOffset;
   uint32_t metaOffset;
   uint32_t crc;

   uint32_t reserved1;
   uint32_t reserved2;
   uint32_t reserved3;
   uint32_t reserved4;
   uint32_t reserved5;

#ifdef __cplusplus
   TrackIndexingItemStruct():
      dataOffset(0),
      metaOffset(0),
      crc(0),
      reserved1(0),
      reserved2(0),
      reserved3(0),
      reserved4(0),
      reserved5(0){}
#endif
} PACKED SONGFILE_TrackIndexingItemStruct;

// Updated Structure with real modified order
PACK typedef struct OffsetTableStruct{
   uint32_t thisOffset;
   uint32_t metaOffset;
   uint32_t songOffset;
   uint32_t tracksDataOffset;
   uint32_t tracksMetaOffset;
   uint32_t tracksIndexingOffset;
   uint32_t autoPilotDataOffset;

   uint32_t reserved2;
   uint32_t reserved3;
   uint32_t reserved4;

   uint32_t thisSize;
   uint32_t metaSize;
   uint32_t songSize;
   uint32_t tracksIndexingSize;
   uint32_t tracksMetaSize;
   uint32_t tracksDataSize;
   uint32_t autoPilotDataSize;

   uint32_t reserved6;
   uint32_t reserved7;
   uint32_t reserved8;

#ifdef __cplusplus
   OffsetTableStruct():
      thisSize(sizeof(struct OffsetTableStruct)),
      metaSize(SONGFILE_META_SIZE),
      songSize(sizeof(SONG_SongStruct)),
      tracksIndexingSize(sizeof(SONGFILE_TrackIndexingItemStruct)),
      tracksMetaSize(0),
      tracksDataSize(0),
      autoPilotDataSize(sizeof(AUTOPILOT_AutoPilotDataStruct)),
      reserved6(0),
      reserved7(0),
      reserved8(0)
   {
      thisOffset = sizeof(SONGFILE_HeaderStruct);
      metaOffset = thisOffset + thisSize;
      songOffset = metaOffset + metaSize;
      tracksIndexingOffset = songOffset + songSize;
      tracksMetaOffset = tracksIndexingOffset + tracksIndexingSize;
      tracksDataOffset = tracksMetaOffset;
      autoPilotDataOffset = tracksDataOffset + tracksDataSize;
      reserved2 = 0;
      reserved3 = 0;
      reserved4 = 0;
   }
#endif
} PACKED SONGFILE_OffsetTableStruct;



PACK typedef struct {
   SONGFILE_HeaderStruct            header;
   SONGFILE_OffsetTableStruct       offsets;
   uint8_t                          metaData[SONGFILE_META_SIZE];
   SONG_SongStruct                  song;
   SONGFILE_TrackIndexingItemStruct trackIndexes[SONGFILE_MAX_TRACKS_PER_SONG];
   uint8_t                          trackMetaBuffer[SONGFILE_TRACK_META_BUFFER_SIZE];
} PACKED SONGFILE_FileStruct;


#endif // SONGFILE_H
