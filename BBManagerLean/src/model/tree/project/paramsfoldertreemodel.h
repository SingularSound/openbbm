#ifndef PARAMSFOLDERTREEMODEL_H
#define PARAMSFOLDERTREEMODEL_H

#include "foldertreeitem.h"
#include "pedalsettingsdefinitions.h"
#include <QFile>

class ParamsFolderTreeModel : public FolderTreeItem
{
public:
   ParamsFolderTreeModel(BeatsProjectModel *p_model, FolderTreeItem *parent);

   // Re-implementation of FolderTreeItem
   virtual bool createProjectSkeleton();

   virtual void prepareSync(const QString &dstPath, QList<QString> *p_cleanUp, QList<QString> *p_copySrc, QList<QString> *p_copyDst);

   static QString infoFileName();
   static QString infoRelativePath();
   static QMap<QString, QVariant> getInfoMap(const QString &path);
   QMap<QString, QVariant> infoMap();
   static bool setInfoMap(const QString &path, const QMap<QString, QVariant> &map);
   bool setInfoMap(const QMap<QString, QVariant> &map);
   static QUuid getProjectUuid(const QString &path);
   QUuid projectUuid();
   static bool setProjectUuid(const QString &path, const QUuid &uuid);
   static QByteArray getLinkHash(const QString &path);
   static bool setLinkHash(const QString &path, const QByteArray &linkHash);
   static bool resetProjectInfo(const QString &path);
   bool resetProjectInfo();

   void parseFsSettings();

   virtual void computeHash(bool recursive);

   STOPPED_ACTION_params primaryStopped() const;
   void setPrimaryStopped(const STOPPED_ACTION_params &primaryStopped);

   STOPPED_ACTION_params secondaryStopped() const;
   void setSecondaryStopped(const STOPPED_ACTION_params &secondaryStopped);

   PLAYING_ACTION_params primaryPlaying() const;
   void setPrimaryPlaying(const PLAYING_ACTION_params &primaryPlaying);

   PLAYING_ACTION_params secondaryPlaying() const;
   void setSecondaryPlaying(const PLAYING_ACTION_params &secondaryPlaying);

private:
   QString m_infoFilePath;
   QString m_footswFilePath;
   QMap<QString, QVariant> m_infoFileContent;

   STOPPED_ACTION_params m_primaryStopped;
   STOPPED_ACTION_params m_secondaryStopped;
   PLAYING_ACTION_params m_primaryPlaying;
   PLAYING_ACTION_params m_secondaryPlaying;
};

#endif // PARAMSFOLDERTREEMODEL_H
