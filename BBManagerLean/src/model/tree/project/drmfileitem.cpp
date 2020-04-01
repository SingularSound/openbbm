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
#include "drmfileitem.h"
#include "drmfoldertreeitem.h"
#include "drmmaker/Model/drmmakermodel.h"

DrmFileItem::DrmFileItem(DrmFolderTreeItem *p_parent):
   AbstractTreeItem(p_parent->model(), p_parent)
{
}

/**
 * @brief DrmFileItem::data
 * @param column
 * @return
 *
 * re-implementation of AbstractTreeItem::data
 */
QVariant DrmFileItem::data(int column)
{
    switch (column) {
    case NAME:
        return QVariant(m_LongName);
    case FILE_NAME:
        return QVariant(m_FileName);
    case ABSOLUTE_PATH:
        return QDir(static_cast<DrmFolderTreeItem *>(parent())->folderFI().absoluteFilePath()).absoluteFilePath(m_FileName);
    case HASH:
        return QVariant(hash());
    default:
        return AbstractTreeItem::data(column);
    }
}
/**
 * @brief DrmFileItem::setData
 * @param column
 * @param value
 * @return
 *
 * re-implementation of AbstractTreeItem::setData
 */
bool DrmFileItem::setData(int column, const QVariant &value)
{
    switch (column) {
    case NAME:
        m_LongName = value.toString();
        return true;
    case FILE_NAME:
        m_FileName = value.toString();
        return true;
    default:
        return AbstractTreeItem::setData(column, value);
    }
}

bool DrmFileItem::compareHash(const QString &path)
{
   QByteArray localHash = this->hash();
   QByteArray remoteHash = getHash(path);

   if(localHash.count() == 0 || remoteHash.count() == 0){
      qWarning() << "DrmFileItem::compareHash - ERROR 1 - empty hash" << localHash.count() << remoteHash.count() << path;
      return false;
   }

   if(localHash.count() != remoteHash.count()){
      qWarning() << "DrmFileItem::compareHash - ERROR 1 - empty hash" << localHash.count() << remoteHash.count() << path;
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
 * @brief DrmFileItem::prepareSync
 * @param dstPath
 * @param p_cleanUp
 * @param p_copySrc
 * @param p_copyDst
 *
 * re-implementation of AbstractTreeItem::prepareSync
 * Called as part of BeatsProjectModel::synchronizeModal.
 */
void DrmFileItem::prepareSync(const QString &dstPath, QList<QString> *p_cleanUp, QList<QString> *p_copySrc, QList<QString> *p_copyDst)
{
   qDebug() << "DrmFileItem::prepareSync - TODO - Drumset hashing not finalized yet";
      
   if(!p_cleanUp || !p_copySrc || !p_copyDst){
      qWarning() << "DrmFileItem::prepareSync - ERROR 1 - return list is null";
      return;
   }

   QFileInfo dstFI(dstPath);

   if(dstFI.exists() && compareHash(dstPath)){
      return;
   }

   // Add (file + hash) path to clean up and to copy

   if(dstFI.exists()){
      p_cleanUp->append(dstFI.absoluteFilePath());
   }
   p_copySrc->append(QString("%1/%2").arg(parent()->data(ABSOLUTE_PATH).toString(), m_FileName));
   p_copyDst->append(dstFI.absoluteFilePath());
}

void DrmFileItem::computeHash(bool /*recursive*/)
{
   // Voluntarly do nothing
}

QByteArray DrmFileItem::hash()
{
   return DrmMakerModel::getCRCStatic(QDir(static_cast<DrmFolderTreeItem *>(parent())->folderFI().absoluteFilePath()).absoluteFilePath(m_FileName));
}

QByteArray DrmFileItem::getHash(const QString &path)
{
   return DrmMakerModel::getCRCStatic(path);
}
