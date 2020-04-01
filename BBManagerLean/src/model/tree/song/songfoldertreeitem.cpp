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

#include <QVariant>
#include <QDebug>
#include <QProgressDialog>
#include <QMessageBox>
#include <QUuid>
#include <QCoreApplication>
#include <QRegularExpression>

#include "songfoldertreeitem.h"
#include "../../filegraph/songpartmodel.h"
#include "../../filegraph/songfilemodel.h"
#include "songfileitem.h"
#include "../project/beatsprojectmodel.h"
#include "../project/effectfoldertreeitem.h"
#include "../project/effectfileitem.h"
#include "../../beatsmodelfiles.h"
#include "portablesongfile.h"

#include "quazip.h"
#include "quazipfile.h"
#include "quazipdir.h"

SongFolderTreeItem::SongFolderTreeItem(BeatsProjectModel *p_Model, SongsFolderTreeItem *parent):
   ContentFolderTreeItem(p_Model, parent)
{
   setName(parent->resolveDuplicateLongName(tr("New Folder"), true));
   setChildrenTypes(QString("%1,%2").arg(QString(BMFILES_MIDI_BASED_SONG_EXTENSION).toUpper()).arg(QString(BMFILES_WAVE_BASED_SONG_EXTENSION).toUpper()));
   setSubFoldersAllowed(false);
   setSubFilesAllowed(true);
   midiId=0;
   setData(LOOP_COUNT, midiId);
}

SongFolderTreeItem::~SongFolderTreeItem()
{
   // Make sure all children SongFileModel (graph) are deleted as well
   for(int i = childCount() - 1; i >= 0; i--){
      SongFileItem *p_songFileItem = static_cast<SongFileItem *>(child(i));
      SongFileModel *p_songFileModel = static_cast<SongFileModel *>(p_songFileItem->filePart());
      removeChildInternal(i); // Delete Tree and make sure superclass doesn't try deleting it after
      delete p_songFileModel; // Delete Graph after tree is deleted
   }

}
/**
 * @brief SongFolderTreeItem::data
 * @param column
 * @return
 *
 * re-implementation of AbstractTreeItem::data
 */
QVariant SongFolderTreeItem::data(int column)
{
    switch (column){
    case NAME:
        return QVariant(name());
    case LOOP_COUNT:
        return midiId;
    case INVALID:
        for(int i = 0; i < childCount(); i++){
            auto invalid = child(i)->data(INVALID).toString();
            if (!invalid.isEmpty()) {
                return tr("\"%1\", song %2 | song #%3", "Leave the right part as is after | symbol!")
                        .arg(data(NAME).toString())
                        .arg(invalid)
                        .arg(i+1);
            }
        }
        return QVariant();
    case MAX_CHILD_CNT:
        return QVariant(MAX_SONGS_PER_FOLDER);
    default:
        return ContentFolderTreeItem::data(column);
    }
}

/**
 * @brief SongFolderTreeItem::setData
 * @param column
 * @param value
 * @return
 *
 * re-implementation of AbstractTreeItem::setData
 * calls computeHash and model()->setArchiveDirty when appropriate
 */

bool SongFolderTreeItem::setData(int column, const QVariant & value)
{
    QString resolvedName;
    switch (column){
    case NAME:
        resolvedName = static_cast<ContentFolderTreeItem *>(parent())->resolveDuplicateLongName(value.toString(), false);
        setName(resolvedName);
        static_cast<ContentFolderTreeItem *>(parent())->updateChildContent(this, true); // NOTE: should already have been created with default name
        // Update hash and propagate change down to root
        computeHash(false);
        propagateHashChange();
        model()->setProjectDirty();
        return true;
    case LOOP_COUNT:
    {
        midiId = value.toInt();
        qDebug() << "setting midi id" << midiId << isFile() << isFolder() << data(NAME);
        static_cast<SongsFolderTreeItem *>(parent())->setMidiId(data(NAME).toString(), midiId);
        return true;
    }
    default:
        return ContentFolderTreeItem::setData(column, value);
    }
}

/**
 * @brief SongFolderTreeItem::flags
 * @param column
 * @return
 *
 * re-implementation of AbstractTreeItem::flag
 */
Qt::ItemFlags SongFolderTreeItem::flags(int column)
{
   Qt::ItemFlags tempFlags = Qt::NoItemFlags;
   switch (column){
      case NAME:
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
 * @brief SongFolderTreeItem::insertNewChildAt
 * @param row
 *
 * re-implementation of AbstractTreeItem::insertNewChildAt
 */
void SongFolderTreeItem::insertNewChildAt(int row)
{
   SongFileModel * p_songFileModel = new SongFileModel;
   SongFileItem * p_FilePartItem = new SongFileItem(p_songFileModel, this);

   insertChildAt(row, p_FilePartItem);
   createChildContentAt(row, true);


   // Update hash and propagate change down to root
   computeHash(true);
   propagateHashChange();
   model()->setProjectDirty();

}

/**
 * @brief SongFolderTreeItem::removeChild
 * @param row
 *
 * re-implementation of AbstractTreeItem::removeChild
 */
void SongFolderTreeItem::removeChild(int row)
{
   // 1 - Retrieve p_songFileModel before deletion in file graph
   SongFileItem *p_songFileItem = static_cast<SongFileItem *>(child(row));
   SongFileModel *p_songFileModel = static_cast<SongFileModel *>(p_songFileItem->filePart());

   // 2 - Clear Effect Ptr
   p_songFileItem->clearEffectUsage();

   // 3 - Delete in Superclass
   ContentFolderTreeItem::removeChild(row);

   // 4 - Delete in File Graph
   // File Part needs to be deleted separately. Will delete the whole subtree.
   delete p_songFileModel;

   // Update hash and propagate change down to root
   computeHash(false);
   propagateHashChange();
   model()->setProjectDirty();

}

/**
 * @brief SongFolderTreeItem::createFileWithData Creates SongFileItem in model out of song file data.
 * @param index
 * @param longName
 * @param fileName
 *
 * re-implementation of ContentFolderTreeItem::createFileWithData
 * Should only be called as part of ContentFolderTreeItem::updateModelWithData
 */
bool SongFolderTreeItem::createFileWithData(int index, const QString& sourceLongName, const QString& sourceFileName)
{
    QString longName(sourceLongName);
    QString fileName(sourceFileName);

   // Verify item can be inserted at this index
   if(index > childCount()){
      qWarning() << "SongFolderTreeItem::createFileWithData - ERROR - cannot insert child at index " << index << ". childCount() = " << childCount();
      return false;
   }

   FilePartItem *child = nullptr;

   // 1 - Verify if child exists at required index
   if(index < childCount() && childItems()->at(index)->data(NAME).toString().compare(longName) == 0){
      // nothing to do at this level
      // set child for exploration
      child = qobject_cast<FilePartItem *>(childItems()->at(index));
      if(!child){
         qWarning() << "SongFolderTreeItem::createFileWithData - ERROR - child of type " << childItems()->at(index)->metaObject()->className() << " is not a FilePartItem.";
         return false;
      }
   }

   // 2 - Verify if child with this name exists at a different index
   if(!child){
      // Technically, all previous indexes have been replaced by csv content
      for(int i = index + 1; i < childCount(); i++){
         if(childItems()->at(i)->data(NAME).toString().compare(longName) == 0){
            child = qobject_cast<FilePartItem *>(childItems()->at(i));
            if(!child){
               qWarning() << "SongFolderTreeItem::createFileWithData - ERROR - child of type " << childItems()->at(i)->metaObject()->className() << " is not a FilePartItem.";
               return false;
            }
            model()->moveItem(child, i, index);
         }
      }
   }

    auto saveCSVandRehash = false;
    // 3 - If the source file name contains full path, make sure it makes it to our content directory
    if (fileName.contains(QRegularExpression("[\\/]"))) {
        // 3.1 - resolve full name
        longName = resolveDuplicateLongName(longName, false);
        // 3.2 - insert it into CSV
        m_CSVFile.insert(index, longName, CsvConfigFile::MIDI_BASED_SONG);
        // 3.3 - get mangled file name from CSV
        QString name = m_CSVFile.fileNameAt(index);
        // 3.4 - copy source file to resolved mangled name inside our directory
        QFile(fileName).copy(QDir(data(ABSOLUTE_PATH).toString()).absoluteFilePath(name));
        // 3.5 - reduce file name from full path to file name only
        fileName = name;
        // 3.6 - we will need to save CSV and rehash afterwards
        saveCSVandRehash = true;
    } else if (!m_CSVFile.containsFileName(fileName)) {
        // refresh CSV file
        m_CSVFile.append(longName, CsvConfigFile::MIDI_BASED_SONG);
    }

    // 4 - Create child if does not exist
    if(!child){
        SongFileModel * p_songFileModel = new SongFileModel;
        QStringList parseErrors;
        // Read file content
        QFile fin(QDir(folderFI().absoluteFilePath()).absoluteFilePath(fileName));
        fin.open(QIODevice::ReadOnly);
        p_songFileModel->readFromFile(fin, &parseErrors);
        fin.close();
        // Create Tree and populated with file graph content
        child = new SongFileItem(p_songFileModel, this, longName, fileName);
        model()->insertItem(child, index);
        if(!parseErrors.empty()){
            // Manage Error by adding them to model
            child->setData(ERROR_MSG, QVariant(parseErrors));
        }
    }


    // 5 - Save CSV and rehash if we created content file from external source
    if (saveCSVandRehash) {
        m_CSVFile.write();
        child->computeHash(true);
        child->propagateHashChange();
        model()->setProjectDirty();
    }

    return true;
}

/**
 * @brief SongFolderTreeItem::importSongsModal
 * @param p_parentWidget
 * @param srcFileNames
 * @param dstRow
 * @return
 *
 * operation of importing external song
 */
bool SongFolderTreeItem::importSongsModal(QWidget *p_parentWidget, const QStringList &srcFileNames, int row)
{
   // Note estimated operation count = srcFileNames.count * 7
   // 1   - open file and extract
   // 2   - Verify version
   // 2   - count effects and update number of operations
   // 3   - copy song
   // 4-5 - copy effects
   // 6   - parse song
   // 7   - confirm changes and close file (flushing)

   const int OP_CNT = 7;

   QProgressDialog progress(tr("Importing Songs..."), tr("Abort"), 0, srcFileNames.count() * OP_CNT, p_parentWidget);
   progress.setWindowModality(Qt::WindowModal);
   progress.setMinimumDuration(0);
   progress.setValue(0);
   
   QDir folderDir(folderFI().absoluteFilePath());

   int remainingFiles = srcFileNames.count();

   foreach(const QString &srcFileName, srcFileNames){
      remainingFiles--;

      // 1 - Open file
      if(progress.wasCanceled()){
         break;
      }

      if(!QFileInfo(srcFileName).exists()){
         progress.show(); // always make sure progress is shown before displaying another dialog.
         QMessageBox::warning(p_parentWidget, tr("Import Song"), tr("Portable Song %1 does not exist\n\nSkipping file...").arg(QFileInfo(srcFileName).absoluteFilePath()));
         progress.setValue(progress.maximum() - (remainingFiles * OP_CNT));
         continue;
      }

      QuaZip zip(srcFileName);

      if(!zip.open(QuaZip::mdUnzip)){
         progress.show(); // always make sure progress is shown before displaying another dialog.
         QMessageBox::warning(p_parentWidget, tr("Import Song"), tr("Cannot extract Portable Song %1\n\nSkipping file...").arg(QFileInfo(srcFileName).absoluteFilePath()));
         progress.setValue(progress.maximum() - (remainingFiles * OP_CNT));
         continue;
      }

      QuaZipFile zipFile(&zip);
      QStringList zipFileNames = zip.getFileNameList();
      QStringList effectsList;
      QString song;

      // find necessary files
      foreach (auto& x, zipFileNames) {
        if (x.startsWith("SONG/", Qt::CaseInsensitive) && x.endsWith("." BMFILES_MIDI_BASED_SONG_EXTENSION, Qt::CaseInsensitive))
            song = song.isEmpty() ? x : "*";
        else if (x.startsWith("EFFECTS/", Qt::CaseInsensitive) && x.endsWith("." BMFILES_WAVE_EXTENSION, Qt::CaseInsensitive))
            effectsList.append(x);
      }
      if (song.size() < 1) {
         progress.show(); // always make sure progress is shown before displaying another dialog.
         QMessageBox::warning(p_parentWidget, tr("Import Song"), tr("Cannot parse Portable Song %1 - not found\n\nSkipping file...").arg(QFileInfo(srcFileName).absoluteFilePath()));

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

      if(!zipFileNames.contains(BMFILES_PORTABLE_SONG_VERSION, Qt::CaseInsensitive)){
         fileVersion = 0;
         fileRevision = 0;
         fileBuild = 0;
      } else {
         zip.setCurrentFile(BMFILES_PORTABLE_SONG_VERSION, QuaZip::csInsensitive);
         if(!zipFile.open(QIODevice::ReadOnly)){
            QMessageBox::warning(p_parentWidget, tr("Import Song"), tr("Unable to determine file version of Portable Song %1\n\nSkipping file...").arg(QFileInfo(srcFileName).absoluteFilePath()));
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
            QMessageBox::warning(p_parentWidget, tr("Import Song"),tr("Unable to determine file version of Portable Song %1\n\nSkipping file...").arg(QFileInfo(srcFileName).absoluteFilePath()));
            zip.close();
            progress.setValue(progress.maximum() - (remainingFiles * OP_CNT));
            continue;
         }
      }

      // Validate that the verison number is supported
      if(!(fileVersion == 0 && fileRevision == 0) &&
         !(fileVersion == 1 && fileRevision == 0)){

         QMessageBox::warning(p_parentWidget, tr("Import Song"), tr("File version for file %1 is not supported (version = %2, revision = %3, build = 0x%4\n\nSkipping file...")
                  .arg(QFileInfo(srcFileName).absoluteFilePath()).arg(fileVersion).arg(fileRevision).arg(fileBuild,4,16,QChar('0')));
         zip.close();
         progress.setValue(progress.maximum() - (remainingFiles * OP_CNT));
         continue;
      }

      progress.setValue(progress.value() + 1);

      // 3 - Count effects and update max value and read config file
      if(progress.wasCanceled()){
         // Clean Up
         zip.close();
         break;
      }

      progress.setMaximum(progress.maximum() - 2 + effectsList.count());
      CsvConfigFile zipEfxConfFile;
      zip.setCurrentFile("EFFECTS/" BMFILES_NAME_TO_FILE_MAPPING, QuaZip::csInsensitive);
      zipEfxConfFile.read(zipFile);

      // Mapping file
      QMap<QString, qint32> effectUsageMap;
      zip.setCurrentFile("EFFECTS/" BMFILES_EFFECT_USAGE_FILE_NAME, QuaZip::csInsensitive);
      if (zipFile.open(QIODevice::ReadOnly)){
          QDataStream usageFin(&zipFile);
          usageFin >> effectUsageMap;
          zipFile.close();
      }

      progress.setValue(progress.value() + 1);

      // 4 - copy song
      if(progress.wasCanceled()){
         // Clean Up
         zip.close();
         break;
      }

      QStringList childrenTypes = data(CHILDREN_TYPE).toString().split(",");
      if(!childrenTypes.contains(song.split(".").last(), Qt::CaseInsensitive)){
         progress.show(); // always make sure progress is shown before displaying another dialog.
         QMessageBox::warning(p_parentWidget, tr("Import Song"), tr("Internal Song Format is not supported for %1\n\nSkipping file...").arg(QFileInfo(srcFileName).absoluteFilePath()));
         zip.close();
         progress.setValue(progress.maximum() - (remainingFiles * OP_CNT));
         continue;
      }

      // retrieve song name
      // NOTE : DIFFERENT IN VERSION 0 than VERSION 1+
      QString songName;
      if(fileVersion == 0){
         songName = resolveDuplicateLongName(song.split(".").first(), false);
      } else {
         // parse csv file if present
         zip.setCurrentFile("SONG/" BMFILES_NAME_TO_FILE_MAPPING, QuaZip::csInsensitive);
         CsvConfigFile zipSongConfFile;
         auto name = QFileInfo(song).fileName().toUpper();
         if (zipSongConfFile.read(zipFile) && zipSongConfFile.containsFileName(name)) {
             songName = resolveDuplicateLongName(zipSongConfFile.fileName2LongName(name), false);
         } else {
             songName = resolveDuplicateLongName(QFileInfo(song).completeBaseName(), false);
         }
      }

      // create reference to new file
      if(!m_CSVFile.insert(row, songName, CsvConfigFile::reverseFileExtensionMap().value(song.split(".").last().toUpper()))){
         progress.show(); // always make sure progress is shown before displaying another dialog.
         QMessageBox::warning(p_parentWidget, tr("Import Song"), tr("Unable to create destination file name %1\n\nSkipping file...").arg(songName));
         zip.close();
         progress.setValue(progress.maximum() - (remainingFiles * OP_CNT));
         continue;
      }

      // Copy song content
      zip.setCurrentFile(song, QuaZip::csInsensitive);
      if(!zipFile.open(QIODevice::ReadOnly)){
         progress.show(); // always make sure progress is shown before displaying another dialog.
         QMessageBox::warning(p_parentWidget, tr("Import Song"), tr("Unable to read song data for file %1 to project\n\nSkipping file...").arg(song));
         zip.close();
         m_CSVFile.removeAt(row);
         progress.setValue(progress.maximum() - (remainingFiles * OP_CNT));
         continue;
      }
      QFile outSongFile(folderDir.absoluteFilePath(m_CSVFile.fileNameAt(row)));
      if(!outSongFile.open(QIODevice::WriteOnly)){
         progress.show(); // always make sure progress is shown before displaying another dialog.
         QMessageBox::warning(p_parentWidget, tr("Import Song"), tr("Unable to copy song data for file %1 to project\n\nSkipping file...").arg(outSongFile.fileName()));
         zipFile.close();
         zip.close();
         m_CSVFile.removeAt(row);
         progress.setValue(progress.maximum() - (remainingFiles * OP_CNT));
         continue;
      }

      outSongFile.write(zipFile.readAll());
      outSongFile.close();
      zipFile.close();

      progress.setValue(progress.value() + 1);

      // 5 - copy effects
      if(progress.wasCanceled()){
         // Clean Up
         zip.close();
         break;
      }

      // Generate Uuid that will replace original one in effect usage
      QUuid newUuid = QUuid::createUuid();

      // NOTE: duplicates are handled by effectFolder
      QList<EffectFileItem *> effectFileItemList;

      foreach(const QString &effectFileName, effectsList){
         if(progress.wasCanceled()){
            // Clean Up perform by next cancel check
            break;
         }

         // Add effect usage. With previously extracted effectUsageMap, each effect usage count is accounted for.
         auto originalLongName = QFileInfo(effectFileName).fileName();
         auto mappedName = zipEfxConfFile.fileName2LongName(originalLongName);
         zip.setCurrentFile(effectFileName, QuaZip::csInsensitive);
         EffectFileItem *p_effectFileItem = model()->effectFolder()->addUse(zipFile, mappedName.isEmpty() ? originalLongName: mappedName, newUuid, false, effectUsageMap.value(originalLongName));
         effectFileItemList.append(p_effectFileItem);
      }
      progress.setValue(progress.value() + 1);

      // 6 - Parse song content and add it to project

      if(progress.wasCanceled()){
         // Clean Up
         zip.close();
         model()->effectFolder()->discardUseChanges(newUuid);
         m_CSVFile.removeAt(row);
         outSongFile.remove();
         break;
      }

      SongFileModel * p_songFileModel = new SongFileModel;

      QStringList parseErrors;

      // Read file content
      QFile fin(folderDir.absoluteFilePath(m_CSVFile.fileNameAt(row)));
      if(!fin.open(QIODevice::ReadOnly)){
         progress.show(); // always make sure progress is shown before displaying another dialog.
         QMessageBox::warning(p_parentWidget, tr("Import Song"), tr("Unable to read copied song data for file %1\n\nSkipping file...").arg(folderDir.absoluteFilePath(m_CSVFile.fileNameAt(row))));
         zip.close();
         model()->effectFolder()->discardUseChanges(newUuid);
         m_CSVFile.removeAt(row);
         outSongFile.remove();
         progress.setValue(progress.maximum() - (remainingFiles * OP_CNT));
         continue;
      }
      p_songFileModel->readFromFile(fin, &parseErrors);
      fin.close();

      if(!parseErrors.empty()){
         QString errorMsg;

         for(int i = 0; i < parseErrors.count(); i++){
            errorMsg += parseErrors.at(i) + "\n";
         }

         QMessageBox::warning(p_parentWidget, tr("Import Song"), tr("Error while parsing song content\nInternal parsing errors:\n%1\n\nSkipping file...").arg(errorMsg));

         zip.close();
         model()->effectFolder()->discardUseChanges(newUuid);
         m_CSVFile.removeAt(row);
         outSongFile.remove();
         progress.setValue(progress.maximum() - (remainingFiles * OP_CNT));
         continue;
      }

      // Replace Uuid to avoid dupplication
      p_songFileModel->setSongUuid(newUuid);

      // Create Tree and populated with file graph content
      SongFileItem *p_songFileItem = new SongFileItem(p_songFileModel, this, m_CSVFile.longNameAt(row), m_CSVFile.fileNameAt(row));

      progress.setValue(progress.value() + 1);

      // 7 - close and confirm
      for(int i = 0; i < effectFileItemList.count(); i++){
         p_songFileItem->replaceEffectFile(effectsList.at(i), effectFileItemList.at(i)->data(FILE_NAME).toString());
      }

      model()->insertItem(p_songFileItem, row);
      model()->effectFolder()->saveUseChanges(p_songFileItem->data(UUID).toUuid());
      m_CSVFile.write();

      computeHash(true);
      propagateHashChange();
      model()->setProjectDirty();

      zip.close();

      ++row;
   }


   if(progress.value() < progress.maximum()){
      return false;
   }
   return true;
}

/**
 * @brief SongFolderTreeItem::exportModal
 * @param p_parentWidget
 * @param dstPath
 * @return
 *
 * operation to export a song folder
 */
bool SongFolderTreeItem::exportModal(QWidget *p_parentWidget, const QString &dstPath)
{

   // Step - weight           - description
   // ------------------------------------------------------------------
   // 1    - childCount       - generate effectList
   // 2    - 1                - create output file
   // 3    - 1                - create internal folders
   // 4    - 1                - Create Version File
   // 5    - 1                - Create params BMFILES_INFO_MAP_FILE_NAME
   // 6    - childCount       - append song files
   // 7    - 1                - append 'song file to song name mapping' file
   // 8    - effectList.count - append effect files
   // 9    - 1                - config file
   // 10   - 1                - effect usage file
   // 11   - 1                - close file (flushing)

   // The operation count will be 2*childCount + 8 + effectList.count
   // The original count assumes 0 effects

   QProgressDialog progress(tr("Exporting Folder..."), tr("Abort"), 0, 2*childCount() + 8, p_parentWidget);
   progress.setWindowModality(Qt::WindowModal);
   progress.setValue(0);

   // 1 - generate effectList
   // Maps song uuid with Effect/usageCount map info
   QMap<QByteArray, QMap<QString, qint32> > effectUsageMap;
   QSet<EffectFileItem *> effectSet;
   QList<EffectFileItem *> effectList;

   for(int i = 0; i < childCount(); i++){
      if(progress.wasCanceled()){
         return false;
      }

      QList<EffectFileItem *> songEffectList = static_cast<SongFileItem *>(child(i))->effectList();
      QMap<QString, qint32> songEffectInternalMap = static_cast<SongFileItem *>(child(i))->effectUsageCountMap();
      if(!songEffectInternalMap.empty()){
         QUuid songUuid = static_cast<SongFileModel *>(static_cast<SongFileItem *>(child(i))->filePart())->songUuid();
         effectUsageMap.insert(songUuid.toByteArray(), songEffectInternalMap);
      }
      foreach(EffectFileItem * p_effect, songEffectList)
      {
         if(!effectSet.contains(p_effect)){
            effectSet.insert(p_effect);
            effectList.append(p_effect);
         }
      }
      progress.setValue(progress.value() + 1);
   }

   // Adjust the maximum progress with the amount of effects to process
   progress.setMaximum(progress.maximum() + effectList.count());

   // 2 - Create output file
   if(progress.wasCanceled()){
      return false;
   }

   QuaZip zip(dstPath);
   if(!zip.open(QuaZip::mdCreate)){
      QMessageBox::critical(p_parentWidget, tr("Export Song"), tr("Unable to create archive %1").arg(dstPath));
      return false;
   }

   QuaZipFile outZipFile(&zip);
   progress.setValue(progress.value() + 1);

   // 3 - create internal folders
   if(progress.wasCanceled()){
      zip.close();
      QFile(dstPath).remove();
      return false;
   }

   if(!outZipFile.open(QIODevice::WriteOnly, QuaZipNewInfo("PARAMS/"))){
      progress.show();
      QMessageBox::critical(p_parentWidget, tr("Export Song"), tr("Unable to create PARAMS folder in archive %1").arg(dstPath));
      zip.close();
      QFile(dstPath).remove();
      progress.setValue(progress.maximum());
      return false;
   }
   outZipFile.close();

   if(!outZipFile.open(QIODevice::WriteOnly, QuaZipNewInfo("SONGS/"))){
      progress.show();
      QMessageBox::critical(p_parentWidget, tr("Export Song"), tr("Unable to create SONGS folder in archive %1").arg(dstPath));
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
   progress.setValue(progress.value() + 1);


   // 4 - Create Version File
   if(progress.wasCanceled()){
      zip.close();
      QFile(dstPath).remove();
      return false;
   }

   QMap<QString, qint32> versionFileContent;
   versionFileContent.insert("version", PORTABLEFOLDERFILE_VERSION);
   versionFileContent.insert("revision", PORTABLEFOLDERFILE_REVISION);
   versionFileContent.insert("build", PORTABLEFOLDERFILE_BUILD);


   if(!outZipFile.open(QIODevice::WriteOnly, QuaZipNewInfo(BMFILES_PORTABLE_FOLDER_VERSION))){
      progress.show();
      QMessageBox::critical(p_parentWidget, tr("Export Song"), tr("Unable to create file %1 in archive %2").arg(BMFILES_PORTABLE_FOLDER_VERSION).arg(dstPath));
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

   // 5 - Create params BMFILES_INFO_MAP_FILE_NAME
   //     This file is only used to store the original folder name
   if(progress.wasCanceled()){
      zip.close();
      QFile(dstPath).remove();
      return false;
   }

   if(!outZipFile.open(QIODevice::WriteOnly, QuaZipNewInfo(QString("PARAMS/%1").arg(BMFILES_INFO_MAP_FILE_NAME)))){
      progress.show();
      QMessageBox::critical(p_parentWidget, tr("Export Song"), tr("Unable to create file %1 in archive %2").arg(QString("PARAMS/%1").arg(BMFILES_INFO_MAP_FILE_NAME)).arg(dstPath));
      zip.close();
      QFile(dstPath).remove();
      progress.setValue(progress.maximum());
      return false;
   } else {
      // In else block in order to reuse fout name
      QDataStream fout(&outZipFile);

      QMap<QString, QVariant> infoMap;
      infoMap.insert("folder_name", data(NAME));
      fout << infoMap;
   }

   outZipFile.close();
   progress.setValue(progress.value() + 1);

   // 6 - append song files
   // Cancel in for loop

   for(int i = 0; i < childCount(); i++){
      if(progress.wasCanceled()){
         zip.close();
         QFile(dstPath).remove();
         return false;
      }

      QFile songFile(child(i)->data(ABSOLUTE_PATH).toString());
      // input file
      if(!songFile.open(QIODevice::ReadOnly)){
         progress.show();
         QMessageBox::critical(p_parentWidget, tr("Export Song"), tr("Unable to open song file %1").arg(songFile.fileName()));
         zip.close();
         QFile(dstPath).remove();
         progress.setValue(progress.maximum());
         return false;
      }
      //output io device
      if(!outZipFile.open(QIODevice::WriteOnly, QuaZipNewInfo(QString("SONGS/%1").arg(child(i)->data(FILE_NAME).toString()), child(i)->data(ABSOLUTE_PATH).toString()))){
         progress.show();
         QMessageBox::critical(p_parentWidget, tr("Export Song"), tr("Unable to create file %1 in archive %2").arg(QString("SONGS/%1").arg(child(i)->data(FILE_NAME).toString())).arg(dstPath));
         songFile.close();
         zip.close();
         QFile(dstPath).remove();
         progress.setValue(progress.maximum());
         return false;
      }

      // Copy content
      outZipFile.write(songFile.readAll());

      // close input and output
      outZipFile.close();
      songFile.close();
      progress.setValue(progress.value() + 1);
   }


   // 7 - append 'song file to song name mapping' file
   if(progress.wasCanceled()){
      zip.close();
      QFile(dstPath).remove();
      return false;
   }

   QFile songConfigFile(m_CSVFile.fileName());

   // input file
   if(!songConfigFile.open(QIODevice::ReadOnly)){
      progress.show();
      QMessageBox::critical(p_parentWidget, tr("Export Song"), tr("Unable to open song config file %1").arg(songConfigFile.fileName()));
      zip.close();
      QFile(dstPath).remove();
      progress.setValue(progress.maximum());
      return false;
   }
   //output io device
   if(!outZipFile.open(QIODevice::WriteOnly, QuaZipNewInfo(QString("SONGS/%1").arg(QFileInfo(m_CSVFile.fileName()).fileName()), m_CSVFile.fileName()))){
      progress.show();
      QMessageBox::critical(p_parentWidget, tr("Export Song"), tr("Unable to create file %1 in archive %2").arg(QString("SONGS/%1").arg(QFileInfo(m_CSVFile.fileName()).fileName())).arg(dstPath));
      songConfigFile.close();
      zip.close();
      QFile(dstPath).remove();
      progress.setValue(progress.maximum());
      return false;
   }

   // Copy content
   outZipFile.write(songConfigFile.readAll());

   // close input and output
   outZipFile.close();
   songConfigFile.close();

   progress.setValue(progress.value() + 1);

   // 8 - append effect files
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
         QMessageBox::critical(p_parentWidget, tr("Export Song"), tr("Unable to open effect file %1").arg(efxFile.fileName()));
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

   // 9 - write config file content
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

   // 10 - write usage
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

   // 11 - close file
   if(progress.wasCanceled()){
      zip.close();
      QFile(dstPath).remove();
      return false;
   }
   zip.close();
   progress.setValue(progress.value() + 1);

   return true;
}

/**
 * @brief SongFolderTreeItem::manageParsingErrors
 * @param p_parent
 *
 * Function that manages parsing errors recursively
 * Called in the process of BeatsProjectModel::manageParsingErrors
 */
void SongFolderTreeItem::manageParsingErrors(QWidget *p_parent)
{
   for(int row = 0; row < childCount(); row++){
      SongFileItem *p_songFile = static_cast<SongFileItem *>(child(row));
      p_songFile->manageParsingErrors(p_parent);
   }
}

/**
 * @brief replaceDrumsetModal
 * @param p_parentWidget
 * @param oldLongName
 * @param drmSourcePath
 *
 * Browse all children to rename default drumsets
 */
void SongFolderTreeItem::replaceDrumsetModal(QProgressDialog *p_progress, const QString &oldLongName, DrmFileItem *p_drmFileItem)
{
   for(int row = 0; row < childCount(); row++){
      if(p_progress->wasCanceled()){
         return;
      }
      SongFileItem *p_songFile = static_cast<SongFileItem *>(child(row));
      p_songFile->replaceDrumset(oldLongName, p_drmFileItem);

      p_progress->setValue(p_progress->value() + 1);
   }
}

/**
 * @brief SongFolderTreeItem::childWithFileName
 * @param fileName
 * @return child with given file name, or NULL if not found
 */
SongFileItem* SongFolderTreeItem::childWithFileName(const QString &fileName) const{
    SongFileItem *tmpChild;
    for(int row = 0; row < childCount(); row++){
        tmpChild = static_cast<SongFileItem *>(child(row));
        if(tmpChild->fileName().compare(fileName,Qt::CaseInsensitive) == 0){
            return tmpChild;
        }
    }
    return nullptr;
}


