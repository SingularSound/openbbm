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
#include "songsfoldertreeitem.h"
#include "effectfoldertreeitem.h"
#include "../song/songfoldertreeitem.h"
#include "../project/effectfileitem.h"
#include "beatsprojectmodel.h"
#include "../../filegraph/songfilemodel.h"
#include "../../beatsmodelfiles.h"
#include "../song/portablesongfile.h"

#include "quazip.h"
#include "quazipfile.h"
#include "quazipdir.h"

#include <QDir>
#include <QUuid>
#include <QMessageBox>
#include <QRegularExpression>

SongsFolderTreeItem::SongsFolderTreeItem(BeatsProjectModel *p_model, FolderTreeItem *parent):
   ContentFolderTreeItem(p_model, parent)
{
   setName("SONGS");
   setFileName("SONGS");
   setSubFoldersAllowed(true);
   setSubFilesAllowed(false);
   setChildrenTypes(nullptr);
}

/**
 * @brief SongsFolderTreeItem::data
 * @param column
 * @return
 *
 * re-implementation of AbstractTreeItem::data
 */
QVariant SongsFolderTreeItem::data(int column)
{
   switch (column){
      case INVALID:
         for(int i = 0; i < childCount(); i++){
            auto invalid = child(i)->data(INVALID).toString();
            if (!invalid.isEmpty()) {
                return tr("Folder %1 | folder #%2", "Leave the right part as is after | symbol!")
                    .arg(invalid)
                    .arg(i+1);
            }
         }
         return QVariant();

      default:
         return ContentFolderTreeItem::data(column);
   }
}

/**
 * @brief SongsFolderTreeItem::insertNewChildAt
 * @param row
 *
 * re-implementation of AbstractTreeItem::insertNewChildAt
 */
void SongsFolderTreeItem::insertNewChildAt(int row){

   // Create and insert new child
   SongFolderTreeItem *p_NewSongsFolder = new SongFolderTreeItem(model(), this);
   insertChildAt(row, p_NewSongsFolder);

   createChildContentAt(row, true);
   p_NewSongsFolder->createProjectSkeleton();


   // Update hash and propagate change down to root
   p_NewSongsFolder->computeHash(true);
   computeHash(false);
   propagateHashChange();
   model()->setProjectDirty();
}

/**
 * @brief SongsFolderTreeItem::removeChild
 * @param row
 *
 * re-implementation of AbstractTreeItem::removeChild
 * Removes child completely. However, it does not perform nodifications. Use Model->remove... if notifications are required
 *
 * NOTE: hashing and setting model dirty done by superclass ContentFolderTreeItem::removeChild
 */
void SongsFolderTreeItem::removeChild(int row)
{

   // Remove all children first
   for(int i = 0; i < child(row)->childCount(); i++){
      child(row)->removeChild(i);
   }
   QString childPath = child(row)->data(ABSOLUTE_PATH).toString();
   ContentFolderTreeItem::removeChild(row);

   QDir childDir(childPath);
   if(childDir.exists()){
      if(!childDir.removeRecursively()){
         qWarning() << "SongsFolderTreeItem::removeChild - ERROR 1 - Unable to delete folder " << childPath;
      }
   } else {
      qWarning() << "SongsFolderTreeItem::removeChild - ERROR 2 - Folder does not exist " << childPath;
   }
}

bool qCopyDirectoryRecursively(const QString& srcFilePath, const QString& tgtFilePath);

/**
 * @brief SongsFolderTreeItem::createFolderWithData
 * @param index
 * @param longName
 * @param fileName
 * @param cleanAll
 *
 * re-implementation of ContentFolderTreeItem::createFolderWithData
 * called as part of project constructor
 */
void SongsFolderTreeItem::createFolderWithData(int index, const QString& sourceLongName, const QString& sourceFileName, bool cleanAll)
{
    QString longName(sourceLongName);
    QString fileName(sourceFileName);

    // Verify item can be inserted at this index
   if(index > childCount()){
      qWarning() << "ContentFolderTreeItem::createFolderWithData - ERROR - cannot insert child at index " << index << ". childCount() = " << childCount();
      return;
   }

   ContentFolderTreeItem *child = nullptr;

   // 1 - Verify if child exists at required index
   if(index < childCount() && childItems()->at(index)->data(NAME).toString().compare(longName) == 0){
      // nothing to do at this level
      // set child for exploration
      child = qobject_cast<ContentFolderTreeItem *>(childItems()->at(index));
      if(!child){
         qWarning() << "ContentFolderTreeItem::createFolderWithData - ERROR - child of type " << childItems()->at(index)->metaObject()->className() << " is not a ContentFolderTreeItem.";
         return;
      }
   }


   // 2 - Verify if child with this name exists at a different index
   if(!child){
      // Technically, all previous indexes have been replaced by csv content
      for(int i = index + 1; i < childCount(); i++){
         if(childItems()->at(i)->data(NAME).toString().compare(fileName) == 0){
            child = qobject_cast<ContentFolderTreeItem *>(childItems()->at(i));
            if(!child){
               qWarning() << "ContentFolderTreeItem::createFolderWithData - ERROR - child of type " << childItems()->at(i)->metaObject()->className() << " is not a ContentFolderTreeItem.";
               return;
            }
            model()->moveItem(child, i, index);
         }
      }
   }

    auto saveCSVandRehash = false;
    // 3 - If the source directory name contains full path, make sure it makes it to our content directory
    if (fileName.contains(QRegularExpression("[\\/]"))) {
        // 3.1 - resolve full name
        longName = resolveDuplicateLongName(longName, false);
        // 3.2 - insert it into CSV
        m_CSVFile.insert(index, longName);
        // 3.3 - get mangled directory name from CSV
        QString name = m_CSVFile.fileNameAt(index);
        // 3.4 - copy source directory to resolved mangled name inside our directory recursively
        qCopyDirectoryRecursively(fileName, QDir(data(ABSOLUTE_PATH).toString()).absoluteFilePath(name));
        // 3.5 - reduce file name from full path to file name only
        fileName = name;
        // 3.6 - we will need to save CSV and rehash afterwards
        saveCSVandRehash = true;
    }

   // 3 - Create child if does not exist
   if(!child){
      // Note: this will work only because SongFolderTreeItem is the only folder that can be inserted in a content folder
      child = new SongFolderTreeItem(model(), this);
      child->setName(longName);
      child->setFileName(fileName);
      model()->insertItem(child, index);
   }

   // 4 - Update child content
   child->updateModelWithData(cleanAll);

    // 5 - Save CSV and rehash if we created content directory from external source
    if (saveCSVandRehash) {
        m_CSVFile.write();
        child->computeHash(true);
        child->propagateHashChange();
        model()->setProjectDirty();
    }
}

/**
 * @brief SongsFolderTreeItem::importFoldersModal
 * @param p_parentWidget
 * @param srcFileNames
 * @param dstRow
 * @return
 *
 * Implementation of importing folder to project
 */
bool SongsFolderTreeItem::importFoldersModal(QWidget *p_parentWidget, const QStringList &srcFileNames, int dstRow)
{
   // Step - weight       - description
   // ------------------------------------------------------------------
   // 1    - 1            - open file and extract
   // 2    - 1            - Verify version
   // 3    - 1            - count effects, update number of operations and read config file
   // 4    - 1            - count songs and update number of operations
   // 5    - 1            - create folder
   // 6    - 1            - copy config file
   // 7    - song count   - copy songs
   // 8    - effect count - copy effects / register usage
   // 9    - song count   - Replace Effect Name and UUID in copied songs
   // 10   - 1            - parse folder
   // 11   - 1            - confirm changes and close file (flushing)

   // For each imported file, the operation count will be ( (2*song count) + effect count + 8)
   // The original count assumes 0 effects and 0 songs
   const int OP_CNT = 8;

   QProgressDialog progress(tr("Importing Folders..."), tr("Abort"), 0, srcFileNames.count() * OP_CNT, p_parentWidget);
   progress.setWindowModality(Qt::WindowModal);
   progress.setMinimumDuration(0);
   progress.setValue(0);

   int remainingFiles = srcFileNames.count();

   foreach(const QString &srcFileName, srcFileNames){
      remainingFiles--;

      // 1 - Open file
      if(progress.wasCanceled()){
         break;
      }

      if(!QFileInfo(srcFileName).exists()){
         progress.show(); // always make sure progress is shown before displaying another dialog.
         QMessageBox::warning(p_parentWidget, tr("Import Folder"), tr("Portable Song Folder %1 does not exist\n\nSkipping file...").arg(QFileInfo(srcFileName).absoluteFilePath()));
         progress.setValue(progress.maximum() - (remainingFiles * OP_CNT));
         continue;
      }

      QuaZip zip(srcFileName);

      if(!zip.open(QuaZip::mdUnzip)){
         progress.show(); // always make sure progress is shown before displaying another dialog.
         QMessageBox::warning(p_parentWidget, tr("Import Folder"), tr("Cannot Parse Portable Folder %1\n\nSkipping file...").arg(QFileInfo(srcFileName).absoluteFilePath()));
         progress.setValue(progress.maximum() - (remainingFiles * OP_CNT));
         continue;
      }

      QuaZipFile zipFile(&zip);
      QStringList zipFileNames = zip.getFileNameList();
      QStringList effectsList, songsList;

      // find necessary files
      foreach (auto& x, zipFileNames) {
        if (x.startsWith("SONGS/", Qt::CaseInsensitive) && x.endsWith("." BMFILES_MIDI_BASED_SONG_EXTENSION, Qt::CaseInsensitive))
            songsList.append(x);
        else if (x.startsWith("EFFECTS/", Qt::CaseInsensitive) && x.endsWith("." BMFILES_WAVE_EXTENSION, Qt::CaseInsensitive))
            effectsList.append(x);
      }
      if (effectsList.isEmpty() && songsList.isEmpty()) {
        progress.show(); // always make sure progress is shown before displaying another dialog.
        QMessageBox::warning(p_parentWidget, tr("Import Folder"), tr("Cannot Parse Portable Folder %1\nNo songs or Accent Hit effects found\n\nSkipping file...").arg(QFileInfo(srcFileName).absoluteFilePath()));
        zip.close();
        progress.setValue(progress.maximum() - (remainingFiles * OP_CNT));
        continue;
      }

      progress.setValue(progress.value() + 1);

      // 2 - Determine file version
      if(progress.wasCanceled()){
         zip.close();
         break;
      }

      QMap<QString, qint32> versionFileContent;
      int fileVersion = -1;
      int fileRevision = -1;
      int fileBuild = -1;

      if(!zipFileNames.contains(BMFILES_PORTABLE_FOLDER_VERSION, Qt::CaseInsensitive)){
         fileVersion = 0;
         fileRevision = 0;
         fileBuild = 0;
      } else {
         zip.setCurrentFile(BMFILES_PORTABLE_FOLDER_VERSION, QuaZip::csInsensitive);
         if(!zipFile.open(QIODevice::ReadOnly)){
            QMessageBox::warning(p_parentWidget, tr("Import Folder"), tr("Unable to determine file version of Portable Folder %1\nCannot open version file\n\nSkipping file...").arg(QFileInfo(srcFileName).absoluteFilePath()));
            zip.close();
            progress.setValue(progress.maximum() - (remainingFiles * OP_CNT));
            continue;
         }
         QDataStream fin(&zipFile);
         fin >> versionFileContent;
         zipFile.close();

         fileVersion = versionFileContent.value("version", -1);
         fileRevision = versionFileContent.value("revision", -1);
         fileBuild = versionFileContent.value("build", -1);

         if(fileVersion < 0 || fileRevision < 0 || fileBuild < 0){
            QMessageBox::warning(p_parentWidget, tr("Import Folder"), tr("Unable to determine file version of Portable Folder %1\nInvalid version file content\n\nSkipping file...").arg(QFileInfo(srcFileName).absoluteFilePath()));
            zip.close();
            progress.setValue(progress.maximum() - (remainingFiles * OP_CNT));
            continue;
         }
      }

      // Validate that the verison number is supported
      if(!(fileVersion == 0 && fileRevision == 0) &&
         !(fileVersion == 1 && fileRevision == 0)){
         QMessageBox::warning(p_parentWidget, tr("Import Folder"), tr("File version for file %1 is not supported (version = %2, revision = %3, build = 0x%4\n\nSkipping file...").arg(QFileInfo(srcFileName).absoluteFilePath()).arg(fileVersion).arg(fileRevision).arg(fileBuild,4,16,QChar('0')));
         zip.close();
         progress.setValue(progress.maximum() - (remainingFiles * OP_CNT));
         continue;
      }

      progress.setValue(progress.value() + 1);

      // 3 - Count effects, update number of operations and read config file
      // 3.1 Count the amount of wave file in archive
      // 3.2 Count the amount of files in CONFIG.CSV
      // 3.3 Compare counts
      // 3.4 Read USAGE.BCF to create effectUsageMap
      if(progress.wasCanceled()){
         // Clean Up
         zip.close();
         break;
      }

      // 3.1 Count the amount of wave file in archive
      progress.setMaximum(progress.maximum() + effectsList.count());

      // 3.2 Count the amount of files in CONFIG.CSV
      zip.setCurrentFile("EFFECTS/" BMFILES_NAME_TO_FILE_MAPPING, QuaZip::csInsensitive);
      CsvConfigFile zipEfxConfFile;
      zipEfxConfFile.read(zipFile);

      // 3.3 Read USAGE.BCF to create effectUsageMap
      // Mapping file
      zip.setCurrentFile("EFFECTS/" BMFILES_EFFECT_USAGE_FILE_NAME, QuaZip::csInsensitive);
      if(!zipFile.open(QIODevice::ReadOnly)){
         progress.show(); // always make sure progress is shown before displaying another dialog.
         QMessageBox::warning(p_parentWidget, tr("Import Folder"), tr("Missing Accent Hit Effect to Song mapping info\nUnable to open Accent Hit Effect to Song mapping information of Portable Folder %1\n\nSkipping file...")
                    .arg(QFileInfo(srcFileName).absoluteFilePath()));
         zip.close();
         progress.setValue(progress.maximum() - (remainingFiles * OP_CNT));
         continue;
      }

      QDataStream usageFin(&zipFile);
      QMap<QByteArray, QMap<QString, qint32>> effectUsageMap;
      usageFin >> effectUsageMap;
      zipFile.close();

      progress.setValue(progress.value() + 1);

      // 4 - count songs and update number of operations
      if(progress.wasCanceled()){
         // Clean Up
         zip.close();
         break;
      }

      // There are 2 steps that loop on all songs
      progress.setMaximum(progress.maximum() + 2*songsList.count());

      progress.setValue(progress.value() + 1);

      // 5 - create folder
      if(progress.wasCanceled()){
         // Clean Up
         zip.close();
         break;
      }

      // read info file in order to retrieve expected folder name
      zip.setCurrentFile("PARAMS/" BMFILES_INFO_MAP_FILE_NAME, QuaZip::csInsensitive);
      if(!zipFile.open(QIODevice::ReadOnly)){
         progress.show(); // always make sure progress is shown before displaying another dialog.
         QMessageBox::warning(p_parentWidget, tr("Import Folder"), tr("Unable to open Song Folder name information of Portable Folder %1\n\nSkipping file...").arg(QFileInfo(srcFileName).absoluteFilePath()));
         zip.close();
         progress.setValue(progress.maximum() - (remainingFiles * OP_CNT));
         continue;
      }

      QDataStream paramsInfoFin(&zipFile);
      QMap<QString, QVariant> paramsInfoMap;
      paramsInfoFin >> paramsInfoMap;
      zipFile.close();

      // Retrieve name
      if(!paramsInfoMap.contains("folder_name")){
         progress.show(); // always make sure progress is shown before displaying another dialog.
         QMessageBox::warning(p_parentWidget, tr("Import Folder"), tr("Invalid content in Song Folder name information of Portable Folder %1\n\nSkipping file...")
                  .arg(QFileInfo(srcFileName).absoluteFilePath()));
         zip.close();
         progress.setValue(progress.maximum() - (remainingFiles * OP_CNT));
         continue;
      }

      // Create folder
      if(!model()->insertRow(dstRow, model()->songsFolderIndex())){
         progress.show(); // always make sure progress is shown before displaying another dialog.
         QMessageBox::critical(p_parentWidget, tr("Import Folder"), tr("Unable to add a new Song Folder for Portable Folder %1.\n\nAborting import...").arg(QFileInfo(srcFileName).absoluteFilePath()));
         zip.close();
         progress.setValue(progress.maximum());
         return false;
      }

      // Rename Folder
      // Note: setData will manage duplicate names
      model()->songsFolder()->child(dstRow)->setData(NAME, paramsInfoMap.value("folder_name")); // bypass undo/redo capturing data change

      SongFolderTreeItem *p_songFolderModel = static_cast<SongFolderTreeItem *>(child(dstRow));
      QDir folderDir(p_songFolderModel->folderFI().absoluteFilePath());

      progress.setValue(progress.value() + 1);

      // 6 - copy config file
      //     Dummy copy, content will be parsed at the end
      if(progress.wasCanceled()){
         // Clean Up

         // Remove folder from file system first!
         if(folderDir.exists()){
            folderDir.removeRecursively();
         }
         // Remove folder from model
         // NOTE : Only works once folder removed from file system
         model()->removeRow(dstRow, model()->songsFolderIndex());

         zip.close();
         break;
      }

      QFile configFile(folderDir.absoluteFilePath(BMFILES_NAME_TO_FILE_MAPPING));

      zip.setCurrentFile("SONGS/" BMFILES_NAME_TO_FILE_MAPPING, QuaZip::csInsensitive);
      if(!zipFile.open(QIODevice::ReadOnly)){
         progress.show(); // always make sure progress is shown before displaying another dialog.
         QMessageBox::warning(p_parentWidget, tr("Import Folder"), tr("Unable to copy songs config file from Portable Folder %1\n\nSkipping file...").arg(QFileInfo(srcFileName).absoluteFilePath()));
         // Remove folder from file system first!
         if(folderDir.exists()){
            folderDir.removeRecursively();
         }
         // Remove folder from model
         // NOTE : Only works once folder removed from file system
         model()->removeRow(dstRow, model()->songsFolderIndex());

         zip.close();
         progress.setValue(progress.maximum() - (remainingFiles * OP_CNT));
         continue;
      }

      // Use truncate in order to make sure we overwrite
      if(!configFile.open(QIODevice::WriteOnly | QIODevice::Truncate)){
         progress.show(); // always make sure progress is shown before displaying another dialog.
         QMessageBox::warning(p_parentWidget, tr("Import Folder"), tr("Unable to copy songs config file from Portable Folder %1\n\nSkipping file...").arg(QFileInfo(srcFileName).absoluteFilePath()));
         zipFile.close();

         // Remove folder from file system first!
         if(folderDir.exists()){
            folderDir.removeRecursively();
         }
         // Remove folder from model
         // NOTE : Only works once folder removed from file system
         model()->removeRow(dstRow, model()->songsFolderIndex());

         zip.close();
         progress.setValue(progress.maximum() - (remainingFiles * OP_CNT));
         continue;
      }

      // Copy
      configFile.write(zipFile.readAll());
      configFile.close();
      zipFile.close();

      progress.setValue(progress.value() + 1);

      // 7 - copy songs folder content (create new uuid for each copied song)
      //     Dummy copy, content will be parsed at the end
      if(progress.wasCanceled()){
         // Clean Up

         // Remove folder from file system first!
         if(folderDir.exists()){
            folderDir.removeRecursively();
         }
         // Remove folder from model
         // NOTE : Only works once folder removed from file system
         model()->removeRow(dstRow, model()->songsFolderIndex());

         zip.close();
         break;
      }


      QStringList parseErrors;

      QList<QUuid> oldUuidList;
      QList<QUuid> newUuidList;
      QMap<QByteArray, QUuid> newUuidMap;
      QFileInfoList newSongsInfoList;

      // in order to allow continue out of internal for loop
      bool internalError = false;

      foreach (auto& songFullName, songsList) {
         auto songFileName = QFileInfo(songFullName).fileName().toUpper();
         if(progress.wasCanceled()){
            // Clean up will be performed by next cancel check
            break;
         }

         QStringList childrenTypes = p_songFolderModel->data(CHILDREN_TYPE).toString().split(",");
         if(!childrenTypes.contains(songFileName.split(".").last(), Qt::CaseInsensitive)){
            progress.show(); // always make sure progress is shown before displaying another dialog.
            QMessageBox::warning(p_parentWidget, tr("Import Folder"), tr("Internal Song Format is not supported for %1\n\nSkipping file...").arg(QFileInfo(srcFileName).absoluteFilePath()));

            // Clean up will be performed out of for loop
            internalError = true;
            break;
         }
         // Dummy copy of file Model will be updated later by parsing content
         QFileInfo outSongFI(folderDir.absoluteFilePath(songFileName.split("/").last()));
         QFile outSongFile(outSongFI.absoluteFilePath());

         zip.setCurrentFile(songFullName, QuaZip::csInsensitive);
         if(!zipFile.open(QIODevice::ReadOnly)){
            progress.show(); // always make sure progress is shown before displaying another dialog.
            QMessageBox::warning(p_parentWidget, tr("Import Folder"), tr("Unable to read song file from Portable Folder %1. Unable to open input file %2\n\nSkipping file...").arg(QFileInfo(srcFileName).absoluteFilePath()).arg(songFileName));

            // Clean up will be performed out of for loop
            internalError = true;
            break;
         }

         if(!outSongFile.open(QIODevice::WriteOnly)){
            progress.show(); // always make sure progress is shown before displaying another dialog.
            QMessageBox::warning(p_parentWidget, tr("Import Folder"), tr("Unable to copy song file from Portable Folder %1. Unable to create output file %2\n\nSkipping file...") .arg(QFileInfo(srcFileName).absoluteFilePath()) .arg(outSongFI.absoluteFilePath()));

            zipFile.close();
            // Clean up will be performed out of for loop
            internalError = true;
            break;
         }
         // Copy
         outSongFile.write(zipFile.readAll());
         outSongFile.close();
         zipFile.close();

         // Create new UUID for song
         oldUuidList.append(SongFileModel::extractUuid(zipFile, &parseErrors));
         newUuidList.append(QUuid::createUuid());
         newUuidMap.insert(oldUuidList.last().toByteArray(), newUuidList.last());
         newSongsInfoList.append(folderDir.absoluteFilePath(outSongFI.fileName()));

         if(!parseErrors.empty()){
            QString errorMsg;
            for(int i = 0; i < parseErrors.count(); i++){
               errorMsg += parseErrors.at(i) + "\n";
            }
            progress.show(); // always make sure progress is shown before displaying another dialog.
            QMessageBox::warning(p_parentWidget, tr("Import Folder"), tr("Error while copying Accent Hit effects:\n%1").arg(errorMsg));
         }
         progress.setValue(progress.value() + 1);
      }

      if(internalError){
         // Clean Up

         // Remove folder from file system first!
         if(folderDir.exists()){
            folderDir.removeRecursively();
         }
         // Remove folder from model
         // NOTE : Only works once folder removed from file system
         model()->removeRow(dstRow, model()->songsFolderIndex());

         zip.close();
         progress.setValue(progress.maximum() - (remainingFiles * OP_CNT));
         continue;
      }

      // 8 - copy effects
      if(progress.wasCanceled()){
         // Clean Up

         // Remove folder from file system first!
         if(folderDir.exists()){
            folderDir.removeRecursively();
         }
         // Remove folder from model
         // NOTE : Only works once folder removed from file system
         model()->removeRow(dstRow, model()->songsFolderIndex());

         zip.close();
         break;
      }

      // NOTE: duplicates are handled by effectFolder
      QMap<QString, QString> effectNewNameMap;
      QList<EffectFileItem *> effectFileItemList;

      foreach (auto& effectFullName, effectsList) {
         auto effectFileName = QFileInfo(effectFullName).fileName().toUpper();
         if(progress.wasCanceled()){
            // Clean up will be performed by next cancel check
            break;
         }

         // Add effect usage. With previously extracted effectUsageMap, each effect usage count is accounted for.
         QString originalLongName = zipEfxConfFile.fileName2LongName(effectFileName);

         zip.setCurrentFile(effectFullName, QuaZip::csInsensitive);

         EffectFileItem *p_effectFileItem = nullptr;
         foreach(const QByteArray &oldUuid, effectUsageMap.keys()){
            // Determine if effect that we want to add is used by song
            QMap<QString, qint32> innerMap = effectUsageMap.value(oldUuid);

            //if(innerMap.contains(originalLongName)){
            if(innerMap.contains(effectFileName)){
               // find corresponding new uuid
               p_effectFileItem = model()->effectFolder()->addUse(zipFile, originalLongName, newUuidMap.value(oldUuid), false, innerMap.value(effectFileName));
            }
         }

         if(p_effectFileItem != nullptr){
            effectFileItemList.append(p_effectFileItem);
            effectNewNameMap.insert(originalLongName, p_effectFileItem->data(NAME).toString());
         }
         progress.setValue(progress.value() + 1);
      }

      // 9 - Replace Effect Name and UUID in copied songs
      if(progress.wasCanceled()){
         // Clean Up
         // Discard changes from Accent hit usage (created in step 8)
         foreach(const QUuid &newUuid, newUuidList){
            model()->effectFolder()->discardUseChanges(newUuid);
         }

         // Remove folder from file system first!
         if(folderDir.exists()){
            folderDir.removeRecursively();
         }
         // Remove folder from model
         // NOTE : Only works once folder removed from file system
         model()->removeRow(dstRow, model()->songsFolderIndex());

         zip.close();
         break;
      }

      internalError = false;

      foreach(const QFileInfo &songFI, newSongsInfoList){
         if(progress.wasCanceled()){
            // Clean up will be performed by next cancel check
            break;
         }

         SongFileModel * p_songFileModel = new SongFileModel;

         parseErrors.clear();

         // Read file content
         QFile finout(folderDir.absoluteFilePath(songFI.fileName()));

         if(!finout.open(QIODevice::ReadOnly)){
            qWarning() << "SongsFolderTreeItem::importFoldersModal - ERROR - Unable to open file " << QFileInfo(finout).absoluteFilePath() << ", exists:" << finout.exists();

            progress.show(); // always make sure progress is shown before displaying another dialog.
            QMessageBox::warning(p_parentWidget, tr("Import Folder"), tr("Unable to open file %1\n\nSkipping file...").arg(QFileInfo(finout).absoluteFilePath()));

            // Clean up will be performed out of for loop
            internalError = true;
            break;
         }
         p_songFileModel->readFromFile(finout, &parseErrors);
         finout.close();

         if(!parseErrors.empty()){
            QString errorMsg;

            for(int i = 0; i < parseErrors.count(); i++){
               errorMsg += parseErrors.at(i) + "\n";
            }
            progress.show(); // always make sure progress is shown before displaying another dialog.

            QMessageBox::warning(p_parentWidget, tr("Import Folder"), tr("Error while parsing song content:\n%1").arg(errorMsg));

            // Clean up will be performed out of for loop
            internalError = true;
            break;
         }

         // Replace Uuid to avoid dupplication
         p_songFileModel->setSongUuid(newUuidMap.value(p_songFileModel->songUuid().toByteArray()));

         // Replace Effect
         foreach(const QString &oldName, effectNewNameMap.keys()){
            p_songFileModel->replaceEffectFile(oldName, effectNewNameMap.value(oldName));
         }

         if(!finout.open(QIODevice::WriteOnly)){
            qWarning() << "SongsFolderTreeItem::importFoldersModal - ERROR - Unable to open file " << QFileInfo(finout).absoluteFilePath() << ", exists:" << finout.exists();

            progress.show(); // always make sure progress is shown before displaying another dialog.
            QMessageBox::warning(p_parentWidget, tr("Import Folder"), tr("Unable to open file %1\n\nSkipping file...").arg(QFileInfo(finout).absoluteFilePath()));

            // Clean up will be performed out of for loop
            internalError = true;
            break;
         }
         p_songFileModel->writeToFile(finout);
         finout.close();

         progress.setValue(progress.value() + 1);

      }

      if(internalError){
         // Clean Up
         // Discard changes from Accent hit usage (created in step 8)
         foreach(const QUuid &newUuid, newUuidList){
            model()->effectFolder()->discardUseChanges(newUuid);
         }

         // Remove folder from file system first!
         if(folderDir.exists()){
            folderDir.removeRecursively();
         }
         // Remove folder from model
         // NOTE : Only works once folder removed from file system
         model()->removeRow(dstRow, model()->songsFolderIndex());

         zip.close();
         progress.setValue(progress.maximum() - (remainingFiles * OP_CNT));
         continue;
      }

      // 10 - Parse folder content and add it to project
      //      This is the last step where cancel is allowed

      if(progress.wasCanceled()){
         // Clean Up
         // Discard changes from Accent hit usage (created in step 8)
         foreach(const QUuid &newUuid, newUuidList){
            model()->effectFolder()->discardUseChanges(newUuid);
         }

         // Remove folder from file system first!
         if(folderDir.exists()){
            folderDir.removeRecursively();
         }
         // Remove folder from model
         // NOTE : Only works once folder removed from file system
         model()->removeRow(dstRow, model()->songsFolderIndex());

         zip.close();
         progress.setValue(progress.maximum() - (remainingFiles * OP_CNT));
         continue;
      }

      p_songFolderModel->updateModelWithData(true);

      progress.setValue(progress.value() + 1);

      // 11 - close and confirm
      // Save Effect for each song
      foreach(const QUuid &newUuid, newUuidList){
         model()->effectFolder()->saveUseChanges(newUuid);
      }


      p_songFolderModel->computeHash(true);
      computeHash(false);
      propagateHashChange();
      model()->setProjectDirty();

      zip.close();

      dstRow++;

   }

   if(progress.value() < progress.maximum()){
      progress.setValue(progress.maximum());
      return false;
   }
   return true;
}

/**
 * @brief SongsFolderTreeItem::manageParsingErrors
 * @param p_parent
 *
 * Function that manages parsing errors recursively
 * Called in the process of BeatsProjectModel::manageParsingErrors
 */
void SongsFolderTreeItem::manageParsingErrors(QWidget *p_parent)
{
   for(int row = 0; row < childCount(); row++){
      SongFolderTreeItem *p_songFolder = static_cast<SongFolderTreeItem *>(child(row));
      p_songFolder->manageParsingErrors(p_parent);
   }
}

/**
 * @brief SongsFolderTreeItem::replaceDrumsetModal
 * @param p_parentWidget
 * @param oldLongName
 * @param drmSourcePath
 *
 * Browse all children to rename default drumsets
 */
void SongsFolderTreeItem::replaceDrumsetModal(QWidget *p_parentWidget, const QString &oldLongName, DrmFileItem *p_drmFileItem)
{
   // retrieve songs count
   int songCount = childCount();
   if(songCount <= 0){
      return;
   }

   // create progress dialog
   QProgressDialog progress(tr("Refreshing Default Drumset names in project..."), tr("Abort"), 0, songCount, p_parentWidget);
   progress.setWindowModality(Qt::WindowModal);
   progress.setMinimumDuration(0);

   progress.setValue(0);


   for(int row = 0; row < childCount(); row++){
      if(progress.wasCanceled()){
         return;
      }

      SongFolderTreeItem *p_songFolder = static_cast<SongFolderTreeItem *>(child(row));
      p_songFolder->replaceDrumsetModal(&progress, oldLongName, p_drmFileItem);
   }
}


int SongsFolderTreeItem::songCount(){

   int count = 0;

   for(int row = 0; row < childCount(); row++){
      count += child(row)->childCount();
   }
   return count;
}

void SongsFolderTreeItem::initMidiIds() {

    SongFolderTreeItem *tmpChild;
    bool doInit = true;
    int id = 0;
    int ones = 0;
    int zeros = 0;
    for(int row = 0; row < childCount(); row++){
       tmpChild = static_cast<SongFolderTreeItem *>(child(row));
       for (int row2 = 0; row2 < tmpChild->childCount(); row2++) {
           if (tmpChild->child(row2)->data(LOOP_COUNT).toInt() > 1) {
               doInit = false;
               id = tmpChild->child(row2)->data(LOOP_COUNT).toInt();
               break;
           } else if (tmpChild->child(row2)->data(LOOP_COUNT).toInt() < 1) {
               zeros++;
           } else {
               ones++;
           }
       }
       if (!doInit)
           break;
    }
    qDebug() << "doInit" << doInit << id << ones << zeros;
    if (doInit && ones>zeros)
        for(int row = 0; row < childCount(); row++){
            tmpChild = static_cast<SongFolderTreeItem *>(child(row));
            for (int row2 = 0; row2 < tmpChild->childCount(); row2++) {
                tmpChild->child(row2)->setData(LOOP_COUNT, 0);
            }
        }

}

/**
 * @brief SongsFolderTreeItem::childWithFileName
 * @param fileName
 * @return child with given file name, or NULL if not found
 */
SongFolderTreeItem *SongsFolderTreeItem::childWithFileName(const QString &fileName) const{

    SongFolderTreeItem *tmpChild;
    for(int row = 0; row < childCount(); row++){
        tmpChild = static_cast<SongFolderTreeItem *>(child(row));
        if(tmpChild->fileName().compare(fileName, Qt::CaseInsensitive) == 0){
            return tmpChild;
        }
    }
    return nullptr;
}

void SongsFolderTreeItem::setMidiId(QString name, int midiId) {
    int idx = m_CSVFile.indexOfLongName(name);
    qDebug() << "setting midiId" << midiId << m_CSVFile.fileName() << m_CSVFile.children()
             << "config:" << (idx<0 ? "ERNT" : m_CSVFile.longNameAt(idx)) << idx << name;

    qDebug() << "~~~~setMidiId" << idx << midiId;
    if (idx>-1) {
        bool b = m_CSVFile.setMidiId(idx, midiId);
        if (b) m_CSVFile.write();
        b = m_CSVFile.setMidiId(idx, midiId);
        if (b) m_CSVFile.write();
    }
}
