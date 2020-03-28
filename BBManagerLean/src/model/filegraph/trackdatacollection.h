#ifndef TRACKDATACOLLECTION_H
#define TRACKDATACOLLECTION_H

#include "filepartcollection.h"
class TrackDataCollection : public FilePartCollection
{
   Q_OBJECT
public:
   explicit TrackDataCollection();

   uint32_t readFromBuffer(uint8_t * p_Buffer, uint32_t size, QList<uint32_t> dataOffsetList, QStringList *p_ParseErrors);
};

#endif // TRACKDATACOLLECTION_H
