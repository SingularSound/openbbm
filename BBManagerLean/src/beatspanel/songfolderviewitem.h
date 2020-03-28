#ifndef SONGFOLDERVIEWITEM_H
#define SONGFOLDERVIEWITEM_H

#include <QWidget>
#include <QList>
#include <QModelIndex>
#include <QDebug>
#include <QDir>

/**
 * @brief The SongFolderViewItem class
 *
 * The main purpose of this class is to manage Tree View common recursive functions
 *
 */
class SongFolderViewItem : public QWidget
{
   Q_OBJECT
public:
   explicit SongFolderViewItem(class BeatsProjectModel* p_Model, QWidget* parent = nullptr);
   ~SongFolderViewItem();

   QRect visualRect(const QModelIndexList &modelIndexPath, int currentDepth) const;
   SongFolderViewItem * itemForIndex(const QModelIndex &index) const;
   QModelIndex indexAt(const QPoint &point);
   void childrenDataChanged(const QModelIndexList &modelIndexPath, const QModelIndex &topLeft, const QModelIndex &bottomRight, int currentDepth);
   void childrenRowsInserted(const QModelIndexList &modelIndexPath, int start, int end, int currentDepth);
   void childrenRowsRemoved(const QModelIndexList &modelIndexPath, int start, int end, int currentDepth);
   void childrenRowsMovedGet(const QModelIndexList &modelIndexPath, int start, int end, QList<SongFolderViewItem *> *p_List, int currentDepth);
   void childrenRowsMovedInsert(const QModelIndexList &modelIndexPath, int start, QList<SongFolderViewItem *> *p_List, int currentDepth);
   void childrenRowsMovedRemove(const QModelIndexList &modelIndexPath, int start, int end, int currentDepth);


   // Accessors
   QModelIndex modelIndex();
   BeatsProjectModel* model() { return mp_Model; }

   virtual void populate(QModelIndex const& modelIndex) = 0;
   virtual void updateLayout() = 0;
   virtual void dataChanged(const QModelIndex &left, const QModelIndex &right) = 0;
   // Hack for header column width
   virtual int headerColumnWidth(int columnIndex) = 0;
   virtual void updateMinimumSize() = 0;

signals:
   void sigSubWidgetClicked(const QModelIndex &index);

public slots:
   virtual void setSelected(bool selected);
   virtual void setCurrentIndex(bool selected);
   void slotSubWidgetClicked();
   void slotSubWidgetClicked(const QModelIndex &index);
   virtual void slotSetPlayerEnabled(bool enabled);

protected:

   void setModelIndex(const QModelIndex & modelIndex);
   virtual void rowsInserted(int start, int end);
   virtual void rowsRemoved(int start, int end);
   virtual void rowsMovedGet(int start, int end, QList<SongFolderViewItem *> *p_List);
   virtual void rowsMovedInsert(int start, QList<SongFolderViewItem *> *p_List);
   virtual void rowsMovedRemove(int start, int end);


   QDir copyClipboardDir();
   QDir pasteClipboardDir();
   QDir dragClipboardDir();
   QDir dropClipboardDir();


   QList<SongFolderViewItem*> *mp_ChildrenItems;
   bool m_CurrentIndex;

private:
   BeatsProjectModel* mp_Model;

   QPersistentModelIndex m_ModelIndex;
};

#endif // SONGFOLDERVIEWITEM_H
