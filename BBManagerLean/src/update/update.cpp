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
#include "update.h"
#include <QDebug>
#include <QObject>
#include <QXmlStreamReader>
#include <versioninfo.h>

Update::Update(QObject *parent) : QObject(parent)
{
    connect(&updateXmlDownloader, &FileDownloader::downloaded, this, &Update::updateXmlDownloaded);
}

void Update::showUpdateDialog(){
    this->ud.show();
    updateXmlDownloader.download(QUrl(DEFAULT_UPDATE_URL));
}

void Update::updateXmlDownloaded(const QByteArray& xml){
    QXmlStreamReader versionInformationXmlReader(xml);

    QString versionAvailabilityMessage = "Unabe to get new version information.";

    while(versionInformationXmlReader.readNextStartElement()){
        if(versionInformationXmlReader.name()=="version"){
            auto versionAttributes = versionInformationXmlReader.attributes();

            VersionInfo advertisedVersion(versionAttributes.value("major").toUInt(),
                                          versionAttributes.value("minor").toUInt(),
                                          versionAttributes.value("patch").toUInt(),
                                          versionAttributes.value("build").toUInt());
            if(VersionInfo::RunningAppVersionInfo()>=advertisedVersion){
                versionAvailabilityMessage = "You are running the latest version of the software.";
            }else{
                QString downloadUrl = versionAttributes.value("download").toString();
                versionAvailabilityMessage = "There is an upgrade available. Get the new version at <a href='" + downloadUrl + "'>here</a>.";
            }

            break;
        }

    }

    this->ud.setInformationLabelText(versionAvailabilityMessage);
}
