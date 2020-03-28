#ifndef AUTOPILOTTRACKMODEL_H
#define AUTOPILOTTRACKMODEL_H

#include <QObject>

#include "abstractfilepartmodel.h"
#include "autopilot.h"
#include "autopilotdatafillmodel.h"
#include "autopilotdatapartmodel.h"

class AutoPilotDataModel : public AbstractFilePartModel
{
public:
    AutoPilotDataModel();


    uint32_t readFromBuffer(uint8_t *p_Buffer, uint32_t size, QStringList *p_ParseErrors);
    void adjustOffsets();
    AutoPilotDataPartModel *getIntroModel();
    AutoPilotDataPartModel *getOutroModel();
    AutoPilotDataPartModel* getPartModel( int part );

    void moveParts(int firstIndex, int lastIndex, int delta);
    void setAutoPilotEnabled(bool state );
    void setAutoPilotValid(bool state);
    bool getAutoPilotEnabled();
    bool getAutoPilotValid();
    void insertAutoPilotPart(int index);
    void removeAutoPilotPart(int index);
    void moveAutoPilotPartDown(int oldIndex);
    void moveAutoPilotPartUp(int oldIndex);

    void deletePart(int i);
    void appendNewPart();
    void insertNewPart(int i);
private:
    AutoPilotDataStruct_InternalData m_AutoPilotData;

    uint32_t maxInternalSize();
    uint32_t minInternalSize();
    uint8_t *internalData();
};

#endif // AUTOPILOTTRACKMODEL_H
