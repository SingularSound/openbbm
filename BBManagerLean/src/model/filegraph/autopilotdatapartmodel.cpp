#include "autopilotdatafillmodel.h"
#include "autopilotdatapartmodel.h"
#include "song.h"

AutoPilotDataPartModel::AutoPilotDataPartModel()
{
    m_InternalSize = minInternalSize();

    // Intro Fill
    mp_SubParts->append(new AutoPilotDataFillModel());
    // Outro Fill
    mp_SubParts->append(new AutoPilotDataFillModel());


    // Create empty parts since AUTOPILOT_Fill has a fixed size
    for(int i = 0; i < MAX_DRUM_FILLS; i++){
       mp_SubParts->append(new AutoPilotDataFillModel());
    }
}

uint32_t AutoPilotDataPartModel::readFromBuffer(uint8_t * p_Buffer, uint32_t size, QStringList *p_ParseErrors)
{

    uint32_t usedSize = AbstractFilePartModel::readFromBuffer(p_Buffer, size, p_ParseErrors);
    uint32_t totalUsedSize = usedSize;
    p_Buffer += usedSize;
    size -= usedSize;

    qDeleteAll(*mp_SubParts);
    mp_SubParts->clear();

    AutoPilotDataFillModel *p_AutoPilotFillModel;

    // 1 - Read Main Loop
    p_AutoPilotFillModel = new AutoPilotDataFillModel();
    usedSize = p_AutoPilotFillModel->readFromBuffer(p_Buffer, size, p_ParseErrors);
    if((int)usedSize < 0){
        p_ParseErrors->append(tr("SongModel::readFromBuffer - ERROR - error while reading intro part data"));
       delete p_AutoPilotFillModel;
       return -1;
    }
    totalUsedSize += usedSize;
    p_Buffer += usedSize;
    size -= usedSize;
    mp_SubParts->append(p_AutoPilotFillModel);

    // 2 - Read Trans Fill
    p_AutoPilotFillModel = new AutoPilotDataFillModel();
    usedSize = p_AutoPilotFillModel->readFromBuffer(p_Buffer, size, p_ParseErrors);
    if((int)usedSize < 0){
        p_ParseErrors->append(tr("SongModel::readFromBuffer - ERROR - error while reading intro part data"));
       delete p_AutoPilotFillModel;
       return -1;
    }
    totalUsedSize += usedSize;
    p_Buffer += usedSize;
    size -= usedSize;
    mp_SubParts->append(p_AutoPilotFillModel);


    // 3 - Read Drum Fills
    for(int i = 0; i < MAX_DRUM_FILLS; i++){
        p_AutoPilotFillModel = new AutoPilotDataFillModel();
        usedSize = p_AutoPilotFillModel->readFromBuffer(p_Buffer, size, p_ParseErrors);
        if((int)usedSize < 0){
            p_ParseErrors->append(tr("SongModel::readFromBuffer - ERROR - error while reading intro part data"));
            delete p_AutoPilotFillModel;
            return -1;
        }
        totalUsedSize += usedSize;
        p_Buffer += usedSize;
        size -= usedSize;
        mp_SubParts->append(p_AutoPilotFillModel);
    }

    return totalUsedSize;
}

QString AutoPilotDataPartModel::verifyPartValid(int nbFill)
{
    QString returnString = "";
    //Verify Main Trigger At >= Main Reset After
    if(getMainLoop()->getPlayAt() < getMainLoop()->getPlayFor() && getMainLoop()->getPlayAt() > 0)
    {
        returnString += QString(tr("Main Drum Trigger at value < Main Drum Reset after value.\n")) ;
    }

    //Verify Drum Fill(i) play at < Drum Fill(i+1) play at
    //Verify Drum Fill(i) play at < Main Loop Trigger At

    for(int i = 0; i < nbFill-1; i++)
    {

        if(getDrumFill(i)->getPlayAt() >= getMainLoop()->getPlayAt() && getMainLoop()->getPlayAt() > 0)
        {
            returnString += QString(tr("Drum Fill #%1 Trigger at value > Main Drum Trigger at value.\n")).arg(i+1) ;
        }
        if(getDrumFill(i)->getPlayAt() >= getDrumFill(i+1)->getPlayAt())
        {
            returnString += QString(tr("Drum Fill #%1 Trigger at value > Drum Fill #%2 Trigger at value.\n"))
                    .arg(i+1)
                    .arg(i+2);
        }
    }

    if(getDrumFill(nbFill-1)->getPlayAt() >= getMainLoop()->getPlayAt()
            && nbFill > 0 && getMainLoop()->getPlayAt() > 0)
    {
        returnString += QString(tr("Drum Fill #%1 Trigger at value > Main Drum Trigger at value.\n")).arg(nbFill) ;
    }

    return returnString;
}

AutoPilotDataFillModel *AutoPilotDataPartModel::getMainLoop()
{
    return static_cast<AutoPilotDataFillModel*>(mp_SubParts->at(0));
}

AutoPilotDataFillModel *AutoPilotDataPartModel::getTransitionFill()
{
    return static_cast<AutoPilotDataFillModel*>(mp_SubParts->at(1));
}

AutoPilotDataFillModel *AutoPilotDataPartModel::getDrumFill(int index)
{
    return static_cast<AutoPilotDataFillModel*>(mp_SubParts->at(index+2));
}

uint32_t AutoPilotDataPartModel::maxInternalSize()
{
    return (sizeof(AutoPilotDataPartStruct_InternalData));
}

uint32_t AutoPilotDataPartModel::minInternalSize()
{
    return (sizeof(AutoPilotDataPartStruct_InternalData));
}

uint8_t *AutoPilotDataPartModel::internalData()
{
    return (uint8_t *)&m_AutoPilotPart;
}
