/*
  	This software and the content provided for use with it is Copyright © 2014-2020 Singular Sound 
 	BeatBuddy Manager is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2 as published by
    the Free Software Foundation.
    
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
    
    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/
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
