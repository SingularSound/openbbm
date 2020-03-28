#ifndef PLATFORM_H
#define PLATFORM_H

#include <QString>
#include <QDebug>

class Platform
{

public :
    static bool SetFileHidden(const QString &path);
    static bool CreateProjectShellIcon(const QString &path);
};

#endif // PLATFORM_H
