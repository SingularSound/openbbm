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
#include <QScrollBar>
#include <QWheelEvent>
#include <QApplication>
#include <QDebug>
#include <QMessageBox>
#include <QBoxLayout>
#include <QFileDialog>
#include <QDesktopServices>

#include "songfolderview.h"
#include "songwidget.h"
#include "newsongwidget.h"

#include <model/tree/abstracttreeitem.h>
#include <model/tree/song/songfileitem.h>
#include <model/tree/song/songfoldertreeitem.h>
#include <model/beatsmodelfiles.h>

#include <workspace/workspace.h>
#include <workspace/libcontent.h>
#include <workspace/contentlibrary.h>

SongFolderView::SongFolderView(QWidget *parent) :
    QAbstractItemView(parent),
    mp_Header(nullptr)
{
    mp_ChildrenItems = new QList<SongFolderViewItem*>;
    mp_NewSongWidgets = new QList<NewSongWidget*>;

    horizontalScrollBar()->setRange(0, 0);
    verticalScrollBar()->setRange(0, 0);
    m_ContentSize = QSize(0,0);

    // Disable any default edit trigger. Editing is performed by widgets
    setEditTriggers(QAbstractItemView::NoEditTriggers);
    setSelectionMode(QAbstractItemView::SingleSelection);
    setSelectionBehavior(QAbstractItemView::SelectItems);
}

SongFolderView::~SongFolderView()
{
    delete mp_ChildrenItems;
}

// returns empty list if index is invalid.
QModelIndexList SongFolderView::pathToIndex(const QModelIndex &index ) const
{
    QModelIndexList path;

    QModelIndex currentIndex = index;

    while (currentIndex.isValid() && currentIndex != rootIndex()){
        path.insert(0, currentIndex);
        currentIndex = currentIndex.parent();
    }

    return path;
}

QRect SongFolderView::visualRect( const QModelIndex &index ) const
{
    // NOTE: as a simplification, always returns the full song rectangle

    // Determine the QModelIndex from rootIndex to index
    QModelIndexList modelIndexPath = pathToIndex(index);

    // Invalid index or not found, return invalid visualRect
    if (modelIndexPath.size() <= 0){
        return QRect();
    }

   // Stop immediately if modified item is not being displayed
    if(!rootIndex().isValid() ||
            !modelIndexPath.first().parent().isValid() ||
            (modelIndexPath.first().parent().internalPointer() != rootIndex().internalPointer())){
        return QRect();
    }

    const SongFolderViewItem *p_SongFolderViewItem = itemForIndex(modelIndexPath.at(0));
    if (p_SongFolderViewItem == nullptr){
        return QRect();
    }

    // starting from here, the processing is performed recursively in children
    return p_SongFolderViewItem->visualRect(modelIndexPath, 0);
}


SongFolderViewItem * SongFolderView::itemForIndex(const QModelIndex &index) const
{
    if (!index.isValid()){
        return nullptr;
    }

    // Normally, row index should be the same as list index
    if( (index.row() < mp_ChildrenItems->size()) && (mp_ChildrenItems->at(index.row())->modelIndex().internalId() == index.internalId()) ){
        return mp_ChildrenItems->at(index.row());
    }

    // Search at other indexes in list
    for (int i = 0; i < mp_ChildrenItems->size(); i++){
        if(mp_ChildrenItems->at(i)->modelIndex().internalId() == index.internalId()){
            qWarning() << "SongFolderView::itemForIndex - index not where expected";
            return mp_ChildrenItems->at(i);
        }
    }

    // Not found
    qWarning() << "SongFolderView::itemForIndex (row = "<< index.row() << ") - NOT FOUND - " << index.data().toString();
    return nullptr;
}

int SongFolderView::horizontalOffset() const
{
   return horizontalScrollBar()->value();
}

int SongFolderView::verticalOffset() const
{
   return verticalScrollBar()->value();
}

QModelIndex SongFolderView::moveCursor( CursorAction cursorAction,Qt::KeyboardModifiers /*modifiers*/ )
{
   QModelIndex current = currentIndex();

   switch (cursorAction) {
      case MoveLeft:
         if (current.parent() != rootIndex()){
            current = current.parent();
         }
         break;
      case MoveRight:
         // Only allow 1 level deep for now
         if (current.parent() == rootIndex()){
            current = current.child(0,0);
         }
         break;
      case MoveUp:
         if (current.row() > 0){
            current = model()->index(current.row() - 1, 0, current.parent());
         } else {
            if (current.parent().row() > 0){
               QModelIndex currentParent = current.parent().sibling(current.parent().row() - 1, 0);
               if(model()->rowCount(currentParent) > 0){
                  current = model()->index(model()->rowCount(currentParent) - 1, 0, currentParent);
               } else {
                  current = currentParent;
               }
            } else {
               current = model()->index(0, 0, current.parent());
            }
         }

         break;
      case MoveDown:
         if (current.row() < model()->rowCount(current.parent()) - 1){
            current = model()->index(current.row() + 1, 0, current.parent());
         } else {
            if (current.parent() != rootIndex() &&
                (current.parent().row() <
                 model()->rowCount(current.parent().parent()) - 1)){

               QModelIndex currentParent = current.parent().sibling(current.parent().row() + 1, 0);
               if(model()->rowCount(currentParent) > 0){
                  current = model()->index(0, 0, currentParent);
               } else {
                  current = currentParent;
               }

            } else {
               current = model()->index(model()->rowCount(current.parent()) - 1, current.column(),
                                        current.parent());
            }
         }

         break;
      default:
         break;
   }

   viewport()->update();

   return current;
}

void SongFolderView::setSelection( const QRect &rect, QItemSelectionModel::SelectionFlags flags )
{
   // NOTE: as a simplification, always select the full song. Only height is used.
   int top = rect.y();
   int bot = top + rect.height() - 1;

   QModelIndexList intersectedIndexes;

   // find each direct child
   for(int i = 0; i < mp_ChildrenItems->size(); i++){
      int childTop = mp_ChildrenItems->at(i)->y();
      int childBot = childTop + mp_ChildrenItems->at(i)->height();

      // Intersected if not bot > top
      if(!(bot < childTop || childBot < top)){
         intersectedIndexes.append(mp_ChildrenItems->at(i)->modelIndex());
      }
   }

   if (intersectedIndexes.size() == 0){
      QModelIndex noIndex;
      QItemSelection selection(noIndex, noIndex);
      selectionModel()->select(selection, flags);
   } else {
      // we know that sub view are in the same order as rows

      int firstRow = intersectedIndexes[0].row();
      int lastRow = intersectedIndexes[intersectedIndexes.size() - 1].row();

      QItemSelection selection(
               model()->index(firstRow, 0, rootIndex()),
               model()->index(lastRow, 0, rootIndex()));
      selectionModel()->select(selection, flags);
   }

   update();

}

void SongFolderView::scrollTo( const QModelIndex &index, ScrollHint /*hint*/ )
{
   // NOTE: as a simplification, always scroll to a song.

   QRect area = viewport()->rect();
   QRect rect = visualRect(index);

   if (rect.left() < area.left()) {
       horizontalScrollBar()->setValue(
           horizontalScrollBar()->value() + rect.left() - area.left());
   } else if (rect.right() > area.right()) {
       horizontalScrollBar()->setValue(
           horizontalScrollBar()->value() + qMin(
               rect.right() - area.right(), rect.left() - area.left()));
   }
   if(mp_Header){

      if (rect.top() < (area.top() + mp_Header->height())) {
          verticalScrollBar()->setValue(
              verticalScrollBar()->value() + rect.top() - (area.top() + mp_Header->height()));
      } else if (rect.bottom() > area.bottom()) {
          verticalScrollBar()->setValue(
              verticalScrollBar()->value() + qMin(
                  rect.bottom() - area.bottom(), rect.top() - area.top()));
      }
   } else {

      if (rect.top() < (area.top())) {
          verticalScrollBar()->setValue(
              verticalScrollBar()->value() + rect.top() - (area.top()));
      } else if (rect.bottom() > area.bottom()) {
          verticalScrollBar()->setValue(
              verticalScrollBar()->value() + qMin(
                  rect.bottom() - area.bottom(), rect.top() - area.top()));
      }
   }

   update();

}

void SongFolderView::dataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight, const QVector<int> & /*roles*/)
{
   // if Root is being modified
   if(topLeft.row() <= rootIndex().row() && bottomRight.row() >= rootIndex().row()){
      if(topLeft.sibling(rootIndex().row(), AbstractTreeItem::NAME).internalPointer() == rootIndex().internalPointer()){
         dataChangedInternal(topLeft, bottomRight);
      }
   }

   
   QModelIndexList modelIndexPath = pathToIndex(model()->index(topLeft.row(), 0, topLeft.parent()));

   // Invalid index or not found, return invalid visualRect
   // TODO standardize to make similar to rowInserted implementation

   if (modelIndexPath.size() <= 0){
      return ;
   }

   if(!rootIndex().isValid() ||
         !modelIndexPath.first().parent().isValid() ||
         (modelIndexPath.first().parent().internalPointer() != rootIndex().internalPointer())){
      return;
   }

   SongFolderViewItem *p_ChildView = nullptr;

   if (modelIndexPath.size() <= 1){
      // Create a list of items to modify
      for(int row = topLeft.row(); row <= bottomRight.row(); row++){
         p_ChildView = itemForIndex(model()->index(row, 0, rootIndex()));
         if (p_ChildView != nullptr){
            p_ChildView->dataChanged(model()->index(row, topLeft.column(), rootIndex()), model()->index(row, bottomRight.column(), rootIndex()));
         } else {
            // NOTE: might not be a real error
            qWarning() << "SongFolderView::dataChanged - ERROR 1 - (p_ChildView == 0)";
         }
      }
   } else {
      p_ChildView = itemForIndex(modelIndexPath.at(0));
      if (p_ChildView != nullptr){
         p_ChildView->childrenDataChanged(modelIndexPath, topLeft, bottomRight, 1);
      } else {
         // NOTE: might not be a real error
         qWarning() << "SongFolderView::dataChanged - ERROR 2 - (p_ChildView == 0)";
      }
   }

}

void SongFolderView::dataChangedInternal(const QModelIndex &topLeft, const QModelIndex &bottomRight)
{
   // loop on each data and update required data
   for(int column = topLeft.column(); column <= bottomRight.column(); column++){
      QModelIndex index = topLeft.sibling(rootIndex().row(), column);
      if(!index.isValid()){
         continue;
      }
      switch(column){
         case AbstractTreeItem::NAME:
            emit sigSetTitle(tr("Beats - %1").arg(index.data().toString()));
            break;
         default:
            break;
      }
   }
}


void SongFolderView::rowsAboutToBeRemoved(const QModelIndex & parent, int start, int end)
{

   // if current root is being removed from model
   if(rootIndex().isValid() && parent == rootIndex().parent()){
      if(rootIndex().row() >= start && rootIndex().row() <= end){
         // Should not happen
         qWarning() << "SongFolderView::rowsAboutToBeRemoved - ERROR - rootIndex is being removed. As a security, rootIndex is set to null";
         setRootIndex(QModelIndex());
      }

      return;
   }

   if (parent == rootIndex()) {
      if(mp_ChildrenItems->size() <= end){
         qWarning() << "SongFolderView::rowsToBeRemoved - FAILED";
         return;
      }

      removeRowsInternal(start, end);

   } else {
      QModelIndexList modelIndexPath = pathToIndex(parent);
      if (modelIndexPath.size() <= 0){
         qWarning() << "SongFolderView::rowsToBeRemoved - FAILED";
         return;
      }

      // Stop immediately if modified item is not being displayed
      if(!rootIndex().isValid() ||
            !modelIndexPath.first().parent().isValid() ||
            (modelIndexPath.first().parent().internalPointer() != rootIndex().internalPointer())){
         return;
      }

      SongFolderViewItem *p_ChildView = itemForIndex(modelIndexPath.at(0));
      if (p_ChildView == nullptr){
         qWarning() << "SongFolderView::rowsToBeRemoved - FAILED - (p_ChildView == 0)";
         return;
      }
      p_ChildView->childrenRowsRemoved(modelIndexPath, start, end, 0);
   }

   updateLayout(QPoint(-horizontalScrollBar()->value(),-verticalScrollBar()->value()));


}


void SongFolderView::removeRowsInternal(int start, int end)
{
   // remove all widgets corresponding to rows
   NewSongWidget *p_NewSongWidget;
   SongFolderViewItem *p_SongFolderViewItem;

   for(int i = end; i >= start; i--){
      p_NewSongWidget = mp_NewSongWidgets->at(i + 1);
      mp_NewSongWidgets->removeAt(i + 1);
      delete p_NewSongWidget;

      p_SongFolderViewItem = mp_ChildrenItems->at(i);
      mp_ChildrenItems->removeAt(i);
      delete p_SongFolderViewItem;
   }

   // Determine if new part widgets can be enabled
   int maxChildrenCnt = rootIndex().sibling(rootIndex().row(), AbstractTreeItem::MAX_CHILD_CNT).data().toInt();

   // Note, the row count is the count before removal thus the <=
   if(model()->rowCount(rootIndex()) <= maxChildrenCnt){
      for(int i = 0; i < mp_NewSongWidgets->count(); i++){
         mp_NewSongWidgets->at(i)->setEnabled(true);
      }
   }

   updateOrderSlots();

   qWarning() << "TODO : SongFolderView::insertRowsInternal : refresh labels to display new indexes song's move handle widget and title widget" << endl;

}

void SongFolderView::rowsInserted(const QModelIndex &parent, int start, int end)
{

   if (parent == rootIndex()) {
      if(start > mp_ChildrenItems->size()){
         qWarning() << "SongFolderView::rowsInserted - (start > mp_ChildrenItems->size()) in " << metaObject()->className();
         return;
      }

      insertRowsInternal(start, end);

   } else {
      QModelIndexList modelIndexPath = pathToIndex(parent);
      if (modelIndexPath.size() <= 0){
         qWarning() << "SongFolderView::rowsInserted - (modelIndexPath.size() <= 0) in " << metaObject()->className();
         return ;
      }

      // Stop immediately if modified item is not being displayed
      if(!rootIndex().isValid() ||
            !modelIndexPath.first().parent().isValid() ||
            (modelIndexPath.first().parent().internalPointer() != rootIndex().internalPointer())){
         return;
      }

      SongFolderViewItem *p_ChildView = itemForIndex(modelIndexPath.at(0));
      if (p_ChildView == nullptr){
         qWarning() << "SongFolderView::rowsInserted - (p_ChildView == 0) in " << metaObject()->className();
         return;
      }
      p_ChildView->childrenRowsInserted(modelIndexPath, start, end, 0);
   }

   updateLayout(QPoint(-horizontalScrollBar()->value(),-verticalScrollBar()->value()));

}



void SongFolderView::insertRowsInternal(int start, int end, bool modal)
{
   // Add all items from model
   NewSongWidget *p_NewSongWidget;
   SongWidget *p_SongWidget;

   QProgressDialog *p_progress = nullptr;
   if(modal){
      p_progress = new QProgressDialog(tr("Loading Folder Content"), tr("Hide"), start - 1, end, this);
      p_progress->setWindowModality(Qt::WindowModal);
      p_progress->setMinimumDuration(1);

      p_progress->setValue(start - 1);
   }

   for(int i = start; i <= end; i++){
      QModelIndex modelIndex = model()->index(i, 0, rootIndex());
      if (modelIndex.isValid()){

         // Create a "song widget"
         p_SongWidget = new SongWidget(model(), this->viewport());
         p_SongWidget->populate(modelIndex);
         mp_ChildrenItems->insert(i, p_SongWidget);
         connect(p_SongWidget, SIGNAL(sigSubWidgetClicked(QModelIndex)), this, SLOT(slotSubWidgetClicked(QModelIndex)));
         connect(p_SongWidget, &SongWidget::sigSelectTrack, this, &SongFolderView::slotSelectTrack);

         p_SongWidget->show(); // need to call show when widget created at runtime

         // Create a "new song widget" after "song widget"
         p_NewSongWidget = new NewSongWidget(this->viewport());
         mp_NewSongWidgets->insert(i + 1, p_NewSongWidget);
         connect(p_NewSongWidget, SIGNAL(sigAddSongToRow(NewSongWidget*, bool)), this, SLOT(slotInsertSong(NewSongWidget*, bool)));

         p_NewSongWidget->show(); // need to call show when widget created at runtime
      }

      if(p_progress != nullptr && !p_progress->wasCanceled()){
         p_progress->setValue(i);
      }
   }

   if(modal){
      delete p_progress;
   }

   // Determine if new part widgets can be enabled
   int maxChildrenCnt = rootIndex().sibling(rootIndex().row(), AbstractTreeItem::MAX_CHILD_CNT).data().toInt();

   // Note: new widgets are enabled by default
   if(model()->rowCount(rootIndex()) >= maxChildrenCnt){
      for(int i = 0; i < mp_NewSongWidgets->count(); i++){
         mp_NewSongWidgets->at(i)->setEnabled(false);
      }
   }

   updateOrderSlots();

}

void SongFolderView::selectionChanged( const QItemSelection &selected, const QItemSelection &deselected )
{
   // Test on song label only
   for(int i = 0; i < mp_ChildrenItems->size(); i++){
      if(deselected.contains(mp_ChildrenItems->at(i)->modelIndex())){
         mp_ChildrenItems->at(i)->setSelected(false);
      }
      if(selected.contains(mp_ChildrenItems->at(i)->modelIndex())){
         mp_ChildrenItems->at(i)->setSelected(true);
      }
   }

   setUpdatesEnabled(true);
   QAbstractItemView::selectionChanged(selected, deselected);
}


void SongFolderView::currentChanged(const QModelIndex & current, const QModelIndex & previous)
{
   if(!rootIndex().isValid()){
      return;
   }

   // Determine if the root index needs to change
   QModelIndex index = current;
   while(index.isValid()){
      if(index != rootIndex() && index.parent() == rootIndex().parent()){
         setRootIndex(index);
         break;
      }
      index = index.parent();
   }

   // Test on song label only
   for(int i = 0; i < mp_ChildrenItems->size(); i++){

      if(mp_ChildrenItems->at(i)->modelIndex() == previous){
         mp_ChildrenItems->at(i)->setCurrentIndex(false);
      }

      if(mp_ChildrenItems->at(i)->modelIndex() == current){
         mp_ChildrenItems->at(i)->setCurrentIndex(true);
      }
   }
   QAbstractItemView::currentChanged(current, previous);

}

QModelIndex SongFolderView::indexAt(const QPoint &point) const{

   // NOTE: as a simplification, we stop at song level

   // Find recursively the deepest item that the point
   // NOTE: recursivity removed for simplification
   for(int i = 0; i < mp_ChildrenItems->size(); i++){
      if(mp_ChildrenItems->at(i)->geometry().contains(point)){

         return mp_ChildrenItems->at(i)->modelIndex();
         
      }
   }


   return QModelIndex();
}

bool SongFolderView::isIndexHidden( const QModelIndex &/*index*/ ) const{
   return false;
}

QRegion SongFolderView::visualRegionForSelection( const QItemSelection &selection ) const{

   // NOTE: as a simplification, always return a song selection
   int ranges = selection.count();

   if (ranges == 0){
       return QRect();
   }

   QRegion region;

   for (int i = 0; i < ranges; ++i) {

       QItemSelectionRange range = selection.at(i);
       for (int row = range.top(); row <= range.bottom(); ++row) {
          // Always return selection for column 0
          QModelIndex index = model()->index(row, 0, rootIndex());
          region += visualRect(index);
       }
   }

   return region;
}

void SongFolderView::setModel(QAbstractItemModel *model)
{

   if(this->model() == model){
      return;
   }

   if(this->model()){
      disconnect(this->model(), SIGNAL(layoutChanged(QList<QPersistentModelIndex>,QAbstractItemModel::LayoutChangeHint)), this, SLOT(slotLayoutChanged(QList<QPersistentModelIndex>,QAbstractItemModel::LayoutChangeHint)));
      disconnect(this->model(), SIGNAL(rowsMoved(QModelIndex,int,int,QModelIndex,int)), this, SLOT(slotRowsMoved(QModelIndex,int,int,QModelIndex,int)));
   }


   // DELETE PREVIOUS WIDGETS
   qDeleteAll(*mp_ChildrenItems);
   mp_ChildrenItems->clear();

   qDeleteAll(*mp_NewSongWidgets); // Note: signals are disconnected automatically
   mp_NewSongWidgets->clear();

   QAbstractItemView::setModel(model);

   // HEADER
   if(!mp_Header && model){
      mp_Header = new QHeaderView(Qt::Horizontal, this);
      mp_Header->setStretchLastSection(true);
      mp_Header->setDefaultAlignment(Qt::AlignCenter|Qt::AlignVCenter);
      mp_Header->show();
   } else if(mp_Header && !model){
      delete mp_Header;
      mp_Header = nullptr;
   }

   if(mp_Header && model){
      mp_Header->setModel(model);
      for(int i = 0; i < model->columnCount(); i++){
         mp_Header->setSectionResizeMode(i, QHeaderView::Fixed);
      }

      // Hack : keep only the first column + the five last columns. The 4 last columns are dummy and used only for this display
      for(int i = 1; i < model->columnCount() - 5; i++){
         mp_Header->setSectionHidden(i, true);
      }
   }

   // ROOT Index
   if(model && model->rowCount(static_cast<BeatsProjectModel *>(model)->songsFolderIndex()) > 0){
      // First index of songsFolderIndex
	  setRootIndex(model->index(0,0,static_cast<BeatsProjectModel *>(model)->songsFolderIndex()));
   }

   // Custom connections
   if(model){
      connect(model, SIGNAL(layoutChanged(QList<QPersistentModelIndex>,QAbstractItemModel::LayoutChangeHint)), this, SLOT(slotLayoutChanged(QList<QPersistentModelIndex>,QAbstractItemModel::LayoutChangeHint)));
      connect(model, SIGNAL(rowsMoved(QModelIndex,int,int,QModelIndex,int)), this, SLOT(slotRowsMoved(QModelIndex,int,int,QModelIndex,int)));
   }
    horizontalScrollBar()->setVisible(model);
    verticalScrollBar()->setVisible(model);
}


void SongFolderView::setRootIndex(const QModelIndex & index)
{

   if(index == rootIndex()){

      return;
   }

   // DELETE PREVIOUS WIDGETS
   qDeleteAll(*mp_ChildrenItems);
   mp_ChildrenItems->clear();

   qDeleteAll(*mp_NewSongWidgets); // Note: signals are disconnected automatically
   mp_NewSongWidgets->clear();

   // Reset scroll bars when there is nothing displayed
   horizontalScrollBar()->setValue(0);
   verticalScrollBar()->setValue(0);
   updateGeometries();


   // SET ROOT INDEX
   QAbstractItemView::setRootIndex(index);
   emit rootIndexChanged(index);

   // Handle invalid root index by re-creating nothing
   if(!index.isValid()){
      if(mp_Header){
         delete mp_Header;
         mp_Header = nullptr;
      }

      return;
   }

   if(!mp_Header){
      mp_Header = new QHeaderView(Qt::Horizontal, this);
      mp_Header->setStretchLastSection(true);
      mp_Header->setDefaultAlignment(Qt::AlignCenter|Qt::AlignVCenter);
      mp_Header->show();
      mp_Header->setModel(model());
      for(int i = 0; i < model()->columnCount(); i++){
         mp_Header->setSectionResizeMode(i, QHeaderView::Fixed);
      }

      // Hack : keep only the first column + the five last columns. The 4 last columns are dummy and used only for this display
      for(int i = 1; i < model()->columnCount() - 5; i++){
         mp_Header->setSectionHidden(i, true);
      }
   }

   // RE-CREATE VIEW CONTENT
   // insert initial "new song widget" at the beginning to allow inserting song at beginning
   NewSongWidget *p_NewSongWidget = new NewSongWidget(this->viewport());
   mp_NewSongWidgets->append(p_NewSongWidget);
   connect(p_NewSongWidget, SIGNAL(sigAddSongToRow(NewSongWidget*, bool)), this, SLOT(slotInsertSong(NewSongWidget*, bool)));
   p_NewSongWidget->show();

   // Add all items from model
   insertRowsInternal(0, model()->rowCount(rootIndex())-1, true);

   // UPDATE LAYOUT
   updateLayout(QPoint(-horizontalScrollBar()->value(),-verticalScrollBar()->value()));

}

// Mandatory to catch resizeEvent in order to resize subcomponents
void SongFolderView::resizeEvent(QResizeEvent * /* event */)
{
   updateLayout(QPoint(-horizontalScrollBar()->value(),-verticalScrollBar()->value()));
}

void SongFolderView::updateLayout(QPoint topLeftCorner)
{

   if(!model()){
      return;
   }
   
   for(int i = 0; i < mp_ChildrenItems->size(); i++) {
      mp_ChildrenItems->at(i)->updateMinimumSize();
   }

   // Update m_ContentSize for layout
   int maxMinimumWidth=0;
   int totalHeight=mp_Header->height(); // NOTE : mp_Header exists if model() not null
   for(int i = 0; i < mp_ChildrenItems->size(); i++){
      // Create a "new song widget" before "song widget"
      totalHeight += mp_NewSongWidgets->at(i)->minimumHeight();
      totalHeight += mp_ChildrenItems->at(i)->minimumHeight();

      if (mp_ChildrenItems->at(i)->minimumWidth() > maxMinimumWidth){
         maxMinimumWidth = mp_ChildrenItems->at(i)->minimumWidth();
      }
   }

   totalHeight += mp_NewSongWidgets->last()->minimumHeight();

   m_ContentSize.setHeight(totalHeight);

   if(viewport()->width() > maxMinimumWidth){
      m_ContentSize.setWidth(viewport()->width());
      mp_Header->setFixedSize(viewport()->width(), 30);
   } else {
      m_ContentSize.setWidth(maxMinimumWidth);
      mp_Header->setFixedSize(maxMinimumWidth, 30);
   }

   QPoint songOrigin(topLeftCorner.x(), topLeftCorner.y() + mp_Header->height());
   for(int i = 0; i < mp_ChildrenItems->size(); i++){
      // Position a "new song widget" before each "song widget"
      mp_NewSongWidgets->at(i)->setGeometry(QRect(songOrigin, QSize(m_ContentSize.width(), mp_NewSongWidgets->at(i)->minimumHeight())));
      songOrigin.setY(songOrigin.y() + mp_NewSongWidgets->at(i)->height());


      // Set geometry relative to self
      mp_ChildrenItems->at(i)->setGeometry(QRect(songOrigin, QSize(m_ContentSize.width(), mp_ChildrenItems->at(i)->minimumHeight())));
      mp_ChildrenItems->at(i)->updateLayout();
      songOrigin.setY(songOrigin.y() + mp_ChildrenItems->at(i)->height());
   }

   // Position a final "new song widget" at the end to allow appending song at the end
   mp_NewSongWidgets->last()->setGeometry(QRect(songOrigin, QSize(m_ContentSize.width(), mp_NewSongWidgets->last()->minimumHeight())));

   // Hack to get header column width
   if(mp_ChildrenItems->size() > 0){

      if(mp_ChildrenItems->at(0)->headerColumnWidth(0) > 0 && mp_Header->count() > 0){
         mp_Header->resizeSection(0, mp_ChildrenItems->at(0)->headerColumnWidth(0));

         for(int i = 1; i < model()->columnCount() - mp_Header->hiddenSectionCount(); i++){
            mp_Header->resizeSection(i+mp_Header->hiddenSectionCount(), mp_ChildrenItems->at(0)->headerColumnWidth(i));
         }
      } else {
         for(int i = 0; i < model()->columnCount(); i++){
            mp_Header->resizeSection(i, mp_Header->width() / (model()->columnCount() - mp_Header->hiddenSectionCount()));
         }
      }
   } else {
      for(int i = 0; i < model()->columnCount(); i++){
         mp_Header->resizeSection(i, mp_Header->width() / (model()->columnCount() - mp_Header->hiddenSectionCount()));
      }
   }

   updateGeometries();
}



void SongFolderView::updateGeometries()
{

   horizontalScrollBar()->setPageStep(viewport()->width());
   horizontalScrollBar()->setRange(0, qMax(0, m_ContentSize.width() - viewport()->width()));
   verticalScrollBar()->setPageStep(viewport()->height());
   verticalScrollBar()->setRange(0, qMax(0, m_ContentSize.height() - viewport()->height()));

}

void SongFolderView::wheelEvent ( QWheelEvent * event )
{

   // Increase the mouse wheel sensibility * 10

   QWheelEvent newEvent(event->posF(),             // const QPointF & pos,
                        event->globalPosF(),       // const QPointF & globalPos
                        event->pixelDelta() * 10,  // QPoint pixelDelta
                        event->angleDelta() * 10,  // QPoint angleDelta
                        event->delta() * 10,       // int qt4Delta
                        event->orientation(),      // Qt::Orientation qt4Orientation
                        event->buttons(),          // Qt::MouseButtons buttons
                        event->modifiers());       // Qt::KeyboardModifiers modifiers

   if (event->orientation() == Qt::Horizontal){
      QApplication::sendEvent(horizontalScrollBar(), &newEvent);
   }
   else{
      QApplication::sendEvent(verticalScrollBar(), &newEvent);
   }

   event->setAccepted(newEvent.isAccepted());

}

void SongFolderView::scrollContentsBy(int dx, int dy)
{
   viewport()->scroll(dx, dy);
   if(mp_Header){
      mp_Header->scroll(dx, 0);
   }

}

void SongFolderView::import(const QModelIndex& index, int preferred, const QStringList& files)
{
    // 1 - Validate index
    auto cur = static_cast<AbstractTreeItem*>(index.internalPointer());
    auto song = qobject_cast<SongFileItem*>(cur);
    auto folder = qobject_cast<SongFolderTreeItem*>(song ? song->parent() : cur);

    if (!folder) {
        QMessageBox::warning(this, tr("No valid folder selected"), tr("A folder must be selected in order to import"));
        return;
    }

    // 2 - Choose songs/folders to insert
    Workspace w;
    QString filter(preferred ? preferred > 0 ? BMFILES_PORTABLE_FOLDER : BMFILES_PORTABLE_SONG : BMFILES_PORTABLE);
    QStringList srcFileNames = !files.isEmpty() ? files : QFileDialog::getOpenFileNames(
                this,
                tr("Import Song or Folder"),
                w.userLibrary()->libSongs()->currentPath(),
                BMFILES_PORTABLE_DIALOG_FILTER,
                &filter);

    QStringList songs;
    QStringList folders;
    QStringList garbage;
    if(srcFileNames.isEmpty()){
        return;
    } else foreach (auto f, srcFileNames) {
        (f.endsWith("." BMFILES_PORTABLE_FOLDER_EXTENSION) ? folders : f.endsWith("." BMFILES_PORTABLE_SONG_EXTENSION) ? songs : garbage).append(f);
    }

    // 3 - Make sure imported songs don't exceed the maximum
    int maxChildrenCnt = folder->data(AbstractTreeItem::MAX_CHILD_CNT).toInt();

    if((folder->childCount() + songs.count()) > maxChildrenCnt){
       QMessageBox::critical(this, tr("Import Song"), tr("Cannot add %1 songs to the current %2 songs\nMaximum song count per folder (%3) reached").arg(folder->childCount()).arg(maxChildrenCnt).arg(srcFileNames.count()));
       return;
    }

    w.userLibrary()->libSongs()->setCurrentPath(QFileInfo(srcFileNames.first()).absolutePath());

    // 4 - Perform operation
    model()->importModal(this, index, songs, folders);

    // 5 - Complain about garbage
    if (!garbage.isEmpty()) {
        auto msg = tr("The following files were not imported:%1%2%3%4%5(_._)\n\nNot a portable file format");
        while (msg.indexOf('%')+1) {
            msg = msg.arg(garbage.isEmpty() ? "" : "\n" + garbage.takeAt(0));
        }
        msg = msg.replace("(_._)", garbage.isEmpty() ? "" : tr("(...and %1 more)").arg(garbage.size()));
        QMessageBox::warning(this, tr("Import Song or Folder"), msg);
    }
}

void SongFolderView::slotInsertSong(NewSongWidget* w, bool _import)
{

   // Determine if new song can be added
   int maxChildrenCnt = rootIndex().sibling(rootIndex().row(), AbstractTreeItem::MAX_CHILD_CNT).data().toInt();

   if(model()->rowCount(rootIndex()) >= maxChildrenCnt){
      QMessageBox::critical(this, tr("Add Song"), tr("Cannot add new song\nMaximum song count per folder (%1) reached").arg(maxChildrenCnt));
      return;
   }

    if (_import) {
        import(rootIndex().child(mp_NewSongWidgets->indexOf(w), 0), -1);
    } else {
        model()->createNewSong(rootIndex(), mp_NewSongWidgets->indexOf(w));
    }

}

void SongFolderView::slotSubWidgetClicked(const QModelIndex &index)
{
   selectionModel()->select(index, QItemSelectionModel::SelectCurrent | QItemSelectionModel::Clear);
   selectionModel()->setCurrentIndex(index, QItemSelectionModel::SelectCurrent);
}

void SongFolderView::slotLayoutChanged(const QList<QPersistentModelIndex> & /*parents*/, QAbstractItemModel::LayoutChangeHint /*hint*/)
{
   qWarning() << "TODO SongFolderView::slotLayoutChanged" << endl;
}

void SongFolderView::slotRowsMoved(const QModelIndex & sourceParent, int sourceStart, int sourceEnd, const QModelIndex & destinationParent, int destinationRow)
{
   QList<SongFolderViewItem *> list;

   // SPECIAL CASE when in root
   if(sourceParent == rootIndex() && destinationParent == rootIndex()){
      rowsMovedGetInternal(sourceStart, sourceEnd, &list);
      if(destinationRow > sourceStart){
         rowsMovedInsertInternal(destinationRow, &list);
         rowsMovedRemoveInternal(sourceStart, sourceEnd);
      } else {
         rowsMovedRemoveInternal(sourceStart, sourceEnd);
         rowsMovedInsertInternal(destinationRow, &list);
      }

      updateOrderSlots();

      updateLayout(QPoint(-horizontalScrollBar()->value(),-verticalScrollBar()->value()));

      if(destinationRow > sourceStart){
         // Scroll Down
         scrollTo(list.last()->modelIndex());
      } else {
         // Scroll Up
         scrollTo(list.first()->modelIndex());
      }
      return;
   }

   QModelIndexList sourceModelIndexPath = pathToIndex(sourceParent);
   QModelIndexList destModelIndexPath = pathToIndex(destinationParent);

   if (sourceModelIndexPath.size() <= 0){
      qWarning() << "SongFolderView::slotRowsMoved - FAILED - (sourceModelIndexPath.size() <= 0)";

      return;
   }
   if (destModelIndexPath.size() <= 0){
      qWarning() << "SongFolderView::slotRowsMoved - FAILED - (destModelIndexPath.size() <= 0)";

      return;
   }

   if(!rootIndex().isValid() ||
         !sourceModelIndexPath.first().parent().isValid() ||
         !destModelIndexPath.first().parent().isValid() ||
         (sourceModelIndexPath.first().parent().internalPointer() != rootIndex().internalPointer()) ||
         (destModelIndexPath.first().parent().internalPointer() != rootIndex().internalPointer())){

      return;
   }

   SongFolderViewItem *p_ChildView;

   // 1 - Retrieve items in source

   p_ChildView = itemForIndex(sourceModelIndexPath.at(0));
   if (p_ChildView == nullptr){
      qWarning() << "SongFolderView::slotRowsMoved - 1 - Retrieve items in source - FAILED - (p_ChildView == 0)";

      return;
   }
   p_ChildView->childrenRowsMovedGet(sourceModelIndexPath, sourceStart, sourceEnd, &list, 0);


   if(destinationRow > sourceStart){
      // 2 - Insert items in destination
      p_ChildView = itemForIndex(destModelIndexPath.at(0));
      if (p_ChildView == nullptr){
         qWarning() << "SongFolderView::slotRowsMoved - 2 - Insert items in destination - FAILED - (p_ChildView == 0)";
//         qDebug() << "SongFolderView::slotRowsMoved - EXIT";
         return;
      }
      p_ChildView->childrenRowsMovedInsert(destModelIndexPath, destinationRow, &list, 0);


      // 3 - Remove items in destination
      p_ChildView = itemForIndex(sourceModelIndexPath.at(0)); // NOTE : p_ChildView already verified at step 1
      p_ChildView->childrenRowsMovedRemove(sourceModelIndexPath, sourceStart, sourceEnd, 0);

   } else {

      // 3 - Remove items in destination
      p_ChildView = itemForIndex(sourceModelIndexPath.at(0)); // NOTE : p_ChildView already verified at step 1
      p_ChildView->childrenRowsMovedRemove(sourceModelIndexPath, sourceStart, sourceEnd, 0);

      // 2 - Insert items in destination
      p_ChildView = itemForIndex(destModelIndexPath.at(0));
      if (p_ChildView == nullptr){
         qWarning() << "SongFolderView::slotRowsMoved - 2 - Insert items in destination - FAILED - (p_ChildView == 0)";

         return;
      }
      p_ChildView->childrenRowsMovedInsert(destModelIndexPath, destinationRow, &list, 0);
   }

   updateLayout(QPoint(-horizontalScrollBar()->value(),-verticalScrollBar()->value()));


}


void SongFolderView::rowsMovedGetInternal(int start, int end, QList<SongFolderViewItem *> *p_List)
{

   for(int i = start; i <= end; i++){
      p_List->append(mp_ChildrenItems->at(i));
   }
}

void SongFolderView::rowsMovedInsertInternal(int start, QList<SongFolderViewItem *> *p_List)
{

   for(int i = 0; i < p_List->count(); i++){
      mp_ChildrenItems->insert(start++, p_List->at(i));
      if(p_List->at(i)->parent() != this->viewport()){
         p_List->at(i)->setParent(this->viewport());
      }
   }

}

void SongFolderView::rowsMovedRemoveInternal(int start, int end)
{

   for(int i = end; i >= start; i--){
      mp_ChildrenItems->removeAt(i);
   }
}

void SongFolderView::updateOrderSlots()
{
   // Update is first, is last and OrderChanged slots
   for(int i = 0; i < mp_ChildrenItems->count(); i++){
      static_cast<SongWidget *>(mp_ChildrenItems->at(i))->slotIsFirst(i == 0);
      static_cast<SongWidget *>(mp_ChildrenItems->at(i))->slotIsLast(i == mp_ChildrenItems->count() - 1);
      static_cast<SongWidget *>(mp_ChildrenItems->at(i))->slotOrderChanged(i+1);
   }

}

void SongFolderView::slotSelectTrack(const QByteArray &trackData, int trackIndex, int typeId, int partIndex)
{
  emit sigSelectTrack(trackData, trackIndex, typeId, partIndex);
}

void SongFolderView::slotSetPlayerEnabled(bool enabled)
{
   for(int i = 0; i < mp_ChildrenItems->count(); i++){
      mp_ChildrenItems->at(i)->slotSetPlayerEnabled(enabled);
   }
}

void SongFolderView::setSongsEnabled(bool enabled)
{
   // All Songs
   for(int i = 0; i < mp_ChildrenItems->count(); i++){
      mp_ChildrenItems->at(i)->setEnabled(enabled);
   }
   // All New Song Buttons
   for(int i = 0; i < mp_NewSongWidgets->count(); i++){
      mp_NewSongWidgets->at(i)->setEnabled(enabled);
   }
}
