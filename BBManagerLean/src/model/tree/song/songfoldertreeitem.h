#ifndef SONGFOLDERITEM_H
#define SONGFOLDERITEM_H
#include "../abstracttreeitem.h"
#include "../project/contentfoldertreeitem.h"
#include "../project/songsfoldertreeitem.h"
#include <QProgressDialog>

#define MAX_SONGS_PER_FOLDER 99


class DrmFileItem;
class SongFileItem;

class SongFolderTreeItem : public ContentFolderTreeItem
{
   Q_OBJECT
public:
   SongFolderTreeItem(BeatsProjectModel *model, SongsFolderTreeItem *parent);
   ~SongFolderTreeItem();

   // Data interaction
   virtual QVariant data(int column);
   // Will actually create the data if column = 0 ?
   virtual bool setData(int column, const QVariant & value);
   virtual Qt::ItemFlags flags(int column);

   void insertNewChildAt(int row);
   virtual void removeChild(int row);

   bool importSongsModal(QWidget *p_parentWidget, const QStringList &srcFileNames, int row);
   bool exportModal(QWidget *p_parentWidget, const QString &dstPath);

   void manageParsingErrors(QWidget *p_parent);

   // required to importFolder
   inline CsvConfigFile *csvFileRef(){
      return &m_CSVFile;
   }

   void replaceDrumsetModal(QProgressDialog *p_progresss, const QString &oldLongName, DrmFileItem *p_drmFileItem);

   SongFileItem* childWithFileName(const QString &fileName) const;

protected:
   bool createFileWithData(int index, const QString &longName, const QString& fileName);

private:

};

#endif // SONGFOLDERITEM_H
