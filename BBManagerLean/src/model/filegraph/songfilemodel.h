#ifndef SONGFILEMODEL_H
#define SONGFILEMODEL_H

#include <QList>
#include <QIODevice>

#include "songfile.h"
#include "abstractfilepartmodel.h"
#include "filepartcollection.h"

class FileHeaderModel;
class FileOffsetTableModel;
class SongModel;
class SongTracksModel;
class SongFileMeta;
class SongFolderTreeItem;
class AutoPilotDataModel;

class SongFileModel : public FilePartCollection
{
   Q_OBJECT
public:
   explicit SongFileModel();

   virtual uint32_t readFromBuffer(uint8_t * p_Buffer, uint32_t size, QStringList *p_ParseErrors);

   QUuid songUuid();
   void setSongUuid(const QUuid &uuid);

   QString defaultDrmFileName();
   void setDefaultDrmFileName(const QString &defaultDrmFileName);

   QString defaultDrmName();
   void setDefaultDrmName(QString drmName);

   FileHeaderModel *getFileHeaderModel();
   FileOffsetTableModel *getFileOffsetTableModel();
   SongFileMeta *getMeta();
   SongModel *getSongModel();
   SongTracksModel *getSongTracksModel();
   AutoPilotDataModel *getAutoPilotDataModel();

   static QUuid extractUuid(QIODevice &file, QStringList *p_ParseErrors);
   static QUuid extractUuid(const QString & filePath, QStringList *p_ParseErrors);
   static bool isFileValidStatic(const QString & filePath);

   void replaceEffectFile(const QString &originalName, const QString &newName);

protected:
   virtual void prepareData();

};

#endif // SONGFILEMODEL_H
