#ifndef EFFECTPTRITEM_H
#define EFFECTPTRITEM_H
#include "../abstracttreeitem.h"
#include "../project/effectfileitem.h"
#include "../project/effectfoldertreeitem.h"

class EffectPtrItem : public AbstractTreeItem
{
   Q_OBJECT
public:
   explicit EffectPtrItem(AbstractTreeItem *parent, EffectFolderTreeItem * p_EffectFolderTreeItem, EffectFileItem * p_EffectFileItem = nullptr);

   virtual QVariant data(int column);
   virtual bool setData(int column, const QVariant & value);
   void setName(const QString &name, bool init);
   void clearEffectUsage();
   bool exportTo(const QString &dstDirPath);

private:
   EffectFolderTreeItem * mp_EffectFolderTreeItem; // Reference received in constructor, no need to delete
   EffectFileItem * mp_EffectFileItem;             // Reference received in constructor, no need to delete
};

#endif // EFFECTPTRITEM_H
