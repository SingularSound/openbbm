#ifndef VARIABLESIZEFILEPARTCOLLECTION_H
#define VARIABLESIZEFILEPARTCOLLECTION_H
#include "filepartcollection.h"

class TrackMetaCollection : public FilePartCollection
{
   Q_OBJECT
public:
   explicit TrackMetaCollection();

   uint32_t readFromBuffer(uint8_t * p_Buffer, uint32_t size, QList<uint32_t> metaOffsetList, QStringList *p_ParseErrors);
};

#endif // VARIABLESIZEFILEPARTCOLLECTION_H
