#ifndef DRMLISTMODEL_H
#define DRMLISTMODEL_H

#include <QFileSystemWatcher>
#include <QStandardItemModel>
#include <QDateTime>
#include <QFileInfo>
#include <QTimer>

class BeatsProjectModel;
class DrmMakerModel;
class Workspace;

class DrmListModel : public QStandardItemModel
{
   Q_OBJECT
public:
   enum Column
   {
      NAME,
      FILE_NAME,
      ABSOLUTE_PATH,
      TYPE,              // project, user or default
      CRC,
      LAST_CHANGE,
      FILE_SIZE,
      OPENED,
      IN_PROJECT,
      ERROR_MSG,
      ENUM_SIZE
   };

   explicit DrmListModel(QWidget *p_parentWidget, Workspace *p_workspace);

   void setBeatsModel(BeatsProjectModel *p_beatsModel);

   QString importDrm(const QString &srcPath, bool forceCopy = false);
   QString importDrmFromPrj(const QString &longName);
   bool addDrmToList(const QFileInfo &drmFI, const QString &type);

   inline void setDrmMakerModel(DrmMakerModel *p_drmMakerModel){mp_drmMakerModel = p_drmMakerModel;}

   void deleteDrmModal(const QModelIndex &index);

   // Re implementation
   bool setData(const QModelIndex &index, const QVariant &value, int role);

   class QUndoStack* undoStack();

signals:
   void sigOpenedDrmNameChanged(const QString & originalName, const QString & newName);

public slots:
   void slotOnDrmFileOpened(const QString &path);
   void slotOnDrmFileClosed(const QString &path);

private slots:
   void slotOnWorkspaceChange(bool valid);
   void slotOnWatchedDirectoryChange(const QString & path);
   void slotOnWatchedFileChange(const QString & path);
   void slotPeriodicFileCheck();
   void slotOnItemChanged(QStandardItem * item);



private:
   typedef struct ModFileInfo{
      QString   absoluteFilePath;
      QDateTime lastModified;
      qint64    size;
      QString   type;

      ModFileInfo():
         absoluteFilePath(),
         lastModified(),
         size(0),
         type(){}

      ModFileInfo(const QString &absoluteFilePath, const QDateTime &lastModified, qint64 size, const QString &type){
         this->absoluteFilePath = absoluteFilePath;
         this->lastModified     = lastModified;
         this->size             = size;
         this->type             = type;
      }
   } ModFileInfo_t;

private:
   void populate();
   void populateFromPath(const QString &path, const QString &type);
   void addBeatsModelDrmToList(int beatsRow);
   bool refreshDrm(const QFileInfo &drmFI);
   bool refreshDrm(int row);
   bool refreshDrm(int row, const QString &newDrmName, const QByteArray &crc);
   bool refreshDrmInternal(const QFileInfo &drmFI, int row, const QString &newDrmName, const QByteArray &crc);
   bool resloveDuplicateName(int row);

   void removeBeatsModelContent();
   void addBeatsModelContent();

   void refreshToolTip(int row);
   void refreshAllToolTips();

   QWidget *parentWidget();

private:
   QFileSystemWatcher m_fileSystemWatcher;
   QTimer m_timer;
   QMap<QString, ModFileInfo> m_filesBeingAdded;
   QMap<QString, ModFileInfo> m_filesBeingModified;

   BeatsProjectModel *mp_beatsModel;
   DrmMakerModel *mp_drmMakerModel;

   Workspace *mp_workspace;
};

#endif // DRMLISTMODEL_H
