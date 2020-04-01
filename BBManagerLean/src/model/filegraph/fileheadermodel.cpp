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
#include <cstring>
#include <QDebug>
#include <QMessageBox>

#include "fileheadermodel.h"
#include "../../crc32.h"

FileHeaderModel::FileHeaderModel():
   AbstractFilePartModel(),
   m_FileHeader()
{
   m_InternalSize = sizeof (SONGFILE_HeaderStruct);

   m_Default_Name = tr("FileHeaderModel");
   m_Name = m_Default_Name;

}

bool FileHeaderModel::isVersionValid()
{
    if(m_FileHeader.actualVersion <= ACTUAL_SONGFILE_VERSION)
    {
        if(m_FileHeader.actualRevision <= ACTUAL_SONGFILE_REVISION)
        {
            return true;
        }
    }
    return false;
}

bool FileHeaderModel::isHeaderValid()
{
   std::array<int, 2> acceptableRevisions = ACCEPTABLE_REVISIONS;

   if(m_FileHeader.fileType[0] != 'B' ||
         m_FileHeader.fileType[1] != 'B' ||
         m_FileHeader.fileType[2] != 'S' ||
         m_FileHeader.fileType[3] != 'F'){

      qWarning() << "FileHeaderModel::isValid - ERROR - BBSF";
      return false;
   }

   // Need to support version 1.0.1 as well as current version
   // Due to a bug in file version generation - see issue #48
   bool valid = false;
   if(m_FileHeader.backCompVersion == 1){
      if(m_FileHeader.backCompRevision == 0){
         if(m_FileHeader.backCompBuild == 1){
            valid = true;
         }
      }
   } else if(m_FileHeader.backCompVersion == BACK_COMP_SONGFILE_VERSION){
      if(m_FileHeader.backCompRevision == BACK_COMP_SONGFILE_REVISION){
         valid = true;
      } else {
          valid = false;
          for (int i=0; i<acceptableRevisions.size(); i++) {
              if (m_FileHeader.backCompRevision == acceptableRevisions[i]) {
                  valid = true;
                  
              }
          }
      }
   }


   if(!valid){
      qWarning() << "FileHeaderModel::isValid - ERROR - File version is not supported:" <<
                    m_FileHeader.backCompVersion  << "." <<
                    m_FileHeader.backCompRevision << "." <<
                    m_FileHeader.backCompBuild;
   }
   return valid;
}

bool FileHeaderModel::isFileValid()
{
   return ((m_FileHeader.flags & SONGFILE_FLAG_SONG_FILE_INVALID_MASK) == 0);
}

bool FileHeaderModel::isFileValidStatic(QFile &file)
{
   if(!file.isOpen()){
      qWarning() << "FileHeaderModel::isFileValidStatic - ERROR 1 - File needs to be opened";
      return false;
   }
   SONGFILE_HeaderStruct header;
   qint64 size = file.read((char *)&header, sizeof(header));

   if(size != sizeof(header)){
      qWarning() << "FileHeaderModel::isFileValidStatic - ERROR 2 - unable to read whole header";
      return false;
   }

   return ((header.flags & SONGFILE_FLAG_SONG_FILE_INVALID_MASK) == 0);
}


void FileHeaderModel::setFileValid(bool valid)
{
   if (!valid){
      qDebug() << "File is set as INVALID";
      m_FileHeader.flags |= (1 << SONGFILE_FLAG_SONG_FILE_INVALID_SHIFT);
   } else {
      qDebug() << "File is set as VALID";
      m_FileHeader.flags &= ~(1 << SONGFILE_FLAG_SONG_FILE_INVALID_SHIFT);
   }
}

uint32_t FileHeaderModel::maxInternalSize()
{
   return sizeof (SONGFILE_HeaderStruct);
}

uint32_t FileHeaderModel::minInternalSize()
{
   return sizeof (SONGFILE_HeaderStruct);
}

uint8_t *FileHeaderModel::internalData()
{
   return (uint8_t *)&m_FileHeader;
}

void FileHeaderModel::setCRC(uint32_t crc)
{
   m_FileHeader.crc = crc;
}

uint32_t FileHeaderModel::crc() const
{
   return m_FileHeader.crc;
}

void FileHeaderModel::computeCRC(uint8_t *data, int length)
{
   Crc32 crc32;
   crc32.update(data, length);
   m_FileHeader.crc = crc32.getCRC(true);
}

uint32_t FileHeaderModel::actualVersion()
{
    return m_FileHeader.actualVersion;
}

void FileHeaderModel::setActualVersion(uint32_t version)
{
    m_FileHeader.actualVersion = version;
}
