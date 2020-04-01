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
#include <QString>
#include <QStringList>

#include "songtrackmetaitem.h"
#include "../../workspace/workspace.h"
#include "../../workspace/contentlibrary.h"
#include "../../workspace/libcontent.h"

SongTrackMetaItem::SongTrackMetaItem() :
   AbstractFilePartModel()
{
   m_Default_Name = tr("SongTrackMetaItem");
   m_Name = m_Default_Name;
   uint8_t data[] = "DUMMY_META_DATA";
#ifdef __clang__
   uint8_t * end = (uint8_t *) memccpy(m_Data, data, 0, SONGFILE_MAX_TRACK_META_SIZE);
#else
   uint8_t * end = (uint8_t *) _memccpy(m_Data, data, 0, SONGFILE_MAX_TRACK_META_SIZE);
#endif
   m_InternalSize = end - m_Data;
}

QString SongTrackMetaItem::fullFilePath()
{
    // Correct the path here with workspace or hardcoded paths
    QString resolvedPath((char *)internalData());

    // Real workpsace path substitution
    if(resolvedPath.startsWith("%WORKSPACE%")){
        Workspace w;
        resolvedPath = resolvedPath.replace(
                    "%WORKSPACE%",
                    w.defaultPath());
    }

    return resolvedPath;
}

QString SongTrackMetaItem::fileName()
{
   return fullFilePath().split("/").last();
}

QString SongTrackMetaItem::trackName()
{
   return fileName().split(".").first();
}

uint32_t SongTrackMetaItem::maxInternalSize()
{
   return SONGFILE_MAX_TRACK_META_SIZE;
}

uint32_t SongTrackMetaItem::minInternalSize()
{
   return 0;
}

uint8_t *SongTrackMetaItem::internalData()
{
   return m_Data;
}

void SongTrackMetaItem::print()
{
   QTextStream(stdout) << "PRINT Object Name = " << metaObject()->className() << endl;
   QTextStream(stdout) << "      maxInternalSize() = " << maxInternalSize() << endl;
   QTextStream(stdout) << "      minInternalSize() = " << minInternalSize() << endl;
   QTextStream(stdout) << "      m_InternalSize = " << m_InternalSize << endl;
   QTextStream(stdout) << "      Content:" << endl;
   QTextStream(stdout) << "         " << (char*)m_Data << endl;
}
