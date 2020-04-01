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
#include "dirlistallsubfilesmodal.h"

#include "../model/beatsmodelfiles.h"

DirListAllSubFilesModal::DirListAllSubFilesModal(const QString &location, int rootConfig, QWidget *p_parent, bool absolute):
   m_locations(),
   m_rootConfig(rootConfig),
   mp_parent(p_parent)
{
   m_locations.append(location);
   m_absolute = absolute;
   m_rootDir = QDir(location);
   m_totalFileSize = 0;
}

DirListAllSubFilesModal::DirListAllSubFilesModal(const QStringList &locations, int rootConfig, QWidget *p_parent, const QString &rootForRelative):
m_locations(locations),
m_rootConfig(rootConfig),
mp_parent(p_parent)
{
   if(rootForRelative.isEmpty()){
      m_absolute = true;
   } else {
      m_rootDir = QDir(rootForRelative);
      m_absolute = false;
   }
   m_totalFileSize = 0;
}

bool DirListAllSubFilesModal::run(bool ignoreFootsw_ini, const QStringList &nameFilters)
{
   // prepare dialog
   QProgressDialog progress(QObject::tr("Listing files to process..."), QObject::tr("Abort"), 0, m_locations.count(), mp_parent);
   progress.setWindowModality(Qt::WindowModal);
   progress.setMinimumDuration(0);

   progress.setValue(0);

   foreach(const QString &location, m_locations){
      if(progress.wasCanceled()){
         return false;
      }

      QFileInfo info(location);
      if(m_absolute){
         if(!info.exists()){
            progress.setValue(progress.value() + 1);
         } else {
            if(m_rootConfig & rootFirst){
               m_result.append(info.absoluteFilePath());
               if(info.isFile()){
                  m_totalFileSize += info.size();
               }
            }
            exploreChildren(info.absoluteFilePath(), progress, ignoreFootsw_ini, nameFilters);

            if(m_rootConfig & rootLast){
               m_result.append(info.absoluteFilePath());
               if(info.isFile()){
                  m_totalFileSize += info.size();
               }
            }
            progress.setValue(progress.value() + 1);
         }
      } else {
         if(!info.exists() || !info.absoluteFilePath().startsWith(m_rootDir.absolutePath())){
            progress.setValue(progress.value() + 1);
            continue;
         }
         
         // Make sure path is relative
         QString explorePath(info.absoluteFilePath());
         explorePath.remove(0, m_rootDir.absolutePath().count());
         if(explorePath.startsWith("/")){
            explorePath.remove(0, 1);
         }

         if(info.isFile()){
            m_result.append(explorePath);
            m_totalFileSize += info.size();
         } else {
            // All folder except for root end with "/" (root is empty)
            if(!explorePath.isEmpty() && !explorePath.endsWith("/")){
               explorePath.append("/");
            }

            if((m_rootConfig & rootFirst) && !explorePath.isEmpty()){ // don't list root which is empty
               m_result.append(explorePath);
            }

            exploreChildren(explorePath, progress, ignoreFootsw_ini, nameFilters);

            if((m_rootConfig & rootLast) && !explorePath.isEmpty()){ // don't list root which is empty
               m_result.append(explorePath);
            }
         }

         progress.setValue(progress.value() + 1);
      }
   }
   if(progress.wasCanceled()){
      return false;
   }
   return true;
}

void DirListAllSubFilesModal::exploreChildren(QString dirPath, QProgressDialog &progress, bool ignoreFootsw_ini, const QStringList &nameFilters)
{
   if(progress.wasCanceled()){
      return;
   }

   QDir dir(dirPath);
   if(!m_absolute){
      dir = QDir(m_rootDir.absoluteFilePath(dirPath));
   }

   // 1 - Add the directory count to the amount of items to process
   int dirCount = dir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot | QDir::Hidden).count();

   if(dirCount != 0){
      progress.setMaximum(progress.maximum() + dirCount);
   }

   // 2 - retrieve all files (leafs)
   foreach(const QFileInfo &info, dir.entryInfoList(nameFilters, QDir::Files | QDir::Hidden | QDir::NoDotAndDotDot)) {
       if(progress.wasCanceled()){
           return;
       }

       //This section skips over FOOTSW.INI when doing an export/import to/from pedal
       //By default, we don't ignore any files
       if (ignoreFootsw_ini) {
           if (dir.dirName() == "PARAMS") {
               if (info.fileName().compare(BMFILES_FOOTSW_CONFIG_FILE_NAME, Qt::CaseInsensitive) == 0 ||
                   info.fileName().compare(BMFILES_HASH_FILE_NAME, Qt::CaseInsensitive) == 0) {
                    continue; //skip over file
               }
           }
       }

       if(m_absolute){
           m_result.append(info.absoluteFilePath());
       } else {
           m_result.append(QString("%1%2").arg(dirPath, info.fileName()));
       }
       m_totalFileSize += info.size();

   }

   // 3 - retrieve and explore all dirs
   foreach(const QFileInfo &info, dir.entryInfoList(QDir::Dirs | QDir::Hidden | QDir::NoDotAndDotDot)) {
      if(progress.wasCanceled()){
         return;
      }

      // start by adding root path path
      if(m_rootConfig & rootFirst){
         if(m_absolute){
            m_result.append(info.absoluteFilePath());
         } else {
            m_result.append(QString("%1%2/").arg(dirPath, info.fileName()));
         }
      }

      // Explore recursively
      if(m_absolute){
         exploreChildren(info.absoluteFilePath(), progress, ignoreFootsw_ini, nameFilters);
      } else {
         exploreChildren(QString("%1%2/").arg(dirPath, info.fileName()), progress, ignoreFootsw_ini, nameFilters);
      }

      // end by adding root path path
      if(m_rootConfig & rootLast){
         if(m_absolute){
            m_result.append(info.absoluteFilePath());
         } else {
            m_result.append(QString("%1%2/").arg(dirPath, info.fileName()));
         }
      }

      progress.setValue(progress.value() + 1);
   }
}
