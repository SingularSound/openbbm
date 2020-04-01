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
#include "compresszipmodal.h"
#include "quazip.h"
#include "quazipfile.h"

CompressZipModal::CompressZipModal(const QString &destZipPath, const QString &sourcePath, const QStringList &sourceFiles, QWidget *p_parent) :
   QObject(nullptr)
{

   mp_parent     = p_parent;

   m_destZipPath = destZipPath;
   m_sourcePath  = sourcePath;
   m_sourceFiles = sourceFiles;

   m_aborted     = false;

   m_progressType = ProgressFileCount; // By default
   m_totalSize = 0; // By default
}


bool CompressZipModal::run(){
   QFileInfo sourceRootFI(m_sourcePath);
   QFileInfo destRootFI(m_destZipPath);
   QDir sourceRootDir(sourceRootFI.absoluteFilePath());
   QuaZip zip(destRootFI.absoluteFilePath());

   if(!sourceRootFI.exists()){
       if (!QString("bcf").compare(sourceRootFI.suffix()))
        QMessageBox::critical(mp_parent, tr("Zip Compress"), tr("Source file %1 does not exist").arg(sourceRootFI.absoluteFilePath()));
        return false;
   }
   if(destRootFI.exists()){
        QMessageBox::critical(mp_parent, tr("Zip Compress"), tr("Destination file %1 already exists").arg(destRootFI.absoluteFilePath()));
        return false;
   }
   if(!zip.open(QuaZip::mdCreate)){
      QMessageBox::critical(mp_parent, tr("Zip Compress"), tr("Unable to create destination file %1").arg(destRootFI.absoluteFilePath()));
      return false;
   }



   int maxValue = m_sourceFiles.count(); // By default
   if(m_progressType == ProgressFileSize){
      maxValue = m_totalSize;
   }

   QuaZipFile outZipFile(&zip);

   QProgressDialog progress(tr("Adding Files..."), tr("Abort"), 0, maxValue, mp_parent);
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
         zip.close();
         return false;
      }

      QFileInfo sourceFI(sourceRootDir.absoluteFilePath(sourceFilePath));

      progress.setLabelText(tr("Adding Files...\n%1").arg(sourceFI.fileName()));
      QApplication::processEvents(); // Make sure file name is updated (even if costy operation)

      // Copy file
      if(!sourceFI.isFile()){
         QString sourceFolderPath = sourceFilePath;
         if(!sourceFolderPath.endsWith("/")){
            sourceFolderPath.append("/");
         }

         if(!outZipFile.open(QIODevice::WriteOnly, QuaZipNewInfo(sourceFolderPath, sourceFI.absoluteFilePath()))){
            progress.show();
            QMessageBox::critical(mp_parent, tr("Zip Compress"), tr("Unable to create destination file %1 in archive %2").arg(sourceFI.absoluteFilePath()).arg(destRootFI.absoluteFilePath()));
            progress.setValue(progress.maximum());
            return false;
         }
         outZipFile.close();

         // Nothing to do with folders
         if(m_progressType == ProgressFileCount){
            progress.setValue(progress.value() + 1);
         } else {
            if(sourceFI.isFile()){
               progress.setValue(progress.value() + sourceFI.size());
            }
         }
         continue;

      }

      // FILE
      QFile inFile(sourceFI.absoluteFilePath());
      if(!inFile.open(QIODevice::ReadOnly)){
         progress.show();
         QMessageBox::critical(mp_parent, tr("Zip Compress"), tr("Unable to read source file %1 in archive %2").arg(sourceFI.absoluteFilePath()).arg(destRootFI.absoluteFilePath()));
         progress.setValue(progress.maximum());
         zip.close();
         return false;
      }

      // Note: since relative, source=dst
      if(!outZipFile.open(QIODevice::WriteOnly, QuaZipNewInfo(sourceFilePath, sourceFI.absoluteFilePath()))){
         progress.show();
         QMessageBox::critical(mp_parent, tr("Zip Compress"), tr("Unable to open destination file %1 in archive %2").arg(sourceFI.absoluteFilePath()).arg(destRootFI.absoluteFilePath()));
         progress.setValue(progress.maximum());
         inFile.close();
         zip.close();
         return false;
      }

      // Copy
      QByteArray buffer;
      int chunksize = (1024*1024); // Whatever chunk size you like
      buffer = inFile.read(chunksize);
      while(!buffer.isEmpty()){
         if(progress.wasCanceled() || m_aborted){
            outZipFile.close();
            inFile.close();
            zip.close();
            return false;
         }
         outZipFile.write(buffer);

         if(m_progressType == ProgressFileSize){
            progress.setValue(progress.value() + buffer.size());
         }
         buffer = inFile.read(chunksize);
      }

      outZipFile.close();
      inFile.close();

      if(m_progressType == ProgressFileCount){
         progress.setValue(progress.value() + 1);
      } else {
         // already precessed
      }
   }
   zip.close();

   if(progress.wasCanceled() || m_aborted){
      return false;
   }
   return true;
}
