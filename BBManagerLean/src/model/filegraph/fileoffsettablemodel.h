#ifndef FILEOFFSETTABLEMODEL_H
#define FILEOFFSETTABLEMODEL_H

#include "songfile.h"
#include "abstractfilepartmodel.h"

class SongTracksModel;

class FileOffsetTableModel : public AbstractFilePartModel
{
   Q_OBJECT
public:
   explicit FileOffsetTableModel();

   void adjustOffsets(SongTracksModel * p_Tracks);

   uint32_t metaOffset() const;
   uint32_t metaSize() const;
   uint32_t songOffset() const;
   uint32_t songSize() const;
   uint32_t tracksIndexingOffset() const;
   uint32_t tracksIndexingSize() const;
   uint32_t tracksMetaOffset() const;
   uint32_t tracksMetaSize() const;
   uint32_t tracksDataOffset() const;
   uint32_t tracksDataSize() const;
   uint32_t autoPilotOffset() const;
   uint32_t autoPilotSize() const;

   void setMetaOffset(uint32_t offset);
   void setMetaSize(uint32_t size);
   void setSongOffset(uint32_t offset);
   void setSongSize(uint32_t size);
   void setTracksIndexingOffset(uint32_t offset);
   void setTracksIndexingSize(uint32_t size);
   void setTracksMetaOffset(uint32_t offset);
   void setTracksMetaSize(uint32_t size);
   void setTracksDataOffset(uint32_t offset);
   void setTracksDataSize(uint32_t size);
   void setAutoPilotOffset(uint32_t offset);
   void setAutoPilotSize(uint32_t size);

protected:
   virtual uint32_t maxInternalSize();
   virtual uint32_t minInternalSize();
   virtual uint8_t *internalData();

private:
   SONGFILE_OffsetTableStruct m_OffsetTable;


};

#endif // FILEOFFSETTABLEMODEL_H
