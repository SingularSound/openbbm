#ifndef FILEPARTCOLLECTION_H
#define FILEPARTCOLLECTION_H

#include "songfile.h"
#include "abstractfilepartmodel.h"

class FilePartCollection : public AbstractFilePartModel
{
   Q_OBJECT
public:
   explicit FilePartCollection();

protected:
   virtual uint32_t maxInternalSize();
   virtual uint32_t minInternalSize();
   virtual uint8_t *internalData();
};

#endif // FILEPARTCOLLECTION_H
