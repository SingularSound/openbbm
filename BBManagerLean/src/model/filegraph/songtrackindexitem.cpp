#include "songtrackindexitem.h"

SongTrackIndexItem::SongTrackIndexItem() :
   AbstractFilePartModel(),
   m_TrackIndexingItem()
{
   m_InternalSize = sizeof (SONGFILE_TrackIndexingItemStruct);
   m_Default_Name = tr("SongTrackIndexItem");
   m_Name = m_Default_Name;
}

uint32_t SongTrackIndexItem::maxInternalSize()
{
   return sizeof (SONGFILE_TrackIndexingItemStruct);
}

uint32_t SongTrackIndexItem::minInternalSize()
{
   return sizeof (SONGFILE_TrackIndexingItemStruct);
}

uint8_t *SongTrackIndexItem::internalData()
{
   return (uint8_t *)&m_TrackIndexingItem;
}

void SongTrackIndexItem::setCRC(uint32_t crc)
{
   m_TrackIndexingItem.crc = crc;
}

uint32_t SongTrackIndexItem::crc() const
{
   return m_TrackIndexingItem.crc;
}

void SongTrackIndexItem::setDataOffset(uint32_t offset)
{
   m_TrackIndexingItem.dataOffset = offset;
}
uint32_t SongTrackIndexItem::dataOffset() const
{
   return m_TrackIndexingItem.dataOffset;
}

void SongTrackIndexItem::setMetaOffset(uint32_t offset)
{
   m_TrackIndexingItem.metaOffset = offset;
}
uint32_t SongTrackIndexItem::metaOffset() const
{
   return m_TrackIndexingItem.metaOffset;
}

void SongTrackIndexItem::print()
{
   QTextStream(stdout) << "PRINT Object Name = " << metaObject()->className() << endl;
   QTextStream(stdout) << "      maxInternalSize() = " << maxInternalSize() << endl;
   QTextStream(stdout) << "      minInternalSize() = " << minInternalSize() << endl;
   QTextStream(stdout) << "      m_InternalSize = " << m_InternalSize << endl;
   QTextStream(stdout) << "      Content:" << endl;
   QTextStream(stdout) << "         dataOffset = " << m_TrackIndexingItem.dataOffset << endl;
   QTextStream(stdout) << "         metaOffset = " << m_TrackIndexingItem.metaOffset << endl;
   QTextStream(stdout) << "         crc = " << m_TrackIndexingItem.crc << endl;

}
