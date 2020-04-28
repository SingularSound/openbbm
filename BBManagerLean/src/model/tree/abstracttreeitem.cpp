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

#include "project/beatsprojectmodel.h"
#include "abstracttreeitem.h"

AbstractTreeItem::AbstractTreeItem(BeatsProjectModel *p_model, AbstractTreeItem *parent) :
   QObject(nullptr) // Possible problems with destructor if parent is set in QObject
{
   mp_parentItem = parent;
   mp_Model = p_model;
}

AbstractTreeItem::~AbstractTreeItem()
{
   qDeleteAll(m_childItems);
}

AbstractTreeItem *AbstractTreeItem::child(int row) const
{
    return m_childItems.value(row);
}


int AbstractTreeItem::rowOfChild(AbstractTreeItem *p_Child)
{
   return m_childItems.indexOf(p_Child);
}

void AbstractTreeItem::appendChild(AbstractTreeItem *item)
{
    m_childItems.append(item);
}
void AbstractTreeItem::insertChildAt(int row, AbstractTreeItem *item)
{
    m_childItems.insert(row, item);
}


QVariant AbstractTreeItem::data(int column)
{
    switch (column)
    {
    case CPP_TYPE:
        return metaObject()->className();
    }
    return QVariant();
}

bool AbstractTreeItem::setData(int column, const QVariant& value)
{
    column; value;
    return false;
}

void AbstractTreeItem::insertNewChildAt(int /*row*/)
{
   qWarning() << "DEFAULT IMPLEMENTATION for " << metaObject()->className();
}

void AbstractTreeItem::removeChild(int row)
{
   qWarning() << "DEFAULT IMPLEMENTATION for " << metaObject()->className();
   removeChildInternal(row);
}
void AbstractTreeItem::moveChildren(int /*sourceFirst*/, int /*sourceLast*/, int /*delta*/)
{
   qWarning() << "DEFAULT IMPLEMENTATION in " << metaObject()->className();
}


void AbstractTreeItem::removeChildInternal(int row)
{
   AbstractTreeItem* p_Child = m_childItems.takeAt(row);
   delete p_Child;
}
int AbstractTreeItem::childCount() const
{
    return m_childItems.count();
}

int AbstractTreeItem::row() const
{
    if (mp_parentItem) {
        return mp_parentItem->m_childItems.indexOf(const_cast<AbstractTreeItem*>(this));
    }
    return 0;
}
AbstractTreeItem *AbstractTreeItem::parent() const
{
    return mp_parentItem;
}

BeatsProjectModel *AbstractTreeItem::model() const
{
   return mp_Model;
}


void AbstractTreeItem::computeHash(bool recursive)
{
   qWarning() << "DEFAULT IMPLEMENTATION in " << metaObject()->className();
   if(recursive){
      foreach(AbstractTreeItem* p_child, m_childItems){
         p_child->computeHash(true);
      }
   }
}

QByteArray AbstractTreeItem::hash()
{
   qWarning() << "DEFAULT IMPLEMENTATION in " << metaObject()->className();
   return "N/A"; // maybe this should be a 0 instead?
}

void AbstractTreeItem::setHash(const QByteArray & /*hash*/)
{
   qWarning() << "DEFAULT IMPLEMENTATION in " << metaObject()->className();
}

bool AbstractTreeItem::compareHash(const QString & /*path*/)
{
   qWarning() << "DEFAULT IMPLEMENTATION in " << metaObject()->className();
   return false;
}

void AbstractTreeItem::prepareSync(const QString & /*dstPath*/, QList<QString> * /*p_cleanUp*/, QList<QString> * /*p_copySrc*/, QList<QString> * /*p_copyDst*/)
{
   qWarning() << "DEFAULT IMPLEMENTATION in " << metaObject()->className();
}


void AbstractTreeItem::propagateHashChange()
{
   qDebug() << " (default implementation); propagating for " << data(NAME);

   if(parent()){

      parent()->computeHash(false);
      parent()->propagateHashChange();
   }
}

Qt::ItemFlags AbstractTreeItem::flags(int /*column*/)
{
   Qt::ItemFlags tempFlags = Qt::NoItemFlags;
   if(!model()->isSelectionDisabled()){
      tempFlags |= Qt::ItemIsSelectable | Qt::ItemIsEnabled;
   }
   return tempFlags;
}
