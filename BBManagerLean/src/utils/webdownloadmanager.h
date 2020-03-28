#ifndef WEBDOWNLOADMANAGER_H
#define WEBDOWNLOADMANAGER_H

#include <QObject>
#include <QFile>
#include <QObject>
#include <QQueue>
#include <QTime>
#include <QUrl>
#include <QDir>
#include <QNetworkAccessManager>
#include <QNetworkReply>

class WebDownloadManager : public QObject
{
   Q_OBJECT
public:
   explicit WebDownloadManager(QObject *parent = nullptr, bool overwrite = 0,QString path = QString());

   void downloadUrl(const QStringList & urlList);
   void downloadUrl(const QUrl &url);
   void setDownloadedFilePath(QString path);

   QString saveFileName(const QUrl &url);


signals:
   void downloadFinished(int n_failed);



private slots:
   void slotdownloadProgress(qint64, qint64);
   void startNextDownload();
   void slotDownloadReadyRead();
   void slotDownloadFinished();
   void slotDownloadError(QNetworkReply::NetworkError);


private:

   int m_nFailed;
   QString m_baseDirectory;

   bool m_downloadActive;
   bool m_overwrite;

   QQueue<QUrl> m_DownloadQueue;
   QNetworkAccessManager m_NetManager;
   QNetworkReply *currentDownloadReply;
   QFile m_CurrentFile;


};

#endif // WEBDOWNLOADMANAGER_H
