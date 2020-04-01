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
#include "drmlistmodel.h"
#include "../workspace/workspace.h"
#include "../workspace/contentlibrary.h"
#include "../workspace/libcontent.h"
#include "drmmaker/Model/drmmakermodel.h"
#include "../model/tree/project/beatsprojectmodel.h"
#include "../model/tree/project/drmfoldertreeitem.h"
#include "../model/tree/project/drmfileitem.h"
#include "../model/tree/project/songsfoldertreeitem.h"
#include "../model/tree/abstracttreeitem.h"

#include <QMessageBox>
#include <QApplication>

DrmListModel::DrmListModel(QWidget *p_parentWidget, Workspace *p_workspace) :
   QStandardItemModel(p_parentWidget)
{
   mp_beatsModel = nullptr;
   mp_drmMakerModel = nullptr;
   mp_workspace = p_workspace;
   populate();

   if(mp_workspace->isValid()){
       // connect file system watcher
       connect(&m_fileSystemWatcher, SIGNAL(directoryChanged(QString)), this, SLOT(slotOnWatchedDirectoryChange(QString)));
       connect(&m_fileSystemWatcher, SIGNAL(fileChanged(QString)), this, SLOT(slotOnWatchedFileChange(QString)));
       connect(&m_timer, SIGNAL(timeout()), this, SLOT(slotPeriodicFileCheck()));
   }

   // connect workspace change watching
   connect(mp_workspace, SIGNAL(sigChanged(bool)), this, SLOT(slotOnWorkspaceChange(bool)));
}


bool DrmListModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
   if(index.column() == NAME && (role == Qt::DisplayRole || role == Qt::EditRole)){
      if(item(index.row(), TYPE)->data(Qt::DisplayRole).toString() == "default"){
         QMessageBox::warning(parentWidget(), tr("Editing Drumset"), tr("Default drum sets should not be edited.\n\nChanges ignored."));
         return false;
      }

      if(item(index.row(), TYPE)->data(Qt::DisplayRole).toString() == "project"){
         QMessageBox::warning(parentWidget(), tr("Editing Drumset"), tr("In order to edit project's drumset, copy drumset to workspace first."));
         return false;
      }

      // memorize initial data
      QVariant prevValue = index.data(role);
      if(prevValue != value){
         QByteArray crc = DrmMakerModel::renameDrm(item(index.row(), ABSOLUTE_PATH)->data(Qt::DisplayRole).toString(), value.toString());
         if(crc.isEmpty()){
            QMessageBox::critical(parentWidget(), tr("Editing Drumset"), tr("Error renaming drum set"));
            return false;
         }
         // refresh information
         QFileInfo drmFI(item(index.row(), ABSOLUTE_PATH)->data(Qt::DisplayRole).toString());

         QStandardItemModel::setData(index, value, role);
         item(index.row(),         CRC)->setData(QVariant(crc), Qt::EditRole);
         item(index.row(), LAST_CHANGE)->setData(QVariant(drmFI.lastModified()), Qt::EditRole);
         item(index.row(),   FILE_SIZE)->setData(QVariant(drmFI.size()), Qt::EditRole);

         // Need to refresh drumset in project
         // Need to refresh all default drumsets
         if(mp_beatsModel){
            if(item(index.row(), IN_PROJECT)->data(Qt::DisplayRole).toBool()){
               // retrieve original DRM file name (in project)
               QString oldDrmName = prevValue.toString();
               DrmFileItem *p_drmFileItem = mp_beatsModel->drmFolder()->replaceDrumsetModal(parentWidget(), oldDrmName, item(index.row(), ABSOLUTE_PATH)->data(Qt::DisplayRole).toString());
               // Also replace Default Drumset name in all songs using it
               mp_beatsModel->songsFolder()->replaceDrumsetModal(parentWidget(), oldDrmName, p_drmFileItem);
            }
         }

      }
      return true;

   } else {
      return QStandardItemModel::setData(index, value, role);
   }

}

QUndoStack* DrmListModel::undoStack()
{
    return mp_drmMakerModel->undoStack();
}

QWidget *DrmListModel::parentWidget()
{
   return static_cast<QWidget *>(parent());
}


void DrmListModel::slotOnWorkspaceChange(bool valid){
    // invalidate all pending operations
    m_timer.stop();

    // disconnect file system watcher
    disconnect(&m_fileSystemWatcher, SIGNAL(directoryChanged(QString)), this, SLOT(slotOnWatchedDirectoryChange(QString)));
    disconnect(&m_fileSystemWatcher, SIGNAL(fileChanged(QString)), this, SLOT(slotOnWatchedFileChange(QString)));
    disconnect(&m_timer, SIGNAL(timeout()), this, SLOT(slotPeriodicFileCheck()));

    // Disconnect Project related signals
    if(mp_beatsModel){
        // disconnect check state watcher
        disconnect(this, SIGNAL(itemChanged(QStandardItem*)), this, SLOT(slotOnItemChanged(QStandardItem*)));
    }

    // repopulate from new workspace location
    populate(); // verifies validity

    if(valid){

        // repopulate content from Project
        if(mp_beatsModel){
           addBeatsModelContent();

           // connect check state watcher
           connect(this, SIGNAL(itemChanged(QStandardItem*)), this, SLOT(slotOnItemChanged(QStandardItem*)));
        }

        // reconnect file system watcher
        connect(&m_fileSystemWatcher, SIGNAL(directoryChanged(QString)), this, SLOT(slotOnWatchedDirectoryChange(QString)));
        connect(&m_fileSystemWatcher, SIGNAL(fileChanged(QString)), this, SLOT(slotOnWatchedFileChange(QString)));
        connect(&m_timer, SIGNAL(timeout()), this, SLOT(slotPeriodicFileCheck()));
    }
}

void DrmListModel::populate()
{
   clear();
   setColumnCount(ENUM_SIZE);

   if(mp_workspace->isValid()){
       // Include all drum sets from the default library
       populateFromPath(mp_workspace->userLibrary()->libDrumSets()->defaultPath(), "user");
   }
}

void DrmListModel::populateFromPath(const QString &path, const QString &type)
{
   QDir dir(path);
   if(!dir.exists()){
      qWarning() << "DrmListModel::populateFromPath - ERROR 1 - path " << path << " is not a dir";
   }
   // Add path to file system watcher
   if(!m_fileSystemWatcher.directories().contains(dir.absolutePath(), Qt::CaseInsensitive)){
      m_fileSystemWatcher.addPath(dir.absolutePath());
   }

   QFileInfoList fil = dir.entryInfoList(DrmMakerModel::getFileFilters(),QDir::Files | QDir::Hidden | QDir::NoDotAndDotDot);

   foreach(const QFileInfo &info, fil){
      if(!addDrmToList(info, type)){
         qWarning() << "DrmListModel::populateFromPath - ERROR 2 - error inserting" << info.absoluteFilePath();
      }
   }
}

bool DrmListModel::addDrmToList(const QFileInfo &drmFI, const QString &type)
{
   QString newDrmName = DrmMakerModel::getDrumsetNameStatic(drmFI.absoluteFilePath());
   QByteArray newDrmCRC = DrmMakerModel::getCRCStatic(drmFI.absoluteFilePath());

   if(newDrmName.isEmpty() || newDrmCRC.isEmpty()){
      qWarning() << "DrmListModel::addDrmToList - ERROR 1 - Unable to read name or CRC";
      return false;
   }

   // verify if an equivalent Drm exists in opened project
   int inPrjRow = -1;
   if(mp_beatsModel){
      for(int row = 0; row < rowCount(); row ++){

         if(item(row, NAME)->data(Qt::DisplayRole).toString() == newDrmName       &&
               item(row, TYPE)->data(Qt::DisplayRole).toString() == "project"        &&
               item(row,  CRC)->data(Qt::DisplayRole).toByteArray() == newDrmCRC){
            qDebug() << "DrmListModel::addDrmToList - DEBUG - found on row " << inPrjRow;
            inPrjRow = row;
            break;

         } else {
            qDebug() << "DrmListModel::addDrmToList - DEBUG - not found ";
            qDebug() << "  newDrmName " << newDrmName;
            qDebug() << "  newDrmCRC  " << newDrmCRC.toHex();

            qDebug() << "  item(row, NAME)->data(Qt::DisplayRole).toString()    " << (item(row, NAME)->data(Qt::DisplayRole).toString());
            qDebug() << "  item(row, TYPE)->data(Qt::DisplayRole).toString()    " << (item(row, TYPE)->data(Qt::DisplayRole).toString());
            qDebug() << "  item(row,  CRC)->data(Qt::DisplayRole).toByteArray() " << (item(row,  CRC)->data(Qt::DisplayRole).toByteArray().toHex());

            qDebug() << "  item(row, NAME)->data(Qt::DisplayRole).toString()    == newDrmName " << (item(row, NAME)->data(Qt::DisplayRole).toString() == newDrmName);
            qDebug() << "  item(row, TYPE)->data(Qt::DisplayRole).toString()    == 'project'  " << (item(row, TYPE)->data(Qt::DisplayRole).toString() == "project");
            qDebug() << "  item(row,  CRC)->data(Qt::DisplayRole).toByteArray() == newDrmCRC  " << (item(row,  CRC)->data(Qt::DisplayRole).toByteArray() == newDrmCRC);


         }
      }
   }

   // If drumset exists in opened project
   if(inPrjRow >= 0){
      // Refresh info for inPrjRow
      item(inPrjRow,     FILE_NAME)->setData(QVariant(drmFI.fileName()), Qt::EditRole);
      item(inPrjRow, ABSOLUTE_PATH)->setData(QVariant(drmFI.absoluteFilePath()), Qt::EditRole);
      item(inPrjRow,          TYPE)->setData(QVariant(type), Qt::EditRole);
      item(inPrjRow,   LAST_CHANGE)->setData(QVariant(drmFI.lastModified()), Qt::EditRole);
      item(inPrjRow,     FILE_SIZE)->setData(QVariant(drmFI.size()), Qt::EditRole);

      item(inPrjRow,    IN_PROJECT)->setData(QVariant(true), Qt::EditRole);

      // Note: Need to set checked before seting checkable to avoid triggering drm add to project
      item(inPrjRow, NAME)->setCheckState(Qt::Checked);
      item(inPrjRow, NAME)->setCheckable(true);

      // Clear italic
      QFont currentFont = item(inPrjRow, NAME)->font();
      currentFont.setItalic(false);
      item(inPrjRow, NAME)->setFont(currentFont);

      // Make sure editable
      Qt::ItemFlags currentFlags = item(inPrjRow, NAME)->flags();
      currentFlags |= Qt::ItemIsEditable;
      item(inPrjRow, NAME)->setFlags(currentFlags);

      // verify if drumset is opened
      if(mp_drmMakerModel){
         if(mp_drmMakerModel->drmPath().toUpper() == drmFI.absoluteFilePath().toUpper()){
            item(inPrjRow, OPENED)->setData(QVariant(true), Qt::EditRole);
            // set bold idatic
            QFont currentFont = item(inPrjRow, NAME)->font();
            currentFont.setWeight(QFont::Black);
            currentFont.setItalic(true);
            item(inPrjRow, NAME)->setFont(currentFont);

            // Make not editable while opened
            Qt::ItemFlags currentFlags = item(inPrjRow, NAME)->flags();
            currentFlags &= ~Qt::ItemIsEditable;
            item(inPrjRow, NAME)->setFlags(currentFlags);
         }
      }

      // Refresh tooltip
      refreshToolTip(inPrjRow);

      // If drumset does not exist in opened project
   } else {

      // Insert new item
      QList<QStandardItem *> rowItems;
      for(int i = 0; i < ENUM_SIZE; i++){
         rowItems.append(new QStandardItem());
      }

      rowItems.at(NAME)->setData(QVariant(newDrmName), Qt::EditRole);

      if(mp_beatsModel){
         rowItems.at(IN_PROJECT)->setData(QVariant(false), Qt::EditRole);
         rowItems.at(NAME)->setCheckable(true);
         rowItems.at(NAME)->setCheckState(Qt::Unchecked);
      } else {
         rowItems.at(NAME)->setCheckable(false);
      }

      if(type == "default"){
         // Display semi bold
         QFont currentFont = rowItems.at(NAME)->font();
         currentFont.setWeight(QFont::DemiBold);
         rowItems.at(NAME)->setFont(currentFont);

         // Make sure not editable
         Qt::ItemFlags currentFlags = rowItems.at(NAME)->flags();
         currentFlags &= ~Qt::ItemIsEditable;
         rowItems.at(NAME)->setFlags(currentFlags);
      }

      rowItems.at(FILE_NAME)->setData(QVariant(drmFI.fileName()), Qt::EditRole);
      rowItems.at(ABSOLUTE_PATH)->setData(QVariant(drmFI.absoluteFilePath()), Qt::EditRole);
      rowItems.at(TYPE)->setData(QVariant(type), Qt::EditRole);
      rowItems.at(CRC)->setData(QVariant(newDrmCRC), Qt::EditRole);
      rowItems.at(LAST_CHANGE)->setData(QVariant(drmFI.lastModified()), Qt::EditRole);
      rowItems.at(FILE_SIZE)->setData(QVariant(drmFI.size()), Qt::EditRole);
      rowItems.at(OPENED)->setData(QVariant(false), Qt::EditRole);

      // verify if drumset is opened
      if(mp_drmMakerModel){
         if(mp_drmMakerModel->drmPath().toUpper() == drmFI.absoluteFilePath().toUpper()){
            rowItems.at(OPENED)->setData(QVariant(true), Qt::EditRole);
            // set bold idatic
            QFont currentFont = rowItems.at(NAME)->font();
            currentFont.setWeight(QFont::Black);
            currentFont.setItalic(true);
            rowItems.at(NAME)->setFont(currentFont);

            // Make sure not editable
            Qt::ItemFlags currentFlags = rowItems.at(NAME)->flags();
            currentFlags &= ~Qt::ItemIsEditable;
            rowItems.at(NAME)->setFlags(currentFlags);
         }
      }

      appendRow(rowItems);

      refreshToolTip(rowCount() - 1);

      if(!resloveDuplicateName(rowCount() - 1)){
         qWarning() << "Error resolving duplicate";
      }

   }

   m_fileSystemWatcher.addPath(drmFI.absoluteFilePath());


   return true;
}

void DrmListModel::addBeatsModelDrmToList(int beatsRow)
{
   QModelIndex beatsIndex = mp_beatsModel->index(beatsRow, AbstractTreeItem::NAME, mp_beatsModel->drmFolderIndex());

   QList<QStandardItem *> rowItems;
   for(int i = 0; i < ENUM_SIZE; i++){
      rowItems.append(new QStandardItem());
   }
   rowItems.at(NAME)->setData(beatsIndex.data(Qt::DisplayRole), Qt::EditRole);
   rowItems.at(NAME)->setCheckable(false);

 // display project drm in italic
   QFont currentFont = rowItems.at(NAME)->font();
   currentFont.setItalic(true);
   rowItems.at(NAME)->setFont(currentFont);

   // Make sure not editable
   Qt::ItemFlags currentFlags = rowItems.at(NAME)->flags();
   currentFlags &= ~Qt::ItemIsEditable;
   rowItems.at(NAME)->setFlags(currentFlags);

   // NOTE: file name cannot be used (since hashed)
   rowItems.at(TYPE)->setData(QVariant("project"), Qt::EditRole);
   rowItems.at(CRC)->setData(beatsIndex.sibling(beatsRow, AbstractTreeItem::HASH).data(), Qt::EditRole);
   rowItems.at(OPENED)->setData(QVariant(false), Qt::EditRole);
   rowItems.at(IN_PROJECT)->setData(QVariant(true), Qt::EditRole);

   appendRow(rowItems);

   // Refresh tooltips
   refreshToolTip(rowCount() - 1);
}


bool DrmListModel::refreshDrm(const QFileInfo &drmFI)
{
   int row;

   // 1 - Find out the row that contains this item
   for(row = 0; row < rowCount(); row++){
      if(item(row, ABSOLUTE_PATH)->data(Qt::DisplayRole).toString().compare(drmFI.absoluteFilePath(), Qt::CaseInsensitive) == 0){
         break;
      }
   }

   if(row >= rowCount()){
      qDebug() << "DrmListModel::refreshDrm - ERROR 1 - Not Found " << drmFI.absoluteFilePath();
      return false;
   }

   // 2 - Get original name and CRC
   QString newDrmName = DrmMakerModel::getDrumsetNameStatic(drmFI.absoluteFilePath());
   QByteArray newDrmCRC = DrmMakerModel::getCRCStatic(drmFI.absoluteFilePath());

   if(newDrmName.isEmpty() || newDrmCRC.isEmpty()){
      qWarning() << "DrmListModel::refreshDrm - ERROR 2 - Unable to read name or CRC";
      return false;
   }

   return DrmListModel::refreshDrmInternal(drmFI, row, newDrmName, newDrmCRC);
}

bool DrmListModel::refreshDrm(int row)
{
   QFileInfo drmFI(item(row, ABSOLUTE_PATH)->data(Qt::DisplayRole).toString());
   if(!drmFI.exists()){
      qWarning() << "DrmListModel::refreshDrm - ERROR 3 - !drmFI.exists()";
      return false;
   }

   QString newDrmName = DrmMakerModel::getDrumsetNameStatic(drmFI.absoluteFilePath());
   QByteArray newDrmCRC = DrmMakerModel::getCRCStatic(drmFI.absoluteFilePath());

   if(newDrmName.isEmpty() || newDrmCRC.isEmpty()){
      qWarning() << "DrmListModel::refreshDrm - ERROR 4 - Unable to read name or CRC";
      return false;
   }

   return DrmListModel::refreshDrmInternal(drmFI, row, newDrmName, newDrmCRC);
}

bool DrmListModel::refreshDrm(int row, const QString &newDrmName, const QByteArray &crc)
{
   QFileInfo drmFI(item(row, ABSOLUTE_PATH)->data(Qt::DisplayRole).toString());
   if(!drmFI.exists()){
      qWarning() << "DrmListModel::refreshDrm - ERROR 5 - !drmFI.exists()";
      return false;
   }

   return DrmListModel::refreshDrmInternal(drmFI, row, newDrmName, crc);
}

bool DrmListModel::refreshDrmInternal(const QFileInfo &drmFI, int row, const QString &newDrmName, const QByteArray &crc)
{

   // 1 - resolve duplicate names
   // 1.1 - verify if the name for row exists somewhere else + build a list of all names
   bool duplicateFound = false;

   QSet<QString> drmNames;
   for(int i = 0; i < rowCount(); i++){
      if(i != row){
         drmNames.insert(item(i, NAME)->data(Qt::DisplayRole).toString().toUpper());
         if(item(i, NAME)->data(Qt::DisplayRole).toString().toUpper() == newDrmName.toUpper()){
            duplicateFound = true;
         }
      }
   }

   QString resolvedName = newDrmName;
   QByteArray resolvedCRC = crc;

   if(duplicateFound){
      // 3.2 - Resolve duplicate name
      int duplicates = 1;

      resolvedName = QString("%1(%2)").arg(newDrmName).arg(duplicates);
      while(drmNames.contains(resolvedName.toUpper())){
         resolvedName = QString("%1(%2)").arg(newDrmName).arg(++duplicates);
      }

      // 3.3 - Modify the file in order to set the proper name
      resolvedCRC = DrmMakerModel::renameDrm(drmFI.absoluteFilePath(), resolvedName);

      if(crc.isEmpty()){
         qWarning() << "DrmListModel::refreshDrmInternal - ERROR 1 - renameDrm returned empty CRC";
         return false;
      }
   }


   // 2 - Only modify if there really is a change in order to avoid sending "data changed" signals for no reason

   bool fileContentChange = false;
   QString originalName = item(row, NAME)->data(Qt::DisplayRole).toString();
   if(item(row, NAME)->data(Qt::DisplayRole).toString() != resolvedName){
      item(row, NAME)->setData(QVariant(resolvedName), Qt::EditRole);
      fileContentChange = true;
   }
   if(item(row, CRC)->data(Qt::DisplayRole).toByteArray() != resolvedCRC){
      item(row, CRC)->setData(QVariant(resolvedCRC), Qt::EditRole);
      fileContentChange = true;
   }
   if(item(row, LAST_CHANGE)->data(Qt::DisplayRole).toDateTime() != drmFI.lastModified()){
      item(row, LAST_CHANGE)->setData(QVariant(drmFI.lastModified()), Qt::EditRole);
   }
   if(item(row, FILE_SIZE)->data(Qt::DisplayRole).toLongLong() != drmFI.size()){
      item(row, FILE_SIZE)->setData(QVariant(drmFI.size()), Qt::EditRole);
   }

   if(item(row, OPENED)->data(Qt::DisplayRole).toBool() &&
         (resolvedName != newDrmName)){
      emit sigOpenedDrmNameChanged(newDrmName, resolvedName);
   }

   if(mp_beatsModel && fileContentChange && item(row, IN_PROJECT)->data(Qt::DisplayRole).toBool()){
      DrmFileItem *p_drmFileItem = mp_beatsModel->drmFolder()->replaceDrumsetModal(parentWidget(), originalName, drmFI.absoluteFilePath());
      // Rename Default Drm as well
      mp_beatsModel->songsFolder()->replaceDrumsetModal(parentWidget(), originalName, p_drmFileItem);
   }

   return true;

}


void DrmListModel::slotOnWatchedDirectoryChange(const QString & path)
{
   qDebug() << "DrmListModel::slotOnWatchedDirectoryChange " << path;


   // 1 - Determine the whether the change is on user lib or default lib
   QString type;
   if(path.compare(mp_workspace->userLibrary()->libDrumSets()->defaultPath(), Qt::CaseInsensitive) == 0){
      type = "user";
   } else {
      qWarning() << "DrmListModel::slotOnWatchedDirectoryChange - ERROR 1 - Not Found " << path;
      return;
   }


   // 2 - Create a set with the file paths
   QDir currentDir(path);
   if(!currentDir.exists()){
      qWarning() << "DrmListModel::slotOnWatchedDirectoryChange - ERROR 2 - Directory removed " << path;
      return;
   }

   QSet<QString> currentFilePaths;
   QFileInfoList currentFIL = currentDir.entryInfoList(DrmMakerModel::getFileFilters(), QDir::Files | QDir::Hidden | QDir::NoDotAndDotDot);
   foreach(const QFileInfo &currentFI, currentFIL){
      currentFilePaths.insert(currentFI.absoluteFilePath().toUpper());
   }

   // 3 - Remove row for drumset that doesn't exist anymore
   // A set of listed file paths is created at the same time
   QSet<QString> previousFilePaths;
   for(int row = rowCount() - 1; row >= 0; row--){
      qDebug() << "   row = " << row;
      if(item(row, TYPE)->data(Qt::DisplayRole).toString() == type){

         qDebug() << "      type = " << type;
         if(!currentFilePaths.contains(item(row, ABSOLUTE_PATH)->data(Qt::DisplayRole).toString().toUpper())){
            qDebug() << "         need to remove";
            if(mp_beatsModel &&
                  item(row, IN_PROJECT)->data(Qt::DisplayRole).toBool()){


               // Clear data
               item(row,     FILE_NAME)->setData(QVariant(), Qt::EditRole);
               item(row, ABSOLUTE_PATH)->setData(QVariant(), Qt::EditRole);
               item(row,          TYPE)->setData(QVariant("project"), Qt::EditRole);
               item(row,   LAST_CHANGE)->setData(QVariant(), Qt::EditRole);
               item(row,     FILE_SIZE)->setData(QVariant(), Qt::EditRole);

               // Make uncheckable
               item(row, NAME)->setCheckable(false);
               item(row, NAME)->setData(QVariant(), Qt::CheckStateRole); // Hack to hide checkbox

               // Set Italic
               QFont itemFont = item(row, NAME)->font();
               itemFont.setItalic(true);
               item(row, NAME)->setFont(itemFont);

               // Make sure not editable
               Qt::ItemFlags currentFlags = item(row, NAME)->flags();
               currentFlags &= ~Qt::ItemIsEditable;
               item(row, NAME)->setFlags(currentFlags);

               // Refresh tooltips
               refreshToolTip(row);

            } else {
               if(!removeRow(row)){
                  qWarning() << "DrmListModel::slotOnWatchedDirectoryChange - ERROR 3 - Unable to remove row " << row;
               }
            }



         } else {
            previousFilePaths.insert(item(row, ABSOLUTE_PATH)->data(Qt::DisplayRole).toString().toUpper());
         }
      }
   }

   // 4 - Build a list of modified files for scheduled refresh
   // NOTE : does not seem to work but keep it for the moment
   //        functionality was replaced by watching files (see DrmListModel::slotOnWatchedFileChange)
   for(int row = rowCount() - 1; row >= 0; row--){
      if(item(row, TYPE)->data(Qt::DisplayRole).toString() == type){
         QFileInfo info(item(row, ABSOLUTE_PATH)->data(Qt::DisplayRole).toString());
         if(info.size() != item(row, FILE_SIZE)->data(Qt::DisplayRole).toLongLong() ||
               info.lastModified() != item(row, LAST_CHANGE)->data(Qt::DisplayRole).toDateTime()){

            m_filesBeingModified.insert(
                     info.absoluteFilePath(),
                     ModFileInfo(
                        info.absoluteFilePath(),
                        info.lastModified(),
                        info.size(),
                        type));
         }
      }
   }

   // 5 - Build a list of added files for scheduled refresh

   // Add new items to list Widget
   foreach(const QFileInfo info, currentFIL){
      if(!previousFilePaths.contains(info.absoluteFilePath().toUpper())){

         m_filesBeingAdded.insert(
                  info.absoluteFilePath(),
                  ModFileInfo(
                     info.absoluteFilePath(),
                     info.lastModified(),
                     info.size(),
                     type));

      }
   }

   // 6 - Start timer if required
   if(!m_filesBeingAdded.isEmpty() || !m_filesBeingModified.isEmpty()){
      if(!m_timer.isActive()){
         m_timer.start(500);
         //m_timer.start(1);
      }
   }
}


void DrmListModel::slotOnWatchedFileChange(const QString & path)
{
   //qDebug() << "DrmListModel::slotOnWatchedFileChange - DEBUG - " << path;

   QFileInfo info(path);

   if(!info.exists()){
      // File was deleted. Should be handled by folder watcher.
      // Simply stop watching this file
      m_fileSystemWatcher.removePath(path);
   } else {

      // Determine the row in order to determine the type
      int row;
      for(row = 0; row < rowCount(); row++){
         if(item(row, ABSOLUTE_PATH)->data(Qt::DisplayRole).toString().toUpper() == info.absoluteFilePath().toUpper()){
            break;
         }
      }

      if(row >= rowCount()){
         qWarning() << "DrmListModel::slotOnWatchedFileChange - ERROR 1 - Not found " << info.absoluteFilePath();
         return;
      }

      // Notify that file was changed
      m_filesBeingModified.insert(
               info.absoluteFilePath(),
               ModFileInfo(
                  info.absoluteFilePath(),
                  info.lastModified(),
                  info.size(),
                  item(row, TYPE)->data(Qt::DisplayRole).toString()));

   }

   // Start timer if required
   if(!m_filesBeingAdded.isEmpty() || !m_filesBeingModified.isEmpty()){
      if(!m_timer.isActive()){
         m_timer.start(500);
      }
   }
}

void DrmListModel::slotPeriodicFileCheck()
{
   // 1 - Monitor modified files
   QList<QString> removedModKeys;
   for(QMapIterator<QString, ModFileInfo> iter(m_filesBeingModified); iter.hasNext();){
      iter.next();
      QFileInfo info(iter.key());
      if(!info.exists()){
         qWarning() << "DrmListModel::slotPeriodicFileCheck - ERROR 1 - File disapeared " << iter.key();
         removedModKeys.append(iter.key());
      } else if(iter.value().lastModified == info.lastModified() &&
                iter.value().size         == info.size()){
         // try opening read/write to make sure there is not write handle on file
         QFile file(info.absoluteFilePath());
         if(file.open(QIODevice::ReadWrite)){
            file.close();
            if(refreshDrm(info)){
               removedModKeys.append(iter.key());
            } else {
               qWarning() << "DrmListModel::slotPeriodicFileCheck - ERROR 2 - Unable to refresh " << iter.key();
            }
         } else {
            qDebug() << "DrmListModel::slotPeriodicFileCheck - Refresh " << iter.key();
         }

      } else {
         qDebug() << "DrmListModel::slotPeriodicFileCheck - Refresh " << iter.key();
      }
   }

   // 2 - Monitor Added files
   QList<QString> removedAddKeys;
   for(QMapIterator<QString, ModFileInfo> iter(m_filesBeingAdded); iter.hasNext();){
      iter.next();
      QFileInfo info(iter.key());

      qDebug() << "   info.lastModified() = " << info.lastModified();
      qDebug() << "   info.size() = " << info.size();
      if(!info.exists()){
         qWarning() << "DrmListModel::slotPeriodicFileCheck - ERROR 3 - File disapeared " << iter.key();
         removedAddKeys.append(iter.key());
      } else if(iter.value().lastModified == info.lastModified() &&
                iter.value().size         == info.size()){

         // try opening read/write to make sure there is not write handle on file
         QFile file(info.absoluteFilePath());
         if(file.open(QIODevice::ReadWrite)){
            file.close();
            if(addDrmToList(info, iter.value().type)){
               removedAddKeys.append(iter.key());
            } else {
               qWarning() << "DrmListModel::slotPeriodicFileCheck - ERROR 4 - Unable to refresh " << iter.key();
            }
         } else {
            qDebug() << "DrmListModel::slotPeriodicFileCheck - Refresh " << iter.key();
         }


      } else {
         qDebug() << "DrmListModel::slotPeriodicFileCheck - Refresh " << iter.key();
      }
   }

   // 3 - Remove entries for which processing is over
   foreach(const QString &key, removedModKeys){
      m_filesBeingModified.remove(key);
   }
   foreach(const QString &key, removedAddKeys){
      m_filesBeingAdded.remove(key);
   }

   // 4 - refresh remaining file information
   foreach(const QString &key, m_filesBeingModified.keys()){
      QFileInfo info(key);
      ModFileInfo modFileInfo = m_filesBeingModified.value(key);
      modFileInfo.lastModified = info.lastModified();
      modFileInfo.size         = info.size();
      m_filesBeingModified.insert(key, modFileInfo);
   }

   foreach(const QString &key, m_filesBeingAdded.keys()){
      QFileInfo info(key);
      ModFileInfo modFileInfo = m_filesBeingAdded.value(key);
      modFileInfo.lastModified = info.lastModified();
      modFileInfo.size         = info.size();
      m_filesBeingAdded.insert(key, modFileInfo);
   }

   // 5 - Stop timer if nothing else to do
   if(m_filesBeingAdded.isEmpty() && m_filesBeingModified.isEmpty()){
      m_timer.stop();
   }
}

void DrmListModel::setBeatsModel(BeatsProjectModel *p_beatsModel)
{
   if(mp_beatsModel == p_beatsModel){
      return;
   }

   // Disconnect all signals and remove all previous project content

   if(mp_beatsModel){
      removeBeatsModelContent();
      // disconnect check state watcher
      disconnect(this, SIGNAL(itemChanged(QStandardItem*)), this, SLOT(slotOnItemChanged(QStandardItem*)));
   }

   mp_beatsModel = p_beatsModel;

   // connect all signals
   if(mp_beatsModel){
      addBeatsModelContent();

      // connect check state watcher
      connect(this, SIGNAL(itemChanged(QStandardItem*)), this, SLOT(slotOnItemChanged(QStandardItem*)));
   }
}

void DrmListModel::removeBeatsModelContent()
{
   // 1 - Remove all drumsets that are not in workspace + set all other data not checkable
   for(int row = rowCount() - 1; row >= 0; row--){
      if(item(row, ABSOLUTE_PATH)->data(Qt::DisplayRole).toString().isEmpty() ||
            item(row, TYPE)->data(Qt::DisplayRole).toString() == "project"){
         removeRow(row);
      } else {
         // Note: setCheckable must be set false first in order not to remove from project upon closing it
         item(row, NAME)->setCheckable(false);
         item(row, NAME)->setCheckState(Qt::Unchecked);

         // From http://badlyhonedbytes.wordpress.com/2013/03/31/hiding-the-checkbox-of-a-qlistwidgetitem/
         item(row, NAME)->setData(QVariant(), Qt::CheckStateRole); // Workaround to hide checkbox

         item(row, IN_PROJECT)->setData(QVariant(), Qt::EditRole);

         // Refresh tooltips
         refreshToolTip(row);
      }
   }
}

void DrmListModel::addBeatsModelContent()
{
   // Names are supposed to be unique

   // 1 - Build a map of Names to row + make all items checkable
   bool duplicateNames = false;
   QMap<QString, int> nameToRowMap;
   for(int row = 0; row < rowCount(); row++){
      QString name = item(row, NAME)->data(Qt::DisplayRole).toString();

      if(nameToRowMap.contains(name)){
         duplicateNames = true;
      }
      nameToRowMap.insert(name, row);
      item(row, NAME)->setCheckable(true);
      item(row, IN_PROJECT)->setData(QVariant(false), Qt::EditRole);
   }

   // 2 - Loop on all Beats Model drm and determine whether needs to be created or checked
   if(!duplicateNames){
      // Faster method
      for(int beatsRow = 0; beatsRow < mp_beatsModel->rowCount(mp_beatsModel->drmFolderIndex()); beatsRow++){
         QByteArray beatsDrmCRC = mp_beatsModel->index(beatsRow, AbstractTreeItem::HASH, mp_beatsModel->drmFolderIndex()).data().toByteArray();
         QString beatsDrmName = mp_beatsModel->index(beatsRow, AbstractTreeItem::NAME, mp_beatsModel->drmFolderIndex()).data().toString();

         int row = nameToRowMap.value(beatsDrmName, -1);

         if(row >= 0 && item(row, CRC)->data(Qt::DisplayRole).toByteArray() == beatsDrmCRC){
            item(row, NAME)->setCheckState(Qt::Checked);
            item(row, IN_PROJECT)->setData(QVariant(true), Qt::EditRole);
         } else {
            addBeatsModelDrmToList(beatsRow);
         }
      }
   } else {

      // Slower method, but works even if duplicates
      for(int beatsRow = 0; beatsRow < mp_beatsModel->rowCount(mp_beatsModel->drmFolderIndex()); beatsRow++){
         QByteArray beatsDrmCRC = mp_beatsModel->index(beatsRow, AbstractTreeItem::HASH, mp_beatsModel->drmFolderIndex()).data().toByteArray();
         QString beatsDrmName = mp_beatsModel->index(beatsRow, AbstractTreeItem::NAME, mp_beatsModel->drmFolderIndex()).data().toString();


         // verify if can be found in project
         int listRow;
         for(listRow = 0; listRow < rowCount(); listRow++){
            if(item(listRow, CRC)->data(Qt::DisplayRole).toByteArray() == beatsDrmCRC &&
                  item(listRow, NAME)->data(Qt::DisplayRole).toString() == beatsDrmName){
               break;
            }
         }

         // if found
         if(listRow < rowCount()){
            item(listRow, NAME)->setCheckState(Qt::Checked);
            item(listRow, IN_PROJECT)->setData(QVariant(true), Qt::EditRole);
         } else {
            addBeatsModelDrmToList(beatsRow);
         }
      }
   }

   // 3 - Refresh all tooltips
   refreshAllToolTips();


}


void DrmListModel::slotOnItemChanged(QStandardItem * item)
{
   if(item->column() == NAME){
      if(item->isCheckable() &&
            item->checkState() == Qt::Unchecked &&
            this->item(item->row(),IN_PROJECT)->data(Qt::DisplayRole).toBool()){

         // Add to project
         if(mp_beatsModel->drmFolder()->removeDrmModal(parentWidget(), item->data(Qt::DisplayRole).toString())){
            // Success
            this->item(item->row(),IN_PROJECT)->setData(QVariant(false), Qt::EditRole);

            // Refresh tooltips
            refreshToolTip(item->row());
         } else {
            item->setCheckState(Qt::Checked);
         }
      }

      if(item->isCheckable() &&
            item->checkState() == Qt::Checked &&
            !this->item(item->row(),IN_PROJECT)->data(Qt::DisplayRole).toBool()){
         // Remove from project
         if(mp_beatsModel->drmFolder()->addDrmModal(parentWidget(), this->item(item->row(), ABSOLUTE_PATH)->data(Qt::DisplayRole).toString())){
            // Success
            this->item(item->row(),IN_PROJECT)->setData(QVariant(true), Qt::EditRole);

            // Refresh tooltips
            refreshToolTip(item->row());
         } else {
            item->setCheckState(Qt::Unchecked);
         }
      }
   }
}



QString DrmListModel::importDrm(const QString &srcPath, bool forceCopy)
{
   // 1 - Verify that file exists
   QFileInfo srcFI(srcPath);
   if(!srcFI.exists()){
      QMessageBox::critical(parentWidget(), tr("Import Drumset"), tr("Source file %1 does not exist").arg(srcPath));
      return nullptr;
   }

   // 2 - Extract CRC and file name
   QString srcName = DrmMakerModel::getDrumsetNameStatic(srcPath);
   QByteArray srcCRC = DrmMakerModel::getCRCStatic(srcPath);

   if(srcName.isEmpty() || srcCRC.isEmpty()){
      QMessageBox::critical(parentWidget(), tr("Import Drumset"), tr("Unable to read source file %1").arg(srcPath));
      return nullptr;
   }


   // 3 - Verify if drm name exists + build a set of all drm names + build a set of all drm file names
   bool nameExists = false;
   bool crcExists = false;
   bool prjOnly = false;

   QSet<QString> drmNames;
   QSet<QString> fileNames;

   for(int row = 0; row < rowCount(); row++){
      if(item(row, NAME)->data(Qt::DisplayRole).toString() == srcName){
         nameExists = true;
         if(item(row, CRC)->data(Qt::DisplayRole).toByteArray() == srcCRC){
            crcExists = true;
            if(item(row, TYPE)->data(Qt::DisplayRole).toString() == "project"){
               prjOnly = true;
            }
         }
      }
      drmNames.insert(item(row, NAME)->data(Qt::DisplayRole).toString().toUpper());
      if(item(row, TYPE)->data(Qt::DisplayRole).toString() == "user"){
         fileNames.insert(item(row, FILE_NAME)->data(Qt::DisplayRole).toString().toUpper());
      }

   }

   QString newName;

   // 4 - Resolve duplicate name
   int duplicates;

   if(nameExists && crcExists && !prjOnly && !forceCopy){

      // Note: if nameExists && crcExists && prjOnly, need to copy with same name and CRC

      QMessageBox::warning(parentWidget(), tr("Import Drumset"), tr("Drumset '%1' is already in the project with identical content. Nothing to do here!").arg(srcName));
      return nullptr;

   } else if(nameExists && !prjOnly) {
      // resolve name
      duplicates = 1;
      newName = QString("%1(%2)").arg(srcName).arg(duplicates);
      while(drmNames.contains(newName.toUpper())){
         newName = QString("%1(%2)").arg(srcName).arg(++duplicates);
      }
   }

   // 5 - Resolve duplicate file names
   QString dstFileName = srcFI.fileName();
   duplicates = 1;
   while(fileNames.contains(dstFileName.toUpper())){
      dstFileName = QString("%1(%2).%3").arg(srcFI.baseName()).arg(duplicates++).arg(srcFI.completeSuffix());
   }

   // 6 - Perform copy
   QDir dstDir(mp_workspace->userLibrary()->libDrumSets()->defaultPath());
   if(newName.isEmpty()){
      // perform a simple copy
      QFile srcFile(srcFI.absoluteFilePath());
      if(!srcFile.copy(dstDir.absoluteFilePath(dstFileName))){
         return nullptr;
      }

   } else {
      // Change drumset name during copy
      if(!DrmMakerModel::copyDrmNewName(srcFI.absoluteFilePath(), dstDir.absoluteFilePath(dstFileName), newName )){
         return nullptr;
      }
   }

   return dstDir.absoluteFilePath(dstFileName);
}


QString DrmListModel::importDrmFromPrj(const QString &longName)
{
   if(!mp_beatsModel){
      qWarning() << "DrmListModel::importDrmFromPrj - ERROR 1 - no project opened";
      return nullptr;
   }

   // 1 - Determine source path
   QString srcPath = mp_beatsModel->drmFolder()->pathForLongName(longName);
   if(srcPath.isEmpty()){
      qWarning() << "DrmListModel::importDrmFromPrj - ERROR 2 - srcPath.isEmpty()";
      return nullptr;
   }

   // 2 - verify that file exists
   QFileInfo srcFI(srcPath);
   if(!srcFI.exists()){
      qWarning() << "DrmListModel::importDrmFromPrj - ERROR 1 - !srcFI.exists()";
      return nullptr;
   }

   // 3 - Build a set of all drm file names in user dir
   QSet<QString> fileNames;
   for(int row = 0; row < rowCount(); row++){
      if(item(row, TYPE)->data(Qt::DisplayRole).toString() == "user"){
         fileNames.insert(item(row, FILE_NAME)->data(Qt::DisplayRole).toString().toUpper());
      }
   }

   // 4 - Resolve duplicate file names
   QString dstFileName = QString("%1.%2").arg(longName, srcFI.completeSuffix());
   int duplicates = 1;
   while(fileNames.contains(dstFileName.toUpper())){
      dstFileName = QString("%1(%2).%3").arg(longName).arg(duplicates++).arg(srcFI.completeSuffix());
   }

   // 5 - Perform copy (simple copy since drmName not changed)
   QDir dstDir(mp_workspace->userLibrary()->libDrumSets()->defaultPath());

   QFile srcFile(srcFI.absoluteFilePath());
   if(!srcFile.copy(dstDir.absoluteFilePath(dstFileName))){
      return nullptr;
   }

   return dstDir.absoluteFilePath(dstFileName);
}



bool DrmListModel::resloveDuplicateName(int row)
{
   qDebug() << "DrmListModel::resloveDuplicateName " << row;

   QString originalName = item(row, NAME)->data(Qt::DisplayRole).toString();

   // 1 - verify if the name for row exists somewhere else + build a list of all names
   bool duplicateFound = false;

   QSet<QString> drmNames;
   for(int i = 0; i < rowCount(); i++){
      if(i != row){
         drmNames.insert(item(i, NAME)->data(Qt::DisplayRole).toString().toUpper());
         if(item(i, NAME)->data(Qt::DisplayRole).toString().toUpper() == originalName.toUpper()){

            // Note: should have already verified if there in project

            duplicateFound = true;
         }
      }
   }

   qDebug() << "DrmListModel::resloveDuplicateName - DEBUG - " << duplicateFound;
   if(!duplicateFound){
      // no duplicates to manage
      return true;
   }


   // 2 - Resolve duplicate name
   int duplicates = 1;

   QString newName = QString("%1(%2)").arg(originalName).arg(duplicates);
   while(drmNames.contains(newName.toUpper())){
      newName = QString("%1(%2)").arg(originalName).arg(++duplicates);
   }

   qDebug() << "DrmListModel::resloveDuplicateName - DEBUG - " << newName;

   // 3 - Modify the file in order to set the proper name
   QByteArray crc = DrmMakerModel::renameDrm(item(row, ABSOLUTE_PATH)->data(Qt::DisplayRole).toString(), newName);

   if(crc.isEmpty()){
      qWarning() << "DrmListModel::resloveDuplicateName - ERROR 1 - renameDrm returned empty CRC";
      return false;
   }

   // 4 - Rename in model
   item(row, NAME)->setData(QVariant(newName), Qt::EditRole);

   // 5 - Refresh File info
   return refreshDrm(row, newName, crc);

}

void DrmListModel::slotOnDrmFileOpened(const QString &path)
{
   // Path may be empty (if creating new file by example)
   if(path.isEmpty()){
      return;
   }

   // verify if file exists
   QFileInfo drmFI(path);

   if(!drmFI.exists()){
      qWarning() << "DrmListModel::slotOnDrmFileOpened - ERROR 1 - !drmFI.exists()";
   }

   // verify if file in list
   int row;
   for(row = 0; row < rowCount(); row++){
      if(item(row, ABSOLUTE_PATH)->data(Qt::DisplayRole).toString().toUpper() == drmFI.absoluteFilePath().toUpper()){
         break;
      }
   }

   if(row >= rowCount()){
      qDebug() << "DrmListModel::slotOnDrmFileOpened - not in list";
      return;
   }

   // NOTE: we might need to make sure no other DRM is opened

   // set Bold Italic
   QFont currentFont = item(row, NAME)->font();
   currentFont.setWeight(QFont::Black);
   currentFont.setItalic(true);
   item(row, NAME)->setFont(currentFont);

   // set Opened
   item(row, OPENED)->setData(QVariant(true), Qt::EditRole);

   // Make sure not editable
   Qt::ItemFlags currentFlags = item(row, NAME)->flags();
   currentFlags &= ~Qt::ItemIsEditable;
   item(row, NAME)->setFlags(currentFlags);

   // Refresh tooltips
   refreshToolTip(row);

}

void DrmListModel::slotOnDrmFileClosed(const QString &path)
{
   // Path may be empty (if closing new file by example)
   if(path.isEmpty()){
      return;
   }

   // verify if file exists
   QFileInfo drmFI(path);

   if(!drmFI.exists()){
      qWarning() << "DrmListModel::slotOnDrmFileOpened - ERROR 1 - !drmFI.exists()";
   }

   // verify if file in list
   int row;
   for(row = 0; row < rowCount(); row++){
      if(item(row, ABSOLUTE_PATH)->data(Qt::DisplayRole).toString().toUpper() == drmFI.absoluteFilePath().toUpper()){
         break;
      }
   }

   if(row >= rowCount()){
      qDebug() << "DrmListModel::slotOnDrmFileOpened - not in list";
      return;
   }

   // clear Opened
   item(row, OPENED)->setData(QVariant(false), Qt::EditRole);

   if(item(row, TYPE)->data(Qt::DisplayRole).toString() == "user"){
      // set Normal Weight/Style
      QFont currentFont = item(row, NAME)->font();
      currentFont.setWeight(QFont::Normal);
      currentFont.setItalic(false);
      item(row, NAME)->setFont(currentFont);

      // Make sure editable
      Qt::ItemFlags currentFlags = item(row, NAME)->flags();
      currentFlags |= Qt::ItemIsEditable;
      item(row, NAME)->setFlags(currentFlags);

   } else if(item(row, TYPE)->data(Qt::DisplayRole).toString() == "default"){
      // set Normal Weight/Style
      QFont currentFont = item(row, NAME)->font();
      currentFont.setWeight(QFont::DemiBold);
      currentFont.setItalic(false);
      item(row, NAME)->setFont(currentFont);

      // Make sure not editable
      Qt::ItemFlags currentFlags = item(row, NAME)->flags();
      currentFlags &= ~Qt::ItemIsEditable;
      item(row, NAME)->setFlags(currentFlags);

   } else if(item(row, TYPE)->data(Qt::DisplayRole).toString() == "project"){
      // set Normal Italic
      QFont currentFont = item(row, NAME)->font();
      currentFont.setWeight(QFont::Normal);
      currentFont.setItalic(true);
      item(row, NAME)->setFont(currentFont);

      // Make sure not editable
      Qt::ItemFlags currentFlags = item(row, NAME)->flags();
      currentFlags &= ~Qt::ItemIsEditable;
      item(row, NAME)->setFlags(currentFlags);
   } else {
      qWarning() << "DrmListModel::slotOnDrmFileClosed - ERROR 2 - Unknown type " << item(row, TYPE)->data(Qt::DisplayRole).toString();
   }
}


/**
 * @brief DrmListModel::deleteDrmModal
 * @param index
 *
 * Function that deletes a drumset. Modal because it will request confirmation, display errors,
 * and progress bar (through mp_beatsModel->drmFolder()->removeDrmModal() call)
 */
void DrmListModel::deleteDrmModal(const QModelIndex &index)
{
   // Determine if default: cannot delete if default
   if(index.sibling(index.row(), DrmListModel::TYPE).data().toString() == "default"){
      QMessageBox::warning(parentWidget(), tr("Delete Drumset"), tr("Selected Drumset is a built-in drumset.\nIt cannot be deleted"));
      return;
   }
   // Determine if opened: cannot delete if opened
   if(index.sibling(index.row(), DrmListModel::OPENED).data().toBool()){
      QMessageBox::critical(parentWidget(), tr("Delete Drumset"), tr("Cannot delete an opened drumset.\nClose the drumset before continuing"));
      return;
   }
   // Determine if user or project and perform specific operations
   if(index.sibling(index.row(), DrmListModel::TYPE).data().toString() == "user"){
      // if user, simply delete file and let the background methods do the rest
      QFile drmFile(index.sibling(index.row(), DrmListModel::ABSOLUTE_PATH).data().toString());
      // Prompt to to confirm deletion if no shift is being held
      if (!(QApplication::keyboardModifiers() & Qt::ShiftModifier) && QMessageBox::Yes != QMessageBox::question(parentWidget(), tr("Delete Drumset"), tr("Are you sure you want to delete drumset from the workspace?"))){
         return;
      }
      if(!drmFile.remove()){
         QMessageBox::critical(parentWidget(), tr("Delete Drumset"), tr("Unable to delete drumset file.\nMake sure it is not being used somewhere else"));
         return;
      }
      if(!index.sibling(index.row(), DrmListModel::IN_PROJECT).data().toBool()){
         // If not in project, operation completed
         return;
      }
      // Prompt to ask if needs to be removed from project as well if no shift is being held
      if(!(QApplication::keyboardModifiers() & Qt::ShiftModifier) && QMessageBox::Yes != QMessageBox::question(parentWidget(), tr("Delete Drumset"), tr("Do you also want to also delete drumset from current project?"))){
         return;
      }
      if(mp_beatsModel->drmFolder()->removeDrmModal(parentWidget(), index.data(Qt::DisplayRole).toString())){
         if(!removeRow(index.row())){
            qWarning() << "DrmListModel::deleteDrmModal - ERROR 1 - Unable to remove row " << index.row();
         }
      } else {
         qWarning() << "DrmListModel::deleteDrmModal - ERROR 2 - Unable to delete drm " << index.data(Qt::DisplayRole).toString() << " from beatsModel";
      }

   } else if (index.sibling(index.row(), DrmListModel::TYPE).data().toString() == "project") {
      // Prompt to confirm deletion if no shift is being held
      if (!(QApplication::keyboardModifiers() & Qt::ShiftModifier) && QMessageBox::Yes != QMessageBox::question(parentWidget(), tr("Delete Drumset"), tr("Are you sure you want to delete drumset from the project?"))){
         return;
      }

      if(mp_beatsModel->drmFolder()->removeDrmModal(parentWidget(), index.data(Qt::DisplayRole).toString())){
         if(!removeRow(index.row())){
            qWarning() << "DrmListModel::deleteDrmModal - ERROR 3 - Unable to remove row " << index.row();
         }
      } else {
         qWarning() << "DrmListModel::deleteDrmModal - ERROR 4 - Unable to delete drm " << index.data(Qt::DisplayRole).toString() << " from beatsModel";
      }

   } else {
      qWarning() << "DrmListModel::deleteDrmModal - ERROR 5 - Unknown type";
   }
}

void DrmListModel::refreshToolTip(int row)
{

   QString toolTipStr;

   // Type
   if(item(row, TYPE)->data(Qt::DisplayRole).toString() == "project"){
      toolTipStr = tr("Drumset local to project");
   } else if(item(row, TYPE)->data(Qt::DisplayRole).toString() == "default"){
      toolTipStr = tr("Default Library Drumset");
   } else {
      toolTipStr = tr("User Library Drumset");
   }

   // Opened Status
   if(item(row, OPENED)->data(Qt::DisplayRole).toBool()){
      toolTipStr += tr(", Currently Opened");
   }

   // Single Click operation
   if(item(row, TYPE)->data(Qt::DisplayRole).toString() == "user" &&
         !item(row, OPENED)->data(Qt::DisplayRole).toBool()){
      
      toolTipStr += tr("\nSingle click on selected to rename");
   }

   // Double click operation
   if(item(row, OPENED)->data(Qt::DisplayRole).toBool()){
      toolTipStr += tr("\nDouble Click to re-open");
   } else if(item(row, TYPE)->data(Qt::DisplayRole).toString() == "project"){
      toolTipStr += tr("\nDouble click to copy and open");
   } else if(item(row, TYPE)->data(Qt::DisplayRole).toString() == "default"){
      toolTipStr += tr("\nDouble click to copy and open");
   } else {
      toolTipStr += tr("\nDouble click to open");
   }

   // Checkbox operation
   if(mp_beatsModel){
      if(item(row, IN_PROJECT)->data(Qt::DisplayRole).toBool()){
         toolTipStr += tr("\nUncheck to remove from project");
      } else {
         toolTipStr += tr("\nCheck to add to project");
      }
   }

   item(row, NAME)->setToolTip(toolTipStr);
}

void DrmListModel::refreshAllToolTips()
{
   for(int row = 0; row < rowCount(); row++){
      refreshToolTip(row);
   }
}
