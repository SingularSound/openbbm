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
#include "filecompare.h"
#include "../crc32.h"

#include <QDebug>

FileCompare::FileCompare()
{
   m_file1size = 0;
   m_file2size = 0;
}
FileCompare::FileCompare(const QString &path1, const QString &path2)
{
   setPath1(path1);
   setPath2(path2);
}

bool FileCompare::areIdentical()
{
   return m_file1Crc != 0 &&
         m_file2Crc != 0 &&
         m_file1size == m_file2size &&
         m_file1Crc == m_file2Crc;

}

void FileCompare::setPath1(const QString &file1Path)
{
   QFile file1(file1Path);
   setFile1(file1);
}

void FileCompare::setFile1(QIODevice &file1)
{
   // Note: file needs to be opened in order to retrieve size due to QuaZipFile limitations
   if(!file1.open(QIODevice::ReadOnly)){
      m_file1size = 0;
      m_file1Crc = 0;
      return;
   }
   m_file1size = file1.size();
   file1.close();

   m_file1Crc = computeCRC(file1);
}

void FileCompare::setPath2(const QString &file2Path)
{
   QFile file2(file2Path);
   setFile2(file2);
}
void FileCompare::setFile2(QIODevice &file2)
{
   // Note: file needs to be opened in order to retrieve size due to QuaZipFile limitations
   if(!file2.open(QIODevice::ReadOnly)){
      m_file1size = 0;
      m_file1Crc = 0;
      return;
   }
   m_file2size = file2.size();
   file2.close();
   m_file2Crc = computeCRC(file2);
}


uint32_t FileCompare::computeCRC(const QString &filePath)
{
   QFile file(filePath);
   return computeCRC(file);
}

uint32_t FileCompare::computeCRC(QIODevice &file)
{
   if(!file.open(QIODevice::ReadOnly)){
      return 0;
   }

   QByteArray buffer = file.readAll();
   file.close();

   uint8_t *fileMap = (uint8_t *)buffer.data();

   Crc32 crc32;
   crc32.update(fileMap, buffer.size());


   return crc32.getCRC(true);
}
