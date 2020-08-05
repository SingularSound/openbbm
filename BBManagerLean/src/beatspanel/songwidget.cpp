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
#include <QFile>
#include <QDebug>
#include <QPainter>
#include <QStyleOption>
#include <QList>
#include <QMessageBox>
#include <QApplication>

#include "songwidget.h"
#include "songpartwidget.h"
#include <stylesheethelper.h>
#include "movehandlewidget.h"
#include "deletehandlewidget.h"
#include "songtitlewidget.h"
#include "newpartwidget.h"

#include <model/tree/abstracttreeitem.h>
#include <model/tree/project/beatsprojectmodel.h>

SongWidget::SongWidget(BeatsProjectModel *p_Model, QWidget *parent) :
   SongFolderViewItem(p_Model, parent)
 , m_MaxChildrenWidth(0)
{
   mp_NewPartWidgets = new QList<NewPartWidget*>;
   mp_SongPartItems = new QList<SongPartWidget*>;
   /* Create the song title widget */
   mp_SongTitleWidget = new SongTitleWidget(p_Model, this);

   /* Create the moving arrow */
   mp_MoveHandleWidget = new MoveHandleWidget(this);
   mp_MoveHandleWidget->setMinimumWidth(mp_MoveHandleWidget->PREFERRED_WIDTH);
   mp_MoveHandleWidget->setMaximumWidth(mp_MoveHandleWidget->PREFERRED_WIDTH);

   /* Create the delet */
   mp_DeleteHandleWidget = new DeleteHandleWidget(this,1);
   mp_DeleteHandleWidget->setMinimumWidth(mp_DeleteHandleWidget->PREFERRED_WIDTH);
   mp_DeleteHandleWidget->setMaximumWidth(mp_DeleteHandleWidget->PREFERRED_WIDTH);

   // connect slots to signals
   connect(this, SIGNAL(sigIsFirst(bool)), mp_MoveHandleWidget, SLOT(slotIsFirst(bool)));
   connect(this, SIGNAL(sigIsLast (bool)), mp_MoveHandleWidget, SLOT(slotIsLast (bool)));
   connect(mp_MoveHandleWidget, SIGNAL(sigSubWidgetClicked()), this, SLOT(slotSubWidgetClicked()));
   connect(mp_MoveHandleWidget, SIGNAL(sigUpClicked()), this, SLOT(slotMoveSongUpClicked()));
   connect(mp_MoveHandleWidget, SIGNAL(sigDownClicked()), this, SLOT(slotMoveSongDownClicked()));

   connect(this, SIGNAL(sigDefaultTempoChangedByModel (int)), mp_SongTitleWidget, SLOT(slotTempoChangeByModel (int)));
   connect(this, SIGNAL(sigDefaultAutoPilotEnabledChangedByModel(bool)), mp_SongTitleWidget, SLOT(slotAutoPilotChangeByModel (bool)));
   connect(mp_SongTitleWidget, SIGNAL(sigTempoChangeByUI (int)), this, SLOT(slotDefaultTempoChangedByUI (int)));
   connect(mp_SongTitleWidget, SIGNAL(sigTitleChangeByUI (QString)), this, SLOT(slotTitleChangeByUI (QString)));
   connect(mp_SongTitleWidget, SIGNAL(sigNumberChangeByUI (QString)), this, SLOT(slotNumberChangeByUI (QString)));
   connect(mp_SongTitleWidget, SIGNAL(sigSubWidgetClicked()), this, SLOT(slotSubWidgetClicked()));
   connect(mp_SongTitleWidget, SIGNAL(sigAPEnableChangeByUI (bool)), this, SLOT(slotAPEnableChangeByUI (bool)));
   connect(mp_SongTitleWidget, SIGNAL(sigGetAPState()), this, SLOT(slotAPStateRequested ()));
   connect(this, SIGNAL(sigAPUpdate(bool)), mp_SongTitleWidget, SLOT(slotAutoPilotChangeByModel(bool)));

   connect(mp_SongTitleWidget, SIGNAL(sigSubWidgetClicked()), this, SLOT(slotSubWidgetClicked()));
   connect(mp_DeleteHandleWidget, SIGNAL(sigDeleteClicked()), this, SLOT(slotDeleteButton()));
   connect(mp_DeleteHandleWidget, SIGNAL(sigSubWidgetClicked()), this, SLOT(slotSubWidgetClicked()));
}

/**
 * @brief SongWidget::~SongWidget
 *    Destructor: No special action. Only destroys member pointers
 */
SongWidget::~SongWidget(){

   delete mp_SongTitleWidget;
   delete mp_MoveHandleWidget;
   delete mp_DeleteHandleWidget;
}

// Required to apply stylesheet
void SongWidget::paintEvent(QPaintEvent * /*event*/)
{
   QStyleOption opt;
   opt.init(this);
   QPainter p(this);
   style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);
}

void SongWidget::setSongTitle(QString const& title)
{
   // Configure widgets
   mp_SongTitleWidget->setTitle(title);
}

void SongWidget::populate(QModelIndex const& modelIndex)
{
   // Delete all previous children widgets (if any)
   for(int i = 0; i < mp_ChildrenItems->size(); i++){
      delete mp_ChildrenItems->at(i);
   }

   mp_ChildrenItems->clear();

   for(int i = 0; i < mp_NewPartWidgets->size(); i++){
      disconnect(mp_NewPartWidgets->at(i), SIGNAL(sigAddPartToRow(int)), this, SLOT(slotInsertPart(int)));
      delete mp_NewPartWidgets->at(i);
   }

   mp_NewPartWidgets->clear();

   // copy new model index
   setModelIndex(modelIndex);

   // Populate self's data
   mp_SongTitleWidget->setTitle(modelIndex.data().toString());
   mp_SongTitleWidget->setNumber(modelIndex.sibling(modelIndex.row(), AbstractTreeItem::LOOP_COUNT).data().toInt());
   mp_SongTitleWidget->setUnsavedChanges(modelIndex.sibling(modelIndex.row(), AbstractTreeItem::SAVE).data().toBool());
   mp_SongTitleWidget->slotTempoChangeByModel(modelIndex.sibling(modelIndex.row(), AbstractTreeItem::TEMPO).data().toInt());
   mp_SongTitleWidget->populateDrmCombo(modelIndex.sibling(modelIndex.row(), AbstractTreeItem::DEFAULT_DRM));
   connect(mp_SongTitleWidget, SIGNAL(sigDefaultDrmChangeByUI(QString,QString)), this, SLOT(slotDefaultDrmChangedByUI(QString,QString)));

   mp_MoveHandleWidget->setLabelText(QString::number(modelIndex.row() + 1));


   // Add all items from model
   // For now, we only read the immediate children
   SongPartWidget *p_SongPartWidget;
   NewPartWidget *p_NewPartWidget;

   for(int i = 0; i < modelIndex.model()->rowCount(modelIndex); i++){
      QModelIndex childIndex = modelIndex.model()->index(i, 0, modelIndex); // NOTE: Widgets are associated to column 0
      if (childIndex.isValid()){

         QString path =  model()->index(modelIndex.row(), AbstractTreeItem::ABSOLUTE_PATH, modelIndex.parent()).data().toString();
         p_SongPartWidget = new SongPartWidget(model(), this, path);

         if (i == 0){
            p_SongPartWidget->setIntro(true);
         } else if (i == modelIndex.model()->rowCount(modelIndex) - 1){
            p_SongPartWidget->setOutro(true);
            p_NewPartWidget = new NewPartWidget(this,i);
            connect(p_NewPartWidget, SIGNAL(sigAddPartToRow(int)), this, SLOT(slotInsertPart(int)));
            mp_NewPartWidgets->append(p_NewPartWidget);
         } else {
            p_NewPartWidget = new NewPartWidget(this,i);
            connect(p_NewPartWidget, SIGNAL(sigAddPartToRow(int)), this, SLOT(slotInsertPart(int)));
            mp_NewPartWidgets->append(p_NewPartWidget);
         }
         p_SongPartWidget->populate(childIndex);
         mp_SongPartItems->append(p_SongPartWidget);
         mp_ChildrenItems->append(p_SongPartWidget);

         connect(p_SongPartWidget, SIGNAL(sigSubWidgetClicked(QModelIndex)), this, SLOT(slotSubWidgetClicked(QModelIndex)));
         connect(p_SongPartWidget, &SongPartWidget::sigSelectTrack, this, &SongWidget::slotSelectTrack);
         connect(p_SongPartWidget, &SongPartWidget::sigUpdateAP, this, &SongWidget::slotAPUpdate);
         connect(p_SongPartWidget, &SongPartWidget::sigMoveUp, this, &SongWidget::slotSawapPart);
         connect(p_SongPartWidget, &SongPartWidget::sigMoveDown, this, &SongWidget::slotSawapPart);
      }
   }

   // Determine if new part widgets can be enabled
   int maxChildrenCnt = modelIndex.sibling(modelIndex.row(), AbstractTreeItem::MAX_CHILD_CNT).data().toInt();

   // Note: new widgets are enabled by default
   if(model()->rowCount(modelIndex) >= maxChildrenCnt){
      for(int i = 0; i < mp_NewPartWidgets->count(); i++){
         mp_NewPartWidgets->at(i)->setEnabled(false);
      }
   }

   // Disable when playing
   if (modelIndex.sibling(modelIndex.row(), AbstractTreeItem::PLAYING).data().toBool() == true) {
      setEnabled(false);
   } else {
      setEnabled(true);
   }

   if (modelIndex.sibling(modelIndex.row(), AbstractTreeItem::PLAYING).data().toBool() == true) {
      setEnabled(false);
   } else {
      setEnabled(true);
   }

   // Sets the AutoPilot CheckBox in every SongTitle to its desired state on populate.
   UpdateAP();
   emit sigAPUpdate(model()->data(modelIndex.sibling(modelIndex.row(), AbstractTreeItem::AUTOPILOT_ON)).toBool());

   // Row related signals
   updateOrderSlots();
}

void SongWidget::updateMinimumSize()
{
   // Recursively update all children first
   for(int i = 0; i < mp_ChildrenItems->size(); i++) {
      mp_ChildrenItems->at(i)->updateMinimumSize();
   }

   // Width
   int width = mp_MoveHandleWidget->minimumWidth() +
         mp_DeleteHandleWidget->minimumWidth() +
         mp_SongTitleWidget->minimumWidth();

   m_MaxChildrenWidth = 0;
   for(int i = 0; i < mp_ChildrenItems->size(); i++){
      if(mp_ChildrenItems->at(i)->minimumWidth() > m_MaxChildrenWidth){
         m_MaxChildrenWidth = mp_ChildrenItems->at(i)->minimumWidth();
      }
   }
   width += m_MaxChildrenWidth;

   setMinimumWidth(width);

   // Height
   int height = 0;

   for(int i = 0; i < mp_ChildrenItems->size(); i++){
      height += mp_ChildrenItems->at(i)->minimumHeight();
   }

   setMinimumHeight(height);
   setMaximumHeight(height);
}

void SongWidget::updateLayout()
{
   int variableWidth = width() - mp_MoveHandleWidget->minimumWidth() - mp_DeleteHandleWidget->minimumWidth();
   int variableMinWidth = mp_SongTitleWidget->minimumWidth() + m_MaxChildrenWidth;

   // Items are placed from left to right
   // Note that drawCursor x coordinates are multiplied by variableWidth in order to avoid cumulative rounding errors
   int startXMult = 0;
   int nextXMult = mp_MoveHandleWidget->minimumWidth()*variableMinWidth; // NOTE: multiply by variableMinWidth for static sizes
   QPoint drawStartCursor(0,1); // Note: static subcomponent size is 2 px less in hight and width to leave sonc separation untouched
   QPoint drawEndCursor(mp_MoveHandleWidget->width()-1,height()-2);
   mp_MoveHandleWidget->setGeometry(QRect(drawStartCursor, drawEndCursor));

   startXMult = nextXMult;
   nextXMult += mp_SongTitleWidget->minimumWidth() * variableWidth;
   drawStartCursor.setX(startXMult/variableMinWidth);
   drawEndCursor.setX(nextXMult/variableMinWidth - 1);
   mp_SongTitleWidget->setGeometry(QRect(drawStartCursor, drawEndCursor));

   startXMult = nextXMult;
   nextXMult += m_MaxChildrenWidth * variableWidth;
   drawStartCursor.setX(startXMult/variableMinWidth);
   drawEndCursor.setX(nextXMult/variableMinWidth - 1);
   drawStartCursor.setY(0);
   for(int i = 0; i < mp_ChildrenItems->size(); i++){

      drawEndCursor.setY(drawStartCursor.y() + mp_ChildrenItems->at(i)->minimumHeight() - 1);

      mp_ChildrenItems->at(i)->setGeometry(QRect(drawStartCursor, drawEndCursor));
      mp_ChildrenItems->at(i)->updateLayout();

      if ( i > 0 ){
         mp_NewPartWidgets->at(i - 1)->setGeometry(QRect(QPoint(drawStartCursor.x(), drawStartCursor.y() - mp_NewPartWidgets->at(i - 1)->minimumHeight()/2), mp_NewPartWidgets->at(i - 1)->minimumSize()));
      }

      drawStartCursor.setY(drawStartCursor.y() + mp_ChildrenItems->at(i)->height());
   }

   startXMult = nextXMult;
   nextXMult += mp_DeleteHandleWidget->minimumWidth() * variableMinWidth; // NOTE: multiply by variableMinWidth for static sizes
   drawStartCursor.setX(startXMult/variableMinWidth);
   drawEndCursor.setX(nextXMult/variableMinWidth - 1);
   drawStartCursor.setY(1);
   drawEndCursor.setY(height()-2);
   mp_DeleteHandleWidget->setGeometry(QRect(drawStartCursor, drawEndCursor));
}

// Redefine
void SongWidget::setSelected(bool selected)
{
   qDebug() << "setSelected" << m_CurrentIndex << selected;
   if(selected != m_CurrentIndex){
      setStyleSheet(StyleSheetHelper::getStateStyleSheet(selected));
   }

   m_CurrentIndex = selected;
}

// Redefine
void SongWidget::setCurrentIndex(bool selected)
{
   if(selected != m_CurrentIndex){
      setStyleSheet(StyleSheetHelper::getStateStyleSheet(selected));
   }

   m_CurrentIndex = selected;
}

void SongWidget::slotDefaultTempoChangedByUI(int tempo)
{
   QModelIndex songTempo = model()->index(modelIndex().row(), AbstractTreeItem::TEMPO, modelIndex().parent());

   if(model()->data(songTempo).toInt() != tempo){
      model()->setData(songTempo, QVariant(tempo));
   }
}
void SongWidget::slotDefaultDrmChangedByUI(const QString &drmName, const QString &drmFileName)
{
   // create data to insert
   QVariant drmData(QString("%1,%2").arg(drmFileName, drmName)); // File name already upper case
   model()->setData(modelIndex().sibling(modelIndex().row(), AbstractTreeItem::DEFAULT_DRM), drmData);
}

void SongWidget::slotInsertPart(int row)
{
   emit sigSubWidgetClicked(modelIndex());

   // Determine if new part widgets can be enabled
   int maxChildrenCnt = modelIndex().sibling(modelIndex().row(), AbstractTreeItem::MAX_CHILD_CNT).data().toInt();

   if(model()->rowCount(modelIndex()) >= maxChildrenCnt){
      QMessageBox::critical(this, tr("Add Song Part"), tr("Unable to add new Song Part\nLimit of %1 reached").arg(maxChildrenCnt - 2));
      return;
   }

   model()->createNewSongPart(modelIndex(), row);
}

void SongWidget::slotDeleteButton()
{
    // if no Shift is held, ask for confirmation
    if (!(QApplication::keyboardModifiers() & Qt::ShiftModifier) &&
        QMessageBox::Yes != QMessageBox::question(this, tr("Delete Song"), tr("Are you sure you want to delete the song from the folder?\nNOTE You can undo the deletion up until the software is closed!")
#ifndef Q_OS_MAC
            + tr("\n\nTip: Hold Shift to hide this confirmation box")
#endif
        , QMessageBox::Yes|QMessageBox::Cancel, QMessageBox::Yes)) {
            return;
    }
    model()->deleteSong(modelIndex());
}


void SongWidget::dataChanged(const QModelIndex &left, const QModelIndex &right)
{


   if(!left.isValid() || !right.isValid()){
      return;
   }

   for(int i = left.column(); i <= right.column(); i++){
      switch (i){
         case AbstractTreeItem::NAME:
            mp_SongTitleWidget->setTitle(left.sibling(left.row(), i).data().toString());
            break;
         case AbstractTreeItem::TEMPO:
            emit sigDefaultTempoChangedByModel(left.sibling(left.row(), i).data().toInt());
            break;
      case AbstractTreeItem::AUTOPILOT_ON:
         emit sigDefaultAutoPilotEnabledChangedByModel(left.sibling(left.row(), i).data().toBool());
         break;
         case AbstractTreeItem::SAVE:
            mp_SongTitleWidget->setUnsavedChanges(left.sibling(left.row(), AbstractTreeItem::SAVE).data().toBool());
            break;
         case AbstractTreeItem::PLAYING:
            if (left.sibling(left.row(), i).data().toBool() == true) {
               setEnabled(false);
            } else {
               setEnabled(true);
            }
            break;
         case AbstractTreeItem::DEFAULT_DRM:
            mp_SongTitleWidget->slotDefaultDrmChangeByModel(left.sibling(left.row(), i));
            break;
         default:
            //Do nothing
            break;
      }
   }
}

int SongWidget::headerColumnWidth(int columnIndex)
{
   if(mp_ChildrenItems->count() <= 0){
      return 0;
   }

   if (columnIndex == 0){
      return mp_MoveHandleWidget->width() +
            mp_SongTitleWidget->width() +
            mp_ChildrenItems->at(0)->headerColumnWidth(columnIndex) +
            1; // Hack to improve header apearance
   }
   if (columnIndex == 5){
      return mp_DeleteHandleWidget->width() + mp_ChildrenItems->at(0)->headerColumnWidth(columnIndex);
   }

   return mp_ChildrenItems->at(0)->headerColumnWidth(columnIndex);
}


void SongWidget::rowsInserted(int start, int end)
{
   if(start == 0 || end == mp_NewPartWidgets->size() + 1){
      // Cannot add a new intro or outro
      return;
   }

   // Add all items from model
   // For now, we only read the immediate children
   SongPartWidget *p_SongPartWidget;
   NewPartWidget *p_NewPartWidget;

   for(int i = start; i <= end; i++){
      QModelIndex childIndex = model()->index(i, 0, modelIndex()); // NOTE: Widgets are associated to column 0

      if (childIndex.isValid()){
         // Add a new part widget at the end
         p_NewPartWidget = new NewPartWidget(this,mp_ChildrenItems->size());
         connect(p_NewPartWidget, SIGNAL(sigAddPartToRow(int)), this, SLOT(slotInsertPart(int)));
         mp_NewPartWidgets->append(p_NewPartWidget);
         p_NewPartWidget->show();

         QString path =  model()->index(modelIndex().row(), AbstractTreeItem::ABSOLUTE_PATH, modelIndex().parent()).data().toString();
         p_SongPartWidget = new SongPartWidget(model(), this, path);
         p_SongPartWidget->populate(childIndex);
         mp_ChildrenItems->insert(i, p_SongPartWidget);
         mp_SongPartItems->insert(i,p_SongPartWidget);
         p_SongPartWidget->show();

         connect(p_SongPartWidget, SIGNAL(sigSubWidgetClicked(QModelIndex)), this, SLOT(slotSubWidgetClicked(QModelIndex)));
         connect(p_SongPartWidget, &SongPartWidget::sigSelectTrack, this, &SongWidget::slotSelectTrack);
         connect(p_SongPartWidget, &SongPartWidget::sigUpdateAP, this, &SongWidget::slotAPUpdate);
      }
   }

   // Make sure all new song widgets are on top
   for(int i = 0; i < mp_NewPartWidgets->size(); i++){
      mp_NewPartWidgets->at(i)->raise();
   }

   // Determine if new part widgets can be enabled
   int maxChildrenCnt = modelIndex().sibling(modelIndex().row(), AbstractTreeItem::MAX_CHILD_CNT).data().toInt();

   if(model()->rowCount(modelIndex()) >= maxChildrenCnt){
      for(int i = 0; i < mp_NewPartWidgets->count(); i++){
         mp_NewPartWidgets->at(i)->setEnabled(false);
      }
   }
   UpdateAP();
   updateOrderSlots();

}

void SongWidget::rowsRemoved(int start, int end)
{
   // remove all widgets corresponding to rows
   SongFolderViewItem *p_SongPart;
   NewPartWidget *p_NewPartWidget;

   for(int i = end; i >= start; i--){
      p_SongPart = mp_ChildrenItems->at(i);

      mp_ChildrenItems->removeAt(i);
      if(i == mp_SongPartItems->size()-2){//if the last part(excluding outro) was deleted
          mp_SongPartItems->at(i-1)->setLast(true);
      }
      mp_SongPartItems->at(i)->readUpdatePartName('D');
      mp_SongPartItems->removeAt(i);
      delete p_SongPart;

      p_NewPartWidget = mp_NewPartWidgets->last();
      mp_NewPartWidgets->removeLast();
      delete p_NewPartWidget;
   }

   // Make sure all new song widgets are on top
   for(int i = 0; i < mp_NewPartWidgets->size(); i++){
      mp_NewPartWidgets->at(i)->raise();
   }

   // Determine if new part widgets can be enabled
   int maxChildrenCnt = modelIndex().sibling(modelIndex().row(), AbstractTreeItem::MAX_CHILD_CNT).data().toInt();

   // Note, the row count is the count before removal
   if(model()->rowCount(modelIndex()) <= maxChildrenCnt){
      for(int i = 0; i < mp_NewPartWidgets->count(); i++){
         mp_NewPartWidgets->at(i)->setEnabled(true);
      }
   }
   UpdateAP();
   updateOrderSlots();
}


void SongWidget::slotIsFirst(bool first)
{
   emit sigIsFirst(first);
}

void SongWidget::slotIsLast(bool last)
{
   emit sigIsLast(last);
}

void SongWidget::slotOrderChanged(int number)
{
   mp_MoveHandleWidget->setLabelText(QString::number(number));
}

/**
 * @brief SongWidget::slotTitleChangeByUI
 * @param title string containing the new title edited by the UI
 */
void SongWidget::slotTitleChangeByUI(const QString &title)
{

   if(title.isEmpty()){
      mp_SongTitleWidget->setTitle(modelIndex().data().toString());
      qWarning() << "SongWidget::slotTitleChangeByUI - WARNING - title.isEmpty()";
      return;
   }

   if(title.compare(modelIndex().data().toString(), Qt::CaseSensitive) == 0){
      qDebug() << "SongWidget::slotTitleChangeByUI - DEBUG - If no change";
      return;
   }

   if(title.compare(modelIndex().data().toString(), Qt::CaseInsensitive) == 0){
      model()->setData(modelIndex(), QVariant(title));
      return;
   }

   model()->setData(modelIndex(), QVariant(title));
}

/**
 * @brief SongWidget::slotNumberChangeByUI
 * @param num containing the new number edited by the UI
 */
void SongWidget::slotNumberChangeByUI(const QString &num)
{
   qDebug() << "num changed:" << num;
   model()->setData(modelIndex().sibling(modelIndex().row(), AbstractTreeItem::LOOP_COUNT), num);
}

void SongWidget::slotAPEnableChangeByUI(const bool state)
{
    QModelIndex songAPEnable = model()->index(modelIndex().row(), AbstractTreeItem::AUTOPILOT_ON, modelIndex().parent());

    if(model()->data(songAPEnable).toBool() != state){
       model()->setData(songAPEnable, QVariant(state));
        UpdateAP();
    }
}

void SongWidget::slotAPStateRequested()
{
    emit sigAPUpdate(model()->data(modelIndex().sibling(modelIndex().row(), AbstractTreeItem::AUTOPILOT_ON)).toBool());
}

void SongWidget::slotAPUpdate()
{
    UpdateAP();
}
void SongWidget::slotSawapPart(int start, int end){

    mp_SongPartItems->swap(start,end);
}

void SongWidget::slotMoveSongUpClicked()
{
    model()->moveItem(modelIndex(), -1);
    UpdateAP();
}

void SongWidget::slotMoveSongDownClicked()
{
    model()->moveItem(modelIndex(), 1);
    UpdateAP();
}

void SongWidget::rowsMovedGet(int start, int end, QList<SongFolderViewItem *> *p_List)
{
   for(int i = start; i <= end; i++){
      p_List->append(mp_ChildrenItems->at(i));
   }
}

void SongWidget::rowsMovedInsert(int start, QList<SongFolderViewItem *> *p_List)
{
   for(int i = 0; i < p_List->count(); i++){
      mp_ChildrenItems->insert(start++, p_List->at(i));
      mp_SongPartItems->insert(start++,(SongPartWidget*)p_List->at(i));//to do this might be inserting in the wrong place, better check this out
      if(p_List->at(i)->parent() != this){
         p_List->at(i)->setParent(this);
      }
   }

   updateOrderSlots();
}

void SongWidget::rowsMovedRemove(int start, int end)
{
   for(int i = end; i >= start; i--){
      mp_ChildrenItems->removeAt(i);
      mp_SongPartItems->removeAt(i);
   }

   updateOrderSlots();
}

void SongWidget::updateOrderSlots()
{
   // Update is first, is last, is alone and OrderChanged slots
   for(int i = 1; i < mp_ChildrenItems->count() - 1; i++){
      static_cast<SongPartWidget *>(mp_ChildrenItems->at(i))->slotIsFirst(i == 1);
      static_cast<SongPartWidget *>(mp_ChildrenItems->at(i))->slotIsLast(i == mp_ChildrenItems->count() - 2);
      static_cast<SongPartWidget *>(mp_ChildrenItems->at(i))->slotIsAlone(mp_ChildrenItems->count() <= 3);
      static_cast<SongPartWidget *>(mp_ChildrenItems->at(i))->slotOrderChanged(i);
   }
}

void SongWidget::slotSelectTrack(const QByteArray &trackData, int trackIndex, int typeId, int partIndex)
{
  emit sigSelectTrack(trackData, trackIndex, typeId, partIndex);
}

void SongWidget::UpdateAP()
{
    auto size = mp_SongPartItems->size()-1;
    //this next part updates the AP layout for each beat
    for(int i = 1; i < size;i++){//starts on 1 to exclude intro
        mp_SongPartItems->at(i)->parentAPBoxStatusChanged();
    }
    for(int i = 1; i < size-1;i++){//size-1 to avoid last part
     mp_SongPartItems->at(i)->setLast(false);
     mp_SongPartItems->at(i)->updateTransMain(false);
    }
    //handle outro interaction
    mp_SongPartItems->at(size-1)->setLast(true);
    bool hasOutro = !(mp_SongPartItems->at(size)->getChildItemAt(1)->isPartEmpty());
    mp_SongPartItems->at(size-1)->updateTransMain(hasOutro);
}
