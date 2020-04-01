/*
  	This software and the content provided for use with it is Copyright © 2014-2020 Singular Sound 
 	BeatBuddy Manager is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2 as published by
    the Free Software Foundation.
    
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
    
    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/
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
