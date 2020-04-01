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
#include <QFileInfo>
#include <QFile>
#include <QDebug>
#include "songtrack.h"
#include "songfile.h"
#include "trackfile.h"
#include "songtrackindexitem.h"
#include "songtrackdataitem.h"
#include "songtrackmetaitem.h"
#include "crc32.h"
#include "songpartmodel.h"
#include "workspace/workspace.h"


SongTrack::SongTrack(int index)
{
   m_DeleteSubParts = true;
   m_Index = index;
   mp_Meta = new SongTrackMetaItem();
   mp_Data = new SongTrackDataItem();
   mp_Indexes = new SongTrackIndexItem();
   m_Name.clear();
}

SongTrack::SongTrack(int index, SongTrackIndexItem *trackIndexes, SongTrackMetaItem  *trackMeta, SongTrackDataItem  *trackData)
{
   m_DeleteSubParts = false;
   m_Index = index;
   mp_Meta = trackMeta;
   mp_Data = trackData;
   mp_Indexes = trackIndexes;

   m_Name = trackMeta->trackName();
}


SongTrack::~SongTrack()
{
   if (m_DeleteSubParts){
      delete mp_Indexes;
      delete mp_Meta;
      delete mp_Data;
   }
}

bool SongTrack::parseMidiFile(const QString &fileName, int trackType, QStringList *p_ParseErrors)
{
   QFile midiFile(fileName);

   if(!midiFile.exists()){
      qWarning() << "SongTrack::parseMidiFile - ERROR 1 - Midi file does not exist";
      p_ParseErrors->append(QObject::tr("SongTrack::parseMidiFile - ERROR 1 - Midi file does not exist."));
      return false;
   }


   // Create File Path. Replace Workspace path with %WORKSPACE%
   // Note: If workspace is invalid, returns user Documents folder.
   //       As expected, there is much chance no substitution performed
   Workspace w;
   QFileInfo fileInfo(fileName);
   QString fileMeta = fileInfo.absoluteFilePath();
   if(fileMeta.startsWith(w.defaultPath(), Qt::CaseInsensitive)){
      fileMeta.replace(w.defaultPath(), "%WORKSPACE%", Qt::CaseInsensitive);
   }

   // Make sure we can store the modified file path in metadata
   if((uint32_t)fileMeta.size() + 1 > SONGFILE_MAX_TRACK_META_SIZE){
      qWarning() << "SongTrack::parseMidiFile - ERROR 2 - Modified path length is " << fileMeta.size() << " characters. The BBManager only supports path up to " << (SONGFILE_MAX_TRACK_META_SIZE - 1) << " characters";
      p_ParseErrors->append(QObject::tr("SongTrack::parseMidiFile - ERROR 2 - Modified path length is %1 characters. The BBManager only supports path up to %2 characters").arg(fileMeta.size()).arg(SONGFILE_MAX_TRACK_META_SIZE - 1));
      return false;
   }

   // use size + 1 in order to copy \0 termination. Note: QByteArray::data() always return the data with \0 according to doc.
   mp_Meta->readFromBuffer((uint8_t *)fileMeta.toUtf8().data(), fileMeta.size() + 1, p_ParseErrors);


   if (!midiFile.open(QIODevice::ReadOnly)){
      qWarning() << "SongTrack::parseMidiFile - ERROR 3 - Cannot open midiFile";
      p_ParseErrors->append(QObject::tr("SongTrack::parseMidiFile - ERROR 3 - Cannot open midiFile"));
      return false;
   }

   uint8_t *p_Buffer = midiFile.map(0, midiFile.size());

   if(!p_Buffer){
      midiFile.close();
      qWarning() << "SongTrack::parseMidiFile - ERROR 4 - (!p_Buffer)";
      p_ParseErrors->append(QObject::tr("SongTrack::parseMidiFile - ERROR 4 - (!p_Buffer)"));
      return false;
   }
   QStringList tmpParseError;

   bool parsingOK = mp_Data->parseMidi(p_Buffer, midiFile.size(), trackType, &tmpParseError);

   midiFile.unmap(p_Buffer);
   midiFile.close();

   if(!parsingOK){
      qWarning() << "SongTrack::parseMidiFile - ERROR 5 - Midi parser error";
      if(tmpParseError.isEmpty()){
         qWarning() << "SongTrack::parseMidiFile - ERROR 5 - Unknown midi parser error";
         p_ParseErrors->append(QObject::tr("SongTrack::parseMidiFile - ERROR 5 - Unknown midi parser error"));
      } else {
         foreach(const QString &error, tmpParseError){
            p_ParseErrors->append(error);
         }
      }
   }

   // calculate CRC on data and metadata
   Crc32 crc;
   mp_Meta->updateCRC(crc);
   mp_Data->updateCRC(crc);
   mp_Indexes->setCRC(crc.getCRC(true));

   QFileInfo info(fileName);

   m_Name = info.baseName();

   //print();

   return parsingOK;
}

bool SongTrack::importTrackFile(const QString &fileName, QStringList *p_ParseErrors)
{
   QFile trackFile(fileName);

   if(!trackFile.exists()){
      qWarning() << "SongTrack::importTrackFile - ERROR 1 - Track file does not exist";
      p_ParseErrors->append(QObject::tr("SongTrack::importTrackFile - ERROR 1 - Track file does not exist."));
      return false;
   }


   if (!trackFile.open(QIODevice::ReadOnly)){
      qWarning() << "SongTrack::importTrackFile - ERROR 3 - Cannot open trackFile";
      p_ParseErrors->append(QObject::tr("SongTrack::importTrackFile - ERROR 3 - Cannot open trackFile"));
      return false;
   }

   // map trackfile
   uint8_t *p_Buffer = trackFile.map(0, trackFile.size());

   if(!p_Buffer){
      trackFile.close();
      qWarning() << "SongTrack::importTrackFile - ERROR 4 - (!p_Buffer)";
      p_ParseErrors->append(QObject::tr("SongTrack::importTrackFile - ERROR 4 - (!p_Buffer)"));
      return false;
   }

   // map the buffer on the file structure
   SONGTRACK_FileStruct *trackFileContent = (SONGTRACK_FileStruct *) p_Buffer;

   // read meta and data
   mp_Meta->readFromBuffer((uint8_t *)(p_Buffer + trackFileContent->offsets.metaOffset), trackFileContent->offsets.metaSize, p_ParseErrors);
   mp_Data->readFromBuffer((uint8_t *)(p_Buffer + trackFileContent->offsets.dataOffset), trackFileContent->offsets.dataSize, p_ParseErrors);

   // read crc from file as well
   mp_Indexes->setCRC(trackFileContent->trackInfo.trackCRC);

   // retrieve name from path as storred in meta
   m_Name = mp_Meta->trackName();

   return p_ParseErrors->isEmpty();
}

bool SongTrack::parseByteArray(const QByteArray& data)
{
    if (!mp_Data->parseByteArray(data))
        return false;
   // calculate CRC on data and metadata
   Crc32 crc;
   mp_Meta->updateCRC(crc);
   mp_Data->updateCRC(crc);
   mp_Indexes->setCRC(crc.getCRC(true));
    return true;
}

bool SongTrack::extractToTrackFile(const QString &fileName, uint32_t timeSigNum, uint32_t timeSigDen, uint32_t tickPerBar, uint32_t bpm, QStringList *p_ParseErrors)
{
    QFile trackFile(fileName);

    if (!trackFile.open(QIODevice::WriteOnly)){
       qWarning() << "SongTrack::extractToTrackFile - ERROR 1 - Cannot open trackFile";
       p_ParseErrors->append(QObject::tr("SongTrack::extractToTrackFile - ERROR 1 - Cannot open trackFile"));
       return false;
    }

    // create file content in memory
    SONGTRACK_FileStruct trackFileContent;

    // Info
    trackFileContent.trackInfo.timeSigNum = timeSigNum;
    trackFileContent.trackInfo.timeSigDen = timeSigDen;
    trackFileContent.trackInfo.tickPerBar = tickPerBar;
    trackFileContent.trackInfo.bpm        = bpm;
    trackFileContent.trackInfo.trackCRC   = mp_Indexes->crc();

    //offsets and sizes - all offsets are correctly set except for dataSize
    trackFileContent.offsets.metaSize           = mp_Meta->size();
    trackFileContent.offsets.dataSize           = mp_Data->size();
    trackFileContent.offsets.originalDataSize   = 0;                // midi file not storred for now

    trackFileContent.offsets.dataOffset         = trackFileContent.offsets.metaOffset + trackFileContent.offsets.metaSize;
    trackFileContent.offsets.originalDataOffset = 0;                // midi file not storred for now

    // Finish by computing file CRC
    Crc32 crc;

    crc.update((uint8_t *) &trackFileContent.offsets,   sizeof(SONGTRACK_OffsetTableStruct));
    crc.update((uint8_t *) &trackFileContent.trackInfo, sizeof(SONGTRACK_TrackInfo));
    mp_Meta->updateCRC(crc);
    mp_Data->updateCRC(crc);
    // NOTE: no crc on original midi yet

    trackFileContent.header.fileCRC = crc.getCRC(true);

    // Write to file
    trackFile.write((char *) &trackFileContent.header,    sizeof(SONGTRACK_HeaderStruct));
    trackFile.write((char *) &trackFileContent.offsets,   sizeof(SONGTRACK_OffsetTableStruct));
    trackFile.write((char *) &trackFileContent.trackInfo, sizeof(SONGTRACK_TrackInfo));
    mp_Meta->writeToFile(trackFile);
    mp_Data->writeToFile(trackFile);

    trackFile.close();
    return true;
}

void SongTrack::ajustOffsets(SongTrack * previousTrack)
{
   if(previousTrack){
      mp_Indexes->setDataOffset(
               previousTrack->trackIndexes()->dataOffset() +
               (uint32_t)previousTrack->trackData()->size()
               );
      mp_Indexes->setMetaOffset(
               previousTrack->trackIndexes()->metaOffset() +
               (uint32_t)previousTrack->trackMeta()->size()
               );
   } else {
      mp_Indexes->setDataOffset(0);
      mp_Indexes->setMetaOffset(0);
   }
}

SongTrackIndexItem *SongTrack::trackIndexes()
{
   return mp_Indexes;
}

SongTrackMetaItem  *SongTrack::trackMeta()
{
   return mp_Meta;
}

SongTrackDataItem  *SongTrack::trackData()
{
   return mp_Data;
}

void SongTrack::setDeleteSubParts(bool doDelete)
{
   m_DeleteSubParts = doDelete;
}

const QString &SongTrack::name() const
{
   return m_Name;
}

QString SongTrack::fileName() const
{
   return mp_Meta->fileName();
}

QString SongTrack::fullFilePath() const
{
   return mp_Meta->fullFilePath();
}

void SongTrack::setFullPath(const QString& path)
{
   QStringList shit;
   // use size + 1 in order to copy \0 termination. Note: QByteArray::data() always return the data with \0 according to doc.
   mp_Meta->readFromBuffer((uint8_t *)path.toUtf8().data(), path.size() + 1, &shit);
}

void SongTrack::addUser(SongPartModel* user)
{
   m_Users.append(user);
}

void SongTrack::removeUser(SongPartModel* user)
{
   m_Users.removeOne(user);
}

void SongTrack::removeAllUser(SongPartModel* user)
{
   m_Users.removeAll(user);
}

void SongTrack::print()
{
   mp_Indexes->print();
   mp_Meta->print();
   mp_Data->print();
}

