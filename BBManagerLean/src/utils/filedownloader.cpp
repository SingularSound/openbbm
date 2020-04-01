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
// this downloader will generate an SSL error
// More information: https://github.com/opencor/opencor/issues/516

#include "filedownloader.h"

#include <QRegularExpression>

FileDownloader::FileDownloader(QObject *parent)
    : QObject(parent)
{
    connect(&m_WebCtrl, SIGNAL(finished(QNetworkReply*)), this, SLOT(fileDownloaded(QNetworkReply*)));
}

void FileDownloader::download(const QUrl& fileUrl) {
    connect(m_WebCtrl.get(QNetworkRequest(fileUrl)), SIGNAL(downloadProgress(qint64, qint64)), this, SIGNAL(progress(qint64, qint64)));
}

void FileDownloader::fileDownloaded(QNetworkReply* pReply) {
    auto data = pReply->readAll();
    pReply->deleteLater();
    QString response(data);
    if (response.contains(QRegularExpression("<TITLE>Moved Temporarily</TITLE>", QRegularExpression::PatternOption::CaseInsensitiveOption))) {
        auto m = QRegularExpression("<A\\s+HREF\\s*=\\s*\"\\s*([^\"]+)\\s*\"\\s*>", QRegularExpression::PatternOption::CaseInsensitiveOption).match(response);
        if (m.hasMatch()) {
            download(m.captured(1));
            return;
        }
    }
    //emit a signal
    emit downloaded(data);
}
