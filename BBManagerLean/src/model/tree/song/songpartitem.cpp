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
#include "../project/beatsprojectmodel.h"
#include "songpartitem.h"
#include "trackarrayitem.h"
#include "../../filegraph/midiParser.h"
#include "../../beatsmodelfiles.h"

#include <QDebug>

SongPartItem::SongPartItem(SongPartModel * songPart, AbstractTreeItem *parent):
   FilePartItem(songPart, parent),
   m_playing(false)
{
   TrackArrayItem *p_TrackArrayItem;

   QString trackExtensions = QString("%1,%2").arg(BMFILES_MIDI_EXTENSION, BMFILES_SONG_TRACK_EXTENSION);
   QString accentHitExtensions(BMFILES_WAVE_EXTENSION);

   // create the item tree
   if(songPart->isIntro() || songPart->isOutro()){
      p_TrackArrayItem = new TrackArrayItem(tr("Dummy"), 0, nullptr, -1, 0, this);
      appendChild(p_TrackArrayItem);

      p_TrackArrayItem = new TrackArrayItem(songPart->isIntro() ? tr("Intro Fill") : tr("Outro Fill"), 1, trackExtensions, songPart->isIntro()?INTRO_FILL:OUTRO_FILL, 0, this);
      
      if(songPart->mainLoop()){
         p_TrackArrayItem->insertTrackAt(0, songPart->mainLoop());
      }
      connect(p_TrackArrayItem, SIGNAL(sigFileSet(SongTrack *, int)), songPart, SLOT(setMainLoop(SongTrack *, int)));
      connect(p_TrackArrayItem, SIGNAL(sigFileRemoved(int)), songPart, SLOT(deleteMainLoop(int)));
      appendChild(p_TrackArrayItem);

      p_TrackArrayItem = new TrackArrayItem(tr("Dummy"), 0, nullptr, -1, 0, this);
      appendChild(p_TrackArrayItem);

      p_TrackArrayItem = new TrackArrayItem(tr("Dummy"), 0, nullptr, -1, 0, this);
      appendChild(p_TrackArrayItem);
   } else {
      p_TrackArrayItem = new TrackArrayItem(tr("Main Loop"), 1, trackExtensions, MAIN_DRUM_LOOP, 0, this);
      
      if(songPart->mainLoop()){
         p_TrackArrayItem->insertTrackAt(0, songPart->mainLoop());
      }
      connect(p_TrackArrayItem, SIGNAL(sigFileSet(SongTrack *, int)), songPart, SLOT(setMainLoop(SongTrack *, int)));
      connect(p_TrackArrayItem, SIGNAL(sigFileRemoved(int)), songPart, SLOT(deleteMainLoop(int)));
      connect(p_TrackArrayItem, SIGNAL(sigValidityChange()), this, SLOT(slotValidityChanged()));
      appendChild(p_TrackArrayItem);

      p_TrackArrayItem = new TrackArrayItem(tr("Drum Fill"), MAX_DRUM_FILLS, trackExtensions, DRUM_FILL, 0, this);
      if(songPart->drumFillList().count() > 0){
         p_TrackArrayItem->insertTracksAt(0, songPart->drumFillList());
      }
      connect(p_TrackArrayItem, SIGNAL(sigFileSet(SongTrack *, int)), songPart, SLOT(setDrumFill(SongTrack *, int)));
      connect(p_TrackArrayItem, SIGNAL(sigFileRemoved(int)), songPart, SLOT(deleteDrumFill(int)));
      appendChild(p_TrackArrayItem);

      p_TrackArrayItem = new TrackArrayItem(tr("Transition Fill"), 1, trackExtensions, TRANS_FILL, 0, this);
      
      if(songPart->transFill()){
         p_TrackArrayItem->insertTrackAt(0, songPart->transFill());
      }
      connect(p_TrackArrayItem, SIGNAL(sigFileSet(SongTrack *, int)), songPart, SLOT(setTransFill(SongTrack *, int)));
      connect(p_TrackArrayItem, SIGNAL(sigFileRemoved(int)), songPart, SLOT(deleteTransFill(int)));
      appendChild(p_TrackArrayItem);

      p_TrackArrayItem = new TrackArrayItem(tr("Accent Hit"), 1, accentHitExtensions, -1, 0, this);
      appendChild(p_TrackArrayItem);
      connect(p_TrackArrayItem, SIGNAL(sigFileRemoved(int)), songPart, SLOT(deleteEffect(int)));
      if(!songPart->effectFileName().isEmpty()){
         p_TrackArrayItem->insertEffectAt(0,songPart->effectFileName());
      }

      songPart->setLoopCount(loopCount());
   }
}

QVariant SongPartItem::data(int column)
{
   switch (column){
      case PLAYING:
         return m_playing;
      case INVALID:
        if (auto part = (SongPartModel*)filePart()) {
            // Intro and Outro are always valid during validity checkup
            // Check if there is a main loop for the song
            if (!part->isIntro() && !part->isOutro() && child(0)->childCount() != 1) {
                return tr("missing Main Drum Loop");
            }
        }
        return QVariant();
      case LOOP_COUNT:
        return ((SongPartModel*)filePart())->loopCount();
      default:
         return FilePartItem::data(column);
   }
}

bool SongPartItem::setData(int column, const QVariant & value)
{
   switch(column){
      case PLAYING:
         m_playing = value.toBool();
         if(!value.toBool()){
            // clear all children
            for(int i = 0; i < childCount(); i++){
               child(i)->setData(PLAYING, false);
               model()->itemDataChanged(child(i), PLAYING);
            }
         }
         return true;
      case LOOP_COUNT:
         ((SongPartModel*)filePart())->setLoopCount(value.toInt());
             parent()->setData(SAVE, QVariant(true)); // unsaved changes, handles set dirty
             model()->itemDataChanged(parent(), SAVE);
             return true;
      default:
         if(FilePartItem::setData(column, value)){
            parent()->setData(SAVE, QVariant(true)); // unsaved changes, handles set dirty
            model()->itemDataChanged(parent(), SAVE);
            return true;
         }

         return false;
   }


}

void SongPartItem::setEffectFileName(const QString &fileName, bool init)
{
   SongPartModel *songPart = static_cast<SongPartModel *>(filePart());
   songPart->setEffectFileName(fileName);
   if(!init){
      parent()->setData(SAVE, QVariant(true)); // unsaved changes, handles set dirty
      model()->itemDataChanged(parent(), SAVE);
   }
}

QString SongPartItem::effectFileName()
{
   SongPartModel *songPart = static_cast<SongPartModel *>(filePart());
   return songPart->effectFileName();
}

/**
 * @brief SongPartItem::clearEffectUsage
 *
 * called when part is being deleted
 */
void SongPartItem::clearEffectUsage()
{
   SongPartModel *songPart = static_cast<SongPartModel *>(filePart());
   if(!songPart->isIntro() && !songPart->isOutro()){
      static_cast<TrackArrayItem *>(childItems()->last())->clearEffectUsage();
   }
}

void SongPartItem::slotValidityChanged()
{
   model()->itemDataChanged(this, INVALID);
}



uint32_t SongPartItem::timeSigNum()
{
    return static_cast<SongPartModel *>(filePart())->timeSigNum();
}

uint32_t SongPartItem::timeSigDen()
{
    return static_cast<SongPartModel *>(filePart())->timeSigNum();
}

uint32_t SongPartItem::ticksPerBar()
{
    return static_cast<SongPartModel *>(filePart())->ticksPerBar();
}

uint32_t SongPartItem::loopCount()
{
    return static_cast<SongPartModel *>(filePart())->loopCount();
}
