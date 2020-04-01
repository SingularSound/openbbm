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
#include "extractzipmodal.h"
#include "quazip.h"
#include "quazipfile.h"
#include "../platform/platform.h"
#include <QObject>

ExtractZipModal::ExtractZipModal(const QString &destPath, const QString &sourceZipPath, const QStringList &sourceZippedFiles, QWidget *p_parent):
   QObject(nullptr)
{
   m_destPath          = destPath;
   m_sourceZipPath     = sourceZipPath;
   m_sourceZippedFiles = sourceZippedFiles;
   mp_parent           = p_parent;
   m_aborted           = false;

   m_progressType = ProgressFileCount; // By default
}

bool ExtractZipModal::run(){

   QFileInfo sourceRootFI(m_sourceZipPath);
   QFileInfo destRootFI(m_destPath);
   QDir destRootDir(destRootFI.absoluteFilePath());
   QuaZip zip(sourceRootFI.absoluteFilePath());

   if(!sourceRootFI.exists()){
      if (!QString("bcf").compare(sourceRootFI.suffix()))
          QMessageBox::critical(mp_parent, tr("Zip Extract"), tr("Source file %1 does not exist").arg(sourceRootFI.absoluteFilePath()));
      return false;
   }
   if(!destRootFI.exists()){
      QMessageBox::critical(mp_parent, tr("Zip Extract"), tr("Destination file %1 does not exist").arg(destRootFI.absoluteFilePath()));
      return false;
   }
   if(!zip.open(QuaZip::mdUnzip)){
      QMessageBox::critical(mp_parent, tr("Zip Extract"), tr("Unable to read archive %1").arg(sourceRootFI.absoluteFilePath()));
      return false;
   }

   QuaZipFile zipFile(&zip);

   // Loop on all files to retrieve total size
   int maxValue = m_sourceZippedFiles.count(); // By default
   if(m_progressType == ProgressFileSize){
      //retrieve total size
      maxValue = 0;
      foreach (const QString &sourceFilePath, m_sourceZippedFiles) {
         if(sourceFilePath.endsWith("/")){
            continue;
         }
         zip.setCurrentFile(sourceFilePath, QuaZip::csInsensitive);

         if(!zipFile.open(QIODevice::ReadOnly)){
            QMessageBox::critical(mp_parent, tr("Zip Extract"), tr("Unable to open source file %1 in archive %2").arg(sourceFilePath).arg(destRootFI.absoluteFilePath()));
            zip.close();
            return false;
         }

         maxValue += zipFile.usize(); // uncompressed size

         zipFile.close();
      }
   }

   QProgressDialog progress(tr("Extracting File..."), tr("Abort"), 0, maxValue, mp_parent);
   progress.setWindowModality(Qt::WindowModal);
   progress.setMinimumDuration(0);
   progress.setValue(0);

   // Make sure the dialog is displayed and that the content is refreshed
   // before before doing any long operation (like copying drumset), otherwise,
   // the app seems frozen
   progress.show();
   QApplication::processEvents();

   foreach (const QString &sourceFilePath, m_sourceZippedFiles) {
      if(progress.wasCanceled() || m_aborted){
         return false;
      }

      QFileInfo sourceFI(sourceFilePath);

      progress.setLabelText(QObject::tr("Extracting Files ...\n%1").arg(sourceFI.fileName()));
      QApplication::processEvents(); // Make sure file name is updated (even if costy operation)

      // remove all leading characters except for last "/" (doesn't seem to work on drives like "E:/" since drive name include the "/")
      QString destFilePath = sourceFI.absoluteFilePath().remove(0,sourceRootFI.absoluteFilePath().count());
      // In case of a drive, the last "/" was removed
      if(!destFilePath.startsWith("/",Qt::CaseInsensitive)){
         destFilePath = "/" + destFilePath;
      }
      destFilePath = destRootFI.absoluteFilePath() + destFilePath;


      // Copy file or folder
      if(sourceFilePath.endsWith("/")){
         // FOLDER
         if(!destRootDir.mkpath(sourceFilePath)){
            progress.show();
            QMessageBox::critical(mp_parent, tr("Zip Extract"), tr("Unable to create destination folder %1").arg(destRootDir.absoluteFilePath(sourceFilePath)));
            m_aborted = true;
            progress.setValue(progress.maximum());
            zip.close();
            return false;
         }

      } else {
         // FILE
         zip.setCurrentFile(sourceFilePath, QuaZip::csInsensitive);

         if(!zipFile.open(QIODevice::ReadOnly)){
            progress.show();
            QMessageBox::critical(mp_parent, tr("Zip Extract"), tr("Unable to open source file %1 in archive %2").arg(sourceFilePath).arg(destRootFI.absoluteFilePath()));
            m_aborted = true;
            progress.setValue(progress.maximum());
            zip.close();
            return false;
         }

         QFile outFile(destRootDir.absoluteFilePath(sourceFilePath));
         if(!outFile.open(QIODevice::WriteOnly)){
            progress.show();
            QMessageBox::critical(mp_parent, tr("Zip Extract"), tr("Unable to create destination file %1 in folder %2").arg(sourceFilePath).arg(destRootFI.absoluteFilePath()));
            m_aborted = true;
            progress.setValue(progress.maximum());
            zipFile.close();
            zip.close();
            return false;
         }

         // Copy

         QByteArray buffer;
         int chunksize = (1024*1024); // Whatever chunk size you like
         buffer = zipFile.read(chunksize);
         while(!buffer.isEmpty()){
            if(progress.wasCanceled() || m_aborted){
               outFile.close();
               zipFile.close();
               zip.close();
               return false;
            }

            outFile.write(buffer);

            if(m_progressType == ProgressFileSize){
               progress.setValue(progress.value() + buffer.size());
            }

            buffer = zipFile.read(chunksize);
         }

         outFile.close();
         zipFile.close();
      }

      if(m_progressType == ProgressFileCount){
         progress.setValue(progress.value() + 1);
      } else {
         // already precessed
      }
   }

   if(progress.wasCanceled() || m_aborted){
      return false;
   }
   return true;
}
