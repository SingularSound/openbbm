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
#include "Xbutton.h"
#include "../Model/drmmakermodel.h"

XButton::XButton(DrmMakerModel *model, QWidget *parent) :
    QLabel(parent)
{
    mp_Model = model;

    this->setObjectName(QStringLiteral("Remove"));
    this->setGeometry(QRect(0, 0, 15, 15));
    this->setPixmap(QPixmap(QString::fromUtf8(":/drawable/x-mark-small.png")));
    this->setAlignment(Qt::AlignCenter);
    installEventFilter(this);
}

void XButton::mouseReleaseEvent(QMouseEvent *) {
    emit Clicked();
}

bool XButton::eventFilter(QObject*, QEvent* event) {
    if((event->type() == QEvent::Enter) && (mp_Model->getInstrumentCount() > 1)){
        this->setPixmap(QPixmap(QString::fromUtf8(":/drawable/x-mark-small-hover.png")));
        setCursor(Qt::PointingHandCursor);
    } else if(event->type() == QEvent::Leave){
        this->setPixmap(QPixmap(QString::fromUtf8(":/drawable/x-mark-small.png")));
    }
    return false;
}
