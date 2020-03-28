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
