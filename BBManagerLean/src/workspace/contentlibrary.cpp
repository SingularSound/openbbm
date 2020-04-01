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
#include <QMessageBox>
#include <QStandardPaths>

#include "contentlibrary.h"
#include "workspace.h"
#include "libcontent.h"

#define PROJECTS_FOLDER_NAME "projects"
#define SONGS_FOLDER_NAME "songs"
#define DRUM_SETS_FOLDER_NAME "drum_sets"
#define MIDI_SOURCES_FOLDER_NAME "midi_sources"
#define WAVE_SOURCES_FOLDER_NAME "wave_sources"


ContentLibrary::ContentLibrary(Workspace *p_parent, const QString &name) :
   QObject(p_parent),
   mp_parent(p_parent),
   m_name(name)
{
    m_valid = true; // valid until contrary proven
    if(!initialize()){
        invalidate();
    }
}

void ContentLibrary::invalidate()
{
    m_valid = false;
    mp_libProjects->invalidate();
    mp_libSongs->invalidate();
    mp_libDrumSets->invalidate();
    mp_libMidiSources->invalidate();
    mp_libWaveSources->invalidate();
}

void ContentLibrary::resetCurrentPath(){
    if(!m_valid){
        return;
    }
    QSettings settings;

    settings.setValue(QString("%1/current_path").arg(key()), QVariant(m_dir.absolutePath()));

    mp_libProjects->resetCurrentPath();
    mp_libSongs->resetCurrentPath();
    mp_libDrumSets->resetCurrentPath();
    mp_libMidiSources->resetCurrentPath();
    mp_libWaveSources->resetCurrentPath();
}

bool ContentLibrary::create(QWidget *p_parentWidget)
{
    m_valid = true; // valid until contrary proven
    QSettings settings;

    // create directory representation
    m_dir = QDir(mp_parent->dir().absoluteFilePath(m_name));

    // make sure settings are up to date
    // (but if this is always going to point at the same place relative to workspace, why have it exist anyhow?)
    settings.setValue(QString("%1/default_path").arg(key()), QVariant(m_dir.absolutePath()));

    if(!m_dir.exists()){
        if(!m_dir.mkpath(m_dir.absolutePath())){
            QMessageBox::critical(p_parentWidget, tr("Create Content Library"), tr("Unable to create workspace component at location %1").arg(m_dir.absolutePath()));
            this->invalidate();
            return false;
        }
    }

    // Re-create subcomponents
    // Note: Dir must exist to create subcomponents
    mp_libProjects->create(p_parentWidget);
    mp_libSongs->create(p_parentWidget);
    mp_libDrumSets->create(p_parentWidget);
    mp_libMidiSources->create(p_parentWidget);
    mp_libWaveSources->create(p_parentWidget);

    // evaluate validity
    m_valid =
            mp_libProjects->isValid() &&
            mp_libSongs->isValid() &&
            mp_libDrumSets->isValid() &&
            mp_libMidiSources->isValid() &&
            mp_libWaveSources->isValid();

    return m_valid;
}

bool ContentLibrary::initialize()
{
    QSettings settings;

    // use best effort to initialize as much content as possible
    bool valid = true;

    // create directory representation
    m_dir = QDir(mp_parent->dir().absoluteFilePath(m_name));

    // verify if directory exists
    if(!m_dir.exists()){
        qWarning() << "ContentLibrary::initialize - WARNING - " << key() << " - directory does not exist :" << m_dir.absolutePath();
        valid = false;
    }

    // verify if settings exist
    if(!settings.contains(QString("%1/default_path").arg(key()))){
        qWarning() << "ContentLibrary::initialize - WARNING - " << key() << " - default_path subkey does not exist ";
        valid = false;
    }
    if(!settings.contains(QString("%1/current_path").arg(key()))){
        qWarning() << "ContentLibrary::initialize - WARNING - " << key() << " - current_path subkey does not exist ";
        valid = false;
    }

    // Create subcomponents. Note: m_dir must exist to create subcomponents
    mp_libProjects    = new LibContent(this, PROJECTS_FOLDER_NAME);
    mp_libSongs       = new LibContent(this, SONGS_FOLDER_NAME);
    mp_libDrumSets    = new LibContent(this, DRUM_SETS_FOLDER_NAME);
    mp_libMidiSources = new LibContent(this, MIDI_SOURCES_FOLDER_NAME);
    mp_libWaveSources = new LibContent(this, WAVE_SOURCES_FOLDER_NAME);

    return  valid &&
            mp_libProjects->isValid() &&
            mp_libSongs->isValid() &&
            mp_libDrumSets->isValid() &&
            mp_libMidiSources->isValid() &&
            mp_libWaveSources->isValid();
}

QString ContentLibrary::key() const
{
   return QString("%1/%2").arg(mp_parent->name(), name());
}

QDir ContentLibrary::dir() const
{
    if(!m_valid){
        return QDir(QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation));
    }
    return m_dir;
}

QString ContentLibrary::defaultPath() const
{
    if(!m_valid){
        return QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    }
    return m_dir.absolutePath();
}
