#include "versioninfo.h"

VersionInfo::VersionInfo(){}

VersionInfo::VersionInfo(unsigned int major, unsigned int minor, unsigned  int patch, unsigned int build):
    major(major),minor(minor),patch(patch),build(build){};

bool VersionInfo::operator>(const VersionInfo& other){
    bool greaterThan = false;
    if(major>other.major){
        greaterThan = true;
    }else if(major == other.major){
        if(minor>other.minor){
            greaterThan = true;
        }else if(minor == other.minor){
            if(patch>other.patch){
                greaterThan = true;
            }else if(patch == other.patch){
                greaterThan = build>other.build;
            }
        }
    }
    return greaterThan;
}

bool VersionInfo::operator<=(const VersionInfo& other){
    return !this->operator>(other);
}

bool VersionInfo::operator<(const VersionInfo& other){
    return !this->operator==(other) && !this->operator>(other);
}

bool VersionInfo::operator>=(const VersionInfo& other){
    return !this->operator<(other);
}

bool VersionInfo::operator==(const VersionInfo& other){
    return major == other.major && minor == other.minor && patch == other.patch && build == other.build;
}

QString VersionInfo::toQString(){
    QString qsv = QString::number(major) + "." + QString::number(minor) + "." + QString::number(patch) + "." + QString::number(build);
    return qsv;
}

QString VersionInfo::productName(){
    return QString("BeatBuddy Manager Lean");
}

QString VersionInfo::companyName(){
    return QString("Singular Sound");
}

QString VersionInfo::companyDomain(){
    return QString("www.singularsound.com");
}

VersionInfo VersionInfo::RunningAppVersionInfo(){
    VersionInfo info;
    info.major = VERSION_MAJOR;
    info.minor = VERSION_MINOR;
    info.patch = VERSION_PATCH;
    info.build = VERSION_BUILD;
    return info;
}
