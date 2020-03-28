#include "debug.h"

#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QStandardPaths>
#include <QString>

QString msgTypeLoggableRepr(QtMsgType type){
    switch (type) {
    case QtDebugMsg:
        return QString("DBG");
    case QtWarningMsg:
        return QString("WRN");
    case QtCriticalMsg:
        return QString("CRT");
    case QtFatalMsg:
        return QString("FTL");
    case QtInfoMsg:
        return QString("INF");
    }
}

QString debugMessageFormatter(QtMsgType type, const QMessageLogContext &context, const QString &msg){
    QString currentTime = QDateTime::currentDateTime().toString("yyyy-MM-ddThh:mm:ss:zzz");
    QString messageType = msgTypeLoggableRepr(type);

    return QString("[%1|%2::%3:%4::%5] %6").arg(currentTime, messageType, context.file, QString::number(context.line), context.function, msg);
}

void consolidatedLogHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg){
    QString txt = debugMessageFormatter(type,context,msg);

    QFile outFile(LOGGING_PATH);
    QTextStream ts(stdout);
    ts << txt << endl;

}

void setupDebugging(){
    QDir outDir(LOGGING_DIR);
    QFile outFile(LOGGING_PATH);
    if(!outFile.exists()){
        if(!outDir.exists()){
            outDir.mkpath(outDir.absolutePath());
        }
    }
    qInstallMessageHandler(*consolidatedLogHandler);

    qDebug() << "Current path:" << QDir::currentPath();
    qDebug() << "Home path:" << QDir::homePath();
    qDebug() << "Temporary path:" << QDir::tempPath();
    qDebug() << "Root path:" << QDir::rootPath();
    qDebug() << "Log File:" << LOGGING_PATH;
}
