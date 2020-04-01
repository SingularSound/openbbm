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
#include <QDebug>

#include "songtrackdataitem.h"
#include "midiParser.h"



SongTrackDataItem::SongTrackDataItem() :
   AbstractFilePartModel()
{
   m_Default_Name = tr("SongTrackDataItem");
   m_Name = m_Default_Name;
}

bool SongTrackDataItem::parseMidi(uint8_t *file, uint32_t size, int trackType, QStringList *p_ParseErrors)
{
   if(trackType < 0 || trackType >= ENUM_LENGTH){
      qWarning() << "SongTrackDataItem::parseMidi - ERROR - (trackType < 0 || trackType >= ENUM_LENGTH)";
      return false;
   }

   MIDIPARSER_ErrorTypes_t errorType;
   uint32_t ret = midi_ParseFile(file,size,&m_Data,static_cast<MIDIPARSER_TrackType_t>(trackType), (int*)&errorType);

   // Process error codes
   if(errorType & MIDIPARSER_CHANGED_TIME_SIG_DEN_WARN){
      QString msg = tr("Parser - WARNING %1 - Time signature denominator was changed to default value.").arg(MIDIPARSER_CHANGED_TIME_SIG_DEN_WARN);
      qDebug() << msg;
   }
   if(errorType & MIDIPARSER_CHANGED_TIME_SIG_NUM_WARN){
      QString msg = tr("Parser - WARNING %1 - Time signature numerator was changed to default value.").arg(MIDIPARSER_CHANGED_TIME_SIG_NUM_WARN);
      qDebug() << msg;
   }
   if(errorType & MIDIPARSER_CHANGED_TICK_PER_QUARTER_NOTE_WARN){
      QString msg = tr("Parser - WARNING %1 - Tick per quarter notes value was changed to 480.").arg(MIDIPARSER_CHANGED_TICK_PER_QUARTER_NOTE_WARN);
	  qDebug() << msg;
   }
   if(errorType & MIDIPARSER_UNKNOWN_EVENT_CODE_WARN){
      QString msg = tr("Parser - WARNING %1 - There is an unknown event code in the midi file. The event will be skipped").arg(MIDIPARSER_UNKNOWN_EVENT_CODE_WARN); 
      qDebug() << msg;
   }
   if(errorType & MIDIPARSER_END_NO_NOTE_OFF_WARN){
      QString msg = tr("Parser - WARNING %1 - The midi file ends with at least one Note On event with missing Note Off").arg(MIDIPARSER_END_NO_NOTE_OFF_WARN);
      qDebug() << msg;
   }
   if(errorType & MIDIPARSER_EMPTY_TRACK_WARN){
      QString msg = tr("Parser - WARNING %1 - The midi file contains a track of size 0").arg(MIDIPARSER_END_NO_NOTE_OFF_WARN);
      qDebug() << msg;
   }

   if(errorType & MIDIPARSER_INVALID_FILE_ID_ERROR){
      p_ParseErrors->append(tr("Parser - ERROR %1 - Invalid chunk ID found in header. File cannot be recovered.").arg(MIDIPARSER_INVALID_FILE_ID_ERROR));
      qWarning() << p_ParseErrors->last();
   }
   if(errorType & MIDIPARSER_INVALID_HEADER_SIZE_ERROR){
      p_ParseErrors->append(tr("Parser - ERROR %1 - Invalid header size value. File cannot be recovered.").arg(MIDIPARSER_INVALID_HEADER_SIZE_ERROR));
      qWarning() << p_ParseErrors->last();
   }
   if(errorType & MIDIPARSER_NO_EVENT_ERROR){
      p_ParseErrors->append(tr("Parser - ERROR %1 - Parsed midi file does not contain any event. File cannot be recovered.").arg(MIDIPARSER_NO_EVENT_ERROR));
      qWarning() << p_ParseErrors->last();
   }
   if(errorType & MIDIPARSER_TOO_MANY_EVENTS_ERROR){
      p_ParseErrors->append(tr("Parser - ERROR %1 - Parsed midi file contains too many events. File cannot be recovered.").arg(MIDIPARSER_TOO_MANY_EVENTS_ERROR));
      qWarning() << p_ParseErrors->last();
   }
   if(errorType & MIDIPARSER_TIME_SIG_NUM_POWER_2_ERROR){
      p_ParseErrors->append(tr("Parser - ERROR %1 - Parsed midi file resulting time signature denominator is not a power of 2. File cannot be recovered.").arg(MIDIPARSER_TIME_SIG_NUM_POWER_2_ERROR));
      qWarning() << p_ParseErrors->last();
   }
   if(errorType & MIDIPARSER_INVALID_TICK_PER_BEAT_ERROR){
      p_ParseErrors->append(tr("Parser - ERROR %1 - Parsed midi resulting ticks per beat value is invalid (typically 0). File cannot be recovered.").arg(MIDIPARSER_INVALID_TICK_PER_BEAT_ERROR));
      qWarning() << p_ParseErrors->last();
   }
   if(errorType & MIDIPARSER_POST_ANALYSIS_ERROR){
      p_ParseErrors->append(tr("Parser - ERROR %1 - There was an error during the post analysis process. File cannot be recovered.").arg(MIDIPARSER_POST_ANALYSIS_ERROR));
      qWarning() << p_ParseErrors->last();
   }
   if(errorType & MIDIPARSER_INVALID_TRACK_ID_ERROR){
      p_ParseErrors->append(tr("Parser - ERROR %1 - Invalid chunk ID found in track. File cannot be recovered.").arg(MIDIPARSER_POST_ANALYSIS_ERROR));
      qWarning() << p_ParseErrors->last();
   }

   if(ret == 0){
      qWarning() << "SongTrackDataItem::parseMidi - ERROR - The resulting midi file does no have any event";
      p_ParseErrors->append(tr("Midi file does not contain any event"));
      return false;
   }
   m_InternalSize = m_Data.size();
   return true;
}

uint32_t SongTrackDataItem::readFromBuffer(uint8_t * p_Buffer, uint32_t size, QStringList *p_ParseErrors)
{
    p_ParseErrors;
    if(size < minSize()){
        qWarning() << "SongTrackDataItem::readFromFile - ERROR - (size < minSize()), size = " << size << ", minSize() = " << minSize();
        p_ParseErrors->append(tr("File size is less than allowed.\n\nFile size: %1 byte(s)\nMinimum: %2 bytes").arg(size).arg(minSize()));
        return -1;
    }
    m_Data.read((char*)p_Buffer, size);
    return m_InternalSize = m_Data.size();
}

uint32_t SongTrackDataItem::maxInternalSize()
{
   return -1;
}

uint32_t SongTrackDataItem::minInternalSize()
{
   return 0;
}

uint8_t *SongTrackDataItem::internalData()
{
    return (uint8_t*)(m_Cache = m_Data).data();
}

void SongTrackDataItem::print()
{
   QTextStream(stdout) << "PRINT Object Name = " << metaObject()->className() << endl;
   QTextStream(stdout) << "      maxInternalSize() = " << maxInternalSize() << endl;
   QTextStream(stdout) << "      minInternalSize() = " << minInternalSize() << endl;
   QTextStream(stdout) << "      m_InternalSize = " << m_InternalSize << endl;
   QTextStream(stdout) << "      Content:" << endl;
   QTextStream(stdout) << "         format = " << m_Data.format << endl;
   QTextStream(stdout) << "         nTrack = " << m_Data.nTrack << endl;
   QTextStream(stdout) << "         nTick  = " << m_Data.nTick << endl;
   QTextStream(stdout) << "         timeSigNum = " << m_Data.timeSigNum << endl;
   QTextStream(stdout) << "         timeSigDen = " << m_Data.timeSigDen << endl;
   QTextStream(stdout) << "         n32ndNotesPerMIDIQuarterNote = " << m_Data.n32ndNotesPerMIDIQuarterNote << endl;
   QTextStream(stdout) << "         midiClocksPerMetronomeClick = " << m_Data.midiClocksPerMetronomeClick << endl;
   QTextStream(stdout) << "         tpqn = " << m_Data.tpqn << endl;
   QTextStream(stdout) << "         barLength = " << m_Data.barLength << endl;
   QTextStream(stdout) << "         trigPos = " << m_Data.trigPos << endl;
   QTextStream(stdout) << "         bpm = " << m_Data.bpm << endl;
   QTextStream(stdout) << "         index = " << m_Data.index << endl;
   QTextStream(stdout) << "         nEvent = " << m_Data.event.size() << endl;

   for (auto i = 0u; i < m_Data.event.size(); i++){
      QTextStream(stdout) << "            event " << i << " = " << m_Data.event[i].tick << m_Data.event[i].type << m_Data.event[i].note << m_Data.event[i].vel << endl;
   }
}

QByteArray SongTrackDataItem::toByteArray()
{
   qDebug() << "SongTrackDataItem::toByteArray - DEBUG - byteArray.count = " << m_Data.size();
   return m_Data;
}

bool SongTrackDataItem::parseByteArray(const QByteArray& byteArray)
{
    m_InternalSize = (m_Data = byteArray).size();
    return true;
}
