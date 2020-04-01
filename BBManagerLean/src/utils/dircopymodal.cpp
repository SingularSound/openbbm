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
#include "dircopymodal.h"
#include "../platform/platform.h"



DirCopyModal::DirCopyModal(const QString &destPath, const QString &sourcePath, const QStringList &sourceFiles, QWidget *p_parent)
{
   m_destPath    = destPath;
   m_sourcePath  = sourcePath;
   m_sourceFiles = sourceFiles;
   mp_parent     = p_parent;
   m_aborted     = false;
   m_progressType = ProgressFileCount; // By default
   m_totalSize = 0; // By default
}

bool DirCopyModal::run()
{
   QFileInfo sourceRootFI(m_sourcePath);
   QFileInfo destRootFI(m_destPath);

   if(!sourceRootFI.exists()){
      if (!QString("bcf").compare(sourceRootFI.suffix()))
        QMessageBox::critical(mp_parent, QObject::tr("Directory copy"), QObject::tr("Source file %1 does not exist").arg(sourceRootFI.absoluteFilePath()));
      return false;
   }
   if(!destRootFI.exists()){
      QMessageBox::critical(mp_parent, QObject::tr("Directory copy"), QObject::tr("Destination file %1 already exists").arg(destRootFI.absoluteFilePath()));
      return false;
   }

   int maxValue = m_sourceFiles.count(); // By default
   if(m_progressType == ProgressFileSize){
      maxValue = m_totalSize; // this is using a qint64 to signal a progress bar's progress, which is an int; there must be a way to downscale it.
   }

   QProgressDialog progress(QObject::tr("Copying files..."), QObject::tr("Abort"), 0, maxValue, mp_parent);
   progress.setWindowModality(Qt::WindowModal);
   progress.setMinimumDuration(0);
   progress.setValue(0);

   // Make sure the dialog is displayed and that the content is refreshed
   // before before doing any long operation (like copying drumset), otherwise,
   // the app seems frozen
   progress.show();
   QApplication::processEvents();

   foreach (const QString &sourceFilePath, m_sourceFiles) {
      if(progress.wasCanceled() || m_aborted){
         return false;
      }

      QFileInfo sourceFI(sourceFilePath);

      progress.setLabelText(QObject::tr("Copying files...\n%1").arg(sourceFI.fileName()));
      QApplication::processEvents(); // Make sure file name is updated (even if costy operation)

      if(!sourceFI.absoluteFilePath().startsWith(sourceRootFI.absoluteFilePath(), Qt::CaseInsensitive)){
         progress.show(); // always make sure progress is shown before displaying another dialog.
         QMessageBox::warning(mp_parent, QObject::tr("Directory copy"), QObject::tr("Trying to copy file from wrong location %1\nSkipping file...").arg(sourceFilePath));

         if(m_progressType == ProgressFileCount){
            progress.setValue(progress.value() + 1);
         } else {
            if(sourceFI.isFile()){
               progress.setValue(progress.value() + sourceFI.size());
            }
         }
         continue;
      }

      // remove all leading characters except for last "/" (doesn't seem to work on drives like "E:/" since drive name include the "/")
      QString destFilePath = sourceFI.absoluteFilePath().remove(0,sourceRootFI.absoluteFilePath().count());
      // In case of a drive, the last "/" was removed
      if(!destFilePath.startsWith("/",Qt::CaseInsensitive)){
         destFilePath = "/" + destFilePath;
      }
      destFilePath = destRootFI.absoluteFilePath() + destFilePath;

      QFileInfo destFI(destFilePath);
      auto ok = true;
      while (ok && destFI.exists()) {
         progress.show(); // always make sure progress is shown before displaying another dialog.
         QMessageBox msgBox(QMessageBox::Question, QObject::tr("Directory copy"), QObject::tr("Destination file already exists (%1)\nWhat would you like to do?").arg(destFI.absoluteFilePath()), QMessageBox::Discard | QMessageBox::Cancel | QMessageBox::Abort);
         msgBox.button(QMessageBox::Discard)->setText(QObject::tr("Delete First"));
         msgBox.button(QMessageBox::Cancel) ->setText(QObject::tr("Skip"));
         msgBox.setDefaultButton(QMessageBox::Discard);
         switch (msgBox.exec()) {
         case QMessageBox::Abort :
            progress.setValue(progress.maximum());
            m_aborted = true;
            return false;
         case QMessageBox::Discard :
            if(destFI.isFile()){
               QFile destFile(destFI.absoluteFilePath());
               destFile.remove();
            } else {
               QDir destFile(destFI.absoluteFilePath());
               destFile.removeRecursively();
            }
            break;
         default:
             ok = false;
         }
      }
      // If skip was selected, skip and go to next file
      if(destFI.exists()){
         if(m_progressType == ProgressFileCount){
            progress.setValue(progress.value() + 1);
         } else {
            if(sourceFI.isFile()){
               progress.setValue(progress.value() + sourceFI.size());
            }
         }
         continue;
      }

      // Copy File or Dir

      // Dir
      if(sourceFI.isDir()){
         QDir parentDestDir(destFI.absolutePath());
         while(!parentDestDir.mkdir(destFI.fileName())){
            progress.show(); // always make sure progress is shown before displaying another dialog.
            if (QMessageBox::Abort == QMessageBox::question(0, QObject::tr("Directory copy"), QObject::tr("Unable to create destination folder %1\nWhat would you like to do?").arg(destFI.absoluteFilePath()), QMessageBox::Retry | QMessageBox::Abort)){
               progress.setValue(progress.maximum());
               m_aborted = true;
               return false;
            }
         }

      // File
      } else if (sourceFI.isFile()){

         // Proceed differently with small files than with big files
         // Small files are copied directly with copy function
         // Big files are copied by chunks in order to provide responsivity to progress dialog
         // Threshold is set to 10 M for now
         if(sourceFI.size() > (10*1024*1024) ){
            QFile inFile(sourceFI.absoluteFilePath());
            if(!inFile.open(QIODevice::ReadOnly)){
               progress.show();
               QMessageBox::critical(mp_parent, QObject::tr("Directory copy"), QObject::tr("Unable to open source file %1").arg(sourceFI.absoluteFilePath()));
               progress.setValue(progress.maximum());
               return false;
            }

            QFile outFile(destFI.absoluteFilePath());

            // Note: since relative, source=dst
            if(!outFile.open(QIODevice::WriteOnly)){
               progress.show();
               QMessageBox::critical(mp_parent, QObject::tr("Directory copy"), QObject::tr("Unable to create destination file %1").arg(destRootFI.absoluteFilePath()));
               progress.setValue(progress.maximum());
               inFile.close();
               return false;
            }

            // Copy
            QByteArray buffer;
            int chunksize = (1024*1024); // Whatever chunk size you like
            buffer = inFile.read(chunksize);
            while(!buffer.isEmpty()){
               if(progress.wasCanceled() || m_aborted){
                  outFile.close();
                  inFile.close();
                  return false;
               }
               outFile.write(buffer);

               if(m_progressType == ProgressFileSize){
                  progress.setValue(progress.value() + buffer.size());
               }
               buffer = inFile.read(chunksize);
            }

            outFile.close();
            inFile.close();

         } else {
            while (!QFile::copy(sourceFI.absoluteFilePath(), destFI.absoluteFilePath())) {
               progress.show(); // always make sure progress is shown before displaying another dialog.
               if (QMessageBox::Abort == QMessageBox::question(0, QObject::tr("Directory copy"), QObject::tr("Error copying %1 to %2").arg(sourceFI.absoluteFilePath(), destFI.absoluteFilePath()), QMessageBox::Retry | QMessageBox::Abort)){
                  progress.setValue(progress.maximum());
                  m_aborted = true;
                  return false;
               }
            }
         }
      }

      if(m_progressType == ProgressFileCount){
         progress.setValue(progress.value() + 1);
      } else {
         if(sourceFI.isFile() && sourceFI.size() <= (10*1024*1024)){
            progress.setValue(progress.value() + sourceFI.size());
         }
      }
   }

   if(progress.wasCanceled() || m_aborted){
      return false;
   }
   return true;

}
