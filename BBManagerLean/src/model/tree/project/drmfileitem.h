#ifndef DRMFILEITEM_H
#define DRMFILEITEM_H

#include "../abstracttreeitem.h"

class DrmFolderTreeItem;

class DrmFileItem : public AbstractTreeItem
{
   Q_OBJECT
public:
   DrmFileItem(DrmFolderTreeItem *p_parent);

   virtual QVariant data(int column);
   virtual bool setData(int column, const QVariant &value);
   inline virtual bool isFile() {return true;}
   inline virtual bool isFolder() {return false;}

   virtual void computeHash(bool recursive);
   virtual QByteArray hash();
   static QByteArray getHash(const QString &path);

   virtual bool compareHash(const QString &path);
   virtual void prepareSync(const QString &dstPath, QList<QString> *p_cleanUp, QList<QString> *p_copySrc, QList<QString> *p_copyDst);

private:
   QString m_LongName;
   QString m_FileName;
};

#endif // DRMFILEITEM_H
