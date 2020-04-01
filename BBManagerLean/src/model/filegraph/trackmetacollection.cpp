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

#include "trackmetacollection.h"
#include "songtrackmetaitem.h"

TrackMetaCollection::TrackMetaCollection()
{
}


uint32_t TrackMetaCollection::readFromBuffer(uint8_t * p_Buffer, uint32_t size, QList<uint32_t> metaOffsetList, QStringList *p_ParseErrors)
{

   uint32_t totalProcessedSize = 0;
   uint32_t processedSize = 0;


   QList<uint32_t> sizeList;

   for(int i = 1; i < metaOffsetList.count(); i++){
      sizeList.append(metaOffsetList.at(i) - metaOffsetList.at(i-1));

   }

   if(metaOffsetList.count() > 0){
      sizeList.append((uint32_t)size - metaOffsetList.last());
      
   }

   qDeleteAll(*mp_SubParts);
   mp_SubParts->clear();

   for(int i = 0; i < metaOffsetList.count(); i++){
      SongTrackMetaItem *p_Meta = new SongTrackMetaItem();
      processedSize = p_Meta->readFromBuffer(p_Buffer + (uint32_t)metaOffsetList.at(i), (uint32_t)sizeList.at(i), p_ParseErrors);
      if((int)processedSize < 0){
         qWarning() << "TrackMetaCollection::readFromBuffer - ERROR - (processedSize < 0)";
         p_ParseErrors->append(tr("TrackMataCollection::readFromBuffer - ERROR - (processedSize < 0)"));
         return processedSize;
      }

      mp_SubParts->append(p_Meta);
      totalProcessedSize += processedSize;

   }

   return totalProcessedSize;
}
