#ifndef EFFECTFOLDERTREEITEM_H
#define EFFECTFOLDERTREEITEM_H
#include "../abstracttreeitem.h"
#include "contentfoldertreeitem.h"

#include <QMap>
#include <QFile>
#include <QPair>
#include <QUuid>
#include <QByteArray>
#include <QString>

class EffectFileItem;

class EffectFolderTreeItem : public ContentFolderTreeItem
{
   Q_OBJECT
public:
   EffectFolderTreeItem(BeatsProjectModel *p_model, FolderTreeItem *parent);

   virtual bool createProjectSkeleton();
   virtual void updateModelWithData(bool cleanAll);
   virtual void removeChildContentAt(int index, bool save);

   virtual void computeHash(bool recursive);
   virtual void prepareSync(const QString &dstPath, QList<QString> *p_cleanUp, QList<QString> *p_copySrc, QList<QString> *p_copyDst);

   inline EffectFileItem *addUse(const QString &efxSourcePath, const QUuid &songId, bool immediately = false, int useCnt = 1){
      QFileInfo efxInfo(efxSourcePath);
      return addUse(efxSourcePath, efxInfo.baseName(), songId, immediately, useCnt);
   }

   EffectFileItem *addUse(const QString &efxSourcePath, const QString &efxLongName, const QUuid &songId, bool immediately = false, int useCnt = 1);
   EffectFileItem *addUse(QIODevice &efxSourceFile, const QString &efxLongName, const QUuid &songId, bool immediately = false, int useCnt = 1);
   void removeUse(const QString &efxFileName, const QUuid &songId, bool immediately = false, qint32 count = 1);
   void removeAllUse(const QUuid &songId, bool imediately = true);
   void saveUseChanges(const QUuid &songId);
   void saveAllUseChanges();
   void discardUseChanges(const QUuid &songId);
   void discardAllUseChanges();

   EffectFileItem *effectWithFileName(const QString &efxFileName);

   // Not the optimal method of retrieving effects for 1 song. Consider looping on song parts instead
   QList<EffectFileItem *> effectsForSong(const QUuid &songId);

   void loadUsageFile();
   void saveUsageFile();

protected:
   bool createFileWithData(int index, const QString & longName, const QString& fileName);

private:
   QMap<QByteArray, QMap<QString, qint32> > m_UnsavedAddUsageInfo;
   QMap<QByteArray, QMap<QString, qint32> > m_UnsavedRemoveUsageInfo;
   QMap<QString, QMap<QByteArray, qint32> > m_UsageInfo; // efxFileName, songId, times used
   QFile m_UsageFile;

   bool copyConvertWaveFile(const QString &efxSourcePath, const QString &efxFileName);
   bool copyConvertWaveFile(QIODevice &efxSourceFile, const QString &efxFileName);
};

#endif // EFFECTFOLDERTREEITEM_H
