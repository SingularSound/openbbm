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
#include <QList>
#include <QVariant>

#include "treeitem.h"

TreeItem::TreeItem(const QList<QVariant> &data, TreeItem *parent)
{
    parentItem = parent;
    itemData = data;
}

TreeItem::~TreeItem()
{
    qDeleteAll(childItems);
}

void TreeItem::appendChild(TreeItem *item)
{
    childItems.append(item);
}
TreeItem *TreeItem::child(int row)
{
    return childItems.value(row);
}
int TreeItem::childCount() const
{
    return childItems.count();
}
int TreeItem::row() const
{
    if (parentItem){
        return parentItem->childItems.indexOf(const_cast<TreeItem*>(this));
    }
    return 0;
}
int TreeItem::columnCount() const
{
    return itemData.count();
}
QVariant TreeItem::data(int column) const
{
    return itemData.value(column);
}
TreeItem *TreeItem::parent()
{
    return parentItem;
}
