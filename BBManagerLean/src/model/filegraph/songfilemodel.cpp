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
#include "songfilemodel.h"
#include "fileheadermodel.h"
#include "fileoffsettablemodel.h"
#include "songfilemeta.h"
#include "songmodel.h"
#include "songtracksmodel.h"
#include "autopilotdatamodel.h"

#include <QTextStream>
#include <QDebug>
#include <QFileInfo>

SongFileModel::SongFileModel() :
   FilePartCollection()
{
   m_Default_Name = tr("SongFileModel");
   m_Name = m_Default_Name;

   SongTracksModel *p_SongTracksModel = new SongTracksModel;

   mp_SubParts->append(new FileHeaderModel);
   mp_SubParts->append(new FileOffsetTableModel);
   mp_SubParts->append(new SongFileMeta);
   mp_SubParts->append(new SongModel(p_SongTracksModel));
   mp_SubParts->append(p_SongTracksModel);
   mp_SubParts->append(new AutoPilotDataModel());
}


uint32_t SongFileModel::readFromBuffer(uint8_t * p_Buffer, uint32_t size, QStringList *p_ParseErrors)
{
   uint8_t * p_OriginalBuffer = p_Buffer;
   uint32_t totalUsedSize = 0;
   uint32_t usedSize = 0;
   // 1 - prepare date to compare real content
   prepareData();


   // 2 - Validate then Read File Header
   FileHeaderModel tempFileHeaderModel;
   if ((int)tempFileHeaderModel.readFromBuffer(p_Buffer, size, p_ParseErrors) < 0){
      qWarning() << "SongFileModel::readFromBuffer - ERROR - Could not read FileHeaderModel";
      p_ParseErrors->append(tr("SongFileModel::readFromBuffer - ERROR - Could not read FileHeaderModel"));
      return -1;
   }

   if(!tempFileHeaderModel.isHeaderValid()){
      qWarning() << "SongFileModel::readFromBuffer - ERROR - Wrong header version";
      p_ParseErrors->append(tr("SongFileModel::readFromBuffer - ERROR - Wrong header version"));
      return -1;
   }

   if(tempFileHeaderModel.crc() == getFileHeaderModel()->crc()){
      qWarning() << "SongFileModel::readFromBuffer - SUCCESS - file CRC has not changed. No need to parse";
      return size;
   }

   usedSize = getFileHeaderModel()->readFromBuffer(p_Buffer, size, p_ParseErrors);
   size -= usedSize;
   p_Buffer += usedSize;
   totalUsedSize += usedSize;

   // 3 - Validate then File Offset Table
   FileOffsetTableModel tempFileOffsetTableModel;
   if ((int)tempFileOffsetTableModel.readFromBuffer(p_Buffer, size, p_ParseErrors) < 0){
      qWarning() << "SongFileModel::readFromBuffer - ERROR - Could not read FileOffsetTableModel";
      p_ParseErrors->append(tr("SongFileModel::readFromBuffer - ERROR - Could not read FileOffsetTableModel"));
      return -1;
   }

   // TODO validate size with offset table

   usedSize = getFileOffsetTableModel()->readFromBuffer(p_Buffer, size, p_ParseErrors);
   size -= usedSize;
   p_Buffer += usedSize;
   totalUsedSize += usedSize;

   // 4 - Validate then read file meta
   SongFileMeta tempSongFileMeta;
   if ((int)tempSongFileMeta.readFromBuffer(p_Buffer, size, p_ParseErrors) < 0){
      qWarning() << "SongFileModel::readFromBuffer - ERROR - Could not read tempSongFileMeta";
      p_ParseErrors->append(tr("SongFileModel::readFromBuffer - ERROR - Could not read tempSongFileMeta"));
      return -1;
   }

   // TODO validate size with offset table

   usedSize = getMeta()->readFromBuffer(p_Buffer, size, p_ParseErrors);
   size -= usedSize;
   p_Buffer += usedSize;
   totalUsedSize += usedSize;

   // 5 - Create Tracks Data before song in order to be able to use pointers subsequently
   SongTracksModel tempSongTracksModel;
   if((int)tempSongTracksModel.readFromBuffer(p_OriginalBuffer, getFileOffsetTableModel(), p_ParseErrors) < 0){
      qWarning() << "SongFileModel::readFromBuffer - ERROR - Could not read tempSongTracksModel";
      p_ParseErrors->append(tr("SongFileModel::readFromBuffer - ERROR - Could not read tempSongTracksModel"));
      return -1;
   }
   usedSize = getSongTracksModel()->readFromBuffer(p_OriginalBuffer, getFileOffsetTableModel(), p_ParseErrors);

   size -= usedSize;
   totalUsedSize += usedSize;

   // TODO validate size with offset table

   // 6 - Parse the song content from scratch
   delete mp_SubParts->takeAt(3);
   mp_SubParts->insert(3, new SongModel(static_cast<SongTracksModel *>(mp_SubParts->at(3))));
   usedSize = getSongModel()->readFromBuffer(p_OriginalBuffer + (uint32_t)getFileOffsetTableModel()->songOffset(), (uint32_t)getFileOffsetTableModel()->songSize(), p_ParseErrors);

   size -= usedSize;
   p_Buffer += usedSize;
   totalUsedSize += usedSize;

//   // 7 - Parse Autopilot data (if offset and size != 0)
   delete mp_SubParts->takeAt(5);
   mp_SubParts->insert(5, new AutoPilotDataModel());

   if(size != 0){
       uint32_t autoSize =  (uint32_t)getFileOffsetTableModel()->autoPilotSize();
       usedSize = getAutoPilotDataModel()->readFromBuffer(p_OriginalBuffer + (uint32_t)getFileOffsetTableModel()->autoPilotOffset(), autoSize, p_ParseErrors);

       size -= usedSize;
       p_Buffer += usedSize;
       totalUsedSize += usedSize;
   }

   // verify that size == 0
   if(size != 0){
      p_ParseErrors->append(tr("SongFileModel::readFromBuffer - WARNING - A part of the file was not used by parser. The unused size is %1").arg(size));
   }
   if(!getFileHeaderModel()->isVersionValid())
   {
      p_ParseErrors->append(tr("SongFileModel::readFromBuffer - WARNING - The File Version is newer than BeatBuddy Manager's version.\n"
                               "Saving these files may delete some functionalities."));
   }

   return totalUsedSize;

}

void SongFileModel::prepareData()
{
   QTextStream out(stdout);
   

   getSongTracksModel()->adjustOffsets();
   

   // Adjust Offsets before computing CRC
   getFileOffsetTableModel()->adjustOffsets(getSongTracksModel());
   

   // compute subcomponents CRC before writing to file
   Crc32 crc;
   getFileOffsetTableModel()->updateCRC(crc);
   

   getMeta()->updateCRC(crc);
   

   getSongModel()->updateCRC(crc);
   
   getSongTracksModel()->updateCRC(crc);
   

   getAutoPilotDataModel()->updateCRC(crc);

   getFileHeaderModel()->setCRC(crc.getCRC(true));
   
}

QUuid SongFileModel::songUuid()
{
   return getMeta()->uuid();
}
void SongFileModel::setSongUuid(const QUuid &uuid)
{
   getMeta()->setUuid(uuid);
}

QString SongFileModel::defaultDrmFileName()
{
   return getSongModel()->defaultDrmFileName();
}
void SongFileModel::setDefaultDrmFileName(const QString &defaultDrmFileName)
{
   getSongModel()->setDefaultDrmFileName(defaultDrmFileName);
}

QString SongFileModel::defaultDrmName()
{
   return getMeta()->defaultDrmName();
}
void SongFileModel::setDefaultDrmName(QString drmName)
{
   getMeta()->setDefaultDrmName(drmName);
}

FileHeaderModel *SongFileModel::getFileHeaderModel()
{
   return (FileHeaderModel *)mp_SubParts->at(0);
}

FileOffsetTableModel *SongFileModel::getFileOffsetTableModel()
{
   return (FileOffsetTableModel *)mp_SubParts->at(1);
}

SongFileMeta *SongFileModel::getMeta()
{
   return (SongFileMeta *)mp_SubParts->at(2);
}

SongModel *SongFileModel::getSongModel()
{
   return (SongModel *)mp_SubParts->at(3);
}

SongTracksModel *SongFileModel::getSongTracksModel()
{
   return (SongTracksModel *)mp_SubParts->at(4);
}

AutoPilotDataModel *SongFileModel::getAutoPilotDataModel()
{
   return (AutoPilotDataModel *)mp_SubParts->at(5);
}

QUuid SongFileModel::extractUuid(const QString & filePath, QStringList *p_ParseErrors)
{
   QFile file(filePath);
   return extractUuid(file, p_ParseErrors);
}


QUuid SongFileModel::extractUuid(QIODevice &file, QStringList *p_ParseErrors)
{
   if(!file.open(QIODevice::ReadOnly)){
      return QUuid();
   }
   QByteArray buffer = file.readAll();
   file.close();

   SONGFILE_OffsetTableStruct *offsetTable = (SONGFILE_OffsetTableStruct *)(buffer.data() + sizeof(SONGFILE_HeaderStruct));

   SongFileMeta tempSongFileMeta;
   tempSongFileMeta.readFromBuffer((uint8_t *)(buffer.data() + offsetTable->metaOffset), offsetTable->metaSize, p_ParseErrors);

   return tempSongFileMeta.uuid();
}
bool SongFileModel::isFileValidStatic(const QString & filePath)
{
   QFileInfo fi(filePath);

   if(!fi.exists()){
      qWarning() << "SongFileModel::isFileValidStatic - ERROR 1 - NOT FOUND " << filePath;
      return false;
   }

   QFile file(fi.absoluteFilePath());
   if(!file.open(QIODevice::ReadOnly)){
      qWarning() << "SongFileModel::isFileValidStatic - ERROR 2 - Unable to open file " << filePath;
      return false;
   }

   bool valid = FileHeaderModel::isFileValidStatic(file);
   file.close();

   return valid;
}

void SongFileModel::replaceEffectFile(const QString &originalName, const QString &newName)
{
   getSongModel()->replaceEffectFile(originalName, newName);
}
