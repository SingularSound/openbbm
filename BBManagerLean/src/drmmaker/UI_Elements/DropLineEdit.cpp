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
#include "DropLineEdit.h"

// **************************************** CONSTRUCTOR **************************************** //

DropLineEdit::DropLineEdit(QWidget *parent) :
    QLineEdit(parent)
{
    setAcceptDrops(true);
    this->setReadOnly(true);
    this->setStyleSheet("QLineEdit { border: 2px solid gray ; border-radius: 8px ; padding: 0 6px }");
    mValid = false;
}




// **************************************** PUBLIC METHODS **************************************** //

QFileInfo DropLineEdit::getFileInfo() const{
    return mFileInfo;
}

void DropLineEdit::setFileInfo(const QFileInfo fileInfo){
    mFileInfo = fileInfo;
    this->setText(mFileInfo.fileName());
    checkAndUpdate();
}

void DropLineEdit::setFilePath(const QString filePath){
    mFileInfo.setFile(filePath);
    this->setText(mFileInfo.fileName());
    checkAndUpdate();
}

bool DropLineEdit::getValid(){
    // Recheck to make sure
    checkAndUpdate();
    return mValid;
}

void DropLineEdit::setValid(bool value){
    mValid = value;
    if (mValid) {
        this->setStyleSheet("QLineEdit { color : black ; border: 2px solid gray ; border-radius: 8px ; padding: 0 6px }");
    } else {
        this->setStyleSheet("QLineEdit { color : red ; border: 2px solid gray ; border-radius: 8px ; padding: 0 6px }");
    }
}

// **************************************** PRIVATE METHODS **************************************** //

void DropLineEdit::checkAndUpdate() {
    mFileInfo.refresh();
    if (mFileInfo.exists()) {
        mValid = true;
        this->setStyleSheet("QLineEdit { color : black ; border: 2px solid gray ; border-radius: 8px ; padding: 0 6px }");
    } else {
        mValid = false;
        this->setStyleSheet("QLineEdit { color : red ; border: 2px solid gray ; border-radius: 8px ; padding: 0 6px }");
    }
}


// **************************************** PROTECTED METHODS **************************************** //

void DropLineEdit::dragEnterEvent(QDragEnterEvent *event){
    if (mValid) {
        this->setStyleSheet("QLineEdit { color : black ; border: 3px solid black ; border-radius: 8px ; padding: 0 6px }");
    } else {
        this->setStyleSheet("QLineEdit { color : red ; border: 3px solid black ; border-radius: 8px ; padding: 0 6px }");
    }
    event->accept();
}
void DropLineEdit::dragLeaveEvent(QDragLeaveEvent *event){
    if (mValid) {
        this->setStyleSheet("QLineEdit { color : black ; border: 2px solid gray ; border-radius: 8px ; padding: 0 6px }");
    } else {
        this->setStyleSheet("QLineEdit { color : red ; border: 2px solid gray ; border-radius: 8px ; padding: 0 6px }");
    }
    event->accept();
}

void DropLineEdit::dropEvent(QDropEvent *event){
    // Get the data. If multiple files are dropped, take only the first one and fetch save its info
    QList<QUrl> list = event->mimeData()->urls();
    mFileInfo.setFile(list.at(0).toLocalFile());
	this->setText(mFileInfo.fileName());
    event->accept();
}
