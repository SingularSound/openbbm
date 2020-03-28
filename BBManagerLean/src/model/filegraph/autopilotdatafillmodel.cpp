#include "autopilotdatafillmodel.h"

AutoPilotDataFillModel::AutoPilotDataFillModel():
       AbstractFilePartModel()
{
    m_InternalSize = minInternalSize();
}

uint32_t AutoPilotDataFillModel::readFromBuffer(uint8_t * p_Buffer, uint32_t size, QStringList *p_ParseErrors)
{
    p_ParseErrors;
    if(size < minSize()){
        p_ParseErrors->append(tr("File size is less than allowed.\n\nFile size: %1 byte(s)\nMinimum: %2 bytes").arg(size).arg(minSize()));
        return -1;
    }
    m_AutoPilotFill.read((char*)p_Buffer, size);
    return m_InternalSize = m_AutoPilotFill.size();
}


uint32_t AutoPilotDataFillModel::maxInternalSize()
{
    return sizeof(AutoPilotDataFillStruct);
}

uint32_t AutoPilotDataFillModel::minInternalSize()
{
    return sizeof(AutoPilotDataFillStruct);
}

uint8_t *AutoPilotDataFillModel::internalData()
{
    return (uint8_t *)&m_AutoPilotFill;
}

void AutoPilotDataFillModel::setPlayAt(uint32_t playAt)
{
    m_AutoPilotFill.playAt = playAt;
}

void AutoPilotDataFillModel::setPlayFor(uint32_t playFor)
{
    m_AutoPilotFill.playFor = playFor;
}

uint32_t AutoPilotDataFillModel::getPlayAt()
{
    return m_AutoPilotFill.playAt;
}

uint32_t AutoPilotDataFillModel::getPlayFor()
{
    return m_AutoPilotFill.playFor;
}
