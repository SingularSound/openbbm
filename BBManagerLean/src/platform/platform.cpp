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
#include "platform.h"

#include <QString>
#ifdef Q_OS_WIN
#include "windows.h"
#include <pshpack8.h>
#include "Shlobj.h"
#elif defined(Q_OS_OSX)
#include "../platform/macosx/macosxplatform.h"
#endif

#include <QFile>
#include <QDir>

/**
 * @brief SetFileHidden
 * @param path
 * @return
 */
bool Platform::SetFileHidden(const QString &path){
#ifdef Q_OS_WIN
    return true;
#else
    return true;
#endif

}

/**
 * @brief Change the look of the project folder by adding a shell command in the folder and copying an icon
 * @param path
 * @return
 */
bool Platform::CreateProjectShellIcon(const QString &path)
{
#ifdef Q_OS_WIN
    QString shell_path = path + "/Desktop.ini";
    QString data_path = path + "/data";
    QString icon_path = data_path + "/BeatBuddy_Project_Folder.ico";

    /* Create a directory for the icon and make it hidden */
    QDir dir(data_path);
    dir.mkdir(".");
    SetFileHidden(data_path);

    /* Copy the icon from the ressoureces to the data folder and make it hidden */
    QFile::copy(":/images/images/BeatBuddy_Project_Folder.ico",icon_path);
    SetFileHidden(icon_path);

    /* Copy the shell command file from the ressources */
    QFile::copy(":/windows/Desktop.ini",shell_path);

    /* make the file hidden with the windows API */
    SetFileHidden(shell_path);

    /* Make the file "system" so the shell cmd can be executed */
    SetFileHidden(path);
    return true;
#elif defined(Q_OS_OSX)
    return MacOSXPlatform::CreateProjectShellIcon(path);
#else
    return true;
#endif
}

