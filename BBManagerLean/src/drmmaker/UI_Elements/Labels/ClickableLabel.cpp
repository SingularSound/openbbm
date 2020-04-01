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
#include "ClickableLabel.h"
#include "../../Model/drmmakermodel.h"


// ********************************************************************************************* //
// **************************************** CONSTRUCTOR **************************************** //
// ********************************************************************************************* //

ClickableLabel::ClickableLabel(const QString& text, QWidget* parent)
    : QLabel(text, parent)
{
    // Mouse hover management
    installEventFilter(this);
}

void ClickableLabel::mousePressEvent (QMouseEvent *event) {
    emit clicked(event);
}

bool ClickableLabel::eventFilter(QObject*, QEvent* event) {
    if(event->type() == QEvent::Enter){
        this->setStyleSheet("QLabel { text-decoration: underline }");
        setCursor(Qt::PointingHandCursor);
    } else if(event->type() == QEvent::Leave){
        this->setStyleSheet("");
        setCursor(Qt::ArrowCursor);
    }
    return false;
}


BigTextClickableLabelWithErrorIndicator::BigTextClickableLabelWithErrorIndicator(const QString& text, QWidget* parent)
    : ClickableLabel(text, parent)
    , mName(text)
{
    // Set error state
    setErrorState(false);

    // Set the name
    setText(mName);
}

bool BigTextClickableLabelWithErrorIndicator::eventFilter(QObject*, QEvent* event) {
    if(event->type() == QEvent::Enter){
        if (mErrorState) {
            this->setStyleSheet("QLabel { color : red ; font : bold 12px ; text-decoration: underline }");
        } else {
            this->setStyleSheet("QLabel { color : white ; font : bold 12px ; text-decoration: underline }");
        }
        setCursor(Qt::PointingHandCursor);
    } else if(event->type() == QEvent::Leave){
        if (mErrorState) {
            this->setStyleSheet("QLabel { color : red ; font : bold 12px }");
        } else {
            this->setStyleSheet("QLabel { color : white ; font : bold 12px }");
        }
    }
    return false;
}

void BigTextClickableLabelWithErrorIndicator::setErrorState(bool state){
    mErrorState = state;
    // Change label according to error state
    if (mErrorState) {
        this->setStyleSheet("QLabel { color : red ; font : bold 12px }");
    } else {
        this->setStyleSheet("QLabel { color : white ; font : bold 12px }");
    }
}
