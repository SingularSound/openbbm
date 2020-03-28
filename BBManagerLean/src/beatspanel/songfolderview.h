#ifndef SONGFOLDERVIEW_H
#define SONGFOLDERVIEW_H

#include <QAbstractItemView>
#include <QHeaderView>
#include <QSet>

#include <model/tree/project/beatsprojectmodel.h>

class SongFolderViewItem;
class NewSongWidget;

class SongFolderView : public QAbstractItemView
{
   Q_OBJECT
public:
   // Mandatory Constructor
   explicit SongFolderView(QWidget *parent = nullptr);
   ~SongFolderView();

   // Mandatory Re-declaration of purely abstract QAbstractItemView methods
   QModelIndex indexAt(const QPoint &point) const;
   void scrollTo(const QModelIndex &index, ScrollHint hint = EnsureVisible);
   QRect visualRect(const QModelIndex &index) const;
   inline BeatsProjectModel* model() const { return static_cast<BeatsProjectModel*>(QAbstractItemView::model()); }

   // Optional Re-declaration of virtual QAbstractItemView methods
   void setModel(QAbstractItemModel * model);
   void updateLayout(QPoint topLeftCorner);
   void setSongsEnabled(bool enabled);

protected:
   // Mandatory Re-declaration of purely abstract QAbstractItemView methods
   int horizontalOffset() const;
   bool isIndexHidden( const QModelIndex &index ) const;
   QModelIndex moveCursor( CursorAction cursorAction, Qt::KeyboardModifiers modifiers );
   void setSelection( const QRect &rect, QItemSelectionModel::SelectionFlags flags );
   int verticalOffset() const;
   QRegion visualRegionForSelection( const QItemSelection &selection ) const;
   void resizeEvent(QResizeEvent *event);


   void wheelEvent ( QWheelEvent * event );
   void scrollContentsBy(int dx, int dy);
signals:
   void rootIndexChanged(const QModelIndex & index);
   void sigSelectTrack(const QByteArray &trackData, int trackIndex, int typeId, int partIndex);
   void sigSetTitle(const QString &title);

public slots:
   virtual void setRootIndex(const QModelIndex & index);
   void slotSetPlayerEnabled(bool enabled);
   void import(const QModelIndex& index, int preferred = 0, const QStringList& files = QStringList()); // 0 - no matter, > 0 - dirs, < 0 - files

protected slots:
   // Optional Re-declaration of virtual QAbstractItemView slots
   void dataChanged(const QModelIndex & topLeft, const QModelIndex & bottomRight, const QVector<int> & roles = QVector<int> ());
   void rowsAboutToBeRemoved(const QModelIndex & parent, int start, int end);
   void rowsInserted(const QModelIndex &parent, int start, int end);
   void selectionChanged(const QItemSelection &selected, const QItemSelection &deselected);
   void currentChanged(const QModelIndex & current, const QModelIndex & previous);
   void updateGeometries();

   // Custom slots
   void slotInsertSong(NewSongWidget* w, bool import);
   void slotSubWidgetClicked(const QModelIndex &index);
   void slotLayoutChanged(const QList<QPersistentModelIndex> & parents = QList<QPersistentModelIndex> (), QAbstractItemModel::LayoutChangeHint hint = QAbstractItemModel::NoLayoutChangeHint);
   void slotRowsMoved(const QModelIndex & sourceParent, int sourceStart, int sourceEnd, const QModelIndex & destinationParent, int destinationRow);
   void slotSelectTrack(const QByteArray &trackData, int trackIndex, int typeId, int partIndex);

private:
   // Custom method for custom behavior
   SongFolderViewItem * itemForIndex(const QModelIndex &index) const;
   QModelIndexList pathToIndex(const QModelIndex &index ) const;

   void insertRowsInternal(int start, int end, bool modal = false);
   void removeRowsInternal(int start, int end);
   void rowsMovedGetInternal(int start, int end, QList<SongFolderViewItem *> *p_List);
   void rowsMovedInsertInternal(int start, QList<SongFolderViewItem *> *p_List);
   void rowsMovedRemoveInternal(int start, int end);
   void dataChangedInternal(const QModelIndex &topLeft, const QModelIndex &bottomRight);

   void updateOrderSlots();

   QSize m_ContentSize;

   // Custom members
   QList<SongFolderViewItem*> *mp_ChildrenItems;
   QList<NewSongWidget*> *mp_NewSongWidgets;
   QHeaderView *mp_Header;
};

#endif // SONGFOLDERVIEW_H
