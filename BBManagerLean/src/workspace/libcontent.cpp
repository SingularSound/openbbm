/*
   This software and the content provided for use with it is Copyright © 2014-2020 Singular Sound
    	BeatBuddy Manager is free software: you can redistribute it and / or modify
    it under the terms of the GNU General Public License version 2 as published by
    the Free Software Foundation.
    
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
    
    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/
#include "libcontent.h"
#include "contentlibrary.h"

#include <QMessageBox>
#include <QStandardPaths>
#include <QDebug>

LibContent::LibContent(ContentLibrary *p_parent, const QString &name) :
   QObject(p_parent),
   mp_parent(p_parent),
   m_name(name)
{
    m_valid = true; // valid until contrary proven
    if(!initialize()){
        invalidate();
    }
}

void LibContent::invalidate()
{
    m_valid = false;
}

void LibContent::resetCurrentPath(){
    if(!m_valid){
        return;
    }

    QSettings settings;
    settings.setValue(QString("%1/current_path").arg(key()), QVariant(m_dir.absolutePath()));
}

bool LibContent::create(QWidget *p_parentWidget)
{
    m_valid = true; // valid until contrary proven
    QSettings settings;

    // create directory representation
    m_dir = QDir(mp_parent->dir().absoluteFilePath(m_name));

    // overwrite settings
    settings.setValue(QString("%1/default_path").arg(key()), QVariant(m_dir.absolutePath()));

    if(!m_dir.exists()){
        if(!m_dir.mkpath(m_dir.absolutePath())){
            QMessageBox::critical(p_parentWidget, tr("Create LibContent"), tr("Unable to create workspace component at location %1").arg(m_dir.absolutePath()));
            m_valid = false; // note: invalidate will be called by Workspace in the end
            return false;
        }
    }

    m_valid = true;
    return true;
}

bool LibContent::initialize()
{
    QSettings settings;

    // use best effort to initialize as much content as possible
    bool valid = true;

    // create directory representation
    m_dir = QDir(mp_parent->dir().absoluteFilePath(m_name));

    // verify if directory exists
    if(!m_dir.exists()){
        qWarning() << "LibContent::initialize - WARNING - " << key() << " - directory does not exist :" << m_dir.absolutePath();
        valid = false;
    }

    // verify if settings exist
    if(!settings.contains(QString("%1/default_path").arg(key()))){
        qWarning() << "LibContent::initialize - WARNING - " << key() << " - default_path subkey does not exist ";
        valid = false;
    }
    if(!settings.contains(QString("%1/current_path").arg(key()))){
        qWarning() << "LibContent::initialize - WARNING - " << key() << " - current_path subkey does not exist ";
        valid = false;
    }

    return valid;
}

QString LibContent::key() const
{
   return QString("%1/%2").arg(mp_parent->key(), name());
}


QString LibContent::currentPath() const
{
    if(!m_valid){
        return QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    }

    QSettings settings;
    QString currentPath;

    // current path settings was created in create method, still use failsave method

    // Try current_path first
    if(settings.contains(QString("%1/current_path").arg(key()))){
        currentPath = settings.value(QString("%1/current_path").arg(key())).toString();
        if(QDir(currentPath).exists()){
            return currentPath;
        }
    }

    // Use default_path if current_path does not exist
    if (settings.contains(QString("%1/default_path").arg(key()))){
        currentPath = settings.value(QString("%1/default_path").arg(key())).toString();
        if(QDir(currentPath).exists()){
            return currentPath;
        }
    }

    // Use documents folder in case none exist
    return QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
}

void LibContent::setCurrentPath(const QString &path)
{
   QSettings settings;
   settings.setValue(QString("%1/current_path").arg(key()), QVariant(path));
}

QDir LibContent::dir() const
{
    if(!m_valid){
        return QDir(QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation));
    }
    return m_dir;
}

QString LibContent::defaultPath() const
{
    if(!m_valid){
        return QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    }
    return m_dir.absolutePath();
}

