#ifndef COMPRESSZIPMODAL_H
#define COMPRESSZIPMODAL_H

#include <QtCore>
#include <QtWidgets>
#include <QObject>

class CompressZipModal : public QObject
{
   Q_OBJECT
public:
   enum ProgressType {
      ProgressFileCount,
      ProgressFileSize
   };
   CompressZipModal(const QString &destZipPath, const QString &sourcePath, const QStringList &sourceFiles, QWidget *p_parent);

   inline void setProgressType(ProgressType progressType){m_progressType = progressType;}
   inline void setTotalSize(qint64 totalSize){m_totalSize = totalSize;}


   bool run();
private:

   QWidget *mp_parent;

   QString m_destZipPath;
   QString m_sourcePath;
   QStringList m_sourceFiles;
   ProgressType m_progressType;
   qint64 m_totalSize;

   bool m_aborted;

};

#endif // COMPRESSZIPMODAL_H
