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
#include "index.h"

#include <QAbstractItemModel>

Index::Index(const QModelIndex& ix)
    : m_col(ix.column())
{
    for (auto x = ix; x != QModelIndex(); x = x.parent())
        m_path.push_front(x.row());
}
Index::Index(const QPersistentModelIndex& ix)
    : m_col(ix.column())
{
    for (QModelIndex x = ix; x != QModelIndex(); x = x.parent())
        m_path.push_front(x.row());
}

QModelIndex Index::operator()(const QAbstractItemModel* model) const
{
    QModelIndex ret;
    foreach (auto x, m_path) {
        if ((ret = model->index(x, 0, ret)) == QModelIndex()) {
            return ret;
        }
    }
    if (m_col) {
        ret = ret.sibling(ret.row(), m_col);
    }
    return ret;
}
