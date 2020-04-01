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
#include <QCryptographicHash>
#include <QFileInfo>

#include "contentfoldertreeitem.h"
#include "../song/songfoldertreeitem.h"
#include "beatsprojectmodel.h"

ContentFolderTreeItem::ContentFolderTreeItem(BeatsProjectModel *p_model, FolderTreeItem *parent):
   FolderTreeItem(p_model, parent),
   m_SubFoldersAllowed(false),
   m_SubFilesAllowed(false)
{
   setName(tr("Dummy Content Folder"));
   m_pProgressDialog = nullptr;
}

/**
 * @brief ContentFolderTreeItem::createProjectSkeleton
 * @return changes performed
 *
 * if changes are performed, computeHash() and model()->setArchiveDirty() need to be called after this call, depending on the process.
 */
bool ContentFolderTreeItem::createProjectSkeleton()
{
   bool changed = false;

   if(fileName().isEmpty()){
      // Skeleton is only created if folderName exists
      return false;
   }
   if(FolderTreeItem::createProjectSkeleton()){
      changed = true;
   }
   QDir dir(folderFI().absoluteFilePath());
   if(m_CSVFile.setFileName(dir.absoluteFilePath(BMFILES_NAME_TO_FILE_MAPPING))){
      changed = true;
   }
   return changed;
}

/**
 * @brief ContentFolderTreeItem::updateDataWithModel
 * @param cleanAll
 *
 * called as part of project constructor
 * requires model()->setArchiveDirty() to be called
 */
void ContentFolderTreeItem::updateDataWithModel(bool cleanAll)
{
   // 1 - Make sure directory exists
   createFolder();
   QFileInfo fi = folderFI();
   if(!fi.exists()){
      qWarning() << "ContentFolderTreeItem::updateDataWithModel - ERROR - Unable to create directory";
      return;
   }

   QDir dir(fi.absoluteFilePath());

   // 2 - Read content of Config file
   if(m_CSVFile.fileName().isEmpty()){
      m_CSVFile.setFileName(dir.absoluteFilePath(BMFILES_NAME_TO_FILE_MAPPING));
   }

   m_CSVFile.read();

   // 3 - Clean model of content that is not allowed
   if(cleanAll){
      for(int i = childItems()->count() - 1; i >= 0; i--){
         AbstractTreeItem * child = childItems()->at(i);
         if(!areSubFilesAllowed() && child->isFile()){
            model()->removeItem(childItems()->at(i), i);
         } else if (!areSubFoldersAllowed() && child->isFolder()){
            model()->removeItem(childItems()->at(i), i);
         }
      }
   }

   // 4 - Add Model items to folder and CSV file
   for(int i = 0; i < childItems()->count(); i++){
      AbstractTreeItem * child = childItems()->at(i);
      if(child->isFolder()){
         if(!areSubFoldersAllowed()){
            if(cleanAll){
               removeChildContentAt(i, false);
            }
         } else {
            createChildContentAt(i, false);
         }
      } else if(child->isFile()){
         if(!areSubFilesAllowed()){
            removeChildContentAt(i, false);
         } else {
            createChildContentAt(i, false);
         }
      } else {
         qWarning() << "ContentFolderTreeItem::updateDataWithModel - Error - Found children that isn't a File of a Folder";
      }
   }

   // 5 - Remove content that needs to be removed from CSV file if possible
   if(cleanAll){
      for(int i = m_CSVFile.count() - 1; i >= childItems()->count(); i--){
         if(m_CSVFile.fileTypeAt(i) == CsvConfigFile::FOLDER){
            if(dir.exists(m_CSVFile.fileNameAt(i))){
               dir.rmdir(m_CSVFile.fileNameAt(i));
            }
         } else {
            QString filePath = dir.absoluteFilePath(m_CSVFile.fileNameAt(i));
            if(QFile::exists(filePath)){
               QFile::remove(filePath);
            }
         }

         m_CSVFile.removeAt(i);
      }
   }

   // 6 - Remove folders and files that are not included in m_CSVFile
   if(cleanAll){
      QFileInfoList entryList = dir.entryInfoList();
      for(int i = 0; i < entryList.count(); i++){
         QFileInfo entry = entryList.at(i);
         if(entry.fileName().compare(BMFILES_NAME_TO_FILE_MAPPING) != 0 &&
               entry.fileName().compare(".") != 0 &&
               entry.fileName().compare("..") != 0 &&
               !m_CSVFile.containsFileName(entry.fileName())){
            if(entry.isDir()){
               // Create a dir with entry
               QDir removedDir(entry.absoluteFilePath());
               removedDir.removeRecursively();
            } else {
               QFile removedFile(entry.absoluteFilePath());
               removedFile.remove();
            }
         }
      }
   }

   // 7 - Save CSV file
   m_CSVFile.write();

   // 8 - Explore children
   for(int i = 0; i < childCount(); i++){
      if(childItems()->at(i)->isFolder()){
         ContentFolderTreeItem* child = qobject_cast<ContentFolderTreeItem*>(childItems()->at(i));
         if(child){
            child->updateDataWithModel(cleanAll);
         } else {
            qWarning() << "ContentFolderTreeItem::updateDataWithModel - TODO - explore other children";
         }
      } else {
         qWarning() << "ContentFolderTreeItem::updateDataWithModel - TODO - explore other children";
      }
   }
}

/**
 * @brief ContentFolderTreeItem::updateModelWithData
 * @param cleanAll
 *
 * called as part of project constructor (directly or recursively through SongsFolderTreeItem::createFolderWithData)
 * May require model()->setArchiveDirty() to be called if cleanAll is true
 */
void ContentFolderTreeItem::updateModelWithData(bool cleanAll)
{
   // 1 - Make sure directory exists
   createFolder();
   QFileInfo fi = folderFI();

   if(!fi.exists()){
      qWarning() << "ContentFolderTreeItem::updateModelWithData - ERROR - Unable to create directory";
      return;
   }

   QDir dir(fi.absoluteFilePath());

   // 2 - Read content of Config file
   if(m_CSVFile.fileName().isEmpty()){
      m_CSVFile.setFileName(dir.absoluteFilePath(BMFILES_NAME_TO_FILE_MAPPING));
   }

   m_CSVFile.read();

   // 3 - Clean CSV file of content that is not allowed
   if(cleanAll){
      for(int i = m_CSVFile.count() - 1; i >= 0; i--){
         if(!areSubFilesAllowed() && m_CSVFile.fileTypeAt(i) != CsvConfigFile::FOLDER){
            m_CSVFile.removeAt(i);
         } else if (!areSubFoldersAllowed() && m_CSVFile.fileTypeAt(i) == CsvConfigFile::FOLDER){
            m_CSVFile.removeAt(i);
         }
      }
   }

   if (m_pProgressDialog != nullptr) {
      m_pProgressDialog->setValue(0);
      m_pProgressDialog->setMinimum(0);
      m_pProgressDialog->setMaximum(m_CSVFile.count() - 1);
      if (!m_pProgressDialog->isVisible())
         m_pProgressDialog->show();
   }

   // 4 - Add CSV content to Model
   for(int i = 0; i < m_CSVFile.count(); i++){
      if (m_pProgressDialog != nullptr)
      {
          m_pProgressDialog->setValue(i);
      }

      if(m_CSVFile.fileTypeAt(i) == CsvConfigFile::FOLDER){
         if(!areSubFoldersAllowed()){
         } else {
            createFolderWithData(i, m_CSVFile.longNameAt(i), m_CSVFile.fileNameAt(i), cleanAll);
         }
      } else if(areSubFilesAllowed()){
         if(m_CSVFile.fileTypeAt(i) == CsvConfigFile::MIDI_BASED_SONG ||
            m_CSVFile.fileTypeAt(i) == CsvConfigFile::WAVE_FILE ||
            m_CSVFile.fileTypeAt(i) == CsvConfigFile::DRUM_SET){
            createFileWithData(i, m_CSVFile.longNameAt(i), m_CSVFile.fileNameAt(i));
         } else {
            qWarning() << "ContentFolderTreeItem::updateModelWithData - TODO - File Type not supported yet";
         }
      } else {
         qWarning() << "ContentFolderTreeItem::updateModelWithData - ERROR - Trying to create file when subfiles are not allowed";
      }
   }
   if ("SongsFolderTreeItem" == data(CPP_TYPE).toString()) {

        for (int i=0; i<m_CSVFile.count(); i++) {

            childItems()->at(i)->setData(LOOP_COUNT, QVariant(m_CSVFile.midiIdAt(i)));
        }
   }


   // 5 - Remove content that needs to be removed from model if possible
   if(cleanAll){
      qWarning() << "ContentFolderTreeItem::updateModelWithData - TODO - Remove content that needs to be removed from model if possible";
   }

   // 6 - Remove folders and files that are not included in m_CSVFile
   if(cleanAll){
      QFileInfoList entryList = dir.entryInfoList();
      for(int i = 0; i < entryList.count(); i++){
         QFileInfo entry = entryList.at(i);
         if(entry.fileName().compare(BMFILES_NAME_TO_FILE_MAPPING) != 0 &&
               entry.fileName().compare(".") != 0 &&
               entry.fileName().compare("..") != 0 &&
               !m_CSVFile.containsFileName(entry.fileName())){
            if(entry.isDir()){
               // Create a dir with entry
               QDir removedDir(entry.absoluteFilePath());
               removedDir.removeRecursively();
            } else {
               QFile removedFile(entry.absoluteFilePath());
               removedFile.remove();
            }
         }
      }
   }

   // 7 - Save CSV file

   m_CSVFile.write();

}

/**
 * @brief createChildContentAt is to be called as part of a child insertion.
 * @param index
 * @param save
 *
 * computeHash() and model()->setArchiveDirty() need to be called after this call.
 */
void ContentFolderTreeItem::createChildContentAt(int index, bool save)
{
   QFileInfo fi = folderFI();
   QDir dir(fi.absoluteFilePath());

   if(childItems()->at(index)->isFolder()){
      //AbstractTreeItem *absChild = childItems()->at(index);
      FolderTreeItem *child = qobject_cast<FolderTreeItem *>(childItems()->at(index));

      if (!m_CSVFile.containsLongName(child->name())){
         m_CSVFile.insert(index, child->name());
      } else if(m_CSVFile.indexOfLongName(child->name()) != index){
         m_CSVFile.remove(child->name());
         m_CSVFile.insert(index, child->name());
      }

      // Create corresponding folder if required
      child->setFileName(m_CSVFile.fileNameAt(index));
      child->createFolder();

   } else if(childItems()->at(index)->isFile()){

      QString longName = childItems()->at(index)->data(NAME).toString();

      if (!m_CSVFile.containsLongName(longName)){
         qWarning() << "ContentFolderTreeItem::createChildContentAt - TODO: determine real file type. MIDI_BASED_SONG is used as default";
         m_CSVFile.insert(index, longName, CsvConfigFile::MIDI_BASED_SONG);
      } else if(m_CSVFile.indexOfLongName(longName) != index){
         qWarning() << "ContentFolderTreeItem::createChildContentAt - TODO: determine real file type. MIDI_BASED_SONG is used as default";
         m_CSVFile.remove(longName);
         m_CSVFile.insert(index, longName, CsvConfigFile::MIDI_BASED_SONG);
      }

      // Create corresponding file if required
      QString filePath = dir.absoluteFilePath(m_CSVFile.fileNameAt(index));
      if(!QFile::exists(filePath)){
         QFile tempFile(filePath);
         if(tempFile.open(QIODevice::WriteOnly)){
            tempFile.close();
         } else {
            qWarning() << "ContentFolderTreeItem::createChildContentAt - ERROR - unable to create " << filePath;
         }
      }

      // Set foldername in children
      childItems()->at(index)->setData(FILE_NAME, QVariant(m_CSVFile.fileNameAt(index)));

   } else {
      qWarning() << "ContentFolderTreeItem::createChildContentAt - Error - Found child that isn't a File of a Folder";
   }

   if(save){
      m_CSVFile.write();
   }
}

/**
 * @brief ContentFolderTreeItem::removeChildContentAt
 * @param index
 * @param save
 *
 * Does not remove the child completely. Call ContentFolderTreeItem::removeChild to remove child completly (without notification to model). And model->remove... otherwise.
 * computeHash() and model()->setArchiveDirty() need to be called after this call.
 */
void ContentFolderTreeItem::removeChildContentAt(int index, bool save)
{
   QFileInfo fi = folderFI();
   QDir dir(fi.absoluteFilePath());

   if(childItems()->at(index)->isFolder()){
      FolderTreeItem *p_Child = qobject_cast<FolderTreeItem *>(childItems()->at(index));
      p_Child->removeAllChildrenContent(save);

      if (m_CSVFile.containsLongName(p_Child->name())){
         if(dir.exists(m_CSVFile.fileNameAt(index))){
            dir.rmdir(m_CSVFile.fileNameAt(index));
         }

         m_CSVFile.remove(p_Child->name());
      }
   } else if(childItems()->at(index)->isFile()){
      QString longName = childItems()->at(index)->data(NAME).toString();
      if (m_CSVFile.containsLongName(longName)){
         QString filePath = dir.absoluteFilePath(m_CSVFile.fileNameAt(index));

         if(QFile::exists(filePath)){
            QFile::remove(filePath);
         }

         m_CSVFile.remove(longName);
      }

   } else {
      qWarning() << "ContentFolderTreeItem::removeChildContentAt - ERROR - Found child that isn't a File of a Folder";
   }


   if(save){
      m_CSVFile.write();
   }
}

/**
 * @brief ContentFolderTreeItem::updateChildContent
 * @param child
 * @param save
 * computeHash() and model()->setArchiveDirty() need to be called after this call.
 */
void ContentFolderTreeItem::updateChildContent(AbstractTreeItem * child, bool save)
{
   for(int i = 0; i < childCount(); i++){
      // compare pointers
      if(child == this->child(i)){
         updateChildContentAt(i, save);
         return;
      }
   }
   qWarning() << "ContentFolderTreeItem::updateChildContent - ERROR - child not found for " << name();
   qWarning() << "   child.name = " << child->data(NAME).toString();
}

/**
 * @brief ContentFolderTreeItem::updateChildContentAt
 * @param index
 * @param save
 * computeHash() and model()->setArchiveDirty() need to be called after this call.
 */
void ContentFolderTreeItem::updateChildContentAt(int index, bool save)
{
   if (index >= m_CSVFile.count()){
      qWarning() << "ContentFolderTreeItem::updateChildContentAt - ERROR - if (index >= m_CSVFile.count())";
      return;
   }

   if(childItems()->at(index)->isFolder()){
      FolderTreeItem *child = qobject_cast<FolderTreeItem *>(childItems()->at(index));

      if(!m_CSVFile.replaceAt(index, child->name())){
         qWarning() << "ContentFolderTreeItem::updateChildContentAt - ERROR - m_CSVFile.replaceAt returned false";
         return;
      }

      // Create rename folder if required
      if(!child->renameFolder(m_CSVFile.fileNameAt(index))){
         qWarning() << "ContentFolderTreeItem::updateChildContentAt - ERROR - child->renameFolder returned false";
      }
      model()->itemDataChanged(childItems()->at(index), FILE_NAME, ABSOLUTE_PATH);

   } else if(childItems()->at(index)->isFile()){

      QString longName = childItems()->at(index)->data(NAME).toString();
      QString fileName = childItems()->at(index)->data(FILE_NAME).toString();
      QString midiId = childItems()->at(index)->data(LOOP_COUNT).toString();

      qDebug() << "update config" << longName << fileName << midiId;
      CsvConfigFile::FileType fileType(CsvConfigFile::FOLDER);

      if(fileName.split(".").count() > 1){
         fileType = CsvConfigFile::reverseFileExtensionMap().value(fileName.split(".").at(1));
      }

      if(!m_CSVFile.replaceAt(index, longName, fileType, midiId.toInt())){
         qWarning() << "ContentFolderTreeItem::updateChildContentAt - ERROR - m_CSVFile.replaceAt returned false";
         return;
      }

      // Set filename in children
      childItems()->at(index)->setData(FILE_NAME, QVariant(m_CSVFile.fileNameAt(index)));
      model()->itemDataChanged(childItems()->at(index), FILE_NAME, ABSOLUTE_PATH);

   } else {
      qWarning() << "ContentFolderTreeItem::updateChildContentAt - ERROR - Found child that isn't a File of a Folder";
   }

   if(save){
      m_CSVFile.write();
   }
}
/**
 * @brief ContentFolderTreeItem::removeAllChildrenContent. Called as part of removeChildContentAt procedure.
 * @param save
 * re-implementation of FolderTreeItem::removeAllChildrenContent
 * Should only be called as part of ContentFolderTreeItem::removeChildContentAt
 * computeHash() and model()->setArchiveDirty() need to be called after this call.
 */
void ContentFolderTreeItem::removeAllChildrenContent(bool save)
{
   QFileInfo fi = folderFI();
   QDir dir(fi.absoluteFilePath());

   if(m_CSVFile.fileName().isEmpty()){
      m_CSVFile.setFileName(dir.absoluteFilePath(BMFILES_NAME_TO_FILE_MAPPING));
   }

   m_CSVFile.read();

   // 1 - Remove content of model and model
   for(int i = childItems()->count() - 1; i >= 0; i--){
      removeChildContentAt(i, false);
      // TODO determine if this is the place to do so - Probably so since there for a while
      delete childItems()->takeAt(i);
   }

   // 2 - remove remaining content of m_CSVFile that was not part of model
   for(int i = m_CSVFile.count() - 1; i >= 0; i--){
      if(m_CSVFile.fileTypeAt(1) == CsvConfigFile::FOLDER){
         if(dir.exists(m_CSVFile.fileNameAt(i))){
            dir.rmdir(m_CSVFile.fileNameAt(i));
         }
      } else {
         QString filePath = dir.absoluteFilePath(m_CSVFile.fileNameAt(i));

         if(QFile::exists(filePath)){
            QFile::remove(filePath);
         }
      }

      m_CSVFile.removeAt(i);
   }

   // 3 - Remove all remaining files and folders (including CSV file)
   QFileInfoList fileInfoList = dir.entryInfoList();
   for (int i = fileInfoList.count() - 1; i >= 0; i--){
      QFileInfo info = fileInfoList.at(i);

      if(info.isDir()){
         dir.rmdir(info.fileName());
      } else {
         QFile::remove(info.absoluteFilePath());
      }
   }

   if(save){
      m_CSVFile.write();
   }
}

/**
 * @brief ContentFolderTreeItem::moveChildren
 * @param sourceFirst
 * @param sourceLast
 * @param delta
 *
 * Re-implementation of AbstractTreeItem::moveChildren. Self contained. Needs to be called through model to notify views.
 */
void ContentFolderTreeItem::moveChildren(int sourceFirst, int sourceLast, int delta)
{
   // 1 - Validate

   if(sourceFirst > sourceLast){
      qWarning() << "FolderTreeItem::moveChildren : sourceFirst > sourceLast";
      return;
   }
   if(sourceLast >= childCount() || sourceFirst < 0){
      qWarning() << "FolderTreeItem::moveChildren : (sourceLast >= childCount() || sourceFirst < 0)";
      return;
   }

   if(sourceLast + delta >= childCount() || sourceFirst + delta < 0){
      qWarning() << "FolderTreeItem::moveChildren : (sourceLast + delta >= childCount() || sourceFirst + delta < 0), sourceFirst = " << sourceFirst << ",  sourceLast = " << sourceLast << ", delta = " << delta << ", childCount() = " << childCount();
      return;
   }

   // 2 - Move items in tree

   // Create a list of items to move
   QList<AbstractTreeItem *> moved;
   for(int i = sourceLast; i >= sourceFirst; i--){
      moved.append(childItems()->takeAt(i));
   }

   int deltaFirst = sourceFirst + delta;
   int deltaLast = sourceLast + delta;

   //int movedIndex = 0;

   for(int i = deltaFirst; i <= deltaLast; i++){
      insertChildAt(i, moved.takeLast());
   }

   // 3 - Move items in CSV config
   m_CSVFile.moveEntries(sourceFirst, sourceLast, delta);
   m_CSVFile.write();


   // Update hash and propagate change down to root
   computeHash(true);
   propagateHashChange();
   model()->setProjectDirty();
}

/**
 * @brief ContentFolderTreeItem::removeChild
 * @param row
 *
 * Re-implementation of AbstractTreeItem::removeChild. Removes child completely. However, it does not perform nodifications. Use Model->remove... if notifications are required
 */
void ContentFolderTreeItem::removeChild(int row)
{
   // 1 - Delete in Tree csv
   removeChildContentAt(row, false);

   // 2 - Delete in Tree
   removeChildInternal(row);

   // 3 - Save csv
   m_CSVFile.write();

   // Update hash and propagate change down to root
   computeHash(false);
   propagateHashChange();
   model()->setProjectDirty();
}

bool ContentFolderTreeItem::createFileWithData(int /*index*/, const QString & /*longName*/, const QString & /*fileName*/)
{
   qWarning() << "ContentFolderTreeItem::createFileWithData - TODO - child creation is not handled by superclass. Perform operation in " << metaObject()->className();
   return false;
}

void ContentFolderTreeItem::createFolderWithData(int /*index*/, const QString & /*longName*/, const QString & /*fileName*/, bool /*cleanAll*/)
{
   qWarning() << "ContentFolderTreeItem::createFolderWithData - TODO - child creation is not handled by superclass. Perform operation in " << metaObject()->className();
}

/**
 * @brief ContentFolderTreeItem::computeHash
 * @param recursive
 *
 * Re-implementation of AbstractTreeItem::removeChild.
 * model()->setArchiveDirty() need to be called after this call.
 */
void ContentFolderTreeItem::computeHash(bool recursive)
{
   QCryptographicHash cr(QCryptographicHash::Sha256);

   QFile csvFile(m_CSVFile.fileName());
   if(csvFile.open(QIODevice::ReadOnly)){
      cr.addData(&csvFile);
      csvFile.close();
   } else {
      qWarning() << "ContentFolderTreeItem::computeHash - ERROR 1 - Unable to open " << m_CSVFile.fileName() << " in " << folderFI().absoluteFilePath();
   }

   foreach(AbstractTreeItem* p_child, *childItems()){
      if(recursive){
         p_child->computeHash(true);
      }
      cr.addData(p_child->hash());
   }

   setHash(cr.result());
}
/**
 * @brief ContentFolderTreeItem::prepareSync
 * @param dstPath
 * @param p_cleanUp
 * @param p_copySrc
 * @param p_copyDst
 *
 * re-implementation of AbstractTreeItem::prepareSync
 * Called as part of BeatsProjectModel::synchronizeModal.
 */
void ContentFolderTreeItem::prepareSync(const QString &dstPath, QList<QString> *p_cleanUp, QList<QString> *p_copySrc, QList<QString> *p_copyDst)
{
   QFileInfo fi = folderFI();
   QDir dir(fi.absoluteFilePath());

   if(!p_cleanUp || !p_copySrc || !p_copyDst){
      qWarning() << "ContentFolderTreeItem::prepareSync - ERROR 1 - return list is null";
      return;
   }

   QFileInfo dstFI(dstPath);
   QDir dstDir(dstPath);

   if(dstFI.exists() && compareHash(dstPath)){
      return;
   }

   if(!dstFI.exists()){
      // if directory does not exist, simply add everything recursively

      // Append self
      p_copySrc->append(fi.absoluteFilePath());
      p_copyDst->append(dstFI.absoluteFilePath());

      // add csv file
      p_copySrc->append(m_CSVFile.fileName());
      p_copyDst->append(dstDir.absoluteFilePath(QFileInfo(m_CSVFile.fileName()).fileName()));

      // add hash file
      p_copySrc->append(dir.absoluteFilePath(BMFILES_HASH_FILE_NAME));
      p_copyDst->append(dstDir.absoluteFilePath(BMFILES_HASH_FILE_NAME));

      // add sub-folders/files
      for(int i = 0; i < childCount(); i++){
         QString childPath = dstDir.absoluteFilePath(child(i)->data(FILE_NAME).toString());
         child(i)->prepareSync(childPath, p_cleanUp, p_copySrc, p_copyDst);
      }
   } else {

      // Start by listing files/folders that need to be removed
      QSet<QString> srcChildrenFileNames;
      foreach(const QFileInfo &srcChildFI, dir.entryInfoList(QDir::Dirs | QDir::Files | QDir::Hidden | QDir::NoDotAndDotDot)){
         srcChildrenFileNames.insert(srcChildFI.fileName().toUpper());
      }

      // Clean up content that is part of dst but is not present anymore on src

      foreach(const QFileInfo &dstChildFI, dstDir.entryInfoList(QDir::Dirs | QDir::Files | QDir::Hidden | QDir::NoDotAndDotDot)){
         if(!srcChildrenFileNames.contains(dstChildFI.fileName().toUpper())){
            // Explore children first
            if(dstChildFI.isDir()){
               exploreChildrenForRemoval(dstChildFI.absoluteFilePath(), p_cleanUp);
            }

            p_cleanUp->append(dstChildFI.absoluteFilePath());
         }
      }

      // replace csv file
      p_cleanUp->append(dstDir.absoluteFilePath(QFileInfo(m_CSVFile.fileName()).fileName()));
      p_copySrc->append(m_CSVFile.fileName());
      p_copyDst->append(dstDir.absoluteFilePath(QFileInfo(m_CSVFile.fileName()).fileName()));

      // replace hash file
      p_cleanUp->append(dstDir.absoluteFilePath(BMFILES_HASH_FILE_NAME));
      p_copySrc->append(dir.absoluteFilePath(BMFILES_HASH_FILE_NAME));
      p_copyDst->append(dstDir.absoluteFilePath(BMFILES_HASH_FILE_NAME));

      // Loop on all children so that they can decide whether or not to add themselves
      for(int i = 0; i < childCount(); i++){
         QString childPath = dstDir.absoluteFilePath(child(i)->data(FILE_NAME).toString());
         child(i)->prepareSync(childPath, p_cleanUp, p_copySrc, p_copyDst);
      }
   }
}

/**
 * @brief ContentFolderTreeItem::resolveDuplicateLongName
 * @param longName
 * @param newName
 * @return
 *
 * Utility method that does not modify project
 */
QString ContentFolderTreeItem::resolveDuplicateLongName(const QString &longName, bool newName)
{
   int duplicate = 1;

   QString resolvedName = longName;

   // Note: to upper called internally to be case insensitive
   while(containsLongName(resolvedName)){
      if(newName){
         resolvedName = QString("%1 %2 ").arg(longName).arg(duplicate++);
      } else {
         resolvedName = QString("%1(%2)").arg(longName).arg(duplicate++);
      }
   }

   return resolvedName;
}

/**
 * @brief ContentFolderTreeItem::pathForLongName
 * @param longName
 * @return
 *
 * Utility function to access m_CSVFile.longName2FileName(longName). Does not modify project
 */
QString ContentFolderTreeItem::pathForLongName(const QString &longName)
{
   if(!containsLongName(longName)){
      return QString();
   }
   QDir dir(folderFI().absoluteFilePath());
   return dir.absoluteFilePath(m_CSVFile.longName2FileName(longName));
}

/**
 * @brief ContentFolderTreeItem::renameFolder
 *
 * Updates the csv file path and changes directory. Called as part of ContentFolderTreeItem::updateChildContentAt.
 * computeHash() and model()->setArchiveDirty() need to be called after this call.
 */
bool ContentFolderTreeItem::renameFolder(const QString &newName)
{
   if(!FolderTreeItem::renameFolder(newName)){
      return false;
   }
   QDir dir(folderFI().absoluteFilePath());
   m_CSVFile.setFileName(dir.absoluteFilePath(BMFILES_NAME_TO_FILE_MAPPING));
   return true;
}
