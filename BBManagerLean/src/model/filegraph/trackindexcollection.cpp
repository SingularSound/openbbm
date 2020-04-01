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

#include "trackindexcollection.h"
#include "songtrackindexitem.h"

TrackIndexCollection::TrackIndexCollection()
{
}

uint32_t TrackIndexCollection::readFromBuffer(uint8_t * p_Buffer, uint32_t size, QStringList *p_ParseErrors)
{

   uint32_t totalProcessedSize = 0;
   uint32_t processedSize = 0;
   uint32_t remainingSize = size;

   qDeleteAll(*mp_SubParts);
   mp_SubParts->clear();


   while(remainingSize > 0){
      SongTrackIndexItem *p_Index = new SongTrackIndexItem();
      processedSize = p_Index->readFromBuffer(p_Buffer, remainingSize, p_ParseErrors);

      if((int)processedSize < 0){
         qWarning() << "TrackIndexCollection::readFromBuffer - ERROR - (processedSize < 0)";
         p_ParseErrors->append(tr("TrackIndexCollection::readFromBuffer - ERROR - (processedSize < 0)"));
         return processedSize;
      }

      mp_SubParts->append(p_Index);

      totalProcessedSize += processedSize;
      p_Buffer += processedSize;
      remainingSize -= processedSize;
   }

   return totalProcessedSize;
}

QList<uint32_t> TrackIndexCollection::metaOffsetList()
{
   QList<uint32_t> list;
   for(int i = 0; i < mp_SubParts->count(); i++){
      list.append(static_cast<SongTrackIndexItem *>(mp_SubParts->at(i))->metaOffset());
   }
   return list;
}

QList<uint32_t> TrackIndexCollection::dataOffsetList()
{
   QList<uint32_t> list;
   for(int i = 0; i < mp_SubParts->count(); i++){
      list.append(static_cast<SongTrackIndexItem *>(mp_SubParts->at(i))->dataOffset());
   }
   return list;
}
