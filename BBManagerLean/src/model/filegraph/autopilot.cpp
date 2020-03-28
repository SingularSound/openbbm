#include "autopilot.h"


AUTOPILOT_AutoPilotDataFillStruct& AUTOPILOT_AutoPilotDataFillStruct::read(const char* ptr, unsigned size)
{
    memcpy(this, ptr, header_size());
    return *this;
}
