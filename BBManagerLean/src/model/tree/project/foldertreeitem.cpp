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
#include "foldertreeitem.h"
#include "beatsprojectmodel.h"

#include <QCryptographicHash>
#include <QDir>
#include "../../beatsmodelfiles.h"


/**
 * @brief FolderTreeItem::FolderTreeItem
 * @param p_model
 * @param path
 *
 * PROTECTED Constructor used in folder hierarchy. 
 */
FolderTreeItem::FolderTreeItem(BeatsProjectModel *p_model, FolderTreeItem *parent)
    : AbstractTreeItem(p_model, parent)
{
   if(parent == nullptr){
      qWarning() << "FolderTreeItem::FolderTreeItem - ERROR 1 - Parent needs to be defined if not root";
   }
   m_Name = "TBD";
   m_FileName = "TBD";
   m_ErrorMsg = QStringList();
}
/**
 * @brief FolderTreeItem::FolderTreeItem
 * @param p_model
 * @param path
 *
 * Constructor used to create a root item
 */
FolderTreeItem::FolderTreeItem(BeatsProjectModel *p_model):
   AbstractTreeItem(p_model, nullptr)
{
   m_Name = model()->projectDirFI().baseName();
   m_FileName = model()->projectDirFI().baseName();
   m_ErrorMsg = QStringList();
}

FolderTreeItem::~FolderTreeItem(){
}

/**
 * @brief FolderTreeItem::data
 * @param column
 * @return
 *
 * re-implementation of AbstractTreeItem::data
 */
QVariant FolderTreeItem::data(int column)
{
    switch (column){
    case NAME:
        return m_Name;
    case CHILDREN_TYPE:
        return m_ChildrenTypes;
    case FILE_NAME:
        return fileName();
    case ABSOLUTE_PATH:
        return folderFI().absoluteFilePath();
    case ERROR_MSG:
        return m_ErrorMsg;
    case LOOP_COUNT:
        return midiId;
    case SAVE:
        for (int i = 0; i < childCount(); ++i) {
            if (!child(i)->data(SAVE).isNull() && child(i)->data(SAVE).toBool()) {
                return true;
            }
        }
        return false;
    case HASH:
        return hash();
    default:
        return AbstractTreeItem::data(column);
    }
}

/**
 * @brief FolderTreeItem::setData
 * @param column
 * @param value
 * @return
 *
 * re-implementation of AbstractTreeItem::setData
 */
bool FolderTreeItem::setData(int column, const QVariant & value)
{
    auto ret = true;
    switch (column) {
    case NAME:
        m_Name = value.toString();
        return true;
    case CHILDREN_TYPE:
        m_ChildrenTypes = value.toString();
        return true;
    case FILE_NAME:
        // Note : setting file name itself does not make archive dirty
        setFileName(value.toString());
        return true;
    case ERROR_MSG:
        m_ErrorMsg = value.toStringList();
        return true;
    case LOOP_COUNT:
        midiId = value.toInt();
        return true;
    case SAVE:
        if (ret = value.toBool()) {
            model()->setProjectDirty();
        }
        for (int i = 0; i < childCount(); ++i) {
            if (child(i)->setData(SAVE, value)) {
                model()->itemDataChanged(child(i), SAVE);
                ret = true;
            }
        }
        return ret;
    default:
        return AbstractTreeItem::setData(column, value);
    }
}

/**
 * @brief FolderTreeItem::folderFI full path in project directory
 * @return
 *
 * Does not change data
 */
QFileInfo FolderTreeItem::folderFI() const
{
   FolderTreeItem * parentFti = qobject_cast<FolderTreeItem *>(parent());
   if(parentFti){
      QDir parentDir(parentFti->folderFI().absoluteFilePath());
      return QFileInfo(parentDir.absoluteFilePath(fileName()));
   } else {
      return model()->projectDirFI();
   }
}


/**
 * @brief createFolder creates the dir if required
 * @return changes performed (in order to determine if hashing needs to be performed)
 *
 * Called as part of many processes.
 * May require computeHash() and model()->setArchiveDirty() to be called after this call, depending on the process.
 */
bool FolderTreeItem::createFolder()
{
   bool changed = false;

   if(fileName().isEmpty()){
      qWarning() << "FolderTreeItem::createFolder - ERROR - Trying to create folder with empty fileName() for " << name();
      m_ErrorMsg.append(tr("FolderTreeItem::createFolder - ERROR - Trying to create folder with empty fileName() for %1").arg(name()));
      return false;
   }

   QFileInfo fi = folderFI();

   if(!fi.exists()){
      QDir dir(model()->projectDirFI().absoluteFilePath());
      if(!dir.mkpath(fi.absoluteFilePath())){
         qWarning() << "FolderTreeItem::createFolder - ERROR - unable to create dir " << fi.absoluteFilePath();
         m_ErrorMsg.append(tr("FolderTreeItem::createFolder - ERROR - unable to create dir %1").arg(fi.absoluteFilePath()));
         return false;
      }
      changed = true;
   }
   return changed;
}

/**
 * @brief FolderTreeItem::renameFolder
 * @param newName
 * @return
 *
 * Updates the file path after and changes directory. Called as part of ContentFolderTreeItem::updateChildContentAt.
 * computeHash() and model()->setArchiveDirty() need to be called after this call.
 */
bool FolderTreeItem::renameFolder(const QString &newName)
{
   if(fileName().isEmpty()){
      qWarning() << "FolderTreeItem::renameFolder - ERROR - Trying to create folder with empty fileName() for " << name();
      m_ErrorMsg.append(tr("FolderTreeItem::renameFolder - ERROR - Trying to create folder with empty fileName() for %1").arg(name()));
      return false;
   }


   if(fileName() == newName){
      // No change
      return true;
   }

   QFileInfo oldFI = folderFI();
   QDir parentDir(oldFI.absolutePath());

   if(parentDir.exists(newName)){
      qWarning() << "FolderTreeItem::renameFolder - ERROR - parentDir.exists(newName) " << newName;
      return false;
   }
   // (Note for Mac OS, QFile::rename fileName needs to be an absolute path does not seem to be an issue with QDir::rename)
   if(!parentDir.rename(fileName(), newName)){
      qWarning() << "FolderTreeItem::renameFolder - ERROR - renaming '" << fileName() << "' to '" << newName << "'";
      return false;
   }
   setFileName(newName);
   return true;
}

/**
 * @brief FolderTreeItem::projectDirectories
 * @return
 *
 * Does not change data
 */
QFileInfoList FolderTreeItem::projectDirectories() const
{
   QFileInfoList fil;
   if(parent() != nullptr){
      qWarning() << "FolderTreeItem::projectDirectories - ERROR - Trying to retrieve project directory on folder that IS NOT ROOT";
      return fil;
   }
   for(int i = 0; i < childCount(); i++){
      fil.append(static_cast<FolderTreeItem *>(child(i))->folderFI());
   }

   return fil;
}

/**
 * @brief createProjectSkeleton
 * @return changes performed (in order to determine if hashing needs to be performed)
 *
 * if changes are performed, computeHash() and model()->setArchiveDirty() need to be called after this call, depending on the process.
 */
bool FolderTreeItem::createProjectSkeleton()
{
   bool changed = false;
   if(fileName().isEmpty()){
      // Skeleton is only created if folderName exists
      return false;
   }

   changed = createFolder();

   // If folderName created with skeleton, explore children
   for(int i = 0; i < childItems()->count(); i++){
      if(qobject_cast<FolderTreeItem *>(childItems()->at(i))->createProjectSkeleton()){
         changed = true;
      }
   }
   return changed;
}



/**
 * @brief FolderTreeItem::removeAllChildrenContent
 *
 * should only be called as part of ContentFolderTreeItem::removeChildContentAt
 */
void FolderTreeItem::removeAllChildrenContent(bool /*save*/)
{
   qWarning() << "FolderTreeItem::removeAllChildrenContent - DEFAULT IMPLEMENTATION";
}

/**
 * @brief FolderTreeItem::model
 * @return
 */
BeatsProjectModel *FolderTreeItem::model() const
{
   return qobject_cast<BeatsProjectModel*>(AbstractTreeItem::model());
}


/**
 * @brief FolderTreeItem::computeHash
 * @param recursive
 * Called as part of AbstractTreeItem::computeHash
 * model()->setArchiveDirty() need to be called after this call.
 */
void FolderTreeItem::computeHash(bool recursive)
{
   QCryptographicHash cr(QCryptographicHash::Sha256);

   foreach(AbstractTreeItem* p_child, *childItems()){
      if(recursive){
         p_child->computeHash(true);
      }
      cr.addData(p_child->hash());
   }

   setHash(cr.result());
}

QByteArray FolderTreeItem::hash()
{
   QDir dir(folderFI().absoluteFilePath());
   QFile hashFile(dir.absoluteFilePath(BMFILES_HASH_FILE_NAME));
   if(hashFile.exists() && hashFile.open(QIODevice::ReadOnly)){
      QDataStream fin(&hashFile);
      QMap<QString, QVariant> map;
      fin >> map;
      hashFile.close();
      if(!map.contains("hash")){
         qWarning() << "FolderTreeItem::hash - ERROR 1 - Invalid hash file";
         return QByteArray();
      }
      return map.value("hash").toByteArray();
   }
   qWarning() << "FolderTreeItem::hash - ERROR 2 - Unable to open file";
   return QByteArray();
}

QByteArray FolderTreeItem::getHash(const QString &path)
{
   QDir dir(path);
   if(!dir.exists()){
      qWarning() << "FolderTreeItem::getHash - ERROR 1 - if(!dir.exists())";
      return QByteArray();
   }

   QFile hashFile(dir.absoluteFilePath(BMFILES_HASH_FILE_NAME));
   if(hashFile.exists() && hashFile.open(QIODevice::ReadOnly)){
      QDataStream fin(&hashFile);
      QMap<QString, QVariant> map;
      fin >> map;
      hashFile.close();
      if(!map.contains("hash")){
         qWarning() << "FolderTreeItem::getHash - ERROR 2 - Invalid hash file";
         return QByteArray();
      }
      return map.value("hash").toByteArray();
   }

   qWarning() << "FolderTreeItem::getHash - ERROR 3 - Unable to open file";
   return QByteArray();
}
/**
 * @brief FolderTreeItem::setHash
 * @param hash
 * Called as part of AbstractTreeItem::computeHash
 * model()->setArchiveDirty() need to be called after this call.
 */
void FolderTreeItem::setHash(const QByteArray &hash)
{
   QDir dir(folderFI().absoluteFilePath());
   QFile hashFile(dir.absoluteFilePath(BMFILES_HASH_FILE_NAME));
   if(hashFile.open(QIODevice::WriteOnly)){
      QDataStream fout(&hashFile);
      QMap<QString, QVariant> map;
      map.insert("hash", QVariant(hash));
      fout << map;
      hashFile.flush();
      hashFile.close();
   }

   model()->itemDataChanged(this, HASH);

}

/**
 * @brief FolderTreeItem::setHashStatic
 * @param path
 * @param hash
 * @return
 *
 * Static function that modifies hash file. Would require to notify model of change
 * Called as part of computeRootHashStatic during linkProject and resetProjectInfo (save project as).
 * Called during ParamsFolderTreeModel::computeHashStatic during setProjectUuid (part of linking) and resetProjectInfo (save project as)
 */
bool FolderTreeItem::setHashStatic(const QString &path, const QByteArray &hash)
{
   QDir folderDir(path);
   if(!folderDir.exists()){
      return false;
   }

   QFileInfo hashFI(folderDir.absoluteFilePath(BMFILES_HASH_FILE_NAME));
   if(!hashFI.exists() || !hashFI.isFile()){
      return false;
   }

   QFile hashFile(hashFI.absoluteFilePath());
   if(!hashFile.open(QIODevice::WriteOnly)){
      return false;
   }

   QDataStream fout(&hashFile);
   QMap<QString, QVariant> map;
   map.insert("hash", QVariant(hash));
   fout << map;
   hashFile.flush();
   hashFile.close();

   return true;
}


bool FolderTreeItem::compareHash(const QString &path)
{
   QByteArray localHash = this->hash();
   QByteArray remoteHash = getHash(path);

   if(localHash.count() == 0 || remoteHash.count() == 0){
      qWarning() << "FolderTreeItem::compareHash - ERROR 1 - empty hash" << localHash.count() << remoteHash.count() << path;
      return false;
   }

   if(localHash.count() != remoteHash.count()){
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
 * @brief FolderTreeItem::exploreChildrenForRemoval
 * @param dstPath
 * @param p_cleanUp
 *
 * returns a list of path to clean up in p_cleanUp
 */
void FolderTreeItem::exploreChildrenForRemoval(const QString &dstPath, QList<QString> *p_cleanUp)
{
   QDir dstDir(dstPath);

   // 1 - retrieve all files (leafs)
   foreach(const QFileInfo &info, dstDir.entryInfoList(QDir::Files | QDir::Hidden| QDir::NoDotAndDotDot)) {
      p_cleanUp->append(info.absoluteFilePath());
   }

   // 2 - retrieve and explore all dirs
   foreach(const QFileInfo &info, dstDir.entryInfoList(QDir::Dirs | QDir::Hidden |  QDir::NoDotAndDotDot)) {

      // Explore recursively
      exploreChildrenForRemoval(info.absoluteFilePath(), p_cleanUp);

      // End by adding current directory
      p_cleanUp->append(info.absoluteFilePath());
   }
}

/**
 * @brief FolderTreeItem::prepareSync
 * @param dstPath
 * @param p_cleanUp
 * @param p_copySrc
 * @param p_copyDst
 *
 * re-implementation of AbstractTreeItem::prepareSync
 * Called as part of BeatsProjectModel::synchronizeModal.
 */
void FolderTreeItem::prepareSync(const QString &dstPath, QList<QString> *p_cleanUp, QList<QString> *p_copySrc, QList<QString> *p_copyDst)
{
   QFileInfo fi = folderFI();
   QDir dir(fi.absoluteFilePath());

   if(!p_cleanUp || !p_copySrc || !p_copyDst){
      qWarning() << "FolderTreeItem::prepareSync - ERROR 1 - return list is null";
      return;
   }

   QFileInfo dstFI(dstPath);
   QDir dstDir(dstPath);

   if(dstFI.exists() && compareHash(dstDir.absoluteFilePath(BMFILES_HASH_FILE_NAME))){
      return;
   }

   if(!dstFI.exists()){
      // if directory does not exist, simply add everything recursively

      // Append self
      p_copySrc->append(fi.absoluteFilePath());
      p_copyDst->append(dstFI.absoluteFilePath());

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
         srcChildrenFileNames.insert(srcChildFI.fileName());
      }

      // Clean up content that is part of dst but is not present anymore on src
      foreach(const QFileInfo &dstChildFI, dstDir.entryInfoList(QDir::Dirs | QDir::Files | QDir::Hidden | QDir::NoDotAndDotDot)){
         if(!srcChildrenFileNames.contains(dstChildFI.fileName())){
            // Explore children first
            if(dstChildFI.isDir()){
               exploreChildrenForRemoval(dstChildFI.absoluteFilePath(), p_cleanUp);
            }

            p_cleanUp->append(dstChildFI.absoluteFilePath());
         }
      }

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
