#ifndef DRMFOLDERTREEITEM_H
#define DRMFOLDERTREEITEM_H
#include "contentfoldertreeitem.h"
#include <QWidget>


class DrmFileItem;

class DrmFolderTreeItem : public ContentFolderTreeItem
{
   Q_OBJECT
public:
   DrmFolderTreeItem(BeatsProjectModel *p_model, FolderTreeItem *parent);
   bool containsDrm(const QString &drmSourcePath) const;
   QString drmFileName(const QString &longName) const;
   QString drmLongName(const QString &fileName) const;
   DrmFileItem *addDrmModal(QWidget *p_parentWidget, const QString &drmSourcePath);
   bool removeDrmModal(QWidget *p_parentWidget, const QString &longName);

   DrmFileItem *replaceDrumsetModal(QWidget *p_parentWidget, const QString &oldLongName, const QString &drmSourcePath);



protected:

   bool createFileWithData(int index, const QString & longName, const QString& fileName);
};

#endif // DRMFOLDERTREEITEM_H
