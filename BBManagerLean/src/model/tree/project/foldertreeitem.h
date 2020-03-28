#ifndef FOLDERTREEITEM_H
#define FOLDERTREEITEM_H

#include <QObject>
#include <QDebug>
#include <QString>
#include <QDir>

#include "../abstracttreeitem.h"
#include "../../beatsmodelfiles.h"

/**
 * @brief The FolderTreeItem class represents a generic folder
 */

class BeatsProjectModel;

class FolderTreeItem:public AbstractTreeItem
{
   Q_OBJECT
public:
   // Constructor used to create root
   explicit FolderTreeItem(BeatsProjectModel *p_model);
   ~FolderTreeItem();

   virtual QVariant data(int column);
   virtual bool setData(int column, const QVariant & value);

   virtual void computeHash(bool recursive);
   virtual QByteArray hash();
   static QByteArray getHash(const QString &path);
   virtual bool compareHash(const QString &path);
   virtual void prepareSync(const QString &dstPath, QList<QString> *p_cleanUp, QList<QString> *p_copySrc, QList<QString> *p_copyDst);
   static void exploreChildrenForRemoval(const QString &dstPath, QList<QString> *p_cleanUp);

   QFileInfo folderFI() const;

   virtual bool createProjectSkeleton();
   bool createFolder();

   virtual bool renameFolder(const QString &newName);

   virtual void removeAllChildrenContent(bool save);

   QFileInfoList projectDirectories() const;

   inline virtual bool isFolder() {return true;}

   inline const QString & name() {return m_Name;}
   inline void setName(const QString & name) {m_Name = name;}
   inline const QString & fileName() const {return m_FileName;}
   inline void setFileName(const QString & name) {m_FileName = name;}

   inline const QString &childrenTypes() {return m_ChildrenTypes;}
   inline void setChildrenTypes(const QString &types) {m_ChildrenTypes = types;}


   BeatsProjectModel *model() const;
protected:
   // Constructor used by children classes
   FolderTreeItem(BeatsProjectModel *p_model, FolderTreeItem *parent);

   // Intended to be called only by subclasses
//   virtual void removeChildContentAt(int index);

   void setHash(const QByteArray &hash);
   static bool setHashStatic(const QString &path, const QByteArray &hash);

   QStringList m_ErrorMsg;
      int midiId;
private:
   QString m_Name;
   QString m_FileName;
   QString m_ChildrenTypes;

};

#endif // FOLDERTREEITEM_H
