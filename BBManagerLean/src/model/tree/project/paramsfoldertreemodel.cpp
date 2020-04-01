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
#include <QCryptographicHash>
#include <QUuid>
#include "minIni.h"

#include "paramsfoldertreemodel.h"
#include "beatsprojectmodel.h"
#include "../../beatsmodelfiles.h"

ParamsFolderTreeModel::ParamsFolderTreeModel(BeatsProjectModel *p_model, FolderTreeItem *parent):
   FolderTreeItem(p_model, parent)
{
   setName("PARAMS");
   setFileName("PARAMS");

   // default
   m_primaryStopped = STOPPED_ACCENT_HIT;
   m_secondaryStopped = STOPPED_SONG_ADVANCE;
   m_primaryPlaying = PLAYING_ACCENT_HIT;
   m_secondaryPlaying = PLAYING_PAUSE_UNPAUSE;

}

/**
 * @brief ParamsFolderTreeModel::createProjectSkeleton
 * @return
 *
 * re-implementation of ContentFolderTreeItem::createProjectSkeleton
 * if changes are performed, computeHash() and model()->setArchiveDirty() need to be called after this call, depending on the process.
 */
bool ParamsFolderTreeModel::createProjectSkeleton()
{
   QFileInfo fi = folderFI();
   QDir dir(fi.absoluteFilePath());

   bool changed = false;

   if(fileName().isEmpty()){
      // Skeleton is only created if folderName exists
      return false;
   }

   if(FolderTreeItem::createProjectSkeleton()){
      changed = true;
   }

   // File info creation
   m_infoFilePath = dir.absoluteFilePath(BMFILES_INFO_MAP_FILE_NAME);
   QFile infoFile(m_infoFilePath);
   if(!infoFile.exists()){
      m_infoFileContent.insert("project_uuid", QVariant(QUuid::createUuid()));
      m_infoFileContent.insert("link_hash", QVariant());
      if(infoFile.open(QIODevice::WriteOnly)){
         QDataStream fout(&infoFile);
         fout << m_infoFileContent;
      }
      infoFile.flush();
      infoFile.close();
      // NOTE - INFO.BCF excluded from hash calculation since different on pedal and local project
   } else {
      if(infoFile.open(QIODevice::ReadOnly)){
         QDataStream fin(&infoFile);
         fin >> m_infoFileContent;
         infoFile.flush();
         infoFile.close();
      }
   }

   // Foot Switch configuration creation
   m_footswFilePath = dir.absoluteFilePath(BMFILES_FOOTSW_CONFIG_FILE_NAME);
   QFile footswFile(m_footswFilePath);
   if(!footswFile.exists()){
      // Create with default values

      if(!ini_putl("FOOTSWITCH_ACTIONS","PRIMARY_STOPPED",m_primaryStopped, m_footswFilePath.toLocal8Bit().data())){
          qWarning() << "ParamsFolderTreeModel::createProjectSkeleton - ERROR 1 - unable to write primary stopped setting";
      }

      if(!ini_putl("FOOTSWITCH_ACTIONS","PRIMARY_PLAYING",m_primaryPlaying, m_footswFilePath.toLocal8Bit().data())){
         qWarning() << "ParamsFolderTreeModel::createProjectSkeleton - ERROR 2 - unable to write primary playing setting";
      }

      if(!ini_putl("FOOTSWITCH_ACTIONS","SECONDARY_STOPPED",m_secondaryStopped, m_footswFilePath.toLocal8Bit().data())){
         qWarning() << "ParamsFolderTreeModel::createProjectSkeleton - ERROR 3 - unable to write secondary stopped setting";
      }

      if(!ini_putl("FOOTSWITCH_ACTIONS","SECONDARY_PLAYING",m_secondaryPlaying, m_footswFilePath.toLocal8Bit().data())){
         qWarning() << "ParamsFolderTreeModel::createProjectSkeleton - ERROR 4 - unable to write secondary playing setting";
      }
      changed = true;
   } else {
      parseFsSettings();
   }

   return changed;
}

/**
 * @brief ParamsFolderTreeModel::prepareSync
 * @param dstPath
 * @param p_cleanUp
 * @param p_copySrc
 * @param p_copyDst
 *
 * re-implementation of AbstractTreeItem::prepareSync
 * Called as part of BeatsProjectModel::synchronizeModal.
 */
void ParamsFolderTreeModel::prepareSync(const QString & dstPath, QList<QString> *p_cleanUp, QList<QString> *p_copySrc, QList<QString> *p_copyDst)
{
   QFileInfo fi = folderFI();
   QDir dir(fi.absoluteFilePath());

   //NOTE: we ignore PARAMS folder during synchronization
   //We leave the code below in case we want to use it a later time
   return;

   if(!p_cleanUp || !p_copySrc || !p_copyDst){
      qWarning() << "SongFileItem::prepareSync - ERROR 1 - return list is null";
      return;
   }

   // NOTE: INFO.BCF MUST NOT be copied since different on pedal and local project
      

   QFileInfo dstFI(dstPath);
   QDir dstDir(dstPath);

   if(dstFI.exists() && compareHash(dstPath)){
      return;
   }


   if(!dstFI.exists()){
      // if directory does not exist, simply add FOOTSW.INI

      // Append self
      p_copySrc->append(fi.absoluteFilePath());
      p_copyDst->append(dstFI.absoluteFilePath());

      // add FOOTSW.INI file
      p_copySrc->append(dir.absoluteFilePath(BMFILES_FOOTSW_CONFIG_FILE_NAME));
      p_copyDst->append(dstDir.absoluteFilePath(BMFILES_FOOTSW_CONFIG_FILE_NAME));

      // add hash file
      p_copySrc->append(dir.absoluteFilePath(BMFILES_HASH_FILE_NAME));
      p_copyDst->append(dstDir.absoluteFilePath(BMFILES_HASH_FILE_NAME));

   } else {

      // replace FOOTSW.INI file
      p_cleanUp->append(dstDir.absoluteFilePath(BMFILES_FOOTSW_CONFIG_FILE_NAME));
      p_copySrc->append(dir.absoluteFilePath(BMFILES_FOOTSW_CONFIG_FILE_NAME));
      p_copyDst->append(dstDir.absoluteFilePath(BMFILES_FOOTSW_CONFIG_FILE_NAME));

      // replace hash file
      p_cleanUp->append(dstDir.absoluteFilePath(BMFILES_HASH_FILE_NAME));
      p_copySrc->append(dir.absoluteFilePath(BMFILES_HASH_FILE_NAME));
      p_copyDst->append(dstDir.absoluteFilePath(BMFILES_HASH_FILE_NAME));
   }
}

QString ParamsFolderTreeModel::infoFileName()
{
   return BMFILES_INFO_MAP_FILE_NAME;
}
QString ParamsFolderTreeModel::infoRelativePath(){
   return "PARAMS/" + infoFileName();
}

/**
 * @brief ParamsFolderTreeModel::parseFsSettings
 *
 * Parses the footswitch settings. Called as part of createProjectSkeleton
 */
void ParamsFolderTreeModel::parseFsSettings()
{
   m_primaryStopped = (STOPPED_ACTION_params) ini_getl("FOOTSWITCH_ACTIONS","PRIMARY_STOPPED",STOPPED_ACCENT_HIT, m_footswFilePath.toLocal8Bit().data());
   m_secondaryStopped = (STOPPED_ACTION_params) ini_getl("FOOTSWITCH_ACTIONS","SECONDARY_STOPPED",STOPPED_SONG_ADVANCE, m_footswFilePath.toLocal8Bit().data());
   m_primaryPlaying = (PLAYING_ACTION_params) ini_getl("FOOTSWITCH_ACTIONS","PRIMARY_PLAYING",PLAYING_ACCENT_HIT, m_footswFilePath.toLocal8Bit().data());
   m_secondaryPlaying = (PLAYING_ACTION_params) ini_getl("FOOTSWITCH_ACTIONS","SECONDARY_PLAYING",PLAYING_PAUSE_UNPAUSE, m_footswFilePath.toLocal8Bit().data());

}

/**
 * @brief ParamsFolderTreeModel::getInfoMap
 * @param path
 * @return
 *
 *
 * STATIC method that parses the info file (at specified path) and returns content as a map
 */
QMap<QString, QVariant> ParamsFolderTreeModel::getInfoMap(const QString &path)
{
   QDir paramsDir(path);
   if(!paramsDir.exists()){
      return QMap<QString, QVariant>();
   }

   QFileInfo paramsInfoFI(paramsDir.absoluteFilePath(infoFileName()));
   if(!paramsInfoFI.exists() || !paramsInfoFI.isFile()){
      return QMap<QString, QVariant>();
   }

   QFile paramsInfoFile(paramsInfoFI.absoluteFilePath());
   if(!paramsInfoFile.open(QIODevice::ReadOnly)){
      return QMap<QString, QVariant>();
   }

   QDataStream fin(&paramsInfoFile);
   QMap<QString, QVariant> map;
   fin >> map;
   paramsInfoFile.flush();
   paramsInfoFile.close();

   return map;
}

/**
 * @brief ParamsFolderTreeModel::infoMap
 * @return
 *
 * Method that parses the info file (at expected path) and returns content as a map
 */
QMap<QString, QVariant> ParamsFolderTreeModel::infoMap()
{
   QFileInfo paramsInfoFI(m_infoFilePath);
   if(!paramsInfoFI.exists() || !paramsInfoFI.isFile()){
      return QMap<QString, QVariant>();
   }

   QFile paramsInfoFile(paramsInfoFI.absoluteFilePath());
   if(!paramsInfoFile.open(QIODevice::ReadOnly)){
      return QMap<QString, QVariant>();
   }

   QDataStream fin(&paramsInfoFile);
   QMap<QString, QVariant> map;
   fin >> map;
   paramsInfoFile.flush();
   paramsInfoFile.close();

   return map;
}


/**
 * @brief ParamsFolderTreeModel::setInfoMap
 * @param path
 * @param map
 * @return
 *
 * STATIC method that writes the content of the map in the info file (at specified path)
 * computeHash() doesn't not need to be computed since infoMap different on pedal than in project
 * model()->setArchiveDirty() need to be called after this call.
 */
bool ParamsFolderTreeModel::setInfoMap(const QString &path, const QMap<QString, QVariant> &map)
{
   QDir paramsDir(path);
   if(!paramsDir.exists()){
      return false;
   }

   QFileInfo paramsInfoFI(paramsDir.absoluteFilePath(infoFileName()));
   if(!paramsInfoFI.exists() || !paramsInfoFI.isFile()){
      return false;
   }

   QFile paramsInfoFile(paramsInfoFI.absoluteFilePath());
   if(!paramsInfoFile.open(QIODevice::WriteOnly)){
      return false;
   }

   QDataStream fout(&paramsInfoFile);
   fout << map;
   paramsInfoFile.flush();
   paramsInfoFile.close();

   return true;
}

/**
 * @brief ParamsFolderTreeModel::setInfoMap
 * @param map
 * @return
 *
 * method that writes the content of the map in the info file (at expected path)
 * computeHash() doesn't not need to be computed since infoMap different on pedal than in project
 * model()->setArchiveDirty() need to be called after this call.
 */
bool ParamsFolderTreeModel::setInfoMap(const QMap<QString, QVariant> &map)
{
   QFileInfo paramsInfoFI(m_infoFilePath);
   if(!paramsInfoFI.exists() || !paramsInfoFI.isFile()){
      return false;
   }

   QFile paramsInfoFile(paramsInfoFI.absoluteFilePath());
   if(!paramsInfoFile.open(QIODevice::WriteOnly)){
      return false;
   }

   QDataStream fout(&paramsInfoFile);
   fout << map;
   paramsInfoFile.flush();
   paramsInfoFile.close();
   return true;
}

/**
 * @brief ParamsFolderTreeModel::getProjectUuid
 * @param path
 * @return
 *
 * STATIC method that retrieves project uuid out of the map in the info file (at specified path)
 */
QUuid ParamsFolderTreeModel::getProjectUuid(const QString &path)
{
   QMap<QString, QVariant> map = getInfoMap(path);

   if(!map.contains("project_uuid")){
      return QUuid();
   }

   return map.value("project_uuid").toUuid();
}

/**
 * @brief ParamsFolderTreeModel::projectUuid
 * @return
 *
 * method that retrieves project uuid out of the map in the info file (at expected path)
 */
QUuid ParamsFolderTreeModel::projectUuid()
{
   QMap<QString, QVariant> map = infoMap();

   if(!map.contains("project_uuid")){
      return QUuid();
   }

   return map.value("project_uuid").toUuid();
}

/**
 * @brief ParamsFolderTreeModel::setProjectUuid
 * @param path
 * @param uuid
 * @return
 *
 * STATIC method that writes project uuid in the map in the info file (at specified path)
 * computeHash() doesn't not need to be computed since infoMap different on pedal than in project
 * model()->setArchiveDirty() need to be called after this call.
 */
bool ParamsFolderTreeModel::setProjectUuid(const QString &path, const QUuid &uuid)
{
   QMap<QString, QVariant> map = getInfoMap(path);
   map.insert("project_uuid", QVariant(uuid));

   // need to reset link hash for computing folder hash
   map.insert("link_hash", QVariant());

   if(!setInfoMap(path, map)){
      qWarning() << "ParamsFolderTreeModel::setProjectUuid - ERROR 1 - setInfoMap returned false";
      return false;
   }

   return true;
}

/**
 * @brief ParamsFolderTreeModel::getLinkHash
 * @param path
 * @return
 *
 * STATIC method that retrieves project uuid out of the map in the info file (at specified path)
 */
QByteArray ParamsFolderTreeModel::getLinkHash(const QString &path)
{
   QMap<QString, QVariant> map = getInfoMap(path);

   if(!map.contains("link_hash")){
      return QByteArray();
   }

   return map.value("link_hash").toByteArray();
}

/**
 * @brief ParamsFolderTreeModel::setLinkHash
 * @param path
 * @param linkHash
 * @return
 *
 * STATIC method that writes project link hash in the map in the info file (at specified path)
 * Since link hash is not part of folder hash, no need to call computeHashStatic
 * model()->setArchiveDirty() need to be called after this call.
 */
bool ParamsFolderTreeModel::setLinkHash(const QString &path, const QByteArray &linkHash)
{
   QMap<QString, QVariant> map = getInfoMap(path);
   if(map.isEmpty()){
      return false;
   }

   map.insert("link_hash", QVariant(linkHash));

   return setInfoMap(path, map);
}
/**
 * @brief ParamsFolderTreeModel::resetProjectInfo
 * @param path
 * @return
 *
 * STATIC method that clears the link hash and creates a new uuid for the project. The new information is written in the info file (at specified path)
 * model()->setArchiveDirty() need to be called after this call.
 */
bool ParamsFolderTreeModel::resetProjectInfo(const QString &path)
{
   QMap<QString, QVariant> map;
   map.insert("project_uuid", QVariant(QUuid::createUuid()));
   map.insert("link_hash", QVariant());

   if(!setInfoMap(path, map)){
      qWarning() << "ParamsFolderTreeModel::resetProjectInfo - ERROR 1 - setInfoMap returned false";
      return false;
   }

   return true;
}

/**
 * @brief ParamsFolderTreeModel::resetProjectInfo
 * @return
 *
 * method that clears the link hash and creates a new uuid for the project. The new information is written in the info file (at expected path)
 * computeHash() doesn't not need to be computed since infoMap different on pedal than in project
 */
bool ParamsFolderTreeModel::resetProjectInfo()
{
   QMap<QString, QVariant> map;
   map.insert("project_uuid", QVariant(QUuid::createUuid()));
   map.insert("link_hash", QVariant());

   if(!setInfoMap(map)){
      return false;
   }

   model()->setProjectDirty();

   return true;
}

/**
 * @brief ParamsFolderTreeModel::computeHash
 *
 * Re-implementation of AbstractTreeItem::removeChild.
 * model()->setArchiveDirty() need to be called after this call.
 */
void ParamsFolderTreeModel::computeHash(bool /*recursive*/)
{
   QFileInfo fi = folderFI();
   QDir dir(fi.absoluteFilePath());

   // NOTE - INFO.BCF excluded from hash calculation since different on pedal and local project

   QCryptographicHash cr(QCryptographicHash::Sha256);

   QFileInfo paramsFootswFI(dir.absoluteFilePath(BMFILES_FOOTSW_CONFIG_FILE_NAME));
   if(!paramsFootswFI.exists() || !paramsFootswFI.isFile()){
      qWarning() << "ParamsFolderTreeModel::computeHash - ERROR 3 - Unable to open " << paramsFootswFI.absoluteFilePath();

      setHash(QByteArray());
      return;
   }

   QFile paramsFootswFile(paramsFootswFI.absoluteFilePath());
   if(!paramsFootswFile.open(QIODevice::ReadOnly)){
      qWarning() << "ParamsFolderTreeModel::computeHash - ERROR 4 - Unable to open " << paramsFootswFI.absoluteFilePath();
      setHash(QByteArray());
      return;
   }

   cr.addData(&paramsFootswFile);
   paramsFootswFile.close();
   setHash(cr.result());
}

/**
 * @brief ParamsFolderTreeModel::primaryStopped
 * @return
 *
 * Utility that returns the state of Footswitch setting
 */
STOPPED_ACTION_params ParamsFolderTreeModel::primaryStopped() const
{
   return m_primaryStopped;
}

/**
 * @brief ParamsFolderTreeModel::setPrimaryStopped
 * @param primaryStopped
 *
 * Utility function that writes the new Frroswitch setting
 */
void ParamsFolderTreeModel::setPrimaryStopped(const STOPPED_ACTION_params &primaryStopped)
{
   if( m_primaryStopped != primaryStopped){
      m_primaryStopped = primaryStopped;
      if(!ini_putl("FOOTSWITCH_ACTIONS","PRIMARY_STOPPED",primaryStopped, m_footswFilePath.toLocal8Bit().data())){
         qWarning() << "ParamsFolderTreeModel::setPrimaryStopped - ERROR 1 - unable to write primary stopped setting";
      }
      computeHash(false);
      propagateHashChange();
      model()->setProjectDirty();
   }
}

/**
 * @brief ParamsFolderTreeModel::secondaryStopped
 * @return
 *
 * Utility that returns the state of Footswitch setting
 */
STOPPED_ACTION_params ParamsFolderTreeModel::secondaryStopped() const
{
   return m_secondaryStopped;
}

/**
 * @brief ParamsFolderTreeModel::setSecondaryStopped
 * @param secondaryStopped
 *
 * Utility function that writes the new Frroswitch setting
 */
void ParamsFolderTreeModel::setSecondaryStopped(const STOPPED_ACTION_params &secondaryStopped)
{
   if(m_secondaryStopped != secondaryStopped){
      m_secondaryStopped = secondaryStopped;
      if(!ini_putl("FOOTSWITCH_ACTIONS","SECONDARY_STOPPED",secondaryStopped, m_footswFilePath.toLocal8Bit().data())){
         qWarning() << "ParamsFolderTreeModel::setSecondaryStopped - ERROR 1 - unable to write secondary stopped setting";
      }
      computeHash(false);
      propagateHashChange();
      model()->setProjectDirty();
   }
}

/**
 * @brief ParamsFolderTreeModel::primaryPlaying
 * @return
 *
 * Utility that returns the state of Footswitch setting
 */
PLAYING_ACTION_params ParamsFolderTreeModel::primaryPlaying() const
{
   return m_primaryPlaying;
}

/**
 * @brief ParamsFolderTreeModel::setPrimaryPlaying
 * @param primaryPlaying
 *
 * Utility function that writes the new Frroswitch setting
 */
void ParamsFolderTreeModel::setPrimaryPlaying(const PLAYING_ACTION_params &primaryPlaying)
{
   if(m_primaryPlaying != primaryPlaying){
      m_primaryPlaying = primaryPlaying;
      if(!ini_putl("FOOTSWITCH_ACTIONS","PRIMARY_PLAYING",primaryPlaying, m_footswFilePath.toLocal8Bit().data())){
         qWarning() << "ParamsFolderTreeModel::setPrimaryPlaying - ERROR 1 - unable to write primary playing setting";
      }
      computeHash(false);
      propagateHashChange();
      model()->setProjectDirty();
   }
}

/**
 * @brief ParamsFolderTreeModel::secondaryPlaying
 * @return
 *
 * Utility that returns the state of Footswitch setting
 */
PLAYING_ACTION_params ParamsFolderTreeModel::secondaryPlaying() const
{
   return m_secondaryPlaying;
}

/**
 * @brief ParamsFolderTreeModel::setSecondaryPlaying
 * @param secondaryPlaying
 *
 * Utility function that writes the new Frroswitch setting
 */
void ParamsFolderTreeModel::setSecondaryPlaying(const PLAYING_ACTION_params &secondaryPlaying)
{
   if(m_secondaryPlaying != secondaryPlaying){
      m_secondaryPlaying = secondaryPlaying;
      if(!ini_putl("FOOTSWITCH_ACTIONS","SECONDARY_PLAYING",secondaryPlaying, m_footswFilePath.toLocal8Bit().data())){
         qWarning() << "ParamsFolderTreeModel::setSecondaryPlaying - ERROR 1 - unable to write secondary playing setting";
      }
      computeHash(false);
      propagateHashChange();
      model()->setProjectDirty();
   }
}

