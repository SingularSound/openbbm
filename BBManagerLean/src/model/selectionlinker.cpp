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
#include "selectionlinker.h"
#include <QTimer>
#include "tree/project/beatsprojectmodel.h"

SelectionLinker::SelectionLinker(QAbstractProxyModel *proxyModel, QItemSelectionModel *sourceItemSelectionModel, QObject *parent)
    : QItemSelectionModel(proxyModel, parent)
    , m_sourceItemSelectionModel(sourceItemSelectionModel)
    , m_folder(0)
    , m_song(0)
{
   m_ignoreCurrentChanged = false;
   m_sourceModel = sourceItemSelectionModel->model();
   m_proxyModel = proxyModel;

   connect(m_sourceItemSelectionModel, SIGNAL(selectionChanged(QItemSelection,QItemSelection)), this, SLOT(sourceSelectionChanged(QItemSelection,QItemSelection)));
   connect(m_sourceItemSelectionModel, SIGNAL(currentChanged(QModelIndex,QModelIndex)), this, SLOT(sourceCurrentChanged(QModelIndex)));
   connect(this, SIGNAL(currentChanged(QModelIndex,QModelIndex)), this, SLOT(slotProxyCurrentChanged(QModelIndex)));
   connect(this, SIGNAL(selectionChanged(QItemSelection,QItemSelection)), this, SLOT(slotProxySelectionChanged(QItemSelection,QItemSelection)));
}
void SelectionLinker::select(const QModelIndex &index, QItemSelectionModel::SelectionFlags command)
{
   // When an item is removed, the current index is set to the top index in the model.
   // That causes a selectionChanged signal with a selection which we do not want.
   if (m_ignoreCurrentChanged) {
      return;
   }

   QItemSelectionModel::select(QItemSelection(index, index), command);
   if (index.isValid())
      m_sourceItemSelectionModel->select(mapSelectionProxyToSource(QItemSelection(index, index)), command);
   else {
      m_sourceItemSelectionModel->clearSelection();
   }
}

void SelectionLinker::select(const QItemSelection &selection, QItemSelectionModel::SelectionFlags command)
{

   m_ignoreCurrentChanged = true;

   QItemSelectionModel::select(selection, command);
   Q_ASSERT(assertSelectionValid(selection));
   QItemSelection mappedSelection = mapSelectionProxyToSource(selection);
   Q_ASSERT(assertSelectionValid(mappedSelection));
   m_sourceItemSelectionModel->select(mappedSelection, command);

   m_ignoreCurrentChanged = false;
}

void SelectionLinker::slotProxySelectionChanged(const QItemSelection& selected, const QItemSelection& deselected)
{
   Q_ASSERT(assertSelectionValid(selected));
   Q_ASSERT(assertSelectionValid(deselected));
   const QItemSelection mappedDeselection = mapSelectionProxyToSource(deselected);
   const QItemSelection mappedSelection = mapSelectionProxyToSource(selected);

   m_sourceItemSelectionModel->select(mappedDeselection, QItemSelectionModel::Deselect);
   m_sourceItemSelectionModel->select(mappedSelection, QItemSelectionModel::Select);
   QTimer::singleShot(0, this, SIGNAL(ensureSongSelected()));
}

void SelectionLinker::slotProxyCurrentChanged(const QModelIndex& current)
{
   const QModelIndex mappedCurrent = mapProxyToSource(current);
   if (!mappedCurrent.isValid()) {
      return;
   }
   m_sourceItemSelectionModel->setCurrentIndex(mappedCurrent, QItemSelectionModel::NoUpdate);
}

void SelectionLinker::sourceSelectionChanged(const QItemSelection& selected, const QItemSelection& deselected)
{
   Q_ASSERT(assertSelectionValid(selected));
   Q_ASSERT(assertSelectionValid(deselected));
   const QItemSelection mappedDeselection = mapSelectionSourceToProxy(deselected);
   const QItemSelection mappedSelection = mapSelectionSourceToProxy(selected);

   // TODO confirm that the change is intended
   select(mappedDeselection, QItemSelectionModel::Deselect);
   select(mappedSelection, QItemSelectionModel::Select);
   
}

void SelectionLinker::sourceCurrentChanged(const QModelIndex& current)
{
   const QModelIndex mappedCurrent = mapSourceToProxy(current);
   if (!mappedCurrent.isValid()) {
      return;
   }
   setCurrentIndex(mappedCurrent, QItemSelectionModel::NoUpdate);
}

void SelectionLinker::ensureSongSelected()
{
    auto x = selection().indexes();
    if (x.empty()) {
        auto SONGS = model()->index(3, 0);
        auto folder = SONGS.child(m_folder, 0);
        if (folder == QModelIndex() && (folder = SONGS.child(m_folder = model()->rowCount(SONGS), 0)) == QModelIndex())
            return;
        auto song = folder.child(m_song, 0);
        if (song == QModelIndex() && (song = folder.child(m_song = model()->rowCount(folder), 0)) == QModelIndex())
            return;
        BlockUndoForSelectionChangesFurtherInThisScope _;
        setCurrentIndex(song, QItemSelectionModel::SelectCurrent);
    }
    else if (x.size() == 1) {
        auto SONGS = model()->index(3, 0);
        auto sel = x.first();
        auto parent = sel.parent();
        if (parent == SONGS) {
            m_folder = sel.row();
            m_song = 0;
        } else if (parent.parent() == SONGS) {
            m_folder = parent.row();
            m_song = sel.row();
        }
    }
}

QModelIndex SelectionLinker::mapSourceToProxy(const QModelIndex& index) const
{
   const QItemSelection selection = mapSelectionSourceToProxy(QItemSelection(index, index));
   if (selection.isEmpty() || selection.indexes().isEmpty())
      return QModelIndex();

   return selection.indexes().first();
}

QModelIndex SelectionLinker::mapProxyToSource(const QModelIndex& index) const
{
   const QItemSelection selection = mapSelectionProxyToSource(QItemSelection(index, index));
   if (selection.isEmpty() || selection.indexes().isEmpty())
      return QModelIndex();

   return selection.indexes().first();
}
QItemSelection SelectionLinker::mapSelectionSourceToProxy(const QItemSelection& selection) const
{
   if (selection.isEmpty())
      return QItemSelection();

   if (selection.first().model() != m_sourceModel)
      qDebug() << "FAIL" << selection.first().model() << m_sourceModel << m_proxyModel;
   Q_ASSERT(selection.first().model() == m_sourceModel);

   Q_ASSERT(assertSelectionValid(selection));
   QItemSelection proxySelection = m_proxyModel->mapSelectionFromSource(selection);
   Q_ASSERT(assertSelectionValid(proxySelection));

   Q_ASSERT( ( !proxySelection.isEmpty() && proxySelection.first().model() == m_proxyModel ) || true );
   return proxySelection;
}

QItemSelection SelectionLinker::mapSelectionProxyToSource(const QItemSelection& selection) const
{
   if (selection.isEmpty())
      return QItemSelection();

   if (selection.first().model() != m_proxyModel)
      qDebug() << "FAIL" << selection.first().model() << m_sourceModel << m_proxyModel;
   Q_ASSERT(selection.first().model() == m_proxyModel);

   Q_ASSERT(assertSelectionValid(selection));
   QItemSelection sourceSelection = m_proxyModel->mapSelectionToSource(selection);
   Q_ASSERT(assertSelectionValid(sourceSelection));

   Q_ASSERT( ( !sourceSelection.isEmpty() && sourceSelection.first().model() == m_sourceModel ) || true );
   return sourceSelection;
}

