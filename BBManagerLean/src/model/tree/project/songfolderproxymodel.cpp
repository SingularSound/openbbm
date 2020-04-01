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
#include "songfolderproxymodel.h"
#include "../abstracttreeitem.h"
#include "contentfoldertreeitem.h"
#include "beatsprojectmodel.h"
#include <QSet>
#include <QMimeData>
#include <QDataStream>

SongFolderProxyModel::SongFolderProxyModel(QObject *parent) :
   QIdentityProxyModel(parent)
{
   m_virtualRoot = QModelIndex();
   m_maxDepth    = 0;
}


int SongFolderProxyModel::columnCount(const QModelIndex & /*parent*/) const
{
   return 1; // (was 1) I couldn't delete columns so I make them all invisible, except name and midiId (loopCount) - 20 Causes de the app to crash on Song Switch
}

bool SongFolderProxyModel::hasChildren(const QModelIndex & parent) const
{
   if(m_maxDepth <= 0){
      return QIdentityProxyModel::hasChildren(parent);
   }

   // try to find root in parent withing m_maxDepth
   QModelIndex tempParent = parent;
   for(int i = 0; i < m_maxDepth; i++){
      if(!tempParent.isValid()){
         return QIdentityProxyModel::hasChildren(parent);
      }
      tempParent = tempParent.parent();
   }

   return false;
}

QModelIndex	SongFolderProxyModel::index(int row, int column, const QModelIndex & parent) const
{
   if(column > 1 && column != AbstractTreeItem::LOOP_COUNT){
      return QModelIndex();
   }

   if(!hasChildren(parent)){
      return QModelIndex();
   }
   return QIdentityProxyModel::index(row, column, parent);
}

Qt::ItemFlags SongFolderProxyModel::flags(const QModelIndex & index) const
{

   Qt::ItemFlags ret = QIdentityProxyModel::flags(index);

    if (index.parent().parent() != QModelIndex())
        ret &= ~Qt::ItemIsDropEnabled;
    return ret;
}

QVariant SongFolderProxyModel::data(const QModelIndex & index, int role) const
{
   if(index.isValid() && role == Qt::ToolTipRole){
      return QVariant(tr("Double click on selected to rename\nfolder or set/change MIDI Id\nDrag to move items around"));
   }

   QVariant out;
   out = QIdentityProxyModel::data(index, role);

   return out;
}

bool SongFolderProxyModel::setData(const QModelIndex & index, const QVariant & value, int role)
{
   bool ret = QIdentityProxyModel::setData(index, value, role);
   return ret;
}

Qt::DropActions SongFolderProxyModel::supportedDragActions() const
{
    Qt::DropActions ret = QIdentityProxyModel::supportedDragActions();
    return Qt::TargetMoveAction | Qt::CopyAction;
}

Qt::DropActions SongFolderProxyModel::supportedDropActions() const
{
    Qt::DropActions ret = QIdentityProxyModel::supportedDropActions();
    return Qt::TargetMoveAction | Qt::CopyAction;
}

QMimeData* SongFolderProxyModel::mimeData(const QModelIndexList &indexes) const
{
    if (indexes.size() != 1)
        return nullptr;
    QMimeData* ret = QIdentityProxyModel::mimeData(indexes);
    ret->setProperty("index", indexes[0]);
    return ret;
}

bool SongFolderProxyModel::dropMimeData(const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex &parent)
{
    if (column && (row != column || row+1))
        return false;
    else if (action == Qt::IgnoreAction)
        return true;
    QModelIndex index = data->property("index").toModelIndex();
    if (row+1);
    else if (index.parent() == parent)
        return false;
    QModelIndex p = index.parent();
    if (action == Qt::MoveAction); else // XXX : remove this after QTreeView dragging is fixed.
    if (p == parent) {
        if (action != Qt::MoveAction) {
            // move/copy inside a single folder
            return ((BeatsProjectModel*)sourceModel())->moveItem(index, row-(index.row()<row)-index.row());
        } else {
            return ((BeatsProjectModel*)sourceModel())->moveOrCopySong(false, index, index.parent(), row-(index.row()<row)-index.row());
        }
    } else if (p == parent.parent()) {
        // appending folder inside other folder attempt => turn to move
        if (action != Qt::MoveAction) {
            return ((BeatsProjectModel*)sourceModel())->moveItem(index, parent.row()-index.row());
        } 
    } else if ((p = mapToSource(parent)) != ((BeatsProjectModel*)sourceModel())->songsFolderIndex()) {
        // moving/copying file across folders
        return ((BeatsProjectModel*)sourceModel())->moveOrCopySong(action != Qt::MoveAction, index, p, row);
    }
    return false;
}
