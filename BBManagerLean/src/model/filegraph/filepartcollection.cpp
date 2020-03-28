#include "filepartcollection.h"

FilePartCollection::FilePartCollection()
{
   m_InternalSize = 0;
   m_Default_Name = tr("FilePartCollection");
   m_Name = m_Default_Name;
}

uint32_t FilePartCollection::maxInternalSize()
{
   return 0;
}

uint32_t FilePartCollection::minInternalSize()
{
   return 0;
}

uint8_t *FilePartCollection::internalData()
{
   return nullptr;
}
