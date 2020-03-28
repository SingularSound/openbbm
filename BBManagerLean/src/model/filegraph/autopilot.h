#ifndef AUTOPILOT_H
#define AUTOPILOT_H

#include <stdint.h>

#include "midiParser.h"
#include "pragmapack.h"



#define MAX_AUTOPILOT_PARTS  (32)
#define MAX_AUTOPILOT_FILLS  (8)

#define AUTOPILOT_ON_FLAG (1 << 0)
#define AUTOPILOT_VALID_FLAG (1 << 1)


PACK typedef struct AutoPilotDataFillStruct {
    uint32_t playFor;
    uint32_t playAt;
    uint32_t reserved1;
    uint32_t reserved2;


#ifdef __cplusplus

    static unsigned header_size() { return sizeof(AutoPilotDataFillStruct); }
    unsigned size() const { return header_size(); }


    AutoPilotDataFillStruct():
        playFor(0),
        playAt(0),
        reserved1(0),
        reserved2(0)
    {
    }

    AutoPilotDataFillStruct& read(const char* ptr, unsigned size = 0);
    void write_file(const std::string& name) const;
#endif
} PACKED AUTOPILOT_AutoPilotDataFillStruct;


PACK typedef struct AutoPilotDataPartStruct_InternalData {
    uint32_t reserved1;
    uint32_t reserved2;

#ifdef __cplusplus
    AutoPilotDataPartStruct_InternalData():
        reserved1(0),
        reserved2(0)
    {
    }
#endif
} PACKED AUTOPILOT_AutoPilotDataPartStruct_InternalData;


PACK typedef struct AutoPilotDataStruct_InternalData {
    uint32_t autoPilotFlags;          // 1st Bit -> Autopilot Enable
                                      // 2nd Bit -> Autopilot Valid
    uint32_t reserved1;
    uint32_t reserved2;

#ifdef __cplusplus
    AutoPilotDataStruct_InternalData():
       autoPilotFlags(0),
       reserved1(0),
       reserved2(0)
    {
    }
#endif
} PACKED AUTOPILOT_AutoPilotDataStruct_InternalData;



PACK typedef struct AutoPilotDataPartStruct {
    AUTOPILOT_AutoPilotDataPartStruct_InternalData internalData;

    AUTOPILOT_AutoPilotDataFillStruct mainLoop;
    AUTOPILOT_AutoPilotDataFillStruct transitionFill;
    AUTOPILOT_AutoPilotDataFillStruct drumFill[MAX_AUTOPILOT_FILLS];

#ifdef __cplusplus
    AutoPilotDataPartStruct()
    {
    }
#endif
} PACKED AUTOPILOT_AutoPilotDataPartStruct;


PACK typedef struct AutoPilotDataStruct {
    AUTOPILOT_AutoPilotDataStruct_InternalData internalData;
    AUTOPILOT_AutoPilotDataPartStruct intro;
    AUTOPILOT_AutoPilotDataPartStruct outro;
    AUTOPILOT_AutoPilotDataPartStruct part[MAX_AUTOPILOT_PARTS]; // static


#ifdef __cplusplus
    AutoPilotDataStruct()
    {
    }
#endif
} PACKED AUTOPILOT_AutoPilotDataStruct;

#endif // AUTOPILOT_H
