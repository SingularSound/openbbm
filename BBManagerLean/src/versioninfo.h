#ifndef VERSIONINFO_H
#define VERSIONINFO_H

#include<QString>
#include <version_constants.h>

class VersionInfo
{
public:
    VersionInfo();
    VersionInfo(unsigned int major, unsigned int minor, unsigned  int patch, unsigned  int build);

    unsigned int major;
    unsigned int minor;
    unsigned int patch;
    unsigned int build;

    bool operator>(const VersionInfo& other);
    bool operator<(const VersionInfo& other);
    bool operator>=(const VersionInfo& other);
    bool operator<=(const VersionInfo& other);
    bool operator==(const VersionInfo& other);

    static VersionInfo RunningAppVersionInfo();

    QString toQString();
    QString companyDomain();
    QString productName();
    QString companyName();
};



#endif // VERSIONINFO_H
