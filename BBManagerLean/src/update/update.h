#ifndef UPDATE_H
#define UPDATE_H

#include <QObject>
#include "updatedialog.h"
#include "utils/filedownloader.h"

#if defined(Q_OS_MAC)
#define DEFAULT_UPDATE_URL "https://www.singularsound.com/downloadable/latest-macx.xml"
#elif defined(Q_OS_WIN)
#define DEFAULT_UPDATE_URL "https://www.singularsound.com/downloadable/latest-win.xml"
#elif defined(Q_OS_LINUX)
#define DEFAULT_UPDATE_URL "https://www.singularsound.com/downloadable/latest-linux.xml"
#else
#error "Compiling for unsupported OS"
#endif

class Update: public QObject
{
public:
    explicit Update(QObject *parent);
public slots:
    void showUpdateDialog();
private:
    UpdateDialog ud;
    FileDownloader updateXmlDownloader;
private slots:
    void updateXmlDownloaded(const QByteArray&);
};

#endif // UPDATE_H
