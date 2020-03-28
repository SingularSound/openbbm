#ifndef SONGFOLDERPROXYMODEL_H
#define SONGFOLDERPROXYMODEL_H

#include <QIdentityProxyModel>

class SongFolderProxyModel : public QIdentityProxyModel
{
   Q_OBJECT
public:
   explicit SongFolderProxyModel(QObject *parent = nullptr);

   int columnCount(const QModelIndex & parent = QModelIndex()) const;
   bool hasChildren(const QModelIndex & parent = QModelIndex()) const;
   QModelIndex	index(int row, int column, const QModelIndex & parent = QModelIndex()) const;
   Qt::ItemFlags flags(const QModelIndex & index) const;
   QVariant data(const QModelIndex & index, int role = Qt::DisplayRole) const;
   bool setData(const QModelIndex & index, const QVariant & value, int role = Qt::EditRole);

   inline void setMaxDepth(int maxDepth){m_maxDepth = maxDepth;}
   inline int maxDepth(){return m_maxDepth;}

    Qt::DropActions supportedDragActions() const;
    Qt::DropActions supportedDropActions() const;

    QMimeData *mimeData(const QModelIndexList &indexes) const;
    bool dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent);

protected:

private:
   int m_maxDepth;
   QPersistentModelIndex m_virtualRoot;


};

#endif // SONGFOLDERPROXYMODEL_H
