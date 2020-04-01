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
#include "dircleanupmodal.h"
#include <QString>
#include <QStringList>




DirCleanUpModal::DirCleanUpModal(const QList<QString> &paths, QWidget *p_parent):
   m_paths(paths),
   mp_parent(p_parent),
   m_aborted(false)
{
}

DirCleanUpModal::DirCleanUpModal(const QString &path, QWidget *p_parent):
   mp_parent(p_parent),
   m_aborted(false)
{
   // Create a one element list to keep the same data structure
   QStringList list;
   list << path;
}

/**
 * @brief DirCleanUpModal::run
 * @return True is clean on clean up succes
 */
bool DirCleanUpModal::run()
{
   QProgressDialog progress(QObject::tr("Cleaning up previous files..."), QObject::tr("Abort"), 0, m_paths.count(), mp_parent);
   progress.setWindowModality(Qt::WindowModal);
   progress.setMinimumDuration(0);

   progress.setValue(0);

   foreach(const QString &location, m_paths){
      if(progress.wasCanceled() || m_aborted){
         return false;
      }

      QFileInfo info(location);
      if(!info.exists()){
         progress.setValue(progress.value() + 1);
      } else if(info.isFile()){
         QFile file(info.absoluteFilePath());
         while(!file.remove()){

            // File was somewhat removed despite error
            if(!info.exists()){
               break;
            }

            progress.show(); // always make sure progress is shown before displaying another dialog.
            if (QMessageBox::Abort == QMessageBox::question(nullptr, QObject::tr("Directory Cleanup"), QObject::tr("Unable to delete %1\nWhat would you like to do?").arg(info.absoluteFilePath()), QMessageBox::Retry | QMessageBox::Abort, QMessageBox::Retry)) {
               m_aborted = true;
               progress.setValue(progress.maximum());
               break;
            }
         }
         progress.setValue(progress.value() + 1);
      } else if (info.isDir()){
         QDir dir(info.absoluteFilePath());
         while(!dir.removeRecursively()){
            // File was somewhat removed despite error
            if(!info.exists()){
               break;
            }

            progress.show(); // always make sure progress is shown before displaying another dialog.
            if (QMessageBox::Abort == QMessageBox::question(nullptr, QObject::tr("Directory Cleanup"), QObject::tr("Unable to delete %1\nWhat would you like to do?").arg(info.absoluteFilePath()), QMessageBox::Retry | QMessageBox::Abort, QMessageBox::Retry)) {
               m_aborted = true;
               progress.setValue(progress.maximum());
               break;
            }
         }
         progress.setValue(progress.value() + 1);
      } else {
         progress.show(); // always make sure progress is shown before displaying another dialog.
         QMessageBox::warning(mp_parent, QObject::tr("Directory Cleanup"), QObject::tr("Skipping unknown file type"));
         progress.setValue(progress.value() + 1);
      }
   }
   if(progress.wasCanceled() || m_aborted){
      return false;
   }
   return true;
}
