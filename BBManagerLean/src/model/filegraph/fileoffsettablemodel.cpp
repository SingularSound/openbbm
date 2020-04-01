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
#include "fileoffsettablemodel.h"
#include "songtracksmodel.h"
#include "filepartcollection.h"
#include "trackindexcollection.h"
#include "trackmetacollection.h"
#include "trackdatacollection.h"

FileOffsetTableModel::FileOffsetTableModel() :
   AbstractFilePartModel(),
   m_OffsetTable()
{
   m_InternalSize = sizeof(SONGFILE_OffsetTableStruct);
   m_Default_Name = tr("FileOffsetTableModel");
   m_Name = m_Default_Name;
}

void FileOffsetTableModel::adjustOffsets(SongTracksModel * p_Tracks)
{
   setTracksIndexingSize((uint32_t)p_Tracks->trackIndexes()->size());
   setTracksMetaOffset(tracksIndexingOffset() + tracksIndexingSize());
   setTracksMetaSize((uint32_t)p_Tracks->trackMeta()->size());
   setTracksDataOffset(tracksMetaOffset() + tracksMetaSize());
   setTracksDataSize((uint32_t)p_Tracks->trackData()->size());
   setAutoPilotOffset(tracksDataOffset() + tracksDataSize());
   setAutoPilotSize(sizeof(AUTOPILOT_AutoPilotDataStruct));
}

uint32_t FileOffsetTableModel::maxInternalSize()
{
   return sizeof (SONGFILE_OffsetTableStruct);
}

uint32_t FileOffsetTableModel::minInternalSize()
{
   return sizeof (SONGFILE_OffsetTableStruct);
}

uint8_t *FileOffsetTableModel::internalData()
{
   return (uint8_t *)&m_OffsetTable;
}

// Accessors
uint32_t FileOffsetTableModel::metaOffset() const
{
   return m_OffsetTable.metaOffset;
}

uint32_t FileOffsetTableModel::metaSize() const
{
   return m_OffsetTable.metaSize;
}
uint32_t FileOffsetTableModel::songOffset() const
{
   return m_OffsetTable.songOffset;
}
uint32_t FileOffsetTableModel::songSize() const
{
   return m_OffsetTable.songSize;
}

uint32_t FileOffsetTableModel::tracksIndexingOffset() const
{
   return m_OffsetTable.tracksIndexingOffset;
}
uint32_t FileOffsetTableModel::tracksIndexingSize() const
{
   return m_OffsetTable.tracksIndexingSize;
}
uint32_t FileOffsetTableModel::tracksMetaOffset() const
{
   return m_OffsetTable.tracksMetaOffset;
}
uint32_t FileOffsetTableModel::tracksMetaSize() const
{
   return m_OffsetTable.tracksMetaSize;
}
uint32_t FileOffsetTableModel::tracksDataOffset() const
{
   return m_OffsetTable.tracksDataOffset;
}
uint32_t FileOffsetTableModel::tracksDataSize() const
{
   return m_OffsetTable.tracksDataSize;
}

uint32_t FileOffsetTableModel::autoPilotOffset() const
{
    return m_OffsetTable.autoPilotDataOffset;
}

uint32_t FileOffsetTableModel::autoPilotSize() const
{
    return m_OffsetTable.autoPilotDataSize;
}

void FileOffsetTableModel::setMetaOffset(uint32_t offset)
{
   m_OffsetTable.metaOffset = offset;
}

void FileOffsetTableModel::setMetaSize(uint32_t size)
{
   m_OffsetTable.metaSize = size;
}

void FileOffsetTableModel::setSongOffset(uint32_t offset)
{
   m_OffsetTable.songOffset = offset;
}

void FileOffsetTableModel::setSongSize(uint32_t size)
{
   m_OffsetTable.songSize = size;
}

void FileOffsetTableModel::setTracksIndexingOffset(uint32_t offset)
{
   m_OffsetTable.tracksIndexingOffset = offset;
}

void FileOffsetTableModel::setTracksIndexingSize(uint32_t size)
{
   m_OffsetTable.tracksIndexingSize = size;
}

void FileOffsetTableModel::setTracksMetaOffset(uint32_t offset)
{
   m_OffsetTable.tracksMetaOffset = offset;
}

void FileOffsetTableModel::setTracksMetaSize(uint32_t size)
{
   m_OffsetTable.tracksMetaSize = size;
}

void FileOffsetTableModel::setTracksDataOffset(uint32_t offset)
{
   m_OffsetTable.tracksDataOffset = offset;
}

void FileOffsetTableModel::setTracksDataSize(uint32_t size)
{
   m_OffsetTable.tracksDataSize = size;
}

void FileOffsetTableModel::setAutoPilotOffset(uint32_t offset)
{
    m_OffsetTable.autoPilotDataOffset = offset;
}

void FileOffsetTableModel::setAutoPilotSize(uint32_t size)
{
    m_OffsetTable.autoPilotDataSize = size;
}

