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
