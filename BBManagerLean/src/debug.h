#ifndef DEBUG_H
#define DEBUG_H

#include <QString>
#include <QtDebug>

#define LOGGING_DIR QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + QDir::separator()
#define LOGGING_FILENAME "bbmdebug.log"
#define LOGGING_PATH  LOGGING_DIR + LOGGING_FILENAME

QString msgTypeLoggableRepr(QtMsgType type);
QString debugMessageFormatter(QtMsgType type, const QMessageLogContext &context, const QString &msg);
void consolidatedLogHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg);
void setupDebugging();


#endif // DEBUG_H
