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
#include "songfolderviewitem.h"
#include "../workspace/workspace.h"
#include "../model/tree/abstracttreeitem.h"
#include "../model/tree/project/beatsprojectmodel.h"

SongFolderViewItem::SongFolderViewItem(BeatsProjectModel* p_Model, QWidget* parent)
    : QWidget(parent)
{
   // Initialize data
    mp_ChildrenItems = new QList<SongFolderViewItem*>; // Deleted in destructor
    m_CurrentIndex = false;
    mp_Model = p_Model;
}

SongFolderViewItem::~SongFolderViewItem()
{
    delete mp_ChildrenItems;
}


QRect SongFolderViewItem::visualRect(const QModelIndexList &modelIndexPath, int currentDepth) const
{
    // If invalid, return empty rect at parent's origin
    if (!modelIndexPath.at(currentDepth).isValid()){
        return QRect();
    }
    //As a simplification, always return the full song rectangle
    return geometry();
}

SongFolderViewItem * SongFolderViewItem::itemForIndex(const QModelIndex &index) const
{
    if (!index.isValid()){
        qWarning() << "SongFolderViewItem::itemForIndex - INVALID";
        return nullptr;
    }

    // Normally, row index should be the same as list index
    if( (index.row() < mp_ChildrenItems->size()) && (mp_ChildrenItems->at(index.row())->modelIndex().internalId() == index.internalId()) ){
        return mp_ChildrenItems->at(index.row());
    }

    // Search at other indexes in list
    for (int i = 0; i < mp_ChildrenItems->size(); i++){
        if(mp_ChildrenItems->at(i)->modelIndex().internalId() == index.internalId()){
            qWarning() << "SongFolderViewItem::itemForIndex - index not where expected";
            return mp_ChildrenItems->at(i);
        }
    }

    // Not found
    qWarning() << "SongFolderViewItem::itemForIndex (row = "<< index.row() << ") - NOT FOUND - " << index.data().toString();
    return nullptr;
}

QModelIndex SongFolderViewItem::indexAt(const QPoint &point){
   QPoint myCoordPoint = point - geometry().topLeft();

   // Find recursively the deepest item for that the point
   for(int i = 0; i < mp_ChildrenItems->size(); i++){
      if(mp_ChildrenItems->at(i)->geometry().contains(myCoordPoint)){
         return mp_ChildrenItems->at(i)->indexAt(myCoordPoint);
      }
   }

   // if not found in children, confirm that self contains the point
   if (geometry().contains(point)){
      return modelIndex();
   }

   // Otherwise, return dummy index
   return QModelIndex();
}

void SongFolderViewItem::childrenDataChanged(const QModelIndexList &modelIndexPath, const QModelIndex &topLeft, const QModelIndex &bottomRight, int currentDepth)
{
   // Validate current index. If invalid, return immediately
   if (!modelIndexPath.at(currentDepth).isValid()){
      qWarning() << "SongFolderViewItem::childrenDataChanged - ERROR 1 - (!modelIndexPath.at(currentDepth).isValid())";
      return;
   }

   const QAbstractItemModel *p_model = modelIndex().model();

   // If the item at current depth is the last of the list, update all children
   SongFolderViewItem *p_ChildView = nullptr;
   QModelIndex left;
   QModelIndex right;
   if (modelIndexPath.size() <= currentDepth + 1){
      // Modify child items comprized between topLeft and bottomRight
      for(int row = topLeft.row(); row <= bottomRight.row(); row++){
         p_ChildView = itemForIndex(p_model->index(row, 0, modelIndex())); // NOTE: Widgets are associated to column 0
         if (p_ChildView != nullptr){
            left = p_model->index(row, topLeft.column(), modelIndex());
            right = p_model->index(row, bottomRight.column(), modelIndex());
            p_ChildView->dataChanged(left, right);
         }
      }
   // Go deeper in hierarchy
   } else {
      p_ChildView = itemForIndex(modelIndexPath.at(currentDepth));
      if (p_ChildView != nullptr){
         p_ChildView->childrenDataChanged(modelIndexPath, topLeft, bottomRight, currentDepth + 1);
      }
   }
}

void SongFolderViewItem::childrenRowsInserted(const QModelIndexList &modelIndexPath, int start, int end, int currentDepth)
{
   // Validate current index. If invalid, return immediately
   if (!modelIndexPath.at(currentDepth).isValid()){
      qWarning() << "SongFolderViewItem::childrenRowsInserted - (!modelIndexPath.at(currentDepth).isValid())";
      return;
   }

   // If the item at current depth is the last of the list, update all children
   if (modelIndexPath.size() <= currentDepth + 1){
      // Modify child items comprized between topLeft and bottomRight
      rowsInserted(start, end);
   // Go deeper in hierarchy
   } else {
      SongFolderViewItem *p_ChildView = itemForIndex(modelIndexPath.at(currentDepth + 1));
      if (p_ChildView == nullptr){
         qWarning() << "SongFolderViewItem::childrenRowsInserted - (p_ChildView == 0) in " << metaObject()->className();
         return;
      }
      p_ChildView->childrenRowsInserted(modelIndexPath, start, end, currentDepth + 1);
   }
}


void SongFolderViewItem::childrenRowsRemoved(const QModelIndexList &modelIndexPath, int start, int end, int currentDepth)
{
   // Validate current index. If invalid, return immediately
   if (!modelIndexPath.at(currentDepth).isValid()){
      qWarning() << "SongFolderViewItem::childrenRowsRemoved - (!modelIndexPath.at(currentDepth).isValid())";
      return;
   }

   // If the item at current depth is the last of the list, update all children
   if (modelIndexPath.size() <= currentDepth + 1){
      // Modify child items comprized between topLeft and bottomRight
      rowsRemoved(start, end);
   // Go deeper in hierarchy
   } else {
      SongFolderViewItem *p_ChildView = itemForIndex(modelIndexPath.at(currentDepth + 1));
      if (p_ChildView == nullptr){
         qWarning() << "SongFolderViewItem::childrenRowsInserted - (p_ChildView == 0) in " << metaObject()->className();
         return;
      }
      p_ChildView->childrenRowsRemoved(modelIndexPath, start, end, currentDepth + 1);
   }
}

void SongFolderViewItem::childrenRowsMovedGet(const QModelIndexList &modelIndexPath, int start, int end, QList<SongFolderViewItem *> *p_List, int currentDepth)
{
   // Validate current index. If invalid, return immediately
   if (!modelIndexPath.at(currentDepth).isValid()){
      qWarning() << "SongFolderViewItem::childrenRowsMovedGet - (!modelIndexPath.at(currentDepth).isValid())";
      return;
   }

   // If the item at current depth is the last of the list, update all children
   if (modelIndexPath.size() <= currentDepth + 1){
      // Modify child items comprized between topLeft and bottomRight
      rowsMovedGet(start, end, p_List);
   // Go deeper in hierarchy
   } else {
      SongFolderViewItem *p_ChildView = itemForIndex(modelIndexPath.at(currentDepth + 1));
      if (p_ChildView == nullptr){
         qWarning() << "SongFolderViewItem::childrenRowsMovedGet - (p_ChildView == 0)";
         return;
      }
      p_ChildView->childrenRowsMovedGet(modelIndexPath, start, end, p_List, currentDepth + 1);
   }
}

void SongFolderViewItem::childrenRowsMovedInsert(const QModelIndexList &modelIndexPath, int start, QList<SongFolderViewItem *> *p_List, int currentDepth)
{
   // Validate current index. If invalid, return immediately
   if (!modelIndexPath.at(currentDepth).isValid()){
      qWarning() << "SongFolderViewItem::childrenRowsMovedInsert - (!modelIndexPath.at(currentDepth).isValid())";
      return;
   }

   // If the item at current depth is the last of the list, update all children
   if (modelIndexPath.size() <= currentDepth + 1){
      // Modify child items comprized between topLeft and bottomRight
      rowsMovedInsert(start, p_List);
   // Go deeper in hierarchy
   } else {
      SongFolderViewItem *p_ChildView = itemForIndex(modelIndexPath.at(currentDepth + 1));
      if (p_ChildView == nullptr){
         qWarning() << "SongFolderViewItem::childrenRowsMovedInsert - (p_ChildView == 0)";
         return;
      }
      p_ChildView->childrenRowsMovedInsert(modelIndexPath, start, p_List, currentDepth + 1);
   }

}

void SongFolderViewItem::childrenRowsMovedRemove(const QModelIndexList &modelIndexPath, int start, int end, int currentDepth)
{
   // Validate current index. If invalid, return immediately
   if (!modelIndexPath.at(currentDepth).isValid()){
      qWarning() << "SongFolderViewItem::childrenRowsMovedRemove - (!modelIndexPath.at(currentDepth).isValid())";
      return;
   }

   // If the item at current depth is the last of the list, update all children
   if (modelIndexPath.size() <= currentDepth + 1){
      // Modify child items comprized between topLeft and bottomRight
      rowsMovedRemove(start, end);
   // Go deeper in hierarchy
   } else {
      SongFolderViewItem *p_ChildView = itemForIndex(modelIndexPath.at(currentDepth + 1));
      if (p_ChildView == nullptr){
         qWarning() << "SongFolderViewItem::childrenRowsMovedRemove - (p_ChildView == 0)";
         return;
      }
      p_ChildView->childrenRowsMovedRemove(modelIndexPath, start, end, currentDepth + 1);
   }

}

QModelIndex SongFolderViewItem::modelIndex()
{
   // Transform QPersistentModelIndex in QModelIndex
   return m_ModelIndex.model()->index(m_ModelIndex.row(), m_ModelIndex.column(), m_ModelIndex.parent());
}

void SongFolderViewItem::setModelIndex(const QModelIndex & modelIndex)
{
   m_ModelIndex = QPersistentModelIndex(modelIndex);
}

void SongFolderViewItem::setSelected(bool selected)
{
   m_CurrentIndex = selected;
}

void SongFolderViewItem::setCurrentIndex(bool selected)
{
   m_CurrentIndex = selected;
}

void SongFolderViewItem::slotSubWidgetClicked()
{
   emit sigSubWidgetClicked(modelIndex());
}

void SongFolderViewItem::slotSubWidgetClicked(const QModelIndex & /*index*/)
{
   emit sigSubWidgetClicked(modelIndex());
}

void SongFolderViewItem::slotSetPlayerEnabled(bool enabled)
{
   for(int i = 0; i < mp_ChildrenItems->size(); i++){
      mp_ChildrenItems->at(i)->slotSetPlayerEnabled(enabled);
   }
}


/*
 * utility functions
 */


QDir SongFolderViewItem::copyClipboardDir() {
    return static_cast<BeatsProjectModel *>(model())->copyClipboardDir();
}
QDir SongFolderViewItem::pasteClipboardDir() {
    return static_cast<BeatsProjectModel *>(model())->pasteClipboardDir();
}
QDir SongFolderViewItem::dragClipboardDir() {
    return static_cast<BeatsProjectModel *>(model())->dragClipboardDir();
}
QDir SongFolderViewItem::dropClipboardDir() {
    return static_cast<BeatsProjectModel *>(model())->dropClipboardDir();
}

/*
 * VIRTUAL methods default implementation
 */

void SongFolderViewItem::rowsInserted(int /*start*/, int /*end*/)
{qWarning() << metaObject()->className() << "::rowsInserted - NOT IMPLEMENTED";}
void SongFolderViewItem::rowsRemoved(int /*start*/, int /*end*/)
{qWarning() << metaObject()->className() << "::rowsRemoved - NOT IMPLEMENTED";}
void SongFolderViewItem::rowsMovedGet(int /*start*/, int /*end*/, QList<SongFolderViewItem *> * /*p_List*/)
{qWarning() << metaObject()->className() << "::rowsMovedGet - NOT IMPLEMENTED";}
void SongFolderViewItem::rowsMovedInsert(int /*start*/, QList<SongFolderViewItem *> * /*p_List*/)
{qWarning() << metaObject()->className() << "::rowsMovedInsert - NOT IMPLEMENTED";}
void SongFolderViewItem::rowsMovedRemove(int /*start*/, int /*end*/){qWarning() << metaObject()->className() << "::rowsMovedRemove - NOT IMPLEMENTED";}
