#ifndef CSVCONFIGFILE_H
#define CSVCONFIGFILE_H

#include <QObject>
#include <QString>
#include <QFile>
#include <QList>
#include <QSet>
#include <QMap>
#include <QStringList>

#include "../../beatsmodelfiles.h"

class CsvConfigFile : public QObject
{
   Q_OBJECT
public:
   explicit CsvConfigFile(const QString &fileName = QString());

   enum FileType
   {
      FOLDER,
      MIDI_BASED_SONG,
      WAVE_BASED_SONG,
      DRUM_SET,
      WAVE_FILE,
      ENUM_SIZE
   };

   inline bool append(const QString &longName, FileType fileType = FOLDER){return insert(m_FileContent.count(), longName, fileType);}
   bool insert(int i, const QString &longName, FileType fileType = FOLDER, int midiId = 0);
   bool containsLongName(const QString &longName) const;
   bool containsFileName(const QString &fileName) const;
   int indexOfLongName(const QString &longName) const;
   int indexOfFileName(const QString &fileName) const;
   inline int count() const {return m_FileContent.count();}


   bool replaceAt(int i, const QString &longName, FileType fileType = FOLDER, int midiId = 0);

   void removeAt(int i);
   void remove(const QString &longName);
   QString longNameAt(int i) const;
   QString lastLongName() const;
   QString fileNameAt(int i) const;
   QString midiIdAt(int i) const;
   QString lastFileName() const;
   FileType fileTypeAt(int i) const;
   FileType lastFileType() const;

   QString longName2FileName(const QString &longName) const;
   QString fileName2LongName(const QString &fileName) const;

   bool read();
   bool read(QIODevice &inputFile);
   bool write();

   void moveEntries(int first, int last, int delta);

   /**
    * @brief setFileName
    * @param fileName
    * @return return true if there were any change requiring re-hash
    */
   bool setFileName(const QString &fileName);
   QString fileName() const;

   static QMap<FileType, QString> fileExtensionMap() {
      static QMap<FileType, QString> fileExtensionMap = fileExtensionMapInit();
      return fileExtensionMap;
   }
   static QMap<QString, FileType> reverseFileExtensionMap() {
      static QMap<QString, FileType> reverseFileExtensionMap = reverseFileExtensionMapInit();
      return reverseFileExtensionMap;
   }

   bool setMidiId(int idx, int id);
   int midiId(int idx);

private:
   static QMap<FileType, QString> fileExtensionMapInit(){
       QMap<FileType, QString> map;
       map.insert(FOLDER, QString());
      map.insert(MIDI_BASED_SONG, QString(BMFILES_MIDI_BASED_SONG_EXTENSION).toUpper());
      map.insert(WAVE_BASED_SONG, QString(BMFILES_WAVE_BASED_SONG_EXTENSION).toUpper());
      map.insert(DRUM_SET, QString(BMFILES_DRUMSET_EXTENSION).toUpper());
      map.insert(WAVE_FILE, QString(BMFILES_WAVE_EXTENSION).toUpper());
      return map;
   }
   static QMap<QString, FileType> reverseFileExtensionMapInit(){
      QMap<QString, FileType> map;
      map.insert(QString(), FOLDER);
      map.insert(QString(BMFILES_MIDI_BASED_SONG_EXTENSION).toUpper(), MIDI_BASED_SONG);
      map.insert(QString(BMFILES_WAVE_BASED_SONG_EXTENSION).toUpper(), WAVE_BASED_SONG);
      map.insert(QString(BMFILES_DRUMSET_EXTENSION).toUpper(), DRUM_SET);
      map.insert(QString(BMFILES_WAVE_EXTENSION).toUpper(), WAVE_FILE);
      return map;
   }

   QFile m_File;
   QList<QStringList> m_FileContent;
   QSet<QString> m_FileNames; // Keep the filenames in a Hash for faster access
   QSet<QString> m_LongNames; // Keep the longnames in a Hash for faster access
   QSet<QString> m_MidiIds; // Keep the midiIds in a Hash for faster access

};

#endif // CSVCONFIGFILE_H
