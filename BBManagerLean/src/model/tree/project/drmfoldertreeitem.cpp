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
#include "drmfoldertreeitem.h"
#include "drmfileitem.h"
#include "beatsprojectmodel.h"
#include "drmmaker/Model/drmmakermodel.h"

#include <QMessageBox>
#include <QProgressDialog>

DrmFolderTreeItem::DrmFolderTreeItem(BeatsProjectModel *p_model, FolderTreeItem *parent):
   ContentFolderTreeItem(p_model, parent)
{
   setName("DRUMSETS");
   setFileName("DRUMSETS");
   setChildrenTypes(QString(BMFILES_DRUMSET_FILTER).toUpper());
   setSubFoldersAllowed(false);
   setSubFilesAllowed(true);
}

/**
 * @brief DrmFolderTreeItem::containsDrm
 * @param drmSourcePath
 * @return
 *
 * Utility function to access m_CSVFile.containsLongName. Does not modify model
 */
bool DrmFolderTreeItem::containsDrm(const QString &drmSourcePath) const
{
   QString drmLongName = DrmMakerModel::getDrumsetNameStatic(drmSourcePath);
   return m_CSVFile.containsLongName(drmLongName);
}

/**
 * @brief DrmFolderTreeItem::drmFileName
 * @param longName
 * @return
 *
 * Utility function to access m_CSVFile.longName2FileName. Does not modify model.
 */
QString DrmFolderTreeItem::drmFileName(const QString &longName) const
{
   return m_CSVFile.longName2FileName(longName);
}

/**
 * @brief DrmFolderTreeItem::drmLongName
 * @param fileName
 * @return
 * Utility function to access m_CSVFile.fileName2LongName. Does not modify model.
 */
QString DrmFolderTreeItem::drmLongName(const QString &fileName) const
{
    return m_CSVFile.fileName2LongName(fileName);
}

/**
 * @brief DrmFolderTreeItem::addDrmModal
 * @param p_parentWidget
 * @param drmSourcePath
 * @return return resulting object, null on failure
 */
DrmFileItem *DrmFolderTreeItem::addDrmModal(QWidget *p_parentWidget, const QString &drmSourcePath)
{
   QFileInfo fi = folderFI();
   QDir dir(fi.absoluteFilePath());

   const int OP_CNT = 4;

   QProgressDialog progress("Importing Drumset...", "Abort", 0, OP_CNT, p_parentWidget);
   progress.setWindowModality(Qt::WindowModal);
   progress.setMinimumDuration(0);
   progress.setValue(0);

   QFileInfo drmInfo(drmSourcePath);

   // 1 - validate that file exists
   if(progress.wasCanceled()){
      return nullptr;
   }
   if(!drmInfo.exists()){
      qWarning() << "DrmFolderTreeItem::addDrmModal - ERROR - (!drmInfo.exists())";
      progress.setValue(OP_CNT);
      QMessageBox::critical(p_parentWidget, tr("Adding Drumset"), tr("Unable to find drumset %1").arg(drmSourcePath));
      return nullptr;
   }

   progress.setValue(1);

   // 2 - extract effect name out of path
   if(progress.wasCanceled()){
      return nullptr;
   }

   QString drmLongName = DrmMakerModel::getDrumsetNameStatic(drmSourcePath);
   progress.setValue(2);

   // 3 - verify if a drm with this name exists. Perform required operation depending on results
   if(progress.wasCanceled()){
      return nullptr;
   }
   QString drmFileName;
   int index = -1;

   //if no drm with this name
   if(!m_CSVFile.containsLongName(drmLongName)){

      // 3.1.1 - Create File Entry
      m_CSVFile.append(drmLongName, CsvConfigFile::DRUM_SET);

      // 3.1.2 - Generate fileName
      drmFileName = m_CSVFile.lastFileName();

      // 3.1.3 - Copy file
      QFile::copy(drmSourcePath, dir.absoluteFilePath(drmFileName));

      // 3.1.4 - Create corresponding object
      DrmFileItem *p_DrmFile = new DrmFileItem(this);
      p_DrmFile->setData(NAME, QVariant(drmLongName));
      p_DrmFile->setData(FILE_NAME, QVariant(drmFileName));

      index = childCount();

      // will notify views but will not update csv, update hash and set Archive dirty
      model()->insertItem(p_DrmFile, index);


   } else {
      // 3.2.1 Retrieve corresponding object's filename
      qWarning() << "DrmFolderTreeItem::addDrm - WARNING - Drumset already exists";
      index = m_CSVFile.indexOfLongName(drmLongName);
   }
   progress.setValue(3);

   // 4 - save
   m_CSVFile.write();
   progress.setValue(OP_CNT);

   computeHash(true);
   propagateHashChange();
   model()->setProjectDirty();

   // return resulting object
   return static_cast<DrmFileItem *>(child(index));
}

bool DrmFolderTreeItem::removeDrmModal(QWidget *p_parentWidget, const QString &longName)
{

   qDebug() << "DrmFolderTreeItem::removeDrmModal " << longName;
   const int OP_CNT = 3;

   QProgressDialog progress(tr("Removing Drumset..."), tr("Abort"), 0, OP_CNT, p_parentWidget);
   progress.setWindowModality(Qt::WindowModal);
   progress.setMinimumDuration(0);
   progress.setValue(0);

   // 1 - validate that drumset exists
   if(progress.wasCanceled()){
      return false;
   }
   if(!m_CSVFile.containsLongName(longName)){
      qWarning() << "DrmFolderTreeItem::removeDrmModal - ERROR - (!drmInfo.exists())";
      progress.setValue(OP_CNT);
      QMessageBox::critical(p_parentWidget, tr("Removing Drumset"), tr("Unable to find drumset %1").arg(longName));
      return false;
   }

   progress.setValue(1);

   // 2 - retrieve index
   if(progress.wasCanceled()){
      return false;
   }
   int drmIndex = m_CSVFile.indexOfLongName(longName);
   qDebug() << "   drmIndex = " << drmIndex;
   progress.setValue(2);

   // 3 - Perform removal
   if(progress.wasCanceled()){
      return false;
   }

   // Will notify views, update csv, update hash and set Archive dirty
   model()->removeItem(child(drmIndex), drmIndex);

   progress.setValue(OP_CNT);
   return true;
}

/**
 * @brief DrmFolderTreeItem::replaceDrumsetModal
 * @param p_parentWidget
 * @param oldLongName
 * @param drmSourcePath
 * @return return resulting object, null on failure
 */
DrmFileItem *DrmFolderTreeItem::replaceDrumsetModal(QWidget *p_parentWidget, const QString &oldLongName, const QString &drmSourcePath)
{
   // Will notify views, update csv, update hash and set Archive dirty
   if(!removeDrmModal(p_parentWidget, oldLongName)){
      qWarning() << "DrmFolderTreeItem::replaceDrumsetModal - ERROR 1 - removeDrmModal";
      return nullptr;
   }

   // Will notify views, update csv, update hash and set Archive dirty

   DrmFileItem *p_drmFileItem = addDrmModal(p_parentWidget, drmSourcePath);
   if(!p_drmFileItem){
      qWarning() << "DrmFolderTreeItem::replaceDrumsetModal - ERROR 2 - addDrmModal";
      return nullptr;
   }

   return p_drmFileItem;
}



/**
 * @brief DrmFolderTreeItem::createFileWithData
 * @param index
 * @param longName
 * @param fileName
 *
 * re-implementation of ContentFolderTreeItem::createFileWithData
 * Should only be called as part of ContentFolderTreeItem::updateModelWithData
 */
bool DrmFolderTreeItem::createFileWithData(int index, const QString & longName, const QString& fileName)
{
   // Verify item can be inserted at this index
   if(index > childCount()){
      qWarning() << "DrmFolderTreeItem::createFileWithData - ERROR - cannot insert child at index " << index << ". childCount() = " << childCount();
      return false;
   }

   DrmFileItem *child = nullptr;

   // 1 - Verify if child exists at required index
   if(index < childCount() && childItems()->at(index)->data(NAME).toString().compare(longName) == 0){
      // nothing to do at this level
      // set child for exploration
      child = qobject_cast<DrmFileItem *>(childItems()->at(index));
      if(!child){
         qWarning() << "DrmFolderTreeItem::createFileWithData - ERROR - child of type " << childItems()->at(index)->metaObject()->className() << " is not a DrmFileItem.";
         return false;
      }
   }

   // 2 - Verify if child with this name exists at a different index
   if(!child){
      // Technically, all previous indexes have been replaced by csv content
      for(int i = index + 1; i < childCount(); i++){
         if(childItems()->at(i)->data(NAME).toString().compare(fileName) == 0){
            child = qobject_cast<DrmFileItem *>(childItems()->at(i));
            if(!child){
               qWarning() << "DrmFolderTreeItem::createFileWithData - ERROR - child of type " << childItems()->at(i)->metaObject()->className() << " is not a DrmFileItem.";
               return false;
            }
            model()->moveItem(child, i, index);
         }
      }
   }

   // 3 - Create child if does not exist
   if(!child){
      // Create Tree and populated with file graph content
      child = new DrmFileItem(this);
      child->setData(NAME, QVariant(longName));
      child->setData(FILE_NAME, QVariant(fileName));
      model()->insertItem(child, index);
   }

   return true;
}
