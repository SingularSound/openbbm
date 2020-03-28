#ifndef FILEDOWNLOADER_H
#define FILEDOWNLOADER_H

#include <QObject>
#include <QByteArray>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>

class FileDownloader : public QObject
{
    Q_OBJECT
public:
    explicit FileDownloader(QObject* parent = nullptr);
    void download(const QUrl& fileUrl);

signals:
    void progress(qint64 bytesRead, qint64 totalBytes);
    void downloaded(const QByteArray&);

private slots:
    void fileDownloaded(QNetworkReply* pReply);

private:
    QNetworkAccessManager m_WebCtrl;
};

#endif // FILEDOWNLOADER_H
