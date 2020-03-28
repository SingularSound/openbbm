#ifndef SONGSFOLDERTREEITEM_H
#define SONGSFOLDERTREEITEM_H

#include "contentfoldertreeitem.h"
class DrmFileItem;
class SongFolderTreeItem;
class SongsFolderTreeItem : public ContentFolderTreeItem
{
    Q_OBJECT
public:
   SongsFolderTreeItem(BeatsProjectModel *p_model, FolderTreeItem *parent);

   virtual QVariant data(int column);

   void insertNewChildAt(int row);
   void removeChild(int row);

   bool importFoldersModal(QWidget *p_parentWidget, const QStringList &srcFileNames, int row);

   void manageParsingErrors(QWidget *p_parent);

   void replaceDrumsetModal(QWidget *p_parentWidget, const QString &oldLongName, DrmFileItem *p_drmFileItem);

   int songCount();
   void initMidiIds();

   SongFolderTreeItem *childWithFileName(const QString &fileName) const;
   void setMidiId(QString name, int midiId);

protected:
   virtual void createFolderWithData(int index, const QString &longName, const QString& fileName, bool cleanAll);

};

#endif // SONGSFOLDERTREEITEM_H
