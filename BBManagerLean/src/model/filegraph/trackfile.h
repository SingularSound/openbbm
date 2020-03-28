#ifndef TRACKFILE_H
#define TRACKFILE_H

#include "songfile.h"



/**
*   SONGTRACK Version #0 :
*       Revision 0 : Original Format and features
*/

#define SONGTRACK_VERSION  0u
#define SONGTRACK_REVISION 0u
#define SONGTRACK_BUILD    VER_BUILDVERSION


PACK typedef struct SONGTRACK_HeaderStruct_t {
    char     fileType[4];
    uint8_t  version;
    uint8_t  revision;
    uint16_t build;
    uint32_t fileCRC; // CRC is not computed on header

    // Flags definition
    // Bit  0    : song file invalid
    // Bits 1..7 : reserved
    uint8_t  flags;
    uint8_t  reserved1;
    uint16_t reserved2;

    uint32_t reserved3;
    uint32_t reserved4;
    uint32_t reserved5;
    uint32_t reserved6;

#ifdef __cplusplus
    SONGTRACK_HeaderStruct_t():
        version(SONGTRACK_VERSION),
        revision(SONGTRACK_REVISION),
        build(SONGTRACK_BUILD),
        fileCRC(0),
        flags(0),
        reserved1(0),
        reserved2(0),
        reserved3(0),
        reserved4(0),
        reserved5(0),
        reserved6(0)
    {
        fileType[0] = 'B';
        fileType[1] = 'B';
        fileType[2] = 'T';
        fileType[3] = 'F';
    }
#endif
} PACKED SONGTRACK_HeaderStruct;

PACK typedef struct SONGTRACK_TrackInfo_t {
    uint32_t timeSigNum;
    uint32_t timeSigDen;
    uint32_t tickPerBar;
    uint32_t bpm;
    uint32_t trackCRC;

    uint32_t reserved1;
    uint32_t reserved2;
    uint32_t reserved3;

#ifdef __cplusplus
    SONGTRACK_TrackInfo_t():
        timeSigNum(0),
        timeSigDen(0),
        tickPerBar(0),
        bpm(0),
        trackCRC(0),
        reserved1(0),
        reserved2(0),
        reserved3(0)
    {
    }
#endif
} PACKED SONGTRACK_TrackInfo;

PACK typedef struct SONGTRACK_OffsetTableStruct_t{
    uint32_t thisOffset;
    uint32_t infoOffset;
    uint32_t metaOffset;
    uint32_t dataOffset;
    uint32_t originalDataOffset;

    uint32_t reserved1;
    uint32_t reserved2;
    uint32_t reserved3;

    uint32_t thisSize;
    uint32_t infoSize;
    uint32_t metaSize;
    uint32_t dataSize;
    uint32_t originalDataSize;

    uint32_t reserved4;
    uint32_t reserved5;
    uint32_t reserved6;

#ifdef __cplusplus
    SONGTRACK_OffsetTableStruct_t():
        thisSize(sizeof(struct SONGTRACK_OffsetTableStruct_t)),
        infoSize(sizeof(struct SONGTRACK_TrackInfo_t)),
        metaSize(0),
        dataSize(0),
        originalDataSize(0),
        reserved4(0),
        reserved5(0),
        reserved6(0)
    {
        thisOffset = sizeof(SONGTRACK_HeaderStruct);
        infoOffset = thisOffset + thisSize;
        metaOffset = infoOffset + infoSize;
        dataOffset = metaOffset + metaSize;
        originalDataOffset = 0;
        reserved4 = 0;
        reserved5 = 0;
        reserved6 = 0;
    }
#endif
} PACKED SONGTRACK_OffsetTableStruct;

PACK typedef struct SONGTRACK_FileStruct_t{
    SONGTRACK_HeaderStruct           header;
    SONGTRACK_OffsetTableStruct      offsets;
    SONGTRACK_TrackInfo              trackInfo;
    uint8_t                          meta[SONGFILE_MAX_TRACK_META_SIZE];
} PACKED SONGTRACK_FileStruct;

#endif // TRACKFILE_H
