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
#include <QVariant>
#include <QDebug>
#include "songmodel.h"
#include "songpartmodel.h"
#include "songtracksmodel.h"

SongModel::SongModel(SongTracksModel *p_SongTracksModel) :
   AbstractFilePartModel(),
   m_Song()
{
   mp_SongTracksModel = p_SongTracksModel;

   m_Default_Name = tr("SongModel");
   m_Name = m_Default_Name;

   m_InternalSize = minInternalSize();
   SongPartModel *p_SongPartModel;

   p_SongPartModel = new SongPartModel(p_SongTracksModel,true, false);
   mp_SubParts->append(p_SongPartModel);

   p_SongPartModel = new SongPartModel(p_SongTracksModel,false, true);
   mp_SubParts->append(p_SongPartModel);


   // Create empty parts since SONG_SongStruct has a fixed size
   for(int i = 0; i < MAX_SONG_PARTS; i++){
      mp_SubParts->append(new SongPartModel(p_SongTracksModel,false, false));
   }

}

SongModel::~SongModel()
{
}

uint32_t SongModel::readFromBuffer(uint8_t * p_Buffer, uint32_t size, QStringList *p_ParseErrors)
{

   uint32_t totalUsedSize = 0;
   uint32_t usedSize = 0;

   // 1 - Read internal data
   usedSize = AbstractFilePartModel::readFromBuffer(p_Buffer, size, p_ParseErrors);
   if(((int)usedSize) < ((int)minInternalSize())){
      qWarning() << "SongModel::readFromBuffer - ERROR - error while reading internal data";
      p_ParseErrors->append(tr("SongModel::readFromBuffer - ERROR - error while reading internal data"));
      return -1;
   }
   totalUsedSize += usedSize;
   p_Buffer += usedSize;
   size -= usedSize;

   qDeleteAll(*mp_SubParts);
   mp_SubParts->clear();


   SongPartModel *p_SongPartModel;

   // 2 - Read intro
   p_SongPartModel = new SongPartModel(mp_SongTracksModel,true, false);
   usedSize = p_SongPartModel->readFromBuffer(p_Buffer, size, p_ParseErrors);
   if((int)usedSize < 0){
      qWarning() << "SongModel::readFromBuffer - ERROR - error while reading intro part data";
      p_ParseErrors->append(tr("SongModel::readFromBuffer - ERROR - error while reading intro part data"));
      delete p_SongPartModel;
      return -1;
   }
   totalUsedSize += usedSize;
   p_Buffer += usedSize;
   size -= usedSize;
   mp_SubParts->append(p_SongPartModel);

   // 3 - Read outro
   p_SongPartModel = new SongPartModel(mp_SongTracksModel,false, true);
   usedSize = p_SongPartModel->readFromBuffer(p_Buffer, size, p_ParseErrors);
   if((int)usedSize < 0){
      qWarning() << "SongModel::readFromBuffer - ERROR - error while reading outro part data";
      p_ParseErrors->append(tr("SongModel::readFromBuffer - ERROR - error while reading outro part data"));
      delete p_SongPartModel;
      return -1;
   }
   totalUsedSize += usedSize;
   p_Buffer += usedSize;
   size -= usedSize;
   mp_SubParts->append(p_SongPartModel);

   // 4 - Create empty parts (including empty parts since SONG_SongStruct has a fixed size
   for(int i = 0; i < MAX_SONG_PARTS; i++){
      p_SongPartModel = new SongPartModel(mp_SongTracksModel,false, false);
      usedSize = p_SongPartModel->readFromBuffer(p_Buffer, size, p_ParseErrors);

      if((int)usedSize < 0){
         qWarning() << "SongModel::readFromBuffer - ERROR - error while reading part data";
         p_ParseErrors->append(tr("SongModel::readFromBuffer - ERROR - error while reading part %1 data").arg(i));
         delete p_SongPartModel;
         return -1;
      }
      totalUsedSize += usedSize;
      p_Buffer += usedSize;
      size -= usedSize;

      mp_SubParts->append(p_SongPartModel);
   }

   // validate that size = 0
   if(size != 0){
      p_ParseErrors->append(tr("SongModel::readFromBuffer - WARNING - A part of the song structure was not used by parser. The unused size is %1").arg(size));
   }

   return totalUsedSize;
}

uint32_t SongModel::maxInternalSize()
{
   return ((sizeof (SONG_SongStruct)) - ((2+MAX_SONG_PARTS) * (sizeof (SONG_SongPartStruct))));
}

uint32_t SongModel::minInternalSize()
{
   return ((sizeof (SONG_SongStruct)) - ((2+MAX_SONG_PARTS) * (sizeof (SONG_SongPartStruct))));
}

uint8_t *SongModel::internalData()
{
   return (uint8_t *)&m_Song;
}

void SongModel::setLoopSong(uint32_t loopSong)
{
   m_Song.loopSong = loopSong;
}

void SongModel::setBpm(uint32_t bpm)
{
   m_Song.bpm = bpm;
}

void SongModel::insertNewPart(int i)
{
   if(i > (int)m_Song.nPart){
      i = m_Song.nPart;
   }

   if(i >= MAX_SONG_PARTS || i < 0){
      return;
   }

   // Swap the order of the parts
   if(i < (int)m_Song.nPart){
      mp_SubParts->insert(i+2, mp_SubParts->takeAt(m_Song.nPart+2));
   }

   m_Song.nPart++;
}

void SongModel::appendNewPart()
{
   if(m_Song.nPart >= MAX_SONG_PARTS){
      return;
   }


   m_Song.nPart++;
}

void SongModel::deletePart(int i)
{
   if(i >= (int)m_Song.nPart || i < 0){
      return;
   }

   SongPartModel *p_SongPart = (SongPartModel *)mp_SubParts->at(i+2);
   mp_SubParts->removeAt(i+2);

   // Make sure all tracks used by deleted song part are removed if not used somewhere else
   mp_SongTracksModel->removePart(p_SongPart);

   delete p_SongPart;

   // Add empty part at the end
   mp_SubParts->append(new SongPartModel(mp_SongTracksModel,false, false));

   m_Song.nPart--;
}

void SongModel::moveParts(int firstIndex, int lastIndex, int delta)
{
   if(firstIndex > lastIndex){
      qWarning() << "SongModel::moveParts : (firstIndex > lastIndex)";
      return;
   }
   if(lastIndex >= (int)m_Song.nPart || firstIndex < 0){
      qWarning() << "SongModel::moveParts : (lastIndex >= (int)m_Song.nPart || firstIndex < 0)";
      return;
   }

   if(lastIndex + delta >= (int)m_Song.nPart || firstIndex + delta < 0){
      qWarning() << "SongModel::moveParts : (lastIndex + delta >= (int)m_Song.nPart || firstIndex + delta < 0)";
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

uint32_t SongModel::loopSong()
{
   return m_Song.loopSong;
}

uint32_t SongModel::bpm()
{
   return m_Song.bpm;
}

uint32_t SongModel::nPart()
{
   return m_Song.nPart;
}

QString SongModel::defaultDrmFileName()
{
   return (char*)m_Song.defaultDrmName;
}

void SongModel::setDefaultDrmFileName(const QString &defaultDrmFileName)
{
   QByteArray fileNameData = defaultDrmFileName.toLocal8Bit();
   int i;
   for(i = 0; i < fileNameData.count() && i < MAX_DRM_NAME - 1; i++){
      m_Song.defaultDrmName[i] = fileNameData.at(i);
   }

   // make sure 0 terminated (pad remaining of field)
   for(; i < MAX_DRM_NAME; i++){
      m_Song.defaultDrmName[i] = 0;
   }

}


SongPartModel *SongModel::intro()
{
   return (SongPartModel *)mp_SubParts->at(0);
}

SongPartModel *SongModel::outro()
{
   return (SongPartModel *)mp_SubParts->at(1);
}

SongPartModel *SongModel::part(int i)
{
   if(i >= (int)m_Song.nPart || i < 0){
      return nullptr;
   }

   return (SongPartModel *)mp_SubParts->at(i+2);
}
int SongModel::partCount()
{
   return m_Song.nPart;
}

void SongModel::prepareData()
{
   if(m_Song.bpm > MAX_BPM){
      m_Song.bpm = MAX_BPM;
      qWarning() << "SongModel::prepareData : TODO notify file tree of data change";
   } else if (m_Song.bpm < MIN_BPM){
      m_Song.bpm = MIN_BPM;
      qWarning() << "SongModel::prepareData : TODO notify file tree of data change";
   }
}


void SongModel::replaceEffectFile(const QString &originalName, const QString &newName)
{
   if(originalName.compare(newName) == 0){
      return;
   }

   // Loop on all parts except for intro and outro
   for(int i = 0; i < partCount(); i++){
      QString efxFileName = part(i)->effectFileName();
      if(!efxFileName.isEmpty() && efxFileName.compare(originalName) == 0){
         part(i)->setEffectFileName(newName);
      }
   }
}

