#ifndef FIXEDSIZEFILEPARTCOLLECTION_H
#define FIXEDSIZEFILEPARTCOLLECTION_H
#include "filepartcollection.h"

class TrackIndexCollection : public FilePartCollection
{
   Q_OBJECT
public:
   explicit TrackIndexCollection();
   virtual uint32_t readFromBuffer(uint8_t * p_Buffer, uint32_t size, QStringList *p_ParseErrors);

   QList<uint32_t> metaOffsetList();
   QList<uint32_t> dataOffsetList();

private:
   uint32_t m_filePartSize;
};

#endif // FIXEDSIZEFILEPARTCOLLECTION_H
