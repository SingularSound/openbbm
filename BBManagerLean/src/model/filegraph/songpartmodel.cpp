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

#include "songpartmodel.h"

#include "songtracksmodel.h"
#include "songtrack.h"

SongPartModel::SongPartModel(SongTracksModel *p_SongTracksModel, bool isIntro, bool isOutro) :
   AbstractFilePartModel()
{

   mp_SongTracksModel = p_SongTracksModel;
   m_Intro    = isIntro;
   m_Outro    = (isIntro?false:isOutro); // Mutually exclusive, priority to intro
   m_SongPart = ((isIntro || isOutro)?0:1);


   m_Default_Name = tr("SongPartModel");
   m_Name = m_Default_Name;
   m_InternalSize = minInternalSize();

   mp_MainLoop = nullptr;
   mp_TransFill = nullptr;
}

uint32_t SongPartModel::readFromBuffer(uint8_t * p_Buffer, uint32_t size, QStringList *p_ParseErrors)
{

   uint32_t usedSize = AbstractFilePartModel::readFromBuffer(p_Buffer, size, p_ParseErrors);

   // resolve links to mp_SongTracksModel
   if(m_SongPart.mainLoopIndex >= 0){
      mp_MainLoop = mp_SongTracksModel->trackAtIndex(m_SongPart.mainLoopIndex);
      if(mp_MainLoop != nullptr){
         mp_MainLoop->addUser(this);
      } else {
         qWarning() << QString("SongPartModel::readFromBuffer - ERROR - Track Index %1 used for MainLoop doesn not exist").arg(m_SongPart.mainLoopIndex);
         p_ParseErrors->append(tr("SongPartModel::readFromBuffer - ERROR - Track Index %1 used for MainLoop doesn not exist").arg(m_SongPart.mainLoopIndex));
      }
   }

   if(m_SongPart.transFillIndex >= 0){
      mp_TransFill = mp_SongTracksModel->trackAtIndex(m_SongPart.transFillIndex);
      if(mp_MainLoop != nullptr){
         mp_TransFill->addUser(this);
      } else {
         qWarning() << QString("SongPartModel::readFromBuffer - ERROR - Track Index %1 used for TransFill doesn not exist").arg(m_SongPart.mainLoopIndex);
         p_ParseErrors->append(tr("SongPartModel::readFromBuffer - ERROR - Track Index %1 used for TransFill doesn not exist").arg(m_SongPart.transFillIndex));
      }
   }

   for(int i = 0; i < (int)m_SongPart.nDrumFill; i++){
      if(m_SongPart.drumFillIndex[i] < 0){
         qWarning() << "SongPartModel::readFromBuffer - ERROR - Part doesn't contain nDrumFill = " << m_SongPart.nDrumFill << " valid drumfills";
         p_ParseErrors->append(tr("SongPartModel::readFromBuffer - ERROR - Part doesn't contain nDrumFill = %1 valid drumfills").arg(m_SongPart.nDrumFill));
         return -1;
      }

      SongTrack *p_DrumFill = mp_SongTracksModel->trackAtIndex(m_SongPart.drumFillIndex[i]);
      if(p_DrumFill != nullptr){
         m_DrumFills.append(p_DrumFill);
         m_DrumFills.last()->addUser(this);
      } else {
         qWarning() << QString("SongPartModel::readFromBuffer - ERROR - Track Index %1 used for DrumFill %2 doesn not exist").arg(m_SongPart.drumFillIndex[i]).arg(i);
         p_ParseErrors->append(tr("SongPartModel::readFromBuffer - ERROR - Track Index %1 used for DrumFill %2 doesn not exist").arg(m_SongPart.drumFillIndex[i]).arg(i));
      }

   }

   return usedSize;
}

void SongPartModel::prepareData()
{
   QTextStream(stdout) << "SongPartModel::prepareData()" << endl;
   QTextStream(stdout) << "   mp_MainLoop = " <<  mp_MainLoop << ", this = " << this << endl;
   if(mp_MainLoop){
      QTextStream(stdout) << "   mp_MainLoop->index() = " <<  mp_MainLoop->index() << endl;
      m_SongPart.mainLoopIndex = mp_MainLoop->index();
   } else {
      m_SongPart.mainLoopIndex = -1;
   }

   QTextStream(stdout) << "   mp_TransFill = " <<  mp_TransFill << endl;
   if(mp_TransFill){
      QTextStream(stdout) << "   mp_TransFill->index() = " <<  mp_TransFill->index() << endl;
      m_SongPart.transFillIndex = mp_TransFill->index();
   } else {
      m_SongPart.transFillIndex = -1;
   }

   m_SongPart.nDrumFill = m_DrumFills.size();
   for(int i = 0; i < MAX_DRUM_FILLS; i++){
      if(i < (int)m_SongPart.nDrumFill){
         m_SongPart.drumFillIndex[i] = m_DrumFills.at(i)->index();
      } else {
         m_SongPart.drumFillIndex[i] = -1;
      }
   }
}

uint32_t SongPartModel::maxInternalSize()
{
   return sizeof (SONG_SongPartStruct);
}

uint32_t SongPartModel::minInternalSize()
{
   return sizeof (SONG_SongPartStruct);
}

uint8_t *SongPartModel::internalData()
{
   return (uint8_t *)&m_SongPart;
}

void SongPartModel::setRepeatFlag(bool repeat)
{
   m_SongPart.repeatFlag = repeat?1:0;
}

void SongPartModel::setShuffleFlag(bool shuffle)
{
   m_SongPart.shuffleFlag = shuffle?1:0;
}

void SongPartModel::setBpmDelta(int32_t delta)
{
   m_SongPart.bpmDelta = delta;
}

void SongPartModel::setTimeSig(uint32_t num, uint32_t den)
{
   m_SongPart.timeSigNum = num;
   m_SongPart.timeSigDen = den;
}

void SongPartModel::setTicksPerBar(uint32_t ticks)
{
   m_SongPart.tickPerBar = ticks;
}

void SongPartModel::setLoopCount(uint32_t count)
{
   m_SongPart.loopCount = count;
}

void SongPartModel::setPartName(QString partName)
{
  m_Name = partName;
}

void SongPartModel::setMainLoop(SongTrack * p_Track, int /*position*/)
{

   if(mp_MainLoop){
      if(mp_MainLoop == p_Track){
         
         return;
      }
      mp_MainLoop->removeUser(this);
      mp_SongTracksModel->refreshTrackUsage(mp_MainLoop);
   }
   p_Track->addUser(this);
   mp_MainLoop = p_Track;
}

void SongPartModel::setTransFill(SongTrack * p_Track, int /*position*/)
{
   if (m_Intro || m_Outro){
      mp_SongTracksModel->refreshTrackUsage(p_Track);
      return;
   }

   if(mp_TransFill){
      if(mp_TransFill == p_Track){
         
         return;
      }
      mp_TransFill->removeUser(this);
      mp_SongTracksModel->refreshTrackUsage(mp_TransFill);
   }

   p_Track->addUser(this);
   mp_TransFill = p_Track;
}

void SongPartModel::setDrumFill(SongTrack * p_Track, int position)
{
   if (m_Intro || m_Outro){
      mp_SongTracksModel->refreshTrackUsage(p_Track);
      return;
   }
   if (m_DrumFills.size() == position){
      appendDrumFill(p_Track, position);
   } else if (m_DrumFills.size() > position){
      replaceDrumFill(p_Track, position);
   }

   mp_SongTracksModel->refreshTrackUsage(p_Track);
   return;
}

void SongPartModel::appendDrumFill(SongTrack * p_Track, int /*position*/)
{
   if (m_Intro || m_Outro){
      mp_SongTracksModel->refreshTrackUsage(p_Track);
      return;
   }

   if(m_DrumFills.size() >= MAX_DRUM_FILLS){
      mp_SongTracksModel->refreshTrackUsage(p_Track);
      return;
   }

   p_Track->addUser(this);
   m_DrumFills.append(p_Track);
}

void SongPartModel::replaceDrumFill(SongTrack * p_NewTrack, int position)
{
   if (m_Intro || m_Outro){
      mp_SongTracksModel->refreshTrackUsage(p_NewTrack);
      return;
   }

   // NOTE: we use position instead of pointer in order to handle multiple use of a track in the same part
   if (m_DrumFills.size() <= position){
      mp_SongTracksModel->refreshTrackUsage(p_NewTrack);
      return;
   }

   SongTrack * p_PreviousTrack = m_DrumFills.at(position);

   if(p_PreviousTrack == p_NewTrack){
      
      return;
   }

   m_DrumFills.removeAt(position);

   p_PreviousTrack->removeUser(this);
   mp_SongTracksModel->refreshTrackUsage(p_PreviousTrack);


   p_NewTrack->addUser(this);
   m_DrumFills.insert(position, p_NewTrack);
}

void SongPartModel::setMainLoop(const QString &/*midiFilePath*/)
{
   qWarning() << "TODO : SongPartModel::setMainLoop";

}

void SongPartModel::deleteMainLoop(int /*position*/)
{
   if(mp_MainLoop){
      mp_MainLoop->removeUser(this);
      mp_SongTracksModel->refreshTrackUsage(mp_MainLoop);
   }
   mp_MainLoop = nullptr;
}

void SongPartModel::setTransFill(const QString &/*midiFilePath*/)
{
   if (m_Intro || m_Outro){
      return;
   }
   qWarning() << "TODO : SongPartModel::setTransFill";
}

void SongPartModel::deleteTransFill(int /*position*/)
{
   if(mp_TransFill){
      mp_TransFill->removeUser(this);
      mp_SongTracksModel->refreshTrackUsage(mp_TransFill);
   }
   mp_TransFill = nullptr;
}
void SongPartModel::appendDrumFill(const QString &/*midiFilePath*/)
{
   if (m_Intro || m_Outro){
      return;
   }

   if(m_SongPart.nDrumFill >= MAX_DRUM_FILLS){
      return;
   }
   qWarning() << "TODO : SongPartModel::appendDrumFill";
}

void SongPartModel::deleteDrumFill(int position)
{
   // NOTE: we use position instead of track pointer in order to handle multiple use of a track in the same part
   if (m_DrumFills.size() <= position){
      return;
   }

   SongTrack * p_Track = m_DrumFills.at(position);
   m_DrumFills.removeAt(position);

   p_Track->removeUser(this);
   mp_SongTracksModel->refreshTrackUsage(p_Track);
}

void SongPartModel::deleteEffect(int /*position*/)
{
   for(int i = 0; i < MAX_EFFECT_NAME; i++){
      m_SongPart.effectName[i] = 0;
   }
}


bool SongPartModel::repeatFlag()
{
   return m_SongPart.repeatFlag != 0;
}

bool SongPartModel::shuffleFlag()
{
   return m_SongPart.shuffleFlag != 0;
}

int32_t SongPartModel::bpmDelta()
{
   return m_SongPart.bpmDelta;
}

uint32_t SongPartModel::timeSigNum()
{
   return m_SongPart.timeSigNum;
}

uint32_t SongPartModel::timeSigDen()
{
   return m_SongPart.timeSigDen;
}

uint32_t SongPartModel::ticksPerBar()
{
   return m_SongPart.tickPerBar;
}

uint32_t SongPartModel::loopCount()
{
    return m_SongPart.loopCount;
}

uint32_t SongPartModel::mainLoopIndex()
{
   return m_SongPart.mainLoopIndex;
}

SongTrack *SongPartModel::mainLoop()
{
   if(m_SongPart.mainLoopIndex < 0){
      return nullptr;
   }

   // returns null if out of bounds
   return mp_SongTracksModel->trackAtIndex(m_SongPart.mainLoopIndex);
}


// TEST
SongTrack *SongPartModel::mainTrackPtr(){

   return mp_MainLoop;
}

uint32_t SongPartModel::transFillIndex()
{
   return m_SongPart.transFillIndex;
}

SongTrack *SongPartModel::transFill()
{
   // returns null if out of bounds
   if(m_SongPart.transFillIndex < 0){
      return nullptr;
   }
   return mp_SongTracksModel->trackAtIndex(m_SongPart.transFillIndex);
}


const uint32_t *SongPartModel::drumFillIndexes()
{
   return (uint32_t *)m_SongPart.drumFillIndex;
}

uint32_t SongPartModel::drumFillIndex(uint32_t index)
{
   if(index >= MAX_DRUM_FILLS){
      return -1;
   }

   return m_SongPart.drumFillIndex[index];
}

SongTrack *SongPartModel::drumFill(uint32_t index)
{
   if(index >= MAX_DRUM_FILLS){
      return nullptr;
   }
   if(m_SongPart.drumFillIndex[index] < 0){
      return nullptr;
   }

   // returns null if out of bounds
   return mp_SongTracksModel->trackAtIndex(m_SongPart.drumFillIndex[index]);
}
const QList<SongTrack *> &SongPartModel::drumFillList()
{
    return m_DrumFills;
}

uint32_t SongPartModel::nbDrumFills()
{
    return m_DrumFills.size();
}

bool SongPartModel::isIntro()
{
   return m_Intro;
}

bool SongPartModel::isOutro()
{
   return m_Outro;
}

void SongPartModel::setEffectFileName(const QString &fileName)
{
   const QByteArray& hold_this_byte_array_ffs = fileName.toLocal8Bit();
   const char* charPtr = hold_this_byte_array_ffs.data();

   // "<=" to copy the extra character '\0'
   for(int i = 0; i <= fileName.count() && i < (MAX_EFFECT_NAME - 1); i++){
      m_SongPart.effectName[i] = charPtr[i];
   }
}

QString SongPartModel::effectFileName()
{
   return (char*)m_SongPart.effectName;
}

