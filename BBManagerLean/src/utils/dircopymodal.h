#ifndef DIRCOPYMODAL_H
#define DIRCOPYMODAL_H

#include <QtCore>
#include <QtWidgets>

class DirCopyModal
{
public:
   enum ProgressType {
      ProgressFileCount,
      ProgressFileSize
   };

   DirCopyModal(const QString &destPath, const QString &sourcePath, const QStringList &sourceFiles, QWidget *p_parent);

   inline void setProgressType(ProgressType progressType){m_progressType = progressType;}
   inline void setTotalSize(qint64 totalSize){m_totalSize = totalSize;}

   bool run();

private:

   QWidget *mp_parent;

   QString m_destPath;
   QString m_sourcePath;
   QStringList m_sourceFiles;
   ProgressType m_progressType;
   qint64 m_totalSize;

   bool m_aborted;
};

#endif // DIRCOPYMODAL_H
