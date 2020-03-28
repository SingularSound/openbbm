#ifndef EFFECTFILEITEM_H
#define EFFECTFILEITEM_H

#include "../abstracttreeitem.h"

class EffectFolderTreeItem;

class EffectFileItem : public AbstractTreeItem
{
   Q_OBJECT
public:
   explicit EffectFileItem(EffectFolderTreeItem *parent);


   virtual QVariant data(int column);
   virtual bool setData(int column, const QVariant &value);
   inline virtual bool isFile() {return true;}
   inline virtual bool isFolder() {return false;}

   virtual void computeHash(bool recursive);
   virtual QByteArray hash();
   static QByteArray getHash(const QString &path);
   virtual bool compareHash(const QString &path);
   virtual void prepareSync(const QString &dstPath, QList<QString> *p_cleanUp, QList<QString> *p_copySrc, QList<QString> *p_copyDst);

   inline uint getQHash() const {
      return m_qHash;
   }

protected:
   void setHash(const QByteArray &hash);

private:
   QString m_LongName;
   QString m_FileName;

   uint m_qHash;

};

// Required in order to use in QSet


inline uint qHash(EffectFileItem &key)
{
   return key.getQHash();
}
inline bool operator==(const EffectFileItem &e1, const EffectFileItem &e2)
{
   return e1.getQHash() == e2.getQHash();
}


#endif // EFFECTFILEITEM_H
