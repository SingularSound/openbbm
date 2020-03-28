#ifndef DIRLISTALLSUBFILESMODAL_H
#define DIRLISTALLSUBFILESMODAL_H

#include <QtCore>
#include <QtWidgets>



class DirListAllSubFilesModal
{
public:

   enum rootConfig { noFolders        = 0x00,
                     rootFirst        = 0x01,
                     rootLast         = 0x02,
   };
   explicit DirListAllSubFilesModal(const QString &location, int rootConfig, QWidget *p_parent, bool absolute = true);
   explicit DirListAllSubFilesModal(const QStringList &locations, int rootConfig, QWidget *p_parent, const QString &rootForRelative = QString());

   bool run(bool ignoreFootsw_ini = false, const QStringList &nameFilters = QStringList());

   inline QStringList result(){return m_result;}
   inline qint64 totalFileSize(){return m_totalFileSize;}

private:
   QStringList m_locations;
   int m_rootConfig;
   QStringList m_result;
   QWidget *mp_parent;
   bool m_absolute;
   QDir m_rootDir;

   qint64 m_totalFileSize;

   void exploreChildren(QString dirPath, QProgressDialog &progress, bool ignoreFootsw_ini, const QStringList &nameFilters);

};

#endif // DIRLISTALLSUBFILESMODAL_H
