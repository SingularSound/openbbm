#include "player/soundManager.h"
#include "../../version.h"
#include "pragmapack.h"
#include <stdint.h>
#include <stdlib.h>

#ifndef COMMON_H
#define COMMON_H

/**
*   Drm Version #0
*       Revision 0 : Standart drumset features
*       Revision 1 : Added independant velocity control of each instrument
*       Revision 2 : Added Fill Choke Group Poly & Fill Choke Group Delay
*   Drm Version #1 :
*       Revision 0 : Early adopter version
*       Revision 1 : Added Global Drumset Volume
*/

#define DRM_VERSION                         1u
#define DRM_REVISION                        1u
#define DRM_BUILD                           VER_BUILDVERSION

#define DRM_MAX_DRUMSET_VOLUME              159u
#define DRM_MIN_DRUMSET_VOLUME              0u

PACK typedef struct {
    char           header[4];
    unsigned char  version;
    unsigned char  revision;
    unsigned short build;
    unsigned int   crc;
} PACKED DrmHeader_t;

PACK typedef struct {
    uint32_t offset;
    uint32_t size;
} PACKED DrmMetadataHeader_t;

#define DRM_EXTENSION_HEADER_MAGIC_SIZE         4
#define DRM_EXTENSION_HEADER_MAGIC_VALUE        {'e', 'x', 't', 'h'} // "exth"
#define DRM_EXTENSION_ARRAY_COUNT               4


PACK typedef struct {
    uint32_t offset;
    uint32_t size;
} PACKED DrmExtensionPtr_t;

PACK typedef struct {
    char              header[DRM_EXTENSION_HEADER_MAGIC_SIZE];
    DrmExtensionPtr_t offsets[DRM_EXTENSION_ARRAY_COUNT];
} PACKED DrmExtensionHeader_t;

// File format and sizes

#define DRM_HEADER_BYTE_SIZE                    sizeof(DrmHeader_t)                                                  // Should be 12
#define DRM_CRC_OFFSET                          ( DRM_HEADER_BYTE_SIZE - sizeof(unsigned int) )                      // Should be 8

#define DRM_INSTRUMENT_OFFSET                   DRM_HEADER_BYTE_SIZE                                                 // Should be 12
#define DRM_MAX_INSTRUMENT_COUNT                128

#define DRM_INSTRUMENT_BYTE_SIZE                sizeof(Instrument_t)                                                 // Should be (20 + 448) = 468
#define DRM_INSTRUMENT_SECTION_BYTE_SIZE        ( DRM_MAX_INSTRUMENT_COUNT * DRM_INSTRUMENT_BYTE_SIZE)               // Should be (128 * 468) = 59904

#define DRM_METADATA_HEADER_OFFSET              ( DRM_INSTRUMENT_OFFSET + DRM_INSTRUMENT_SECTION_BYTE_SIZE )         // Should be 12 + 59904 = 59916
#define DRM_METADATA_HEADER_BYTE_SIZE           sizeof(DrmMetadataHeader_t)                                          // Should be 8

#define DRM_EXTENSION_HEADER_OFFSET             ( DRM_METADATA_HEADER_OFFSET + DRM_METADATA_HEADER_BYTE_SIZE )       // Should be 59916 + 8 = 59924


#define DRM_EXTENSION_ARRAY_SIZE                ( DRM_EXTENSION_ARRAY_COUNT * sizeof(DrmExtensionHeader_t) )         // Should be 4 * 8 = 32
#define DRM_EXTENSION_HEADER_BYTE_SIZE          ( sizeof(DrmExtensionHeader_t) )                                     //  Should be 4 + 32 = 36

#define DRM_PRE_WAVE_SIZE                       ( DRM_EXTENSION_HEADER_OFFSET + DRM_EXTENSION_HEADER_BYTE_SIZE )     // Should be 59924 + 36 = 59960

#define DRM_WAV_START_OFFSET                    60416u // Arbirtaty 0xEC00
#define DRM_INSTRUMENT_TO_WAVE_PADDING          ( DRM_WAV_START_OFFSET - DRM_PRE_WAVE_SIZE )                         // Should be 60416u - 59960 = 456
#define DRM_WAV_BYTE_ALLIGNMENT                 512u

// Make sure sections don't overlap
static_assert(DRM_WAV_START_OFFSET >= DRM_PRE_WAVE_SIZE, "INSTRUMENT_TO_WAVE_PADDING must be >= than 0");

#define DRM_EXTENSION_VOLUME_INDEX              0
#define DRM_EXTENSION_VOLUME_MAGIC_SIZE         4
#define DRM_EXTENSION_VOLUME_MAGIC              {'v', 'o', 'l', 'g'} // "volg"

PACK typedef struct {
    char              header[DRM_EXTENSION_VOLUME_MAGIC_SIZE];
    uint8_t           padding[3];
    uint8_t           globalVolume;
} PACKED DrmExtensionVolume_t;


#endif // COMMON_H
