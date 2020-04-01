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
#include <QDebug>
#include <QVariant>
#include <QUuid>

#include "trackarrayitem.h"
#include "../../filegraph/songpartmodel.h"
#include "../abstracttreeitem.h"
#include "trackptritem.h"
#include "songpartitem.h"
#include "songfileitem.h"
#include "effectptritem.h"
#include "../project/beatsprojectmodel.h"
#include "../project/effectfoldertreeitem.h"

TrackArrayItem::TrackArrayItem(const QString & name, int maxChildCnt, const QString &childrenType, int trackType, int loopCount, AbstractTreeItem *parent):
   AbstractTreeItem(parent->model(), parent)
{
   m_Name = name;
   m_MaxChildCnt = maxChildCnt;
   m_ChildrenType = childrenType;
   m_trackType = trackType;
   m_loopCount = loopCount;
}

QVariant TrackArrayItem::data(int column)
{
    switch (column){
        case NAME:
            return m_Name;
        case MAX_CHILD_CNT:
            return m_MaxChildCnt;
        case TEMPO:
            return QVariant();  // Does not apply
        case SHUFFLE:
            if (m_MaxChildCnt <= 1) {
                return QVariant();      // Does not apply
            }
            return static_cast<SongPartModel*>(static_cast<SongPartItem*>(parent())->filePart())->shuffleFlag();
        case CHILDREN_TYPE:
            return m_ChildrenType;
        case TRACK_TYPE:
            if(m_trackType >= 0){
                return m_trackType;
            }
            return QVariant();
        case LOOP_COUNT:
            return m_loopCount;
    }
    return AbstractTreeItem::data(column);
}
bool TrackArrayItem::setData(int column, const QVariant & value)
{
    bool ret = true;
    if(!value.isValid()){
        qWarning() << "TrackArrayItem::setData - (!value.isValid())";
        return false;
    }
    auto pp = parent()->parent();
    switch (column) {
        case NAME:
            m_Name = value.toString();
            pp->setData(SAVE, true); // unsaved changes, handles set dirty
            model()->itemDataChanged(pp, SAVE);
            return true;
        case MAX_CHILD_CNT:
            m_MaxChildCnt = value.toInt(&ret);
            pp->setData(SAVE, true); // unsaved changes, handles set dirty
            model()->itemDataChanged(pp, SAVE);
            return ret;
        case SHUFFLE:
            if (m_MaxChildCnt <= 1) {
                qWarning() << "TrackArrayItem::setData - (m_MaxChildCnt <= 1)";
                return false;
            }
            static_cast<SongPartModel*>(static_cast<SongPartItem*>(parent())->filePart())->setShuffleFlag(value.toBool());
            pp->setData(SAVE, true); // unsaved changes, handles set dirty
            model()->itemDataChanged(pp, SAVE);
            return true;
        case CHILDREN_TYPE:
            m_ChildrenType = value.toString().toUpper();
            pp->setData(SAVE, true); // unsaved changes, handles set dirty
            model()->itemDataChanged(pp, SAVE);
            return true;
        case PLAYING:
            if (!value.toBool()) {
                // clear all children
                for (int i = 0; i < childCount(); ++i){
                    child(i)->setData(PLAYING, false);
                    model()->itemDataChanged(child(i), PLAYING);
                }
            }
            return true;
        case LOOP_COUNT:
            m_loopCount = value.toInt();
            return true;
    default:
        return AbstractTreeItem::setData(column, value);
    }
}


void TrackArrayItem::insertTrackAt(int row, SongTrack *p_SongTrackModel, bool init)
{
   SongTracksModel *p_SongTracksModel = static_cast<SongPartModel*>(static_cast<SongPartItem*>(parent())->filePart())->songTracksModel();

   TrackPtrItem *p_Track = new TrackPtrItem(this, p_SongTracksModel, p_SongTrackModel);
   connect(p_Track, SIGNAL(sigFileSet(SongTrack *, int)), this, SLOT(slotFileSet(SongTrack *, int)));
   insertChildAt(row, p_Track);

   if(!init){
      parent()->parent()->setData(SAVE, QVariant(true)); 
      model()->itemDataChanged(parent()->parent(), SAVE);
   }
}


void TrackArrayItem::insertTracksAt(int row, const QList<SongTrack *> &songTrackModelList, bool init)
{
   bool firstTrack = (childCount() == 0);

   SongTracksModel *p_SongTracksModel = static_cast<SongPartModel*>(static_cast<SongPartItem*>(parent())->filePart())->songTracksModel();
   TrackPtrItem *p_Track;
   for(int i = 0; i < songTrackModelList.count(); i++){
      p_Track = new TrackPtrItem(this, p_SongTracksModel, songTrackModelList.at(i));
      connect(p_Track, SIGNAL(sigFileSet(SongTrack *, int)), this, SLOT(slotFileSet(SongTrack *, int)));
      insertChildAt(row + i, p_Track);
   }

   if(!init){
      parent()->parent()->setData(SAVE, QVariant(true)); 
      model()->itemDataChanged(parent()->parent(), SAVE);
   }

   if(firstTrack){
      emit sigValidityChange();
   }
}


void TrackArrayItem::insertEffectAt(int row, QString effectFileName, bool init)
{
   EffectPtrItem * p_EffectPtrItem = new EffectPtrItem(this, model()->effectFolder(), model()->effectFolder()->effectWithFileName(effectFileName));
   insertChildAt(row, p_EffectPtrItem);

   if(!init){
      parent()->parent()->setData(SAVE, QVariant(true)); // unsaved changes, handles set dirty
      model()->itemDataChanged(parent()->parent(), SAVE);
   }
}

void TrackArrayItem::insertNewChildAt(int row)
{
   QTextStream(stdout) << "TrackArrayItem::insertNewChildAt" << endl;
   bool firstTrack = (childCount() == 0);

   if(m_ChildrenType.compare(BMFILES_WAVE_EXTENSION, Qt::CaseInsensitive) == 0){
      QTextStream(stdout) << "   WAV" << endl;

      EffectPtrItem * p_EffectPtrItem = new EffectPtrItem(this, model()->effectFolder());
      insertChildAt(row, p_EffectPtrItem);

      QTextStream(stdout) << "   WAV - DONE" << endl;

   } else {
      QTextStream(stdout) << "   MID" << endl;

      SongTracksModel *p_SongTracksModel = static_cast<SongPartModel*>(static_cast<SongPartItem*>(parent())->filePart())->songTracksModel();

      TrackPtrItem *p_Track = new TrackPtrItem(this, p_SongTracksModel);
      connect(p_Track, SIGNAL(sigFileSet(SongTrack *, int)), this, SLOT(slotFileSet(SongTrack *, int)));
      insertChildAt(row, p_Track);
   }

   parent()->parent()->setData(SAVE, QVariant(true)); // unsaved changes, handles set dirty
   model()->itemDataChanged(parent()->parent(), SAVE);
   if(firstTrack){
      emit sigValidityChange();
   }
}

void TrackArrayItem::slotFileSet(SongTrack *p_Track, int row)
{
   emit sigFileSet(p_Track, row);
}

void TrackArrayItem::removeChild(int row)
{
   // 1 - Clear Ptr if possible
   EffectPtrItem * p_EffectPtrItem = qobject_cast<EffectPtrItem *>(child(row));
   if(p_EffectPtrItem){
      p_EffectPtrItem->clearEffectUsage();
   }

   // 2 - Delete in File Graph
   emit sigFileRemoved(row);
   // 3 - Delete in Tree
   removeChildInternal(row);

   parent()->parent()->setData(SAVE, QVariant(true)); // unsaved changes, handles set dirty
   model()->itemDataChanged(parent()->parent(), SAVE);

   // Notify that validity may have changed
   if(childCount() == 0){
      emit sigValidityChange();
   }
}

/**
 * @brief TrackArrayItem::clearEffectUsage
 *
 * Called when part or effect is being deleted
 */
void TrackArrayItem::clearEffectUsage()
{
   for(int i = 0; i < childCount(); i++){
      EffectPtrItem * p_EffectPtrItem = qobject_cast<EffectPtrItem *>(child(i));
      if(p_EffectPtrItem){
         p_EffectPtrItem->clearEffectUsage();
      }
   }
}

