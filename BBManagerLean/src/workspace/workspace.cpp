/*
    This software and the content provided for use with it is
    Copyright © 2014-2020 Singular Sound
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
#include <QMessageBox>
#include <QString>
#include <QStandardPaths>

#include "workspace.h"
#include "contentlibrary.h"
#include "settings.h"

#define WORKSPACE_FOLDER_NAME "BBWorkspace"
#define USER_LIB_FOLDER_NAME "user_lib"


Workspace::Workspace(QObject *p_parent)
    : QObject(p_parent)
{
    m_valid = true; // valid until contrary proven
    if(!initialize()){
        invalidate();
    }
}

void Workspace::invalidate()
{
    m_valid = false;
    mp_userLibrary->invalidate();
}

void Workspace::resetCurrentPath(){
    if(!m_valid){
        return;
    }

    mp_userLibrary->resetCurrentPath();
}

/**
 * @brief Workspace::create
 * @param location
 * @param p_parentWidget
 * @return
 *
 * NOTE: should only be called from master instance that is stored in main window
 */
bool Workspace::create(const QFileInfo &location, QWidget *p_parentWidget)
{
    emit sigAboutToChange();
    m_valid = true; // valid until contrary proven

    // Overwrite values in QSettings
    Settings::setWorkspaceLocation(location.absoluteFilePath());

    // Create folder if it does not exist
    m_workspaceDirectory = QDir(location.absoluteFilePath());

    if(!m_workspaceDirectory.exists()){
        if(!m_workspaceDirectory.mkpath(m_workspaceDirectory.absolutePath())){
            QMessageBox::critical(p_parentWidget, tr("Unable to create workspace"), tr("Unable to create workspace at location %1\nMake sure you have write access to the selected location").arg(m_workspaceDirectory.absolutePath()));
            invalidate();
            emit sigChanged(false);
            return false;
        }
    }

    // re-create subcomponents
    mp_userLibrary->create(p_parentWidget);

    // evaluate validity
    m_valid = mp_userLibrary->isValid();
    if(!m_valid){
        invalidate();
    }

    emit sigChanged(m_valid);
    return m_valid;

}

bool Workspace::initialize()
{
    QDir possibleWorkspacePath;

    possibleWorkspacePath = Settings::getWorkspaceLocation();
    possibleWorkspacePath.mkpath(".");

    bool valid = possibleWorkspacePath.exists();

    if(valid) {
        m_workspaceDirectory = possibleWorkspacePath;
        Settings::setWorkspaceName(possibleWorkspacePath.absolutePath());
        mp_userLibrary = new ContentLibrary(this, USER_LIB_FOLDER_NAME);

        if(!mp_userLibrary->isValid()){
            mp_userLibrary->create(nullptr);
        }
        Settings::setAutoPilot(mp_userLibrary->dir().exists("autopilot"));
    }

    return valid && mp_userLibrary->isValid();
}

QDir Workspace::dir() const
{
    if(!m_valid){
        return QDir(QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation));
    }
    return m_workspaceDirectory;
}

QString Workspace::defaultPath() const
{
    if(!m_valid){
        return QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    }
    return m_workspaceDirectory.absolutePath();
}
