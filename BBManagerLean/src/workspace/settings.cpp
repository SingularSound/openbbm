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
#include "settings.h"

#include "version.h"
#include "../player/mixer.h"

bool Settings::softwareUuidExists()
{
    QSettings settings;
    bool isValid = settings.contains(KEY_UUID) &&
            !settings.value(KEY_UUID).toUuid().isNull();
    return isValid;
}

QDir Settings::getWorkspaceLocation(){
    QDir possibleWorkspacePath;
    QSettings settings;
    if(settings.contains(KEY_WORKSPACE_LOCATION)) {
        possibleWorkspacePath = QDir(settings.value(KEY_WORKSPACE_LOCATION).toString());
    } else {
        possibleWorkspacePath = QDir(QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)).filePath(WORKSPACE_FOLDER_NAME);
    }
    return possibleWorkspacePath;
}

void Settings::setWorkspaceName(QString path){
    QSettings settings;
    settings.setValue(KEY_WORKSPACE_NAME,path);
}

void Settings::setWorkspaceLocation(QString location){
    QSettings settings;
    settings.setValue(KEY_WORKSPACE_LOCATION, location);
}

void Settings::setAutoPilot(bool value){
    QSettings settings;
    settings.setValue(KEY_AUTOPILOT, value);
}

QUuid Settings::getSoftwareUuid()
{
   QSettings settings;
   if(!settings.contains(KEY_UUID)){
      return QUuid();
   }

   return settings.value(KEY_UUID).toUuid();
}

void Settings::setSoftwareUuid(const QUuid &uuid)
{
   QSettings settings;
   settings.setValue(KEY_UUID, uuid.toString());
}

bool Settings::linkedPrjUuidExists()
{
   QSettings settings;
   return settings.contains(KEY_LINKED_PRJ_UUID);
}

QUuid Settings::getLinkedPrjUuid()
{
   QSettings settings;
   if(!settings.contains(KEY_LINKED_PRJ_UUID)){
      return QUuid();
   }

   return settings.value(KEY_LINKED_PRJ_UUID).toUuid();
}

void Settings::setLinkedPrjUuid(const QUuid &uuid)
{
   QSettings settings;
   settings.setValue(KEY_LINKED_PRJ_UUID, uuid.toString());
}

bool Settings::lastProjectExists()
{
   QSettings settings;
   return settings.contains(KEY_LAST_PROJECT);
}

QString Settings::getLastProject()
{
   QSettings settings;
   if(!settings.contains(KEY_LAST_PROJECT)){
      return nullptr;
   }

   return settings.value(KEY_LAST_PROJECT).toString();
}

void Settings::setLastProject(const QString &path)
{
   QSettings settings;
   settings.setValue(KEY_LAST_PROJECT, QVariant(path));
}



bool Settings::lastSdCardDirExists()
{
   QSettings settings;
   return settings.contains(KEY_SD_CARD);
}

QString Settings::getLastSdCardDir()
{
   QSettings settings;
   if(!settings.contains(KEY_SD_CARD)){
      return nullptr;
   }
   return settings.value(KEY_SD_CARD).toString();
}

void Settings::setLastSdCardDir(const QString &path)
{
   QSettings settings;
   settings.setValue(KEY_SD_CARD, QVariant(path));
}

bool Settings::bufferingTime_msExists()
{
   return QSettings().contains(KEY_BUFFERING_TIME); // Keep released key...
}

int Settings::getBufferingTime_ms()
{
   QSettings settings;
   if(!settings.contains(KEY_BUFFERING_TIME)){
      return MIXER_DEFAULT_BUFFERRING_TIME_MS;
   }
   bool ok = false;
   int bufferingTime_ms = settings.value(KEY_BUFFERING_TIME).toInt(&ok); // Keep released key...
   if(!ok){
      return MIXER_DEFAULT_BUFFERRING_TIME_MS;
   }
   return bufferingTime_ms;
}
void Settings::setBufferingTime_ms(int bufferingTime_ms)
{
   QSettings().setValue(KEY_BUFFERING_TIME, QVariant(bufferingTime_ms)); // Keep released key...
}


bool Settings::helpIndexExists()
{
   QSettings settings;
   return settings.contains(KEY_HELP_INDEX);
}

QString Settings::getHelpIndexDir()
{
   QSettings settings;
   if(!settings.contains(KEY_HELP_INDEX)){
      return nullptr;
   }

   return settings.value(KEY_HELP_INDEX).toString();
}

bool Settings::helpPdfExists()
{
   QSettings settings;
   return settings.contains(KEY_HELP_PDF);
}

QString Settings::getHelpPdfDir()
{
   QSettings settings;
   if(!settings.contains(KEY_HELP_PDF)){
      return nullptr;
   }

   return settings.value(KEY_HELP_PDF).toString();
}

int Settings::getCurrentVersion()
{
    return (VER_MAJOR<<20) + (VER_MINOR<<12) + (VER_REVISION<<4) + VER_BUILD;
}

int Settings::getUpdateVersion()
{
    auto cur = getCurrentVersion();
    auto ret = QSettings().value("general/update_version", cur).toInt();
    return ret > cur ? ret : cur;
}

void Settings::setUpdateVersion(int version)
{
    QSettings().setValue("general/update_version", version > getCurrentVersion() ? version : getCurrentVersion());
}


bool Settings::colorDnDWithdrawExists()
{
   return QSettings().contains(KEY_DND_W);
}
QRgb Settings::getColorDnDWithdraw()
{
    QSettings settings;
    QRgb defaultValue(qRgb(0xad, 0x3e, 0x3e));

    if(!settings.contains(KEY_DND_W)){
       return defaultValue;
    }
    bool ok = false;
    QRgb value = settings.value(KEY_DND_W).toInt(&ok);
    if(!ok){
       return defaultValue;
    }
    return value;
}

void Settings::setColorDnDWithdraw(QRgb value)
{
   QSettings().setValue(KEY_DND_W, QVariant(value));
}

bool Settings::colorDnDCopyExists()
{
   return QSettings().contains(KEY_DND_C);
}
QRgb Settings::getColorDnDCopy()
{
    QSettings settings;
    QRgb defaultValue(qRgb(0x83, 0xca, 0xca));

    if(!settings.contains(KEY_DND_C)){
       return defaultValue;
    }
    bool ok = false;
    QRgb value = settings.value(KEY_DND_C).toInt(&ok);
    if(!ok){
       return defaultValue;
    }
    return value;
}
void Settings::setColorDnDCopy(QRgb value)
{
   QSettings().setValue(KEY_DND_C, QVariant(value));
}

bool Settings::colorDnDTargetExists()
{
   return QSettings().contains(KEY_DND_T);
}
QRgb Settings::getColorDnDTarget()
{
    QSettings settings;
    QRgb defaultValue(qRgb(0xff, 0xfc, 0x96));

    if(!settings.contains(KEY_DND_T)){
       return defaultValue;
    }
    bool ok = false;
    QRgb value = settings.value(KEY_DND_T).toInt(&ok);
    if(!ok){
       return defaultValue;
    }
    return value;
}
void Settings::setColorDnDTarget(QRgb value)
{
   QSettings().setValue(KEY_DND_T, QVariant(value));
}

bool Settings::colorDnDAppendExists()
{
   return QSettings().contains(KEY_DND_A);
}
QRgb Settings::getColorDnDAppend()
{
    QSettings settings;
    QRgb defaultValue(qRgb(0x76, 0xa8, 0x75));

    if(!settings.contains(KEY_DND_A)){
       return defaultValue;
    }
    bool ok = false;
    QRgb value = settings.value(KEY_DND_A).toInt(&ok);
    if(!ok){
       return defaultValue;
    }
    return value;
}
void Settings::setColorDnDAppend(QRgb value)
{
   QSettings().setValue(KEY_DND_A, QVariant(value));
}

bool Settings::windowMaximized()
{
   return QSettings().contains(KEY_MAXIMIZED);
}
bool Settings::getWindowMaximized()
{
   QSettings settings;

   if(!settings.contains(KEY_MAXIMIZED)){
      return true; // Maximized by default
   }

   return settings.value(KEY_MAXIMIZED).toBool();
}
void Settings::setWindowMaximized(bool value)
{
   QSettings().setValue(KEY_MAXIMIZED, QVariant(value));
}

bool Settings::windowXExists()
{
   return QSettings().contains(KEY_X);
}
int  Settings::getWindowX()
{
    QSettings settings;
    int defaultValue(0);

    if(!settings.contains(KEY_X)){
       return defaultValue;
    }
    bool ok = false;
    int value = settings.value(KEY_X).toInt(&ok);
    if(!ok){
       return defaultValue;
    }
    return value;
}
void Settings::setWindowX(int value)
{
   QSettings().setValue(KEY_X, QVariant(value));
}

bool Settings::windowYExists()
{
   return QSettings().contains(KEY_Y);
}
int  Settings::getWindowY()
{
    QSettings settings;
    int defaultValue(0);

    if(!settings.contains(KEY_Y)){
       return defaultValue;
    }
    bool ok = false;
    int value = settings.value(KEY_Y).toInt(&ok);
    if(!ok){
       return defaultValue;
    }
    return value;
}
void Settings::setWindowY(int value)
{
   QSettings().setValue(KEY_Y, QVariant(value));
}

bool Settings::windowHExists()
{
   return QSettings().contains(KEY_H);
}
int  Settings::getWindowH()
{
    QSettings settings;
    int defaultValue(500);

    if(!settings.contains(KEY_H)){
       return defaultValue;
    }
    bool ok = false;
    int value = settings.value(KEY_H).toInt(&ok);
    if(!ok){
       return defaultValue;
    }
    return value;
}
void Settings::setWindowH(int value)
{
   QSettings().setValue(KEY_H, QVariant(value));
}

bool Settings::windowWExists()
{
   return QSettings().contains(KEY_W);
}
int  Settings::getWindowW()
{
    QSettings settings;
    int defaultValue(500);

    if(!settings.contains(KEY_W)){
       return defaultValue;
    }
    bool ok = false;
    int value = settings.value(KEY_W).toInt(&ok);
    if(!ok){
       return defaultValue;
    }
    return value;
}
void Settings::setWindowW(int value)
{
   QSettings().setValue(KEY_W, QVariant(value));
}

void Settings::toggleMidiId() {
    QSettings().setValue(KEY_USE_MIDI_ID, !midiIdEnabled());
}

bool Settings::midiIdEnabled() {
    QSettings settings;
    if (!settings.contains(KEY_USE_MIDI_ID))
        return false;
    return settings.value(KEY_USE_MIDI_ID).toBool();
}
