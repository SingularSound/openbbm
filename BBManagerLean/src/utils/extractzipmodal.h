#ifndef EXTRACTZIPMODAL_H
#define EXTRACTZIPMODAL_H

#include <QtCore>
#include <QtWidgets>
#include <QObject>

class ExtractZipModal : public QObject
{
   Q_OBJECT
public:
   enum ProgressType {
      ProgressFileCount,
      ProgressFileSize
   };
   ExtractZipModal(const QString &destPath, const QString &sourceZipPath, const QStringList &sourceZippedFiles, QWidget *p_parent);

   inline void setProgressType(ProgressType progressType){m_progressType = progressType;}
   //inline void setTotalSize(qint64 totalSize){m_totalSize = totalSize;}

   bool run();
private:

   QWidget *mp_parent;

   QString m_destPath;
   QString m_sourceZipPath;
   QStringList m_sourceZippedFiles;
   ProgressType m_progressType;
   //qint64 m_totalSize;


   bool m_aborted;

};

#endif // EXTRACTZIPMODAL_H
