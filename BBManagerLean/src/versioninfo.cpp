/*
  	This software and the content provided for use with it is Copyright © 2014-2020 Singular Sound 
 	BeatBuddy Manager is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2 as published by
    the Free Software Foundation.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/
#include "versioninfo.h"

VersionInfo::VersionInfo(){}

VersionInfo::VersionInfo(unsigned int major, unsigned int minor, unsigned  int patch, unsigned int build):
    major{major},minor{minor},patch{patch},build{build}{};

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
