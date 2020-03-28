#ifndef AUTOPILOTDATAPARTMODEL_H
#define AUTOPILOTDATAPARTMODEL_H

#include <QObject>
#include "abstractfilepartmodel.h"
#include "autopilot.h"
#include "autopilotdatafillmodel.h"

class AutoPilotDataPartModel : public AbstractFilePartModel
{
    Q_OBJECT
public:
    AutoPilotDataPartModel();

    uint32_t readFromBuffer(uint8_t * p_Buffer, uint32_t size, QStringList *p_ParseErrors);
    AutoPilotDataFillModel * getMainLoop();
    AutoPilotDataFillModel * getTransitionFill();
    AutoPilotDataFillModel * getDrumFill(int index);

    QString verifyPartValid(int nbFill);
private:

    AutoPilotDataPartStruct_InternalData m_AutoPilotPart;

    virtual uint32_t maxInternalSize();
    virtual uint32_t minInternalSize();
    virtual uint8_t *internalData();
};

#endif // AUTOPILOTDATAPARTMODEL_H
