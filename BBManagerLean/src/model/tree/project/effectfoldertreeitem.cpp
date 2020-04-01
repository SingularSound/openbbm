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
#include <QMapIterator>
#include <QUuid>
#include <QList>
#include <QDebug>
#include <QAudioDecoder>
#include <stdint.h>
#include <QCryptographicHash>

#include "beatsprojectmodel.h"
#include "effectfoldertreeitem.h"
#include "effectfileitem.h"
#include "../../beatsmodelfiles.h"

#include "../../../utils/wavfile.h"
#include "../../../utils/filecompare.h"


EffectFolderTreeItem::EffectFolderTreeItem(BeatsProjectModel *p_model, FolderTreeItem *parent):
   ContentFolderTreeItem(p_model, parent)
{
   setName("EFFECTS");
   setFileName("EFFECTS");
   setChildrenTypes(QString(BMFILES_WAVE_EXTENSION).toUpper());
   setSubFoldersAllowed(false);
   setSubFilesAllowed(true);
}

/**
 * @brief EffectFolderTreeItem::createProjectSkeleton
 * @return changes performed
 *
 * re-implementation of ContentFolderTreeItem::createProjectSkeleton
 * if changes are performed, computeHash() and model()->setArchiveDirty() need to be called after this call, depending on the process.
 */
bool EffectFolderTreeItem::createProjectSkeleton()
{
   bool changed = false;

   if(ContentFolderTreeItem::createProjectSkeleton()){
      changed = true;
   }

   QFileInfo fi = folderFI();
   QDir dir(fi.absoluteFilePath());

   m_UsageFile.setFileName(dir.absoluteFilePath(BMFILES_EFFECT_USAGE_FILE_NAME));
   // Make sure file exists
   if(!m_UsageFile.exists()){
      m_UsageFile.open(QIODevice::ReadWrite);
      m_UsageFile.close();
      changed = true;
   }
   return changed;
}

/**
 * @brief EffectFolderTreeItem::updateModelWithData
 * @param cleanAll
 *
 * called as part of project constructor (directly or recursively through SongsFolderTreeItem::createFolderWithData)
 * May require model()->setArchiveDirty() to be called if cleanAll is true
 */
void EffectFolderTreeItem::updateModelWithData(bool cleanAll)
{
   // 1 - Superclass implementation
   ContentFolderTreeItem::updateModelWithData(cleanAll);

   // 2 - Parse m_UsageInfo file
   loadUsageFile();

}


/**
 * @brief EffectFolderTreeItem::loadUsageFile
 *
 * Read usage content from file
 */
void EffectFolderTreeItem::loadUsageFile()
{
   m_UsageFile.open(QIODevice::ReadOnly);
   QDataStream fin(&m_UsageFile);
   fin >> m_UsageInfo;
   m_UsageFile.close();
}
/**
 * @brief EffectFolderTreeItem::saveUsageFile
 *
 * Write usage content to file
 * computeHash() and model()->setArchiveDirty() need to be called after this call.
 */
void EffectFolderTreeItem::saveUsageFile()
{
   m_UsageFile.open(QIODevice::WriteOnly);
   QDataStream fout(&m_UsageFile);
   fout << m_UsageInfo;
   m_UsageFile.close();
}

/**
 * @brief addUse ads useCnt usage of an effect by a song
 * @param efxFileName
 * @param efxSourcePath
 * @param songId
 * @param immediately
 * @param useCnt
 *
 * The rule to follow is that adding a usage is performed immediately. On a discard, corrections are performed.
 * computeHash() and model()->setArchiveDirty() are updated in this call.
 */
EffectFileItem *EffectFolderTreeItem::addUse(const QString &efxSourcePath, const QString &efxLongName, const QUuid &songId, bool immediately, int useCnt)
{
   QFile efxFile(efxSourcePath);
   return addUse(efxFile, efxLongName, songId, immediately, useCnt);
}

EffectFileItem *EffectFolderTreeItem::addUse(QIODevice &efxSourceFile, const QString &efxLongName, const QUuid &songId, bool immediately, int useCnt)
{
   QFileInfo fi = folderFI();
   QDir dir(fi.absoluteFilePath());

   // 1 - validate that file exists
   if(!efxSourceFile.open(QIODevice::ReadOnly)){
      qWarning() << "EffectFolderTreeItem::addUse - ERROR - (!effectInfo.exists())";
      return nullptr;
   }
   efxSourceFile.close();

   // 2 - extract effect name out of path
   QString efxResolvedLongName = efxLongName;

   // 3 - verify if an effect with this name and content exists. Performe required operation depending on results
   QString efxFileName;

   bool identicalFound = false;
   int duplicateCnt = 0;

   FileCompare fc;
   if(m_CSVFile.containsLongName(efxResolvedLongName)){
      fc.setFile1(efxSourceFile);
   }

   while(m_CSVFile.containsLongName(efxResolvedLongName)){
      // verify if identical
      int index = m_CSVFile.indexOfLongName(efxResolvedLongName);
      efxFileName = m_CSVFile.fileNameAt(index);
      fc.setPath2(dir.absoluteFilePath(efxFileName));
      if(fc.areIdentical()){
         identicalFound = true;
         break;
      }

      efxResolvedLongName = QString("%1(%2)").arg(efxLongName).arg(++duplicateCnt);
   }

   //if no effect with this name and content found
   if(!identicalFound){

      // 3.1.1 - Create File Entry
      m_CSVFile.append(efxResolvedLongName, CsvConfigFile::WAVE_FILE);

      // 3.1.2 - Generate fileName
      efxFileName = m_CSVFile.lastFileName();

      // 3.1.3 - Copy or convert file
      if(!copyConvertWaveFile(efxSourceFile, efxFileName)){
         m_CSVFile.removeAt(m_CSVFile.count() - 1);
         qWarning() << "EffectFolderTreeItem::addUse - ERROR - unable to copy or convert file";
         return nullptr;
      }
//      // 3.1.3.1 If supported format, copy file
//      QFile::copy(efxSourceFile, QString("%1/%2").arg(path(), efxFileName));
//      // 3.1.3.2 If unsupported format, convert

      // 3.1.4 - Create corresponding object
      EffectFileItem *p_EfxFile = new EffectFileItem(this);
      p_EfxFile->setData(NAME, QVariant(efxResolvedLongName));
      p_EfxFile->setData(FILE_NAME, QVariant(efxFileName));
      model()->insertItem(p_EfxFile, childCount());

   }

   // 4 - Song Id to byte array for reuse
   QByteArray songIdByteArray = songId.toByteArray();

   // 5 - Update usage info (if usage doesn't exist, insert a value of useCnt (0+useCnt))
   QMap<QByteArray, qint32> efxUsageInfo = m_UsageInfo.value(efxFileName, QMap<QByteArray, qint32>());
   efxUsageInfo.insert(songIdByteArray, efxUsageInfo.value(songIdByteArray, 0) + useCnt);
   m_UsageInfo.insert(efxFileName, efxUsageInfo);

   // 6 - If change is not to be performed immediately, memorize change in order to be able to revert it
   if(!immediately){
      QMap<QString, qint32> songUsageInfo = m_UnsavedAddUsageInfo.value(songIdByteArray, QMap<QString, qint32>());
      songUsageInfo.insert(efxFileName, songUsageInfo.value(efxFileName, 0) + useCnt);
      m_UnsavedAddUsageInfo.insert(songIdByteArray, songUsageInfo);
   }

   // 7 - save
   saveUsageFile();
   m_CSVFile.write();

   // 8 - Update hash and return child
   int index = m_CSVFile.indexOfLongName(efxResolvedLongName);
   EffectFileItem *efxItem = static_cast<EffectFileItem *>(child(index));
   efxItem->computeHash(true);
   computeHash(false);
   propagateHashChange();
   model()->setProjectDirty();

   return efxItem;
}

/**
 * @brief EffectFolderTreeItem::copyConvertWaveFile
 * @param efxSourcePath
 * @param efxFileName
 * @return
 *
 * Copy wave file to Model FolderTreeItem
 * computeHash() and model()->setArchiveDirty() need to be called after this call.
 */
bool EffectFolderTreeItem::copyConvertWaveFile(const QString &efxSourcePath, const QString &efxFileName)
{
   QFileInfo fi = folderFI();
   QDir dir(fi.absoluteFilePath());

   // Determine the Wave file format
   WavFile wavFile;
   if(!wavFile.open(efxSourcePath)){
      qWarning() << "EffectFolderTreeItem::copyConvertWaveFile - ERROR - unable to open or parse wave file";
      return false;
   }

   // If format is supported, simply copy file
   if(wavFile.isFormatSupported()){
      return QFile::copy(efxSourcePath, dir.absoluteFilePath(efxFileName));
   }
   // For now we only copy
   return false;
}

bool EffectFolderTreeItem::copyConvertWaveFile(QIODevice &efxSourceFile, const QString &efxFileName)
{
   QFileInfo fi = folderFI();
   QDir dir(fi.absoluteFilePath());

   if(!WavFile::headerOkAndFormatSupported(efxSourceFile)){
      qWarning() << "EffectFolderTreeItem::copyConvertWaveFile - ERROR - unable to open or parse wave file";
      return false;
   }

   // For now we only copy
   if(!efxSourceFile.open(QIODevice::ReadOnly)){
      qWarning() << "EffectFolderTreeItem::copyConvertWaveFile - ERROR - unable to open wave source";
      return false;
   }

   QFile efxDstFile(dir.absoluteFilePath(efxFileName));
   if(!efxDstFile.open(QIODevice::WriteOnly)){
      qWarning() << "EffectFolderTreeItem::copyConvertWaveFile - ERROR - unable to open wave destination";
      efxSourceFile.close();
      return false;
   }

   // Copy
   QByteArray buffer;
   int chunksize = (1024*1024); // Whatever chunk size you like
   buffer = efxSourceFile.read(chunksize);
   while(!buffer.isEmpty()){
      efxDstFile.write(buffer);
      buffer = efxSourceFile.read(chunksize);
   }
   efxDstFile.close();
   efxSourceFile.close();

   return true;
}

/**
 * @brief EffectFolderTreeItem::removeUse
 * @param efxFileName
 * @param songId
 * @param immediately
 * @param count
 *
 * computeHash() and model()->setArchiveDirty() are called within this call when immediately is true.
 */
void EffectFolderTreeItem::removeUse(const QString &efxFileName, const QUuid &songId, bool immediately, qint32 count)
{
   // 1 - validate request
   if(!m_CSVFile.containsFileName(efxFileName)){
      qWarning() << "EffectFolderTreeItem::removeUse - ERROR 1 - (!m_CSVFile.containsFileName(efxFileName))";
      return;
   }

   if(!m_UsageInfo.contains(efxFileName)){
      qWarning() << "EffectFolderTreeItem::removeUse - ERROR 2 - (!m_UsageInfo.contains(efxFileName))";
      return;
   }

   QByteArray songIdByteArray = songId.toByteArray();
   QMap<QByteArray, qint32> efxUsageInfo = m_UsageInfo.value(efxFileName, QMap<QByteArray, qint32>());

   if(!efxUsageInfo.contains(songIdByteArray)){
      qWarning() << "EffectFolderTreeItem::removeUse - ERROR 3 - (!efxUsageInfo.contains(songIdByteArray))";
      return;
   }

   if(efxUsageInfo.value(songIdByteArray) < count){
      qWarning() << "EffectFolderTreeItem::removeUse - ERROR 4 - (efxUsageInfo.value(songIdByteArray) <= count)";
      return;
   }

   // 2 - Perform required operation
   if (immediately){
      // 2.1 perform imediately

      // 2.1.1 decrease count for this song
      qint32 updatedCount = efxUsageInfo.value(songIdByteArray);
      updatedCount -= count;
      
	  if(updatedCount <= 0){
         efxUsageInfo.remove(songIdByteArray);
      } else {
         efxUsageInfo.insert(songIdByteArray, updatedCount);
      }

      // 2.1.2 Refresh efx usage, verify if efx still used, remove it if not
      if(efxUsageInfo.count() > 0){
         // Still in use
         m_UsageInfo.insert(efxFileName, efxUsageInfo);
      } else {
         // Not used anymore : remove usage, and delete object and data
         m_UsageInfo.remove(efxFileName);
         int index = m_CSVFile.indexOfFileName(efxFileName);
         model()->removeItem(child(index), index); // Note : will perform every step required (including view notification)
      }

      // 2.1.3 Refresh efx usage, verify if efx still used, remove it if not
      saveUsageFile();

      // 3.1.4 Re-Hash with new usage file 
      computeHash(false);
      propagateHashChange();
      model()->setProjectDirty();

   } else {
      // 2.2 Memorize to perform later
      QMap<QString, qint32> songUsageInfo = m_UnsavedRemoveUsageInfo.value(songIdByteArray, QMap<QString, qint32>());
      songUsageInfo.insert(efxFileName, songUsageInfo.value(efxFileName, 0) + count);
      m_UnsavedRemoveUsageInfo.insert(songIdByteArray, songUsageInfo);
   }

}

/**
 * @brief EffectFolderTreeItem::removeAllUse
 * @param songId
 * @param imediately
 *
 * Remove all usage from one song. (usually when song is deleted)
 * Note: computeHash() and model()->setArchiveDirty() handled by subcalls
 */
void EffectFolderTreeItem::removeAllUse(const QUuid &songId, bool imediately)
{
   QByteArray songIdByteArray = songId.toByteArray();

   for(QMapIterator<QString, QMap<QByteArray, qint32> > i(m_UsageInfo); i.hasNext();){
      i.next();
      if(i.value().contains(songIdByteArray)){
         // NOTE - Hash computed in removeUse if immediately is true
         removeUse(i.key(), songIdByteArray, imediately, i.value().value(songIdByteArray));
      }
   }
}

/**
 * @brief EffectFolderTreeItem::saveUseChanges
 * @param songId
 *
 * Confirm usage changes for one song
 * Note: computeHash() and model()->setArchiveDirty() handled by subcalls
 */
void EffectFolderTreeItem::saveUseChanges(const QUuid &songId)
{
   QByteArray songIdByteArray = songId.toByteArray();

   // remove unsaved adds from list since they have already been performed
   if(m_UnsavedAddUsageInfo.contains(songIdByteArray)){
      m_UnsavedAddUsageInfo.remove(songIdByteArray);
   }

   // Perform memorised removals immediately
   if(m_UnsavedRemoveUsageInfo.contains(songIdByteArray)){
      QMap<QString, qint32> songUsageInfo = m_UnsavedRemoveUsageInfo.take(songIdByteArray);

      for(QMapIterator<QString, qint32> i(songUsageInfo); i.hasNext();){
         i.next();
         // NOTE - Hash computed in removeUse since immediately is true
         removeUse(i.key(), songIdByteArray, true, i.value());
      }
   }
}

/**
 * @brief EffectFolderTreeItem::saveAllUseChanges
 *
 * Confirm all usage changes for all songs
 * Note: computeHash() and model()->setArchiveDirty() handled by subcalls
 */
void EffectFolderTreeItem::saveAllUseChanges()
{
   m_UnsavedAddUsageInfo.clear();

   QList<QByteArray> keys = m_UnsavedRemoveUsageInfo.keys();
   for(QListIterator<QByteArray> i(keys);i.hasNext();){
      // NOTE - Hash computed in saveUseChanges
      saveUseChanges(QUuid(i.next()));
   }
}


/**
 * @brief EffectFolderTreeItem::discardUseChanges
 * @param songId
 *
 * Discard all Use Changes for a song
 * Note: computeHash() and model()->setArchiveDirty() handled by subcalls
 */
void EffectFolderTreeItem::discardUseChanges(const QUuid &songId)
{
   QByteArray songIdByteArray = songId.toByteArray();

   // remove unsaved remove from list since they have already been performed
   if(m_UnsavedRemoveUsageInfo.contains(songIdByteArray)){
      m_UnsavedRemoveUsageInfo.remove(songIdByteArray);
   }

   // Revert memorised adds immediately
   if(m_UnsavedAddUsageInfo.contains(songIdByteArray)){
      QMap<QString, qint32> songUsageInfo = m_UnsavedAddUsageInfo.take(songIdByteArray);

      for(QMapIterator<QString, qint32> i(songUsageInfo); i.hasNext();){
         i.next();
         // NOTE - Hash computed in removeUse
         removeUse(i.key(), songIdByteArray, true, i.value());
      }
   }
}

/**
 * @brief EffectFolderTreeItem::discardAllUseChanges
 * Discard all Use Changes for all song
 * Note: computeHash() and model()->setArchiveDirty() handled by subcalls
 */
void EffectFolderTreeItem::discardAllUseChanges()
{
   m_UnsavedRemoveUsageInfo.clear();

   QList<QByteArray> keys = m_UnsavedAddUsageInfo.keys();
   for(QListIterator<QByteArray> i(keys);i.hasNext();){
      // NOTE - Hash computed in discardUseChanges
      discardUseChanges(QUuid(i.next()));
   }
}

/**
 * @brief EffectFolderTreeItem::createFileWithData
 * @param index
 * @param longName
 * @param fileName
 *
 * re-implementation of ContentFolderTreeItem::createFileWithData
 * Should only be called as part of ContentFolderTreeItem::updateModelWithData
 */
bool EffectFolderTreeItem::createFileWithData(int index, const QString & longName, const QString& fileName)
{
   // Verify item can be inserted at this index
   if(index > childCount()){
      qWarning() << "EffectFolderTreeItem::createFileWithData - ERROR - cannot insert child at index " << index << ". childCount() = " << childCount();
      return false;
   }

   EffectFileItem *child = nullptr;

   // 1 - Verify if child exists at required index
   if(index < childCount() && childItems()->at(index)->data(NAME).toString().compare(longName) == 0){
      // nothing to do at this level
      // set child for exploration
      child = qobject_cast<EffectFileItem *>(childItems()->at(index));
      if(!child){
         qWarning() << "EffectFolderTreeItem::createFileWithData - ERROR - child of type " << childItems()->at(index)->metaObject()->className() << " is not a EffectFileItem.";
         return false;
      }
   }

   // 2 - Verify if child with this name exists at a different index
   if(!child){
      // Technically, all previous indexes have been replaced by csv content
      for(int i = index + 1; i < childCount(); i++){
         if(childItems()->at(i)->data(NAME).toString().compare(fileName) == 0){
            child = qobject_cast<EffectFileItem *>(childItems()->at(i));
            if(!child){
               qWarning() << "EffectFolderTreeItem::createFileWithData - ERROR - child of type " << childItems()->at(i)->metaObject()->className() << " is not a EffectFileItem.";
               return false;
            }
            model()->moveItem(child, i, index);
         }
      }
   }

   // 3 - Create child if does not exist
   if(!child){
      // Create Tree and populated with file graph content
      child = new EffectFileItem(this);
      child->setData(NAME, QVariant(longName));
      child->setData(FILE_NAME, QVariant(fileName));
      model()->insertItem(child, index);
   }


   return true;
}


/**
 * @brief EffectFolderTreeItem::effectWithFileName
 * @param efxFileName
 * @return
 *
 * Returns a pointer on corresponging Effect Model Item
 */
EffectFileItem *EffectFolderTreeItem::effectWithFileName(const QString &efxFileName)
{
   if(!m_CSVFile.containsFileName(efxFileName)){
      qWarning() << "EffectFolderTreeItem::effectWithFileName - ERROR - (!m_CSVFile.containsFileName(efxFileName))";
      return nullptr;
   }
   int index = m_CSVFile.indexOfFileName(efxFileName);
   return static_cast<EffectFileItem *>(child(index));
}

/**
 * @brief EffectFolderTreeItem::effectsForSong
 * @param songId
 * @return
 * Returns a list of pointers on corresponging Effect Model Item
 */
QList<EffectFileItem *> EffectFolderTreeItem::effectsForSong(const QUuid &songId)
{
   QList<EffectFileItem *> effectList;
   QByteArray songIdByteArray = songId.toByteArray();

   for(QMapIterator<QString, QMap<QByteArray, qint32> > i(m_UsageInfo); i.hasNext();){
      i.next();

      // if effect is used by song
      if(i.value().contains(songIdByteArray)){
         QString efxFileName = i.key();
         effectList.append(effectWithFileName(efxFileName));
      }
   }

   return effectList;
}

/**
 * @brief EffectFolderTreeItem::computeHash
 * @param recursive
 * Called as part of AbstractTreeItem::computeHash
 * model()->setArchiveDirty() need to be called after this call.
 */
void EffectFolderTreeItem::computeHash(bool recursive)
{
   QCryptographicHash cr(QCryptographicHash::Sha256);

   QFile csvFile(m_CSVFile.fileName());
   if(csvFile.open(QIODevice::ReadOnly)){
      cr.addData(&csvFile);
      csvFile.close();
   } else {
      qWarning() << "EffectFolderTreeItem::computeHash - ERROR 1 - Unable to open " << m_CSVFile.fileName();
   }

   if(m_UsageFile.open(QIODevice::ReadOnly)){
      cr.addData(&m_UsageFile);
      m_UsageFile.close();
   } else {
      qWarning() << "EffectFolderTreeItem::computeHash - ERROR 2 - Unable to open " << m_UsageFile.fileName();
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
 * @brief EffectFolderTreeItem::removeChildContentAt
 * @param index
 * @param save
 *
 * re-implementation of ContentFolderTreeItem::removeChildContentAt
 * Call ContentFolderTreeItem::removeChild to remove child completly (without notification to model). And model->remove... otherwise.
 * computeHash() and model()->setArchiveDirty() need to be called after this call.
 */
void EffectFolderTreeItem::removeChildContentAt(int index, bool save)
{
   QFileInfo fi = folderFI();
   QDir dir(fi.absoluteFilePath());

   QString fileName(m_CSVFile.fileNameAt(index).split('.').at(0) + "." BMFILES_CONFIG_FILE_EXTENSION);
   QFile childHashFile(dir.absoluteFilePath(fileName));
   if(childHashFile.exists()){
      childHashFile.remove();
   }
   ContentFolderTreeItem::removeChildContentAt(index, save);

}

/**
 * @brief EffectFolderTreeItem::prepareSync
 * @param dstPath
 * @param p_cleanUp
 * @param p_copySrc
 * @param p_copyDst
 *
 * re-implementation of AbstractTreeItem::prepareSync
 * Called as part of BeatsProjectModel::synchronizeModal.
 */
void EffectFolderTreeItem::prepareSync(const QString &dstPath, QList<QString> *p_cleanUp, QList<QString> *p_copySrc, QList<QString> *p_copyDst)
{
   QFileInfo fi = folderFI();
   QDir dir(fi.absoluteFilePath());

   if(!p_cleanUp || !p_copySrc || !p_copyDst){
      qWarning() << "EffectFolderTreeItem::prepareSync - ERROR 1 - return list is null";
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

      // add usage file
      p_copySrc->append(dir.absoluteFilePath(BMFILES_EFFECT_USAGE_FILE_NAME));
      p_copyDst->append(dstDir.absoluteFilePath(BMFILES_EFFECT_USAGE_FILE_NAME));

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

      // replace csv file
      p_cleanUp->append(dstDir.absoluteFilePath(QFileInfo(m_CSVFile.fileName()).fileName()));
      p_copySrc->append(m_CSVFile.fileName());
      p_copyDst->append(dstDir.absoluteFilePath(QFileInfo(m_CSVFile.fileName()).fileName()));

      // replace hash file
      p_cleanUp->append(dstDir.absoluteFilePath(BMFILES_HASH_FILE_NAME));
      p_copySrc->append(dir.absoluteFilePath(BMFILES_HASH_FILE_NAME));
      p_copyDst->append(dstDir.absoluteFilePath(BMFILES_HASH_FILE_NAME));

      // replace usage file
      p_cleanUp->append(dstDir.absoluteFilePath(BMFILES_EFFECT_USAGE_FILE_NAME));
      p_copySrc->append(dir.absoluteFilePath(BMFILES_EFFECT_USAGE_FILE_NAME));
      p_copyDst->append(dstDir.absoluteFilePath(BMFILES_EFFECT_USAGE_FILE_NAME));

      // Loop on all children so that they can decide whether or not to add themselves
      for(int i = 0; i < childCount(); i++){
         QString childPath = dstDir.absoluteFilePath(child(i)->data(FILE_NAME).toString());
         child(i)->prepareSync(childPath, p_cleanUp, p_copySrc, p_copyDst);
      }
   }
}
