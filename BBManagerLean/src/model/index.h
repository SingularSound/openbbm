#ifndef __INDEX_H__
#define __INDEX_H__

#ifdef _MSC_VER
#   pragma once
#endif

#include <QList>

// item deletion/creation-persistant int-based index for any model
//
// points to the "spot" (or a position in a model tree) rather than to an item itself
//
// This convenience comes with the drawback - it's slow,
// as we must find an item right from the model's root every time we need it
//
// The deeper the model is - the more performance suffers
//
// The drawback is acceptable, as we use this index for undo/redo operations,
// and they are neither being performed very often nor are time critical
class Index
{
    QList<int> m_path;
    int m_col;

public:
    Index() : m_col(0) {}
    Index(const class QModelIndex& ix);
    Index(const class QPersistentModelIndex& ix);
    Index(int x, int col) : m_col(col) { m_path.push_back(x); }
    Index(int x, int y, int col) : m_col(col) { m_path.push_back(x); m_path.push_back(y); }
    Index(int x, int y, int u, int col) : m_col(col) { m_path.push_back(x); m_path.push_back(y); m_path.push_back(u); }

    Index parent() const { Index ret = *this; if (!ret.m_path.isEmpty()) { ret.m_path.pop_back(); } ret.m_col = 0; return ret; }
    Index child(int row, int col = 0) const { Index ret = *this; ret.m_path.push_back(row); ret.m_col = col; return ret; }

    operator int() const { return m_path.size(); }

    bool operator==(const Index& other) const { return m_col == other.m_col && m_path == other.m_path; }
    bool operator!=(const Index& other) const { return m_col != other.m_col || m_path != other.m_path; }

    QModelIndex operator()(const class QAbstractItemModel* model) const;

    int operator()() const { return m_col; }
    int operator[](int i) const {
        if (i >= 0 && i < m_path.size())
            return m_path[i];
        else if (i < 0 && i >= -m_path.size())
            return m_path[m_path.size()+i];
        else {
            qFatal("Index[] : out of bounds");
            return 0;
        }
    }
};

#endif  __INDEX_H__