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
#include "songfileitem.h"
#include "../../filegraph/songpartmodel.h"
#include "../../filegraph/autopilotdatamodel.h"
#include "songpartitem.h"
#include "../../filegraph/songmodel.h"
#include "../../filegraph/fileheadermodel.h"
#include "../project/contentfoldertreeitem.h"
#include "../project/effectfileitem.h"
#include "../project/effectfoldertreeitem.h"
#include "../project/beatsprojectmodel.h"
#include "../project/drmfileitem.h"
#include "../../beatsmodelfiles.h"
#include "portablesongfile.h"

#include "quazip.h"
#include "quazipfile.h"

#include <QDebug>
#include <QDir>
#include <QUuid>
#include <QProgressDialog>
#include <QMessageBox>

/**
 * @brief SongFileItem::SongFileItem
 * @param filePart
 * @param parent
 *
 * Constructor creating new song (SongFolderTreeItem::insertNewChildAt)
 */
SongFileItem::SongFileItem(SongFileModel * filePart, ContentFolderTreeItem *parent):
   FilePartItem(filePart, parent)
{
   initialize();
   QString resolvedName = parent->resolveDuplicateLongName(tr("New Song"), true);
   this->filePart()->setName(resolvedName);
   // Note: file name needs to be set by parent
}
/**
 * @brief SongFileItem::SongFileItem
 * @param filePart
 * @param parent
 * @param longName
 * @param fileName
 *
 * Constructor used when parsing existing song (SongFolderTreeItem::importSongsModal or SongFolderTreeItem::createFileWithData)
 */
SongFileItem::SongFileItem(SongFileModel * filePart, ContentFolderTreeItem *parent, const QString &longName, const QString &fileName):
   FilePartItem(filePart, parent)
{
   initialize();
   this->filePart()->setName(longName);
   m_FileName = fileName;
}

/**
 * @brief SongFileItem::initialize
 *
 * PRIVATE common part of constructor
 */
void SongFileItem::initialize()
{
   m_playing = false;
   m_UnsavedChanges = false;
   SongModel * p_SongModel = static_cast<SongFileModel *>(filePart())->getSongModel();

   // Intro
   SongPartItem * p_SongPartItem = new SongPartItem(p_SongModel->intro(), this);
   appendChild(p_SongPartItem);

   // Parts
   for(int i = 0; i < p_SongModel->partCount(); i++){
      p_SongPartItem = new SongPartItem(p_SongModel->part(i), this);
      appendChild(p_SongPartItem);
   }

   //Outro
   p_SongPartItem = new SongPartItem(p_SongModel->outro(), this);
   appendChild(p_SongPartItem);
}

/**
 * @brief SongFileItem::data
 * @param column
 * @return
 *
 * re-implementation of AbstractTreeItem::data
 */
QVariant SongFileItem::data(int column)
{
   switch (column){
      case TEMPO:
         return static_cast<SongFileModel *>(filePart())->getSongModel()->bpm();
      case LOOP_COUNT:
		return static_cast<SongFileModel *>(filePart())->getSongModel()->loopSong();
      case SAVE:
         return m_UnsavedChanges;
      case FILE_NAME:
         return m_FileName;
      case ABSOLUTE_PATH:
         return QString("%1/%2").arg(parent()->data(ABSOLUTE_PATH).toString(), m_FileName);
      case MAX_CHILD_CNT:
         // Intro, outro + all song parts
         return MAX_SONG_PARTS + 2;
      case ACTUAL_VERSION:
         return static_cast<SongFileModel *>(filePart())->getFileHeaderModel()->actualVersion();
      case INVALID:
         for(int i = 0; i < childCount(); i++){
            auto invalid = child(i)->data(INVALID).toString();
            if (!invalid.isEmpty()) {
                return tr("\"%1\" is %2 | part #%3", "Leave the right part as is after | symbol!")
                    .arg(data(NAME).toString())
                    .arg(invalid)
                    .arg(i);
            }
         }
         return QVariant();
      case UUID:
         return static_cast<SongFileModel *>(filePart())->songUuid();
      case HASH:
         return hash();
      case PLAYING:
         return m_playing;

      case AUTOPILOT_ON:
      {
          SongFileModel * sfm = static_cast<SongFileModel *>(filePart());
          AutoPilotDataModel* apdm = static_cast<AutoPilotDataModel *>(sfm->getAutoPilotDataModel());
          return apdm->getAutoPilotEnabled();
      }
      case AUTOPILOT_VALID:
      {
          SongFileModel * sfm = static_cast<SongFileModel *>(filePart());
          AutoPilotDataModel* apdm = static_cast<AutoPilotDataModel *>(sfm->getAutoPilotDataModel());
          return apdm->getAutoPilotValid();
      }

      case ERROR_MSG:
         return m_parseErrors;
      case DEFAULT_DRM:
         // File Name always returned to upper case
         return QString("%1,%2").arg(defaultDrmFileName()).arg(defaultDrmName());
      default:
         return FilePartItem::data(column);
   }
}

/**
 * @brief SongFileItem::setData
 * @param column
 * @param value
 * @return
 *
 * re-implementation of AbstractTreeItem::setData
 * calls computeHash and model()->setArchiveDirty when appropriate?
 */
bool SongFileItem::setData(int column, const QVariant & value)
{
   QString resolvedName;
   QString defaultDrmCsv;
   QStringList defaultDrm;

   switch (column){
      case NAME:
         resolvedName = static_cast<ContentFolderTreeItem *>(parent())->resolveDuplicateLongName(value.toString(), false);
         filePart()->setName(resolvedName);
         static_cast<ContentFolderTreeItem *>(parent())->updateChildContent(this, true); // NOTE: should already have been created with default name
         computeHash(false);
         propagateHashChange();
         model()->setProjectDirty();
         // Do not clear playing status, handle by Playback panel
         return true;

      case SAVE:
         if (value.toBool()) {
            m_UnsavedChanges = true;
            model()->setProjectDirty();
            model()->setSongsFolderDirty();
         } else if (m_UnsavedChanges) {
            if(data(AUTOPILOT_ON).toBool()){
                verifyAutoPilot();
            }

            saveFile();
            m_UnsavedChanges = false;
            // Update hash and propagate change down to root
            computeHash(false);
            propagateHashChange();
         }
         return true;
      case ACTUAL_VERSION:
         static_cast<SongFileModel *>(filePart())->getFileHeaderModel()->setActualVersion(value.toInt());
         return true;
      case FILE_NAME:
         if(m_FileName.isEmpty()){
            qWarning() << "SongFileItem::setData - TODO - determine what to do when creating empty file in model";

            m_FileName = value.toString();
            QDir parentDir(static_cast<ContentFolderTreeItem *>(parent())->folderFI().absoluteFilePath());

            QFileInfo fileInfo(parentDir.absoluteFilePath(m_FileName));
            if(fileInfo.exists()){
               // FIXME - RESTORE LOAD if required?
               //loadFile();
            } else {
               saveFile();
            }
         } else {
            QDir parentDir(static_cast<ContentFolderTreeItem *>(parent())->folderFI().absoluteFilePath());
            QFile file(parentDir.absoluteFilePath(m_FileName)); // previous path
            m_FileName = value.toString();
            file.rename(parentDir.absoluteFilePath(m_FileName)); // new path
         }
         return true;
      case TEMPO:
         static_cast<SongFileModel *>(filePart())->getSongModel()->setBpm(value.toUInt());
         m_UnsavedChanges = true;
         model()->setProjectDirty();
         model()->setSongsFolderDirty();
         model()->itemDataChanged(this, SAVE);
         // Clear playing status of all children
         return setData(PLAYING, QVariant(false));
      case LOOP_COUNT:
         qDebug() << "---------------------saving loopSong" << value.toUInt();
         static_cast<SongFileModel *>(filePart())->getSongModel()->setLoopSong(value.toUInt());
         m_UnsavedChanges = true;
         static_cast<ContentFolderTreeItem *>(parent())->updateChildContent(this, true); // NOTE: should already have been created with default name
         model()->setProjectDirty();
         model()->setSongsFolderDirty();
         model()->itemDataChanged(this, SAVE);

       // Clear playing status of all children
       return setData(PLAYING, QVariant(false));
      case PLAYING:
         m_playing = value.toBool();
         if(!value.toBool()){
             // clear all children
             for(int i = 0; i < childCount(); i++){
                 child(i)->setData(PLAYING, false);
                 model()->itemDataChanged(child(i), PLAYING);
             }
         }
       return true;
      case AUTOPILOT_ON:
      {
         SongFileModel * sfm = static_cast<SongFileModel *>(filePart());
         AutoPilotDataModel* apdm = static_cast<AutoPilotDataModel *>(sfm->getAutoPilotDataModel());
         apdm->setAutoPilotEnabled(value.toBool());
         setData(SAVE, QVariant(true));
         model()->itemDataChanged(this, SAVE);
         return true;
      }

      case AUTOPILOT_VALID:
      {
         SongFileModel * sfm = static_cast<SongFileModel *>(filePart());
         AutoPilotDataModel* apdm = static_cast<AutoPilotDataModel *>(sfm->getAutoPilotDataModel());
         apdm->setAutoPilotValid(value.toBool());
         setData(SAVE, QVariant(true));
         model()->itemDataChanged(this, SAVE);
         return true;
      }

      case ERROR_MSG:
         m_parseErrors = value.toStringList();
         return true;
      case DEFAULT_DRM:
         // CSV: "FILE_NAME, NAME"
         defaultDrmCsv = value.toString();
         defaultDrm = defaultDrmCsv.split(",");
         if(defaultDrm.count() != 2){
            return false;
         }

         m_UnsavedChanges = true;
         model()->setProjectDirty();
         model()->setSongsFolderDirty();
         model()->itemDataChanged(this, SAVE);

         setDefaultDrm(defaultDrm.at(1), defaultDrm.at(0));
         return true;
      default:
         // Call parent for default behavior
         if(FilePartItem::setData(column, value)){
            m_UnsavedChanges = true;
            model()->setProjectDirty();
            model()->setSongsFolderDirty();
            model()->itemDataChanged(this, SAVE);
            return setData(PLAYING, QVariant(false));
         }
         return false;
   }
}

/**
 * @brief SongFileItem::flags
 * @param column
 * @return
 *
 * re-implementation of AbstractTreeItem::flags
 */
Qt::ItemFlags SongFileItem::flags(int column)
{
   Qt::ItemFlags tempFlags = Qt::NoItemFlags;
   switch (column){
      case NAME:
      case TEMPO:
         if(!model()->isEditingDisabled()){
            tempFlags |= Qt::ItemIsEditable;
         }
         if(!model()->isSelectionDisabled()){
            tempFlags |= Qt::ItemIsSelectable | Qt::ItemIsEnabled;
         }
         return tempFlags;
      default:
         return AbstractTreeItem::flags(column);
   }
}


/**
 * @brief SongFileItem::verifyFile
 *
 * Sets the proper valitity flag in file data.
 * Performed as part of save operation (does not require model()->setArchiveDirty()).
 * requires computeHash to be called after call
 */
void SongFileItem::verifyFile()
{
    static_cast<SongFileModel*>(filePart())->getFileHeaderModel()->setFileValid(data(INVALID).toString().isEmpty());
}

void SongFileItem::verifyAutoPilot()
{
    QString songName = data(NAME).toString();
    SongFileModel * sfm = static_cast<SongFileModel *>(filePart());
    AutoPilotDataModel* apdm = static_cast<AutoPilotDataModel *>(sfm->getAutoPilotDataModel());
    SongModel * sm = static_cast<SongFileModel *>(filePart())->getSongModel();

    int nbPart = sm->nPart();
    QString warningMsg = "";
    for(int i = 0; i < nbPart; i++)
    {
        int nbFill = sm->part(i)->nbDrumFills();
        QString answer = apdm->getPartModel(i)->verifyPartValid(nbFill);
        if(!answer.isEmpty())
        {
          //Not Valid.
          answer = QString("\n\nIn Part #%1:\n").arg(i+1) + answer;
          warningMsg += answer;

        }
    }

    if(warningMsg.isEmpty()) {
        setData(AUTOPILOT_VALID, true);
    } else {
        setData(AUTOPILOT_VALID, false);
        warningMsg = "Invalid AutoPilot Settings on  song \""+ songName +"\":" + warningMsg;
        QMessageBox::warning(nullptr, tr("Invalid AutoPilot Settings"), tr(warningMsg.toStdString().c_str()));
    }
}

/**
 * @brief SongFileItem::isFileValid
 * @return
 *
 * Returns the validity flag from file
 */
bool SongFileItem::isFileValid()
{
   // Perform check without parsing whole file
   return SongFileModel::isFileValidStatic(data(ABSOLUTE_PATH).toString());
}

/**
 * @brief SongFileItem::saveFile
 *
 * Saves the memory content of file to file
 */
void SongFileItem::saveFile()
{
   verifyFile();

   //Check if version # change is necessary.
   if(data(ACTUAL_VERSION) != ACTUAL_SONGFILE_VERSION)
   {
       setData(ACTUAL_VERSION, ACTUAL_SONGFILE_VERSION);
   }
   // Save changes in .wav usage

   model()->effectFolder()->saveUseChanges(static_cast<SongFileModel *>(filePart())->songUuid());
   QDir parentDir(static_cast<ContentFolderTreeItem *>(parent())->folderFI().absoluteFilePath());
   QFile fout(parentDir.absoluteFilePath(m_FileName));
   fout.open(QIODevice::WriteOnly);

   filePart()->writeToFile(fout);

   fout.flush();
   fout.close();
   m_UnsavedChanges = false;
   model()->itemDataChanged(this, SAVE);
}

/**
 * @brief SongFileItem::insertNewChildAt
 * @param row
 *
 * re-implementation of AbstractTreeItem::insertNewChildAt
 */
void SongFileItem::insertNewChildAt(int row)
{
    // It needs to be done before moving parts, because after moving, index are no longer valid
    // Clear playing status of all children
    setData(PLAYING, QVariant(false));

   SongModel * p_SongModel = static_cast<SongFileModel *>(filePart())->getSongModel();
   p_SongModel->insertNewPart(row - 1);

   AutoPilotDataModel * p_APModel = static_cast<SongFileModel *>(filePart())->getAutoPilotDataModel();
   p_APModel->insertNewPart(row - 1);

   SongPartItem * p_SongPartItem = new SongPartItem(p_SongModel->part(row - 1), this);
   insertChildAt(row, p_SongPartItem);

   m_UnsavedChanges = true;
   model()->setProjectDirty();
   model()->setSongsFolderDirty();
   model()->itemDataChanged(this, SAVE);
}

/**
 * @brief SongFileItem::removeChild
 * @param row
 *
 * re-implementation of AbstractTreeItem::removeChild
 */
void SongFileItem::removeChild(int row)
{
   // It needs to be done before moving parts, because after moving, index are no longer valid
   // Clear playing status of all children
   setData(PLAYING, QVariant(false));

   // 1 - ClearPtr for used effects
   static_cast<SongPartItem *>(child(row))->clearEffectUsage();

   // 2 - Delete file graph
   // NOTE: deletePart skips intro in its count
   static_cast<SongFileModel *>(filePart())->getSongModel()->deletePart(row - 1);
   static_cast<SongFileModel *>(filePart())->getAutoPilotDataModel()->deletePart(row - 1);

   // 3 - Delete tree
   removeChildInternal(row);

   // 4 - Raise a flag to say that there were changes
   m_UnsavedChanges = true;
   model()->setProjectDirty();
   model()->setSongsFolderDirty();
   model()->itemDataChanged(this, SAVE);
}

/**
 * @brief SongFileItem::moveChildren
 * @param sourceFirst
 * @param sourceLast
 * @param delta
 *
 * re-implementation of AbstractTreeItem::moveChildren
 */
void SongFileItem::moveChildren(int sourceFirst, int sourceLast, int delta)
{
   // 1 - Validate

   if(sourceFirst > sourceLast){
      qWarning() << "SongFileItem::moveChildren : sourceFirst > sourceLast";
      return;
   }
   if(sourceLast >= childCount() - 1 || sourceFirst < 1){
      qWarning() << "SongFileItem::moveChildren : sourceFirst >= childCount() - 1 || sourceFirst < 1";
      return;
   }

   if(sourceLast + delta >= childCount() - 1 || sourceFirst + delta < 1){
      qWarning() << "SongFileItem::moveChildren : (sourceLast + delta >= childCount() - 1 || sourceFirst + delta < 1)";
      return;
   }

   // It needs to be done before moving parts, because after moving, index are no longer valid
   // Clear playing status of all children
   setData(PLAYING, QVariant(false));

   // 2 - Move corresponding items in file graph
   // Note : remove 1 to index to obtain part index (excluding intro and outro)
   static_cast<SongFileModel *>(filePart())->getSongModel()->moveParts(sourceFirst - 1, sourceLast - 1, delta);
   static_cast<SongFileModel *>(filePart())->getAutoPilotDataModel()->moveParts(sourceFirst - 1, sourceLast - 1, delta);

   // 3 - Move items in tree

   // Create a list of items to move
   QList<AbstractTreeItem *> moved;
   for(int i = sourceFirst; i <= sourceLast; i++){
      moved.append(childItems()->takeAt(i));
   }

   sourceFirst += delta;
   sourceLast += delta;

   int movedIndex = 0;

   for(int i = sourceFirst; i <= sourceLast; i++, movedIndex++){
      insertChildAt(i, moved.at(movedIndex));
   }

   // 4 - Raise a flag to say that there were changes
   m_UnsavedChanges = true;
   model()->setProjectDirty();
   model()->setSongsFolderDirty();
   model()->itemDataChanged(this, SAVE);
}

/**
 * @brief SongFileItem::clearEffectUsage
 *
 * Called when song is being deleted
 * sub calls handle model()->setArchiveDirty()
 */
void SongFileItem::clearEffectUsage()
{
   model()->effectFolder()->removeAllUse(static_cast<SongFileModel *>(filePart())->songUuid(), true);
}

/**
 * @brief SongFileItem::exportModal
 * @param p_parentWidget
 * @param dstPath
 * @return
 *
 * Operation of exporting song
 */
bool SongFileItem::exportModal(QWidget *p_parentWidget, const QString &dstPath)
{

   // Note operation count = 9 + effectList.count
   // 1      - generate effectList
   // 2      - create output file
   // 3      - create internal folders
   // 4      - generate version file
   // 5      - append song file
   // 6      - song name mapping file
   // 7..n-3 - append effect files
   // n-2    - config file
   // n-1    - effect usage file
   // n      - close file (flushing)

   QProgressDialog progress(tr("Exporting Song..."), tr("Abort"), 0, 9, p_parentWidget);
   progress.setWindowModality(Qt::WindowModal);
   progress.setValue(0);

   // 1 - generate effectList
   if(progress.wasCanceled()){
      return false;
   }

   QList<EffectFileItem *> effectList = this->effectList();
   QMap<QString, qint32> effectUsageMap = effectUsageCountMap();

   progress.setMaximum(progress.maximum() + effectList.count());
   progress.setValue(1);

   // 2 - create output file
   if(progress.wasCanceled()){
      return false;
   }

   QuaZip zip(dstPath);
   if(!zip.open(QuaZip::mdCreate)){
      QMessageBox::critical(p_parentWidget, tr("Export Song"), tr("Unable to create archive at %1").arg(dstPath));
      return false;
   }

   QuaZipFile outZipFile(&zip);
   progress.setValue(2);

   // 3 - create internal folders
   if(progress.wasCanceled()){
      zip.close();
      QFile(dstPath).remove();
      return false;
   }

   if(!outZipFile.open(QIODevice::WriteOnly, QuaZipNewInfo("SONG/"))){
      progress.show();
      QMessageBox::critical(p_parentWidget, tr("Export Song"), tr("Unable to create SONG folder in archive %1").arg(dstPath));
      zip.close();
      QFile(dstPath).remove();
      progress.setValue(progress.maximum());
      return false;
   }
   outZipFile.close();

   if(!outZipFile.open(QIODevice::WriteOnly, QuaZipNewInfo("EFFECTS/"))){
      progress.show();
      QMessageBox::critical(p_parentWidget, tr("Export Song"), tr("Unable to create EFFECTS folder in archive %1").arg(dstPath));
      zip.close();
      QFile(dstPath).remove();
      progress.setValue(progress.maximum());
      return false;
   }
   outZipFile.close();
   progress.setValue(3);


   // 4 - Create Version File
   if(progress.wasCanceled()){
      zip.close();
      QFile(dstPath).remove();
      return false;
   }

   QMap<QString, qint32> versionFileContent;
   versionFileContent.insert("version", PORTABLESONGFILE_VERSION);
   versionFileContent.insert("revision", PORTABLESONGFILE_REVISION);
   versionFileContent.insert("build", PORTABLESONGFILE_BUILD);


   if(!outZipFile.open(QIODevice::WriteOnly, QuaZipNewInfo(BMFILES_PORTABLE_SONG_VERSION))){
      progress.show();
      QMessageBox::critical(p_parentWidget, tr("Export Song"), tr("Unable to create file %1 in archive %2").arg(BMFILES_PORTABLE_SONG_VERSION).arg(dstPath));
      zip.close();
      QFile(dstPath).remove();
      progress.setValue(progress.maximum());
      return false;
   } else {
      // In else block in order to reuse fout name
      QDataStream fout(&outZipFile);
      fout << versionFileContent;
   }

   outZipFile.close();
   progress.setValue(4);

   // 5 - append song file
   if(progress.wasCanceled()){
      zip.close();
      QFile(dstPath).remove();
      return false;
   }

   QFile songFile(data(ABSOLUTE_PATH).toString());
   if(!songFile.open(QIODevice::ReadOnly)){
      progress.show();
      QMessageBox::critical(p_parentWidget, tr("Export Song"), tr("Unable to open song file %1").arg(songFile.fileName()));
      zip.close();
      QFile(dstPath).remove();
      progress.setValue(progress.maximum());
      return false;
   }
   if(!outZipFile.open(QIODevice::WriteOnly, QuaZipNewInfo(QString("SONG/%1").arg(data(FILE_NAME).toString()), data(ABSOLUTE_PATH).toString()))){
      progress.show();
      QMessageBox::critical(p_parentWidget, tr("Export Song"), tr("Unable to create file %1 in archive %2").arg(QString("SONG/%1").arg(data(FILE_NAME).toString())).arg(dstPath));
      songFile.close();
      zip.close();
      QFile(dstPath).remove();
      progress.setValue(progress.maximum());
      return false;
   }

   // Copy content
   outZipFile.write(songFile.readAll());

   outZipFile.close();
   songFile.close();
   progress.setValue(5);

   // 6 - append song file to song name mapping file
   if(progress.wasCanceled()){
      zip.close();
      QFile(dstPath).remove();
      return false;
   }

   if(!outZipFile.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate, QuaZipNewInfo(QString("SONG/%1").arg(BMFILES_NAME_TO_FILE_MAPPING)))){
      progress.show();
      QMessageBox::critical(p_parentWidget, tr("Export Song"), tr("Unable to create file %1 in archive %2").arg(QString("SONG/%1").arg(BMFILES_NAME_TO_FILE_MAPPING)).arg(dstPath));
      zip.close();
      QFile(dstPath).remove();
      progress.setValue(progress.maximum());
      return false;
   } else {

      // Line Format is
      // AAAAAAAA[.BBB],CC. DDDDDDD
      //   AAAAAAAA : File/Folder name
      //   BBB      : File extention (if file)
      //   CC       : Song Index (1 to 99)
      //   DDDDDDD  : Song title
      // In else block in order to reuse fout name
      QTextStream fout(&outZipFile);

      fout << data(FILE_NAME).toString().toLocal8Bit() << ",1. " << data(NAME).toString().toLocal8Bit();
   }

   outZipFile.close();
   progress.setValue(6);


   // 7..n-3 - append effect files
   if(progress.wasCanceled()){
      zip.close();
      QFile(dstPath).remove();
      return false;
   }
   QByteArray configFileContentBuffer;
   QTextStream configFileOut(&configFileContentBuffer, QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate);

   int configIndex = 1;
   foreach(EffectFileItem *effect, effectList){
      if(progress.wasCanceled()){
         zip.close();
         QFile(dstPath).remove();
         return false;
      }

      QFile efxFile(effect->data(ABSOLUTE_PATH).toString());

      if(!efxFile.open(QIODevice::ReadOnly)){
         progress.show();
         QMessageBox::critical(p_parentWidget, tr("Export Song"), tr("Unable to open Accent Hit effect file %1").arg(efxFile.fileName()));
         zip.close();
         QFile(dstPath).remove();
         progress.setValue(progress.maximum());
         return false;
      }
      if(!outZipFile.open(QIODevice::WriteOnly, QuaZipNewInfo(QString("EFFECTS/%1").arg(effect->data(FILE_NAME).toString()), effect->data(ABSOLUTE_PATH).toString()))){
         progress.show();
         QMessageBox::critical(p_parentWidget, tr("Export Song"), tr("Unable to create file %1 in archive %2").arg(QString("EFFECTS/%1").arg(effect->data(FILE_NAME).toString())).arg(dstPath));
         efxFile.close();
         zip.close();
         QFile(dstPath).remove();
         progress.setValue(progress.maximum());
         return false;
      }

      outZipFile.write(efxFile.readAll());
      outZipFile.close();
      efxFile.close();

      // update config file content
      configFileOut << QFileInfo(efxFile).fileName()<< "," << configIndex++ << ". " << effect->data(NAME).toString() << endl;

      progress.setValue(progress.value() + 1);
   }

   // n-2 write config file content
   if(progress.wasCanceled()){
      zip.close();
      QFile(dstPath).remove();
      return false;
   }
   if(!outZipFile.open(QIODevice::WriteOnly, QuaZipNewInfo(QString("EFFECTS/%1").arg(BMFILES_NAME_TO_FILE_MAPPING)))){
      progress.show();
      QMessageBox::critical(p_parentWidget, tr("Export Song"), tr("Unable to create file %1 in archive %2").arg(QString("EFFECTS/%1").arg(BMFILES_NAME_TO_FILE_MAPPING)).arg(dstPath));
      zip.close();
      QFile(dstPath).remove();
      progress.setValue(progress.maximum());
      return false;
   }
   outZipFile.write(configFileContentBuffer);
   outZipFile.close();
   progress.setValue(progress.value() + 1);

   // n-1 write usage
   if(progress.wasCanceled()){
      zip.close();
      QFile(dstPath).remove();
      return false;
   }
   if(!outZipFile.open(QIODevice::WriteOnly, QuaZipNewInfo(QString("EFFECTS/%1").arg(BMFILES_EFFECT_USAGE_FILE_NAME)))){
      progress.show(); // always make sure progress is shown before displaying another dialog.
      QMessageBox::critical(p_parentWidget, tr("Export Song"), tr("Unable to create file %1 in archive %2").arg(QString("EFFECTS/%1").arg(BMFILES_EFFECT_USAGE_FILE_NAME)).arg(dstPath));

      progress.setValue(progress.maximum());
      zip.close();
      QFile(dstPath).remove();
      return false;
   }

   QDataStream usageFout(&outZipFile);
   usageFout << effectUsageMap;
   outZipFile.close();
   progress.setValue(progress.value() + 1);

   // n - close file
   if(progress.wasCanceled()){
      zip.close();
      QFile(dstPath).remove();
      return false;
   }
   zip.close();
   progress.setValue(progress.value() + 1);
   //progress.setValue(progress.maximum());

   return true;
}

QList<EffectFileItem *> SongFileItem::effectList()
{
   QList<EffectFileItem *> effectList;

   // Loop on all parts except for intro and outro
   for(int i = 1; i < childCount() - 1; i++){
      QString efxFileName = static_cast<SongPartItem *>(child(i))->effectFileName();
      if(!efxFileName.isEmpty()){
         EffectFileItem *efx = model()->effectFolder()->effectWithFileName(efxFileName);
         if(efx && !effectList.contains(efx)){
            effectList.append(efx);
         }
      }
   }

   return effectList;
}

QMap<QString, qint32> SongFileItem::effectUsageCountMap()
{
   QMap<QString, qint32> map;

   // Loop on all parts except for intro and outro
   for(int i = 1; i < childCount() - 1; i++){
      QString efxFileName = static_cast<SongPartItem *>(child(i))->effectFileName();
      if(!efxFileName.isEmpty()){
         qint32 count = map.value(efxFileName,0);
         count++;
         map.insert(efxFileName, count);
      }
   }

   return map;
}

/**
 * @brief SongFileItem::replaceEffectFile
 * @param originalName
 * @param newName
 *
 * used to replace an effect with another one (as part of import process)
 */
void SongFileItem::replaceEffectFile(const QString &originalName, const QString &newName)
{
   if(originalName.compare(newName) == 0){
      return;
   }

   // Loop on all parts except for intro and outro
   for(int i = 1; i < childCount() - 1; i++){
      QString efxFileName = static_cast<SongPartItem *>(child(i))->effectFileName();
      if(!efxFileName.isEmpty() && efxFileName.compare(originalName) == 0){
         static_cast<SongPartItem *>(child(i))->setEffectFileName(newName, true);
      }
   }
}

/**
 * @brief SongFileItem::computeHash
 *
 * re-implementation of AbstractTreeItem::computeHash
 */
void SongFileItem::computeHash(bool /*recursive*/)
{
   // Voluntarly do nothing
}

QByteArray SongFileItem::hash()
{
   uint32_t crc = static_cast<SongFileModel *>(filePart())->getFileHeaderModel()->crc();
   // Note : this line is buggy returns a truncated QByteArray if MSBs are 0x00

   QByteArray ret = QByteArray::fromHex(QByteArray::number(crc,16));

   // pad with 0x00
   while(ret.count() < 4){
       ret.append('\0');
   }

   return ret;

}

QByteArray SongFileItem::getHash(const QString &path)
{
   QFile songFile(path);

   if(!songFile.exists() || !songFile.open(QIODevice::ReadOnly)){
      qWarning() << "SongFileItem::getHash - ERROR 1";
      return QByteArray();
   }

   SONGFILE_HeaderStruct *p_header = (SONGFILE_HeaderStruct *)songFile.map(0, sizeof(SONGFILE_HeaderStruct));
   uint32_t crc = p_header->crc;

   songFile.unmap((uchar *)p_header);
   songFile.close();

   QByteArray ret = QByteArray::fromHex(QByteArray::number(crc,16));

   // pad with 0x00
   while(ret.count() < 4){
       ret.append('\0');
   }

   return ret;
}


bool SongFileItem::compareHash(const QString &path)
{
   QByteArray localHash = this->hash();
   QByteArray remoteHash = getHash(path);

   if(localHash.count() == 0 || remoteHash.count() == 0){
      qWarning() << "SongFileItem::compareHash - ERROR 1 - empty hash" << localHash.count() << remoteHash.count() << path;
      return false;
   }

   if(localHash.count() != remoteHash.count()){
      qWarning() << "SongFileItem::compareHash - ERROR 2 - invalid hash length" << localHash.count() << remoteHash.count() << path;
      return false;
   }

   for(int i = 0; i < localHash.count(); i++){
      if(localHash.at(i) != remoteHash.at(i)){
         return false;
      }
   }

   return true;
}

/**
 * @brief SongFileItem::prepareSync
 * @param dstPath
 * @param p_cleanUp
 * @param p_copySrc
 * @param p_copyDst
 *
 * re-implementation of AbstractTreeItem::prepareSync
 * Called as part of BeatsProjectModel::synchronizeModal.
 */
void SongFileItem::prepareSync(const QString &dstPath, QList<QString> *p_cleanUp, QList<QString> *p_copySrc, QList<QString> *p_copyDst)
{
   if(!p_cleanUp || !p_copySrc || !p_copyDst){
      qWarning() << "One of the return lists is null.";
      return;
   }

   QFileInfo dstFI(dstPath);

   if(dstFI.exists() && compareHash(dstPath)){
      return;
   }

   // Add path to clean up and to copy
   if(dstFI.exists()){
      p_cleanUp->append(dstFI.absoluteFilePath());
   }
   p_copySrc->append(QString("%1/%2").arg(parent()->data(ABSOLUTE_PATH).toString(), m_FileName));
   p_copyDst->append(dstFI.absoluteFilePath());

}

/**
 * @brief SongFileItem::manageParsingErrors
 * @param p_parent
 *
 * Function that manages parsing errors recursively
 * Called in the process of BeatsProjectModel::manageParsingErrors
 */
void SongFileItem::manageParsingErrors(QWidget *p_parent)
{
   QStringList errorList = data(ERROR_MSG).toStringList();
   if(!errorList.empty()){


      // 1. Create Error Log
      QString errorLog = "";
      for(int k = 0; k < errorList.count(); k++){
         errorLog += QString("%1 - %2\n").arg(k+1).arg(errorList.at(k));
      }

      // 2. Create temp folder
      QDir tempDir(QString("%1/%2")
                   .arg(QDir::tempPath())
                   .arg(QUuid::createUuid().toString()));

      if(!tempDir.exists()){
         if(!tempDir.mkpath(tempDir.absolutePath())){

            // Save the modified song anyway
            setData(SAVE, QVariant(true));
            setData(SAVE, QVariant(false));

            QMessageBox::warning(p_parent, tr("Processing errors"), tr("Error while parsing song %1 in folder %2\nThe song was recovered and a copy of the faulty song was created at:\n%3\n\nThe error log is:\n%4").arg(data(NAME).toString()).arg(parent()->data(NAME).toString()).arg(tr("Unable to create temporary folder")).arg(errorLog));
         }
      }

      // 3. Copy bad file to temp dir
      QFile file(data(ABSOLUTE_PATH).toString());
      QFileInfo fi(file);
      if(!file.copy(tempDir.absoluteFilePath(fi.fileName()))){
         // Error copying the file
         // Save the modified song anyway
         setData(SAVE, QVariant(true));
         setData(SAVE, QVariant(false));

         QMessageBox::warning(p_parent, tr("Processing errors"), tr("Error while parsing song %1 in folder %2\nThe song was recovered and a copy of the faulty song was created at:\n%3\n\nThe error log is:\n%4").arg(data(NAME).toString()).arg(parent()->data(NAME).toString()).arg(tr("Unable to copy song")).arg(errorLog));

      } else {
         // File copied successfully
         // Save the modified song
         setData(SAVE, QVariant(true));
         setData(SAVE, QVariant(false));
        QString dtlerrlog = (errorLog.contains("A part of the file was not used by parser"))?"Please make sure every song part has a Main Drum loop":errorLog;
        QMessageBox::warning(p_parent, tr("Processing errors"), tr("Error while parsing song %1 in folder %2\nThe song was recovered and a copy of the faulty song was created at:\n%3\n\nThe error log is:\n%4").arg(data(NAME).toString()).arg(parent()->data(NAME).toString()).arg(tempDir.absoluteFilePath(fi.fileName())).arg(dtlerrlog));
      }

      computeHash(false);
      propagateHashChange();
      model()->setProjectDirty();

   }
}


void SongFileItem::setDefaultDrm(const QString &name, const QString &fileName)
{
   static_cast<SongFileModel *>(filePart())->setDefaultDrmName(name);
   static_cast<SongFileModel *>(filePart())->setDefaultDrmFileName(fileName.toUpper()); // Make sure file name storred in upper case
}

QString SongFileItem::defaultDrmName()
{
   return static_cast<SongFileModel *>(filePart())->defaultDrmName();
}

QString SongFileItem::defaultDrmFileName()
{
   return static_cast<SongFileModel *>(filePart())->defaultDrmFileName().toUpper();
}

/**
 * @brief replaceDrumset
 * @param p_parentWidget
 * @param oldLongName
 * @param drmSourcePath
 *
 * Change the default drumset name if the previous name corresponds to oldLongName
 */
void SongFileItem::replaceDrumset(const QString &oldLongName, DrmFileItem *p_drmFileItem)
{
   if(defaultDrmName() != oldLongName){
      return;
   }
   setDefaultDrm(p_drmFileItem->data(NAME).toString(), p_drmFileItem->data(FILE_NAME).toString());

   m_UnsavedChanges = true;
   model()->setProjectDirty();
   model()->setSongsFolderDirty();
   model()->itemDataChanged(this, SAVE);
   model()->itemDataChanged(this, DEFAULT_DRM);

   // No need to re-hash until saved
}
