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
#include "autopilotdatamodel.h"
#include "autopilotdatapartmodel.h"
#include "autopilotdatafillmodel.h"
#include "song.h"
#include <QDebug>

AutoPilotDataModel::AutoPilotDataModel()
{
    m_InternalSize = minInternalSize();

    mp_SubParts->append(new AutoPilotDataPartModel());

    mp_SubParts->append(new AutoPilotDataPartModel());

    // Create empty parts since SONG_SongStruct has a fixed size
    for(int i = 0; i < MAX_SONG_PARTS; i++){
       mp_SubParts->append(new AutoPilotDataPartModel());
    }
}



uint32_t AutoPilotDataModel::readFromBuffer(uint8_t * p_Buffer, uint32_t size, QStringList *p_ParseErrors)
{

   uint32_t totalUsedSize = 0;
   uint32_t usedSize = 0;

   // 1 - Read internal data
   usedSize = AbstractFilePartModel::readFromBuffer(p_Buffer, size, p_ParseErrors);
   if(((int)usedSize) < ((int)minInternalSize())){
      p_ParseErrors->append(tr("AutoPilotDataModel::readFromBuffer - ERROR - error while reading internal data"));
      return -1;
   }
   totalUsedSize += usedSize;
   p_Buffer += usedSize;
   size -= usedSize;

   qDeleteAll(*mp_SubParts);
   mp_SubParts->clear();

   AutoPilotDataPartModel *p_AutoPilotPartModel;

   // 2 - Read intro
   p_AutoPilotPartModel = new AutoPilotDataPartModel();
   usedSize = p_AutoPilotPartModel->readFromBuffer(p_Buffer, size, p_ParseErrors);
   if((int)usedSize < 0){
      p_ParseErrors->append(tr("SongModel::readFromBuffer - ERROR - error while reading intro part data"));
      delete p_AutoPilotPartModel;
      return -1;
   }
   totalUsedSize += usedSize;
   p_Buffer += usedSize;
   size -= usedSize;
   mp_SubParts->append(p_AutoPilotPartModel);

   // 3 - Read outro
   p_AutoPilotPartModel = new AutoPilotDataPartModel();
   usedSize = p_AutoPilotPartModel->readFromBuffer(p_Buffer, size, p_ParseErrors);
   if((int)usedSize < 0){
      p_ParseErrors->append(tr("SongModel::readFromBuffer - ERROR - error while reading outro part data"));
      delete p_AutoPilotPartModel;
      return -1;
   }
   totalUsedSize += usedSize;
   p_Buffer += usedSize;
   size -= usedSize;
   mp_SubParts->append(p_AutoPilotPartModel);

   // 4 - Create empty parts (including empty parts since SONG_SongStruct has a fixed size
   for(int i = 0; i < MAX_SONG_PARTS; i++){
      p_AutoPilotPartModel = new AutoPilotDataPartModel();
      usedSize = p_AutoPilotPartModel->readFromBuffer(p_Buffer, size, p_ParseErrors);

      if((int)usedSize < 0){
         p_ParseErrors->append(tr("SongModel::readFromBuffer - ERROR - error while reading part %1 data").arg(i));
         delete p_AutoPilotPartModel;
         return -1;
      }
      totalUsedSize += usedSize;
      p_Buffer += usedSize;
      size -= usedSize;

      mp_SubParts->append(p_AutoPilotPartModel);
   }

   // validate that size = 0
   if(size != 0){
      p_ParseErrors->append(tr("SongModel::readFromBuffer - WARNING - A part of the song structure was not used by parser. The unused size is %1").arg(size));
   }

   return totalUsedSize;
}

void AutoPilotDataModel::adjustOffsets()
{
}

AutoPilotDataPartModel *AutoPilotDataModel::getIntroModel()
{
    return static_cast<AutoPilotDataPartModel*>(mp_SubParts->at(0));
}

AutoPilotDataPartModel *AutoPilotDataModel::getOutroModel()
{
    return static_cast<AutoPilotDataPartModel*>(mp_SubParts->at(1));
}

AutoPilotDataPartModel *AutoPilotDataModel::getPartModel(int part)
{
    return static_cast<AutoPilotDataPartModel*>(mp_SubParts->at(part+2));
}

void AutoPilotDataModel::insertNewPart(int i)
{
   if(i >= MAX_SONG_PARTS || i < 0){
      return;
   }
     mp_SubParts->insert(i+2, new AutoPilotDataPartModel()); 
}

void AutoPilotDataModel::deletePart(int i)
{
   if(i >= MAX_AUTOPILOT_PARTS || i < 0){
      return;
   }

   AutoPilotDataPartModel *p_AutoPilotDataPart = (AutoPilotDataPartModel *)mp_SubParts->at(i+2);
   mp_SubParts->removeAt(i+2);

   // Make sure all tracks used by deleted song part are removed if not used somewhere else

   delete p_AutoPilotDataPart;

   // Add empty part at the end
   mp_SubParts->append(new AutoPilotDataPartModel());
}

void AutoPilotDataModel::moveParts(int firstIndex, int lastIndex, int delta)
{
   if(firstIndex > lastIndex){
      qWarning() << "AutoPilotDataModel::moveParts : (firstIndex > lastIndex)";
      return;
   }
   if(lastIndex >= MAX_AUTOPILOT_PARTS+2 || firstIndex < 0){
      qWarning() << "AutoPilotDataModel::moveParts : (lastIndex >= MAX_AUTOPILOT_PARTS || firstIndex < 0)";
      return;
   }

   if(lastIndex + delta >= MAX_AUTOPILOT_PARTS+2 || firstIndex + delta < 0){
      qWarning() << "AutoPilotDataModel::moveParts : (lastIndex + delta >= MAX_AUTOPILOT_PARTS || firstIndex + delta < 0)";
      return;
   }

   // Adjust to skip intro and outro
   firstIndex += 2;
   lastIndex += 2;

   // Create a list of items to move
   QList<AbstractFilePartModel *> moved;
   for(int i = firstIndex; i <= lastIndex; i++){
      moved.append(mp_SubParts->takeAt(firstIndex));
   }

   firstIndex += delta;
   lastIndex += delta;

   int movedIndex = 0;
   for(int i = firstIndex; i <= lastIndex; i++, movedIndex++){
      mp_SubParts->insert(i, moved.at(movedIndex));
   }

}


void AutoPilotDataModel::setAutoPilotEnabled(bool state)
{
    if(state)
    {
        m_AutoPilotData.autoPilotFlags |= (AUTOPILOT_ON_FLAG);
    } else {
        m_AutoPilotData.autoPilotFlags &= ~(AUTOPILOT_ON_FLAG);
    }
}

void AutoPilotDataModel::setAutoPilotValid(bool state)
{
    if(state)
    {
        m_AutoPilotData.autoPilotFlags |= (AUTOPILOT_VALID_FLAG);
    } else {
        m_AutoPilotData.autoPilotFlags &= ~(AUTOPILOT_VALID_FLAG);
    }
}

void AutoPilotDataModel::insertAutoPilotPart(int index)
{
    mp_SubParts->insert(index+1, new AutoPilotDataPartModel());
    mp_SubParts->removeLast();
}

void AutoPilotDataModel::removeAutoPilotPart(int index)
{
    mp_SubParts->removeAt(index+1);
    mp_SubParts->append(new AutoPilotDataPartModel());
}

void AutoPilotDataModel::moveAutoPilotPartDown(int oldIndex)
{
    mp_SubParts->swap(oldIndex, oldIndex+1);
}

void AutoPilotDataModel::moveAutoPilotPartUp(int oldIndex)
{
    mp_SubParts->swap(oldIndex, oldIndex-1);
}

bool AutoPilotDataModel::getAutoPilotEnabled()
{
    return (m_AutoPilotData.autoPilotFlags & AUTOPILOT_ON_FLAG) && true;
}

bool AutoPilotDataModel::getAutoPilotValid()
{
    return (m_AutoPilotData.autoPilotFlags & AUTOPILOT_VALID_FLAG) && true;
}

uint32_t AutoPilotDataModel::maxInternalSize()
{
    return (sizeof(AUTOPILOT_AutoPilotDataStruct_InternalData));
}

uint32_t AutoPilotDataModel::minInternalSize()
{
    return (sizeof(AUTOPILOT_AutoPilotDataStruct_InternalData));
}

uint8_t *AutoPilotDataModel::internalData()
{
    return (uint8_t *)&m_AutoPilotData;
}
