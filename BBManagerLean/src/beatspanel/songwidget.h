#ifndef SONGWIDGET_H
#define SONGWIDGET_H

#include <QWidget>
#include <QLabel>
#include <QModelIndex>
#include <QSet>

#include "newpartwidget.h"
#include "songfolderviewitem.h"
#include "songpartwidget.h"

class MoveHandleWidget;
class DeleteHandleWidget;
class SongTitleWidget;

/**
 * @brief The SongWidget class
 */
class SongWidget : public SongFolderViewItem
{
   Q_OBJECT
public:
   explicit SongWidget(BeatsProjectModel *p_Model, QWidget *parent = nullptr);
   ~SongWidget();
   void setSongTitle(QString const& title);

   void populate(QModelIndex const& modelIndex);
   void updateLayout();
   void dataChanged(const QModelIndex &left, const QModelIndex &right);
   void UpdateAP();

   // Hack for header column width
   int headerColumnWidth(int columnIndex);

signals:
   void sigIsFirst(bool first);
   void sigIsLast(bool last);
   void sigDefaultTempoChangedByModel(int tempo);
   void sigDefaultAutoPilotEnabledChangedByModel(bool state);
   void sigSelectTrack(const QByteArray &trackData, int trackIndex, int typeId, int partIndex);
   void sigAPUpdate(bool state);

public slots:
   void slotIsFirst(bool first);
   void slotIsLast(bool last);
   void slotOrderChanged(int number);
   void setSelected(bool selected);
   void setCurrentIndex(bool selected);
   void slotDefaultTempoChangedByUI(int tempo);
   void slotInsertPart( int row );
   void slotDeleteButton();
   void slotTitleChangeByUI(const QString &title);
   void slotNumberChangeByUI(const QString &num);
   void slotMoveSongUpClicked();
   void slotMoveSongDownClicked();
   void updateMinimumSize();
   void slotSelectTrack(const QByteArray &trackData, int trackIndex, int typeId, int partIndex);
   void slotDefaultDrmChangedByUI(const QString &drmName, const QString &drmFileName);
   void slotAPEnableChangeByUI(const bool state);
   void slotAPStateRequested();
   void slotAPUpdate();
   void slotSawapPart(int start, int end);

protected:
   virtual void paintEvent(QPaintEvent * event);
   virtual void rowsInserted(int start, int end);
   virtual void rowsRemoved(int start, int end);
   virtual void rowsMovedGet(int start, int end, QList<SongFolderViewItem *> *p_List);
   virtual void rowsMovedInsert(int start, QList<SongFolderViewItem *> *p_List);
   virtual void rowsMovedRemove(int start, int end);

private:
   void updateOrderSlots();
   int m_MaxChildrenWidth;

   SongTitleWidget *mp_SongTitleWidget;
   MoveHandleWidget *mp_MoveHandleWidget;
   DeleteHandleWidget *mp_DeleteHandleWidget;
   QList<NewPartWidget*> *mp_NewPartWidgets;
   QList<SongPartWidget*> *mp_SongPartItems;

};

#endif // SONGWIDGET_H
