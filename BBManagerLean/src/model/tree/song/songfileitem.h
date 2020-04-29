#ifndef SONGFILEITEM_H
#define SONGFILEITEM_H

#include "filepartitem.h"
#include "../../filegraph/songfilemodel.h"

class ContentFolderTreeItem;
class EffectFileItem;
class DrmFileItem;

class SongFileItem : public FilePartItem
{
   Q_OBJECT
public:
   SongFileItem(SongFileModel * filePart, ContentFolderTreeItem *parent);
   SongFileItem(SongFileModel * filePart, ContentFolderTreeItem *parent, const QString &longName, const QString &fileName);

   virtual QVariant data(int column);
   virtual bool setData(int column, const QVariant & value);
   virtual Qt::ItemFlags flags(int column);
   virtual void computeHash(bool recursive);
   virtual QByteArray hash();
   static QByteArray getHash(const QString &path);
   virtual bool compareHash(const QString &path);
   virtual void prepareSync(const QString &dstPath, QList<QString> *p_cleanUp, QList<QString> *p_copySrc, QList<QString> *p_copyDst);

   void insertNewChildAt(int row);
   void removeChild(int row);

   inline virtual bool isFile() {return true;}

   void moveChildren(int sourceFirst, int sourceLast, int delta);
   void clearEffectUsage();

   bool exportModal(QWidget *p_parentWidget, const QString &dstPath);
   QList<EffectFileItem *> effectList();
   QMap<QString, qint32> effectUsageCountMap();

   void replaceEffectFile(const QString &originalName, const QString &newName);
   bool isFileValid();

   void manageParsingErrors(QWidget *p_parent);

   void replaceDrumset(const QString &oldLongName, DrmFileItem *p_drmFileItem);

    bool& hasUnsavedChanges() { return m_UnsavedChanges; }
    inline QString fileName() { return m_FileName; }

private:
   void saveFile();
   void verifyFile();
   void verifyAutoPilot();
   void initialize();

   void setDefaultDrm(const QString &name, const QString &fileName);
   QString defaultDrmName();
   QString defaultDrmFileName(); // Needs to be returned to upper case

   QString m_FileName;
   bool m_UnsavedChanges;
   bool m_playing;
   QStringList m_parseErrors;
};

#endif // SONGFILEITEM_H
