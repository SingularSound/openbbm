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
#include "effectfileitem.h"
#include "effectfoldertreeitem.h"
#include "beatsprojectmodel.h"

#include <QCryptographicHash>

EffectFileItem::EffectFileItem(EffectFolderTreeItem *parent):
   AbstractTreeItem(parent->model(), parent)
{
}

QVariant EffectFileItem::data(int column)
{
   switch(column){
      case NAME:
         return QVariant(m_LongName);
      case FILE_NAME:
         return QVariant(m_FileName);
      case ABSOLUTE_PATH:
         return QDir(static_cast<EffectFolderTreeItem *>(parent())->folderFI().absoluteFilePath()).absoluteFilePath(m_FileName);
      case HASH:
         return QVariant(hash());
      default:
         return AbstractTreeItem::data(column);
   }
}


/**
 * @brief EffectFileItem::setData
 * @param column
 * @param value
 * @return
 *
 * re-implementation of EffectFileItem::setData
 */
bool EffectFileItem::setData(int column, const QVariant &value)
{
    switch(column){
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

/**
 * @brief EffectFileItem::computeHash
 * re-implementation of AbstractTreeItem::computeHash
 * model()->setArchiveDirty() need to be called after this call.
 */
void EffectFileItem::computeHash(bool /*recursive*/)
{
   QFile file(QString("%1/%2").arg(parent()->data(ABSOLUTE_PATH).toString(), m_FileName));
   if(file.exists() && file.open(QIODevice::ReadOnly)){
      QCryptographicHash cr(QCryptographicHash::Sha256);
      cr.addData(&file);
      file.close();
      setHash(cr.result());
   } else {
      setHash(QByteArray());
   }
}

QByteArray EffectFileItem::hash()
{
   QString hashFileName = m_FileName.split('.').at(0) + "." BMFILES_CONFIG_FILE_EXTENSION;

   QFile hashFile(QString("%1/%2").arg(parent()->data(ABSOLUTE_PATH).toString(), hashFileName));
   if(hashFile.exists() && hashFile.open(QIODevice::ReadOnly)){
      QDataStream fin(&hashFile);
      QMap<QString, QVariant> map;
      fin >> map;
      hashFile.close();
      if(!map.contains("hash")){
         qWarning() << "EffectFileItem::hash - ERROR 1 - Invalid hash file";
         return QByteArray();
      }
      return map.value("hash").toByteArray();
   }

   qWarning() << "EffectFileItem::hash - ERROR 2 - Unable to open file:" << hashFileName
              << QString("%1/%2").arg(parent()->data(ABSOLUTE_PATH).toString(), hashFileName)
              << parent()->data(ABSOLUTE_PATH).toString()
              << data(NAME)
              << data(FILE_NAME);

   return QByteArray();

}

QByteArray EffectFileItem::getHash(const QString &path)
{
   QFileInfo fi(path);
   QDir parentDir(fi.absolutePath());

   QString hashFileName = fi.baseName() + "." BMFILES_CONFIG_FILE_EXTENSION;

   QFile hashFile(parentDir.absoluteFilePath(hashFileName));
   if(hashFile.exists() && hashFile.open(QIODevice::ReadOnly)){
      QDataStream fin(&hashFile);
      QMap<QString, QVariant> map;
      fin >> map;
      hashFile.close();
      if(!map.contains("hash")){
         qWarning() << "EffectFileItem::getHash - ERROR 1 - Invalid hash file";
         return QByteArray();
      }
      return map.value("hash").toByteArray();
   }

   qWarning() << "EffectFileItem::getHash - ERROR 2 - Unable to open file";

   return QByteArray();
}

/**
 * @brief EffectFileItem::setHash
 * @param hash
 *
 * Called as part of AbstractTreeItem::computeHash
 * model()->setArchiveDirty() need to be called after this call.
 */
void EffectFileItem::setHash(const QByteArray &hash)
{
   QString hashFileName = m_FileName.split('.').at(0) + "." BMFILES_CONFIG_FILE_EXTENSION;

   QFile hashFile(QString("%1/%2").arg(parent()->data(ABSOLUTE_PATH).toString(), hashFileName));
   if(hashFile.open(QIODevice::WriteOnly)){
      QDataStream fout(&hashFile);
      QMap<QString, QVariant> map;
      map.insert("hash", QVariant(hash));
      fout << map;
      hashFile.flush();
      hashFile.close();
   }

   m_qHash = ((static_cast<uint>(hash[3]) << 24 ) +
              (static_cast<uint>(hash[2]) << 16 ) +
              (static_cast<uint>(hash[1]) <<  8 ) +
              (static_cast<uint>(hash[0])       ));

   model()->itemDataChanged(this, HASH);
}

bool EffectFileItem::compareHash(const QString &path)
{
   QByteArray localHash = this->hash();
   QByteArray remoteHash = getHash(path);

   if(localHash.count() == 0 || remoteHash.count() == 0){

      qWarning() << "EffectFileItem::compareHash - ERROR 1 - empty hash " << localHash.count() << remoteHash.count() << path;
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
 * @brief EffectFileItem::prepareSync
 * @param dstPath
 * @param p_cleanUp
 * @param p_copySrc
 * @param p_copyDst
 *
 * re-implementation of AbstractTreeItem::prepareSync
 * Called as part of BeatsProjectModel::synchronizeModal.
 */
void EffectFileItem::prepareSync(const QString &dstPath, QList<QString> *p_cleanUp, QList<QString> *p_copySrc, QList<QString> *p_copyDst)
{
   if(!p_cleanUp || !p_copySrc || !p_copyDst){
      qWarning() << "EffectFileItem::prepareSync - ERROR 1 - return list is null";
      return;
   }

   QFileInfo dstFI(dstPath);

   QDir dstParentDir(dstFI.absoluteDir());
   QString dstHashFileName =dstFI.baseName() + "." BMFILES_CONFIG_FILE_EXTENSION;
   QString srcHashFileName = m_FileName.split('.').at(0) + "." BMFILES_CONFIG_FILE_EXTENSION;

   QFileInfo dstHashFI(dstParentDir.absoluteFilePath(dstHashFileName));

   if(dstFI.exists() && compareHash(dstPath)){
      qDebug() << "EffectFileItem::prepareSync - Hash are equal, nothing to do";
      return;
   }
   qDebug() << "dstPath = " << dstPath;
   qDebug() << "dstFI.exists() = " << dstFI.exists();
   qDebug() << "compareHash(dstPath) = " << compareHash(dstPath);

   // Add (file + hash) path to clean up and to copy

   if(dstFI.exists()){
      p_cleanUp->append(dstFI.absoluteFilePath());
   }
   p_copySrc->append(QString("%1/%2").arg(parent()->data(ABSOLUTE_PATH).toString(), m_FileName));
   p_copyDst->append(dstFI.absoluteFilePath());

   if(dstHashFI.exists()){
      p_cleanUp->append(dstParentDir.absoluteFilePath(dstHashFileName));
   }
   p_copySrc->append(QString("%1/%2").arg(parent()->data(ABSOLUTE_PATH).toString(), srcHashFileName));
   p_copyDst->append(dstParentDir.absoluteFilePath(dstHashFileName));



}
