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
#include <QTextStream>
#include <QDebug>
#include <QFileInfo>

#include "csvconfigfile.h"
#include "../../../crc32.h"

CsvConfigFile::CsvConfigFile(const QString &fileName) :
   QObject(),
   m_File(fileName)
{
    
    if(!fileName.isEmpty()){
        m_File.open(QIODevice::ReadWrite | QIODevice::Text);
        m_File.close();
    }
}

bool CsvConfigFile::insert(int i, const QString &longName, FileType fileType, int midiId)
{

    qDebug() << "config inserting" << i << longName << fileType << midiId;

   if(containsLongName(longName)){
      qWarning() << "Already contains longName " << longName;
      return false;
   }

   QStringList line;
   QString extension = fileExtensionMap().value(fileType);
   if(!extension.isEmpty()){
      extension = QString(".%1").arg(extension);
   }

   Crc32 crc;
   crc.update((uint8_t *)longName.toUtf8().data(), longName.count());
   QString fileName = QString("%1%2").arg(crc.peakCRC(true),8,16, QLatin1Char('0')).arg(extension);
   fileName = fileName.toUpper();
   // Resolve duplicates
   while(m_FileNames.contains(fileName)){
      crc.updateByte(0xFF);
      fileName = QString("%1%2").arg(crc.peakCRC(true),8,16, QLatin1Char('0')).arg(extension);
      fileName = fileName.toUpper();
   }

   line += QString("%1").arg(crc.getCRC(true),8,16, QLatin1Char('0')).toUpper();; // Filename without extension
   line += fileExtensionMap().value(fileType);
   line += longName;
   line += QString::number(midiId);

   qDebug() << "inserting line" << line << "at" << i;
   m_FileContent.insert(i, line);
   m_FileNames.insert(fileName.toUpper());
   m_LongNames.insert(longName.toUpper());
   m_MidiIds.insert(QString(midiId));

   return true;
}

bool CsvConfigFile::replaceAt(int i, const QString &longName, FileType fileType, int midiId){
   if(containsLongName(longName)){
      if(indexOfLongName(longName) == i){
         if(fileType == reverseFileExtensionMap().value(m_FileContent.at(i).at(1))){
            if (midiIdAt(i).toInt() == midiId) {
                 // No change required
                qDebug() << "no change required" << midiId << m_FileContent.at(i).at(3) << midiIdAt(i);
                return true;
            }
         } // else, normal processing
      } else {
         qWarning() << "CsvConfigFile::replaceAt - ERROR - Already contains longName '" << longName << "' at a different index";
         return false;
      }
   }
   removeAt(i);
   qDebug() << "update config" << i << longName << fileType << midiId;
   return insert(i, longName, fileType, midiId);
}

bool CsvConfigFile::containsLongName(const QString &longName) const
{
   return m_LongNames.contains(longName.toUpper());
}

bool CsvConfigFile::containsFileName(const QString &fileName) const
{
   return m_FileNames.contains(fileName.toUpper());
}

QString CsvConfigFile::longNameAt(int i) const
{
   return m_FileContent.at(i).at(2);
}

QString CsvConfigFile::lastLongName() const
{
   return m_FileContent.last().at(2);
}

QString CsvConfigFile::fileNameAt(int i) const
{
   QStringList line = m_FileContent.at(i);
   if(line.at(1).isEmpty()){
      return line.at(0);
   }
   return QString("%1.%2").arg(line.at(0)).arg(line.at(1));
}

QString CsvConfigFile::midiIdAt(int i) const
{
   return m_FileContent.at(i).at(3);
}

QString CsvConfigFile::lastFileName() const
{
   return fileNameAt(m_FileContent.count() - 1);
}

CsvConfigFile::FileType CsvConfigFile::fileTypeAt(int i) const
{
   return reverseFileExtensionMap().value(m_FileContent.at(i).at(1), FOLDER);
}

CsvConfigFile::FileType CsvConfigFile::lastFileType() const
{
   return reverseFileExtensionMap().value(m_FileContent.last().at(1), FOLDER);
}

int CsvConfigFile::indexOfLongName(const QString &longName) const
{
    for(int i = 0; i < m_FileContent.count(); i++){
        if(m_FileContent.at(i).at(2).compare(longName) == 0){
            qDebug() << i << m_FileContent.at(i).at(2) << "==" << longName;
            return i;
        }
    }
    return -1;
}

int CsvConfigFile::indexOfFileName(const QString &fileName) const
{
   QStringList splitFileName = fileName.split(".");
   QString baseFileName = splitFileName.at(0);
   QString fileTypeName;

   if(splitFileName.count() > 1){
      fileTypeName = splitFileName.at(1);
   }

   for(int i = 0; i < m_FileContent.count(); i++){
      if(m_FileContent.at(i).at(0).compare(baseFileName) == 0 &&
         m_FileContent.at(i).at(1).compare(fileTypeName) == 0){
         return i;
      }
   }
   return -1;
}

void CsvConfigFile::removeAt(int i)
{
   QStringList line = m_FileContent.takeAt(i);
   m_LongNames.remove(line.at(2).toUpper());

   if(line.at(1).isEmpty()){
      m_FileNames.remove(line.at(0).toUpper());
   } else {
      m_FileNames.remove(QString("%1.%2").arg(line.at(0)).arg(line.at(1)).toUpper());
   }
}

void CsvConfigFile::remove(const QString &longName)
{
   int index = indexOfLongName(longName);
   if(index != -1){
      removeAt(index);
   }
}

bool CsvConfigFile::read()
{
   return read(m_File);
}

bool CsvConfigFile::read(QIODevice &inputFile)
{
   if(inputFile.open(QIODevice::ReadOnly | QIODevice::Text)){
      m_FileContent.clear();
      m_LongNames.clear();
      m_FileNames.clear();
      m_MidiIds.clear();

      QTextStream in(&inputFile);
      QStringList line;
      while (!in.atEnd()) {
         // Line Format is
         // AAAAAAAA[.BBB],CC. DDDDDDD
         //   AAAAAAAA : File/Folder name
         //   BBB      : File extention (if file)
         //   CC       : Song Index (1 to 99)
         //   DDDDDDD  : Song title
         //   EE       : Midi ID (0 to 128) (this one separated by tab)

         // Split AB from CD
         line = in.readLine().split(",");
         if (line.count() != 2) {
            //break;
         }

         // Split A from B
         QStringList fileNameList = line.takeFirst().split(".");
         line.insert(0, fileNameList.at(0));
         // Add empty string if B not present
         if(fileNameList.count() == 2){
            line.insert(1, fileNameList.at(1));
         } else {
            line.insert(1, "");
         }

         // Split C from D
         line += line.takeAt(2).split(". ");
         if(line.count() != 4){
             line.move(2,4);
         }

         // Determine where to insert the line for the m_FileContent to be sorted
         // We consider that the list should already be sorted. Therefore, a sequential
         // search starting from last should be the fastest method.
         bool ok = true;
         int insertedKey = line.at(2).toInt(&ok);
         if(!ok){
            break;
         }
         insertedKey--; // Key is 1 based. Change it to 0 base;
         int insertedIndex = m_FileContent.count();
         for(int i = m_FileContent.count() - 1; i > 0; i--){
            int compareKey = line.at(2).toInt(&ok);
            if(!ok || compareKey ==  insertedKey){
               ok = false;
               break;
            }
            if(compareKey > insertedKey){
               break;
            }
            insertedIndex = i;
         }

         if(!ok){
            break;
         }

         // Remove the index from the line since it will now be function of the position in the list
         line.removeAt(2);
         QStringList nameAndId = line.at(2).split("\t");
         if (nameAndId.length()>1) {
             line.removeAt(2);
             line.insert(2, nameAndId.at(0));
             QString id = nameAndId.at(1);
             qDebug() << "-----------id" << id << id.length() << id.isEmpty() << id.size() << id.toInt();
             line.insert(3, id.length()>0 ? id : "0");
         } else {
             line.insert(3, "0");
             qDebug() << "no midi id found!  inserting 0";
         }
         m_FileContent.insert(insertedIndex, line);
         m_LongNames.insert(line.at(2).toUpper());
         if(line.at(1).isEmpty()){
            m_FileNames.insert(line.at(0).toUpper());
         } else {
            m_FileNames.insert(QString("%1.%2").arg(line.at(0)).arg(line.at(1)).toUpper());
         }
         if (line.size()>3) {
            m_MidiIds.insert(line.at(3));
            qDebug() << "--------------------midiId" << line.at(3) << line.at(2) << line.at(1) << line.at(0);
            if (nameAndId.size()>1) qDebug() << nameAndId.at(1) << nameAndId.at(1).length();
         }
      }

      inputFile.close();
      return true;
   }
   return false;
}

bool CsvConfigFile::write()
{
    bool success = false;

    if(m_File.open(QIODevice::WriteOnly | QIODevice::Text)){
        QTextStream out(&m_File);

        for(int i = 0; i < m_FileContent.count(); i++){
            QStringList line = m_FileContent.at(i);
            QString fileName = line.at(0);
            QString displayText = QString::number(i+1) + ". " + line.at(2);
            QString midiId = "";
            if(!line.at(1).isEmpty()){
                fileName += "." + line.at(1);
            }

            if(line.count()>=4){
                // fourth datum in line may be present in files for newer firmwares, indicating a MIDI ID.
                // incredibly, it's not added to the CSV with a comma to separate the value, but a tab
                if(line.at(3) != "0"){
                    midiId = "\t" + line.at(3);
                }
            }
            // display and midiId aren't separated by commas - if midiId is present it has its own ad hoc \t separator
            out << fileName << "," << displayText << midiId << endl;
        }

        m_File.flush();
        m_File.close();

        success = true;
    } else {
        qWarning() << "Unable to open " << m_File.fileName() << " in WriteOnly | Text";
    }

    return success;
}

void CsvConfigFile::moveEntries(int first, int last, int delta)
{
   // 1 - Validate

   if(first > last){
      qWarning() << "CsvConfigFile::moveEntries - ERROR - (first > last)";
      return;
   }
   if(last >= m_FileContent.count() || first < 0){
      qWarning() << "CsvConfigFile::moveEntries - ERROR - (last >= m_FileContent.count() || first < 0)";
      return;
   }

   if(last + delta >= m_FileContent.count() || first + delta < 0){
      qWarning() << "CsvConfigFile::moveEntries - ERROR - (last + delta >= m_FileContent.count() || first + delta < 0), first = " << first << ",  last = " << last << ", delta = " << delta << ", m_FileContent.count() = " << m_FileContent.count();
      return;
   }

   // 2 - Move items

   // Create a list of items to move
   QList<QStringList> moved;
   for(int i = last; i >= first; i--){
      moved.append(m_FileContent.takeAt(i));
   }

   first += delta;
   last += delta;

   for(int i = first; i <= last; i++){
      m_FileContent.insert(i, moved.takeLast());
   }
}

bool CsvConfigFile::setFileName(const QString &fileName)
{
   bool changed = false;
   if(!m_File.fileName().isEmpty()){
      // If file already has a name, it exists. Make sure that it is renamed.
      if(m_File.fileName().compare(fileName) != 0){
         changed = true;
         if(m_File.exists()){
            // Normal rename case (Note for Mac OS, fileName needs to be an absolute path)
            if(!m_File.rename(m_File.fileName(), fileName)){
               qWarning() << "CsvConfigFile::setFileName - ERROR 1 - unable to rename CSV file";
            }
         } else {
            // if folder containing file was renamed (original file will not exist anymore)
            m_File.setFileName(fileName);
            if(!m_File.exists()){
               qWarning() << "CsvConfigFile::setFileName - ERROR 2 - file with new name does not exist";
            }
         }
      }
   } else {
      m_File.setFileName(fileName);
      // Make sure file exists
      if(!m_File.exists()){
         m_File.open(QIODevice::ReadWrite | QIODevice::Text);
         m_File.close();
         changed = true;
      }
   }
   return changed;
}
QString CsvConfigFile::fileName() const
{
   return m_File.fileName();
}

bool CsvConfigFile::setMidiId(int idx, int id) {
    if (idx<0){ // it's possible we need to check || idx>m_FileContent.length() too
        return false;
    }
    QStringList line = m_FileContent.at(idx);
    qDebug() << "===line1" << line;
    line.removeAt(3);
    line.insert(3, QString::number(id));
    m_FileContent.removeAt(idx);
    m_FileContent.insert(idx, line);
    qDebug() << "===line2" << line << midiId(idx);
    return true;
}

int CsvConfigFile::midiId(int idx) {
    return m_FileContent.at(idx).at(3).toInt();
}


QString CsvConfigFile::longName2FileName(const QString &longName) const
{
   if(!containsLongName(longName)){
      return nullptr;
   }
   return fileNameAt(indexOfLongName(longName));
}

QString CsvConfigFile::fileName2LongName(const QString &fileName) const
{
   if(!containsFileName(fileName)){
      return nullptr;
   }
   return longNameAt(indexOfFileName(fileName));
}
