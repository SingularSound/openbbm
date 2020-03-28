#ifndef AUTOPILOTDATAFILLMODEL_H
#define AUTOPILOTDATAFILLMODEL_H

#include <QObject>

#include "abstractfilepartmodel.h"
#include "autopilot.h"

class AutoPilotDataFillModel : public AbstractFilePartModel
{
public:
    explicit AutoPilotDataFillModel();

    uint32_t readFromBuffer(uint8_t * p_Buffer, uint32_t size, QStringList *p_ParseErrors);
    virtual uint32_t maxInternalSize();
    virtual uint32_t minInternalSize();
    virtual uint8_t *internalData();

    void setPlayAt(uint32_t playAt);
    void setPlayFor(uint32_t playFor);

    uint32_t getPlayAt( void );
    uint32_t getPlayFor(void );

private:
    AutoPilotDataFillStruct m_AutoPilotFill;
};

#endif // AUTOPILOTDATAFILLMODEL_H
