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

#include <QtWidgets>

#include "loopCountDialog.h"

LoopCountDialog::LoopCountDialog(QWidget *parent)
    : QWidget(parent)
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0,0,0,0);
    integerButton = new QPushButton();

}

void LoopCountDialog::setInteger()
{
    emit sigSubWidgetClicked(); // if you click on Loop, you must select song

    bool ok;
    int i = QInputDialog::getInt(this, tr("Loop Count"),
             tr("This is how many times this \npart will loop before continuing.  \n0 means infinite."), loopCount, 0, 1000, 1, &ok);
    
    if (ok) {
       integerButton->setText(tr("Loop\n%1").arg(i));
       loopCount = i;
       emit sigSetLoopCount();
    }
}

int LoopCountDialog::getLoopCount()
{
    return loopCount;
}

void LoopCountDialog::setLoopCount(int i) {
    
    loopCount = i;
    integerButton->setText(QString("Loop\n")+QString::number(loopCount));
}
