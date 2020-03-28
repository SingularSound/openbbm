#ifndef FILEPARTITEM_H
#define FILEPARTITEM_H
#include "../abstracttreeitem.h"

class AbstractFilePartModel;

class FilePartItem : public AbstractTreeItem
{
   Q_OBJECT
public:
   explicit FilePartItem(AbstractFilePartModel * filePart, AbstractTreeItem *parent);
   ~FilePartItem();

   // Data interaction
   virtual QVariant data(int column);
   // Will actually create the data if column = 0 ?
   virtual bool setData(int column, const QVariant & value);

   inline AbstractFilePartModel * filePart(){
      return mp_FilePart;
   }

private:
   AbstractFilePartModel * mp_FilePart; // !!!! File part graph needs to be deleted separately after deleting Tree
};

#endif // FILEPARTITEM_H
