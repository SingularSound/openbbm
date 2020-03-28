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
