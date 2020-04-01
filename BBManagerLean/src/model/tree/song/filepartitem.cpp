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
#include "filepartitem.h"
#include "../../filegraph/abstractfilepartmodel.h"

FilePartItem::FilePartItem(AbstractFilePartModel * filePart, AbstractTreeItem *parent):
   AbstractTreeItem(parent->model(), parent),
   mp_FilePart(filePart)
{
   
}

FilePartItem::~FilePartItem()
{
   // NOTE: mp_FilePart needs to be deleted separately
}

QVariant FilePartItem::data(int column)
{
    switch (column) {
    case NAME:
        if (mp_FilePart->name().compare(mp_FilePart->defaultName()) == 0) {
            return "";
        }
        return mp_FilePart->name();
    default:
        return AbstractTreeItem::data(column);
    }
}

bool FilePartItem::setData(int column, const QVariant & value)
{
    switch (column) {
    case NAME:
        mp_FilePart->setName(value.toString());
        return true;
    default:
        return false;
    }
}
