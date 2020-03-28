#ifndef CONTENTFOLDERTREEITEM_H
#define CONTENTFOLDERTREEITEM_H

#include <QObject>
#include <QDebug>
#include <QString>
#include <QFile>
#include <QProgressDialog>

#include "foldertreeitem.h"
#include "csvconfigfile.h"

/**
 * @brief The ContentFolderTreeItem class represents a folder that contains
 * unspecified content that needs to be storred in 8.3 format. It maps full names
 * to a 8.3 reprensentation using a CSV file
 */

class ContentFolderTreeItem: public FolderTreeItem
{
   Q_OBJECT
public:
   ContentFolderTreeItem(BeatsProjectModel *p_model, FolderTreeItem *parent);

   // Re-implementation of AbstractTreeItem
   virtual void moveChildren(int sourceFirst, int sourceLast, int delta);
   virtual void removeChild(int row);

   // Re-implementation of FolderTreeItem
   virtual bool createProjectSkeleton();

   virtual void computeHash(bool recursive);
   virtual void prepareSync(const QString &dstPath, QList<QString> *p_cleanUp, QList<QString> *p_copySrc, QList<QString> *p_copyDst);


   inline void saveConfigFile(){m_CSVFile.write();}

   virtual void updateDataWithModel(bool cleanAll);
   virtual void updateModelWithData(bool cleanAll);

   void updateChildContent(AbstractTreeItem * child, bool save);

   inline void setSubFoldersAllowed(bool allowed) {m_SubFoldersAllowed = allowed;}
   inline bool areSubFoldersAllowed() {return m_SubFoldersAllowed;}
   inline void setSubFilesAllowed(bool allowed) {m_SubFilesAllowed = allowed;}
   inline bool areSubFilesAllowed() {return m_SubFilesAllowed;}
   inline void attachProgress(QProgressDialog *progressDialog) {m_pProgressDialog = progressDialog;}
   inline void detachProgress() {m_pProgressDialog = nullptr;}

   void removeAllChildrenContent(bool save);


   inline bool containsLongName(const QString &longName) const{return m_CSVFile.containsLongName(longName);}
   inline bool containsFileName(const QString &fileName) const{return m_CSVFile.containsLongName(fileName);}

   QString resolveDuplicateLongName(const QString &longName, bool newName);
   QString pathForLongName(const QString &longName);


protected:
   virtual void createChildContentAt(int index, bool save);
   virtual void removeChildContentAt(int index, bool save);
   virtual void updateChildContentAt(int index, bool save);


   CsvConfigFile m_CSVFile;
   QProgressDialog *m_pProgressDialog;
public:
   virtual void createFolderWithData(int index, const QString &longName, const QString& fileName, bool cleanAll);
   virtual bool createFileWithData(int index, const QString &longName, const QString& fileName);

private:

   // re-implementation of FolderTreeItem::renameFolder
   bool renameFolder(const QString &newName);

private:
   bool m_SubFoldersAllowed;
   bool m_SubFilesAllowed;
};

#endif // CONTENTFOLDERTREEITEM_H
