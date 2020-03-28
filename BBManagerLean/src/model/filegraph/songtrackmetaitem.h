#ifndef SONGTRACKMETAITEM_H
#define SONGTRACKMETAITEM_H

#include "songfile.h"
#include "abstractfilepartmodel.h"

class SongTrackMetaItem : public AbstractFilePartModel
{
   Q_OBJECT
public:
   explicit SongTrackMetaItem();

   QString fullFilePath();
   QString fileName();
   QString trackName();

   void print();

protected:
   virtual uint32_t maxInternalSize();
   virtual uint32_t minInternalSize();
   virtual uint8_t *internalData();

private:
   uint8_t m_Data[SONGFILE_MAX_TRACK_META_SIZE];
};

#endif // SONGTRACKMETAITEM_H
