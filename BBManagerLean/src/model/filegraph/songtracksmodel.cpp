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
#include "songtracksmodel.h"
#include "songtrackindexitem.h"
#include "songtrackmetaitem.h"
#include "songtrackdataitem.h"
#include "filepartcollection.h"
#include "songpartmodel.h"
#include "fileoffsettablemodel.h"
#include "trackdatacollection.h"
#include "trackindexcollection.h"
#include "trackmetacollection.h"
#include "../beatsmodelfiles.h"

#include "songtrack.h"

#include <QDebug>
#include <QFileInfo>

SongTracksModel::SongTracksModel() :
   AbstractFilePartModel()
{
   m_InternalSize = 0;
   m_Default_Name = tr("SongTracksModel");
   m_Name = m_Default_Name;

   mp_SubParts->append(new TrackIndexCollection);
   mp_SubParts->append(new TrackMetaCollection);
   mp_SubParts->append(new TrackDataCollection);

   mp_SongTracks = new QList<SongTrack *>;
}

SongTracksModel::~SongTracksModel()
{
   for(int i = mp_SongTracks->size()-1; i >= 0; i--){
      delete mp_SongTracks->at(i);
   }
   delete mp_SongTracks;
}

uint32_t SongTracksModel::readFromBuffer(uint8_t * p_FullFileBuffer, FileOffsetTableModel *p_OffsetTable, QStringList *p_ParseErrors)
{
   uint32_t totalProcessedSize = 0;

   // TODO make sure it is already offset table is already validated

   totalProcessedSize += trackIndexes()->readFromBuffer(p_FullFileBuffer + (uint32_t)p_OffsetTable->tracksIndexingOffset(), p_OffsetTable->tracksIndexingSize(), p_ParseErrors);

   // 2 - read track Meta
   totalProcessedSize += trackMeta()->readFromBuffer(p_FullFileBuffer + (uint32_t)p_OffsetTable->tracksMetaOffset(), p_OffsetTable->tracksMetaSize(), trackIndexes()->metaOffsetList(), p_ParseErrors);
   // 3 - read track Data
   totalProcessedSize +=trackData()->readFromBuffer(p_FullFileBuffer + (uint32_t)p_OffsetTable->tracksDataOffset(), p_OffsetTable->tracksDataSize(), trackIndexes()->dataOffsetList(), p_ParseErrors);

   // 4 - delete all tracks that existed
   qDeleteAll(*mp_SongTracks);
   mp_SongTracks->clear();

   // 5 - create tracks with new data
   for(int i = 0; i < trackIndexes()->count(); i++){
      SongTrackIndexItem *p_trackIndexes = static_cast<SongTrackIndexItem *>(trackIndexes()->childAt(i));
      SongTrackMetaItem  *p_trackMeta    = static_cast<SongTrackMetaItem  *>(trackMeta()->childAt(i));
      SongTrackDataItem  *p_trackData    = static_cast<SongTrackDataItem  *>(trackData()->childAt(i));


      mp_SongTracks->append(new SongTrack(i, p_trackIndexes, p_trackMeta, p_trackData));
   }

   return totalProcessedSize;
}

uint32_t SongTracksModel::maxInternalSize()
{
   return 0;
}

uint32_t SongTracksModel::minInternalSize()
{
   return 0;
}

uint8_t *SongTracksModel::internalData()
{
   return nullptr;
}

void SongTracksModel::adjustOffsets()
{
   
   if(trackIndexes()->count() > 0){
      static_cast<SongTrackIndexItem *>(trackIndexes()->childAt(0))->setMetaOffset(0);
      static_cast<SongTrackIndexItem *>(trackIndexes()->childAt(0))->setDataOffset(0);
   }

   for(int i = 1; i < trackIndexes()->count(); i++){
      uint32_t dataOffset = static_cast<SongTrackIndexItem *>(trackIndexes()->childAt(i-1))->dataOffset();
      dataOffset += (uint32_t)trackData()->childAt(i-1)->size();
      static_cast<SongTrackIndexItem *>(trackIndexes()->childAt(i))->setDataOffset((uint32_t)dataOffset);

      uint32_t metaOffset = static_cast<SongTrackIndexItem *>(trackIndexes()->childAt(i-1))->metaOffset();
      metaOffset += trackMeta()->childAt(i-1)->size();
      static_cast<SongTrackIndexItem *>(trackIndexes()->childAt(i))->setMetaOffset((uint32_t)metaOffset);
   }
}

SongTrack *SongTracksModel::createTrack(const QString &fileName, int trackType, QStringList *p_ParseErrors)
{
   SongTrack *p_Track = new SongTrack(mp_SongTracks->size());

   // Parse file if it is a midi or a track file
   if(QFileInfo(fileName).suffix().compare(BMFILES_SONG_TRACK_EXTENSION, Qt::CaseInsensitive) == 0 ){
       // Track file
       if(!p_Track->importTrackFile(fileName, p_ParseErrors)){
          delete p_Track;
          return nullptr;
       }
   } else {
       // Midi file
       if(!p_Track->parseMidiFile(fileName, trackType, p_ParseErrors)){
          delete p_Track;
          return nullptr;
       }
   }

   // Verify if this Track exists (comparing CRC)
   for(int i = 0; i < mp_SongTracks->size(); i++){
      // If track exists
      if(mp_SongTracks->at(i)->trackIndexes()->crc() == p_Track->trackIndexes()->crc()){
         // Delete newly created track
         delete p_Track;
         return mp_SongTracks->at(i);
      }
   }

   // set the offset values in indexes
   if (mp_SongTracks->size() > 0){
      p_Track->ajustOffsets(mp_SongTracks->at(mp_SongTracks->size() - 1));
   } else {
      p_Track->ajustOffsets(nullptr);
   }

   // Add track content to internal lists
   mp_SongTracks->append(p_Track);
   trackIndexes()->append(p_Track->trackIndexes());

   trackMeta()->append(p_Track->trackMeta());
   trackData()->append(p_Track->trackData());

   // indexes, meta, and data are now responsible of deleting subparts
   p_Track->setDeleteSubParts(false);

   return p_Track;
}

void SongTracksModel::removePart(SongPartModel * p_Part){
   for(int i = mp_SongTracks->size() - 1; i >= 0; i--){
      mp_SongTracks->at(i)->removeAllUser(p_Part);
      refreshTrackUsage(mp_SongTracks->at(i));
   }
}

void SongTracksModel::refreshTrackUsage(SongTrack * p_Track)
{
   if(p_Track->userCount() > 0){
      return;
   }

   int trackIndex = p_Track->index();

   // Remove track
   // 1- Remove index
   trackIndexes()->removeAt(trackIndex);

   // 2- Remove meta data
   trackMeta()->removeAt(trackIndex);

   // 3- Remove data
   trackData()->removeAt(trackIndex);

   // 4- track itself
   mp_SongTracks->removeAt(trackIndex);

   // 5- refresh following tracks indexes
   for(int i = trackIndex; i < mp_SongTracks->size(); i++){
      mp_SongTracks->at(i)->setIndex(i);
   }

   // 6- delete track
   delete p_Track;
}



SongTrack *SongTracksModel::trackAtIndex(int i)
{
   if (i < mp_SongTracks->size() && i>=0){
      return mp_SongTracks->at(i);
   }
   qWarning() << "SongTracksModel::trackAtIndex - ERROR 1 - track at index " << i << " does not exist (mp_SongTracks->size() = " << mp_SongTracks->size() << ")";
   return nullptr;
}

TrackIndexCollection * SongTracksModel::trackIndexes()
{
   return (TrackIndexCollection *) mp_SubParts->at(0);
}

TrackMetaCollection * SongTracksModel::trackMeta()
{
   return (TrackMetaCollection *)mp_SubParts->at(1);
}

TrackDataCollection * SongTracksModel::trackData()
{
   return (TrackDataCollection *) mp_SubParts->at(2);
}

