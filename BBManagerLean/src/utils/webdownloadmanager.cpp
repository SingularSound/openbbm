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

#include "webdownloadmanager.h"
#include <QFileInfo>
#include <QNetworkRequest>
#include <QNetworkSession>
#include <QNetworkReply>
#include <QString>
#include <QStringList>
#include <QFile>
#include <QDebug>





WebDownloadManager::WebDownloadManager(QObject *parent, bool overwrite, QString path):
   QObject(parent)
{
   m_downloadActive = false;
   m_baseDirectory = path;
   m_overwrite = overwrite;
   m_nFailed = 0;
}

/**
 * @brief
 * @param urlList
 * @return true if URL were add to the queue
 */
void WebDownloadManager::downloadUrl(const QStringList &urlList){
    m_nFailed = 0;
   if (urlList.isEmpty()){
      return;
   }

   // Get all the URLs from the list
   foreach (QString urlString, urlList){
      QUrl url = QUrl::fromEncoded(urlString.toLocal8Bit());
      qDebug() << url.toString();

      downloadUrl(url);
   }

}

/**
 * @brief
 * @param url
 * @return
 */
void WebDownloadManager::downloadUrl(const QUrl &url){
m_nFailed = 0;
   m_DownloadQueue.append(url);

   if (!m_downloadActive){
      emit startNextDownload();
   }

}

/**
 * @brief set the save path
 * @param
 */
void WebDownloadManager::setDownloadedFilePath(QString path)
{
   m_baseDirectory = path;
}

/**
 * @brief WebDownloadManager::StartNextDownload
 */
void WebDownloadManager::startNextDownload(){

   if (m_DownloadQueue.isEmpty()){
      m_downloadActive = false;
      emit downloadFinished(m_nFailed);
      return;
   } else {
      m_downloadActive = true;
   }

   // Get url and convert to a filename
   QUrl url = m_DownloadQueue.dequeue();
   QString filename = saveFileName(url);


   m_CurrentFile.setFileName(m_baseDirectory + filename);

   qDebug() << m_baseDirectory + filename;
   if (!m_CurrentFile.open((QIODevice::WriteOnly))){
      qDebug() << "Error opening the files";
      startNextDownload();
      return;
   }


   QNetworkRequest netRequest(url);
   currentDownloadReply = m_NetManager.get(netRequest);
   connect(currentDownloadReply, SIGNAL(error(QNetworkReply::NetworkError)),SLOT(slotDownloadError(QNetworkReply::NetworkError)));
   connect(currentDownloadReply, SIGNAL(downloadProgress(qint64,qint64)),SLOT(slotdownloadProgress(qint64,qint64)));
   connect(currentDownloadReply, SIGNAL(finished()),SLOT(slotDownloadFinished()));
   connect(currentDownloadReply, SIGNAL(readyRead()),SLOT(slotDownloadReadyRead()));
}


/**
 * @brief SLot to read available downloaded data
 */
void WebDownloadManager::slotDownloadReadyRead()
{
   m_CurrentFile.write(currentDownloadReply->readAll());
}


void WebDownloadManager::slotdownloadProgress(qint64 l,qint64 ll){
   qDebug() << QString("Size:") + QString::number(l);
   (void) ll;

}

/**
 * @brief Slot that is called when the current file downloaded id finished
 */
void WebDownloadManager::slotDownloadFinished()
{

   // Close the current file
   m_CurrentFile.close();

   QNetworkReply::NetworkError error = currentDownloadReply->error();

   // Verify for errors
   if (error != QNetworkReply::NoError){
      qDebug() << error;
      m_nFailed++;
   }

   currentDownloadReply->deleteLater();
   startNextDownload();
}


/**
 * @brief WebDownloadManager::slotDownloadError
 */
void WebDownloadManager::slotDownloadError(QNetworkReply::NetworkError error)
{
    qDebug() << "Network Error number " + QString::number(error);
    slotDownloadFinished();
}


/**
 * @brief Convert the URL to a filename. THis filename can be unique or not if the user sets
 *       the overwrite flag.
 * @param URL of the file
 * @return the basename of the file downloaded
 */
QString WebDownloadManager::saveFileName(const QUrl &url){



   // Convert the URL to the file name
   QString path = url.path();
   QString basename = QFileInfo(path).fileName();

   if (basename.isEmpty()){
      basename = "download";
   }

   if (!m_overwrite){

      // If the file in the download path already exists
      if (QFile::exists(basename)){
         int i = 0;
         basename += "_";

         // Increment until a new namefile is found
         while(QFile::exists(basename + QString::number(i))){
            i++;
            qDebug() << i;
         }

         // add the number to the basename
         basename += QString::number(i);
      }
   }
   return basename;
}








