#ifndef SONGTRACKINDEXITEM_H
#define SONGTRACKINDEXITEM_H

#include "songfile.h"
#include "abstractfilepartmodel.h"

class SongTrackIndexItem : public AbstractFilePartModel
{
   Q_OBJECT
public:
   explicit SongTrackIndexItem();

   void setCRC(uint32_t crc);
   uint32_t crc() const;

   void setDataOffset(uint32_t offset);
   uint32_t dataOffset() const;

   void setMetaOffset(uint32_t offset);
   uint32_t metaOffset() const;

   void print();

protected:
   virtual uint32_t maxInternalSize();
   virtual uint32_t minInternalSize();
   virtual uint8_t *internalData();

private:
   SONGFILE_TrackIndexingItemStruct m_TrackIndexingItem;

};

#endif // SONGTRACKINDEXITEM_H
