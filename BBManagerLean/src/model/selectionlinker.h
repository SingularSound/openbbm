#ifndef SELECTIONLINKER_H
#define SELECTIONLINKER_H

#include <QItemSelectionModel>
#include <QAbstractProxyModel>

#include <QAbstractItemModel>
#include <QWeakPointer>
#include <QAbstractProxyModel>
#include <QItemSelectionModel>
#include <QDebug>

class QAbstractItemModel;
class QModelIndex;
class QItemSelection;

class SelectionLinker : public QItemSelectionModel
{
   Q_OBJECT
public:
   SelectionLinker(QAbstractProxyModel *proxyModel, QItemSelectionModel *sourceItemSelectionModel, QObject *parent = nullptr);
   void select(const QModelIndex &index, QItemSelectionModel::SelectionFlags command);
   void select(const QItemSelection &selection, QItemSelectionModel::SelectionFlags command);


   QModelIndex mapSourceToProxy(const QModelIndex &index) const;
   QModelIndex mapProxyToSource(const QModelIndex &index) const;

   QItemSelection mapSelectionSourceToProxy(const QItemSelection &selection) const;
   QItemSelection mapSelectionProxyToSource(const QItemSelection &selection) const;

private slots:
   void sourceSelectionChanged(const QItemSelection &selected, const QItemSelection &deselected);
   void sourceCurrentChanged(const QModelIndex &current);
   void slotProxySelectionChanged(const QItemSelection &selected, const QItemSelection &deselected);
   void slotProxyCurrentChanged(const QModelIndex &current);
   void ensureSongSelected();

private:
   bool assertValid();

   bool assertSelectionValid(const QItemSelection &selection) const {
      foreach(const QItemSelectionRange &range, selection) {
         if (!range.isValid()) {
            qDebug() << selection << m_sourceModel << m_proxyModel;
         }
         Q_ASSERT(range.isValid());
      }
      return true;
   }

   QItemSelectionModel * const m_sourceItemSelectionModel;
   bool m_ignoreCurrentChanged;

   const QAbstractProxyModel * m_proxyModel;
   const QAbstractItemModel * m_sourceModel;
   int m_folder;
   int m_song;
};

#endif // SELECTIONLINKER_H
