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
#include "drumsetsizeprogressbar.h"
#include <QProgressBar>
#include <QPalette>
#include <QBrush>
#include <QGridLayout>
#include <QLabel>

DrumsetSizeProgressBar::DrumsetSizeProgressBar(QWidget *parent) :
    QWidget(parent)
{
    QGridLayout * p_GridLayout = new QGridLayout();

    this->setToolTip(tr("Display the size of the current drumset\n\n"));
    mp_progressbar = new QProgressBar(this);

    // normal object parameters

    mp_progressbar->setMinimum(0);
    mp_progressbar->setMaximum(100);


    mp_progressbar->setEnabled(false);
    mp_progressbar->setTextVisible(false);
    mp_progressbar->setToolTip(tr("Display the drumset size"));

    mp_RessourceText = new QLabel(this);
    mp_RessourceText->setText(tr("%1%").arg(0));

    this->setContentsMargins(0,0,30,0);
#ifdef Q_OS_MAC
    this->setMaximumHeight(20);
#else
    this->setMaximumHeight(10);
#endif

    this->setStyleSheet("QProgressBar::chunk:horizontal {background: qlineargradient(x1: .98, y1: 0.5, x2: 1, y2: 0.5, stop: 0 green, stop: 1 transparent);}"
                        "QProgressBar::horizontal {border: 1px solid gray; border-radius: 3px; background: transparent; padding: 0px; text-align: centered; margin-right: 4ex;}");

    p_GridLayout->setContentsMargins(25,0,0,0);
    p_GridLayout->addWidget(mp_progressbar,0,1,1,1);
    p_GridLayout->addWidget(mp_RessourceText,0,0,1,1);
    setLayout(p_GridLayout);

}

void DrumsetSizeProgressBar::setDrumsetSize(quint32 size)
{
    quint32 utilisation = size / (1024 *1024);

    if (utilisation < 80){

        mp_progressbar->setStyleSheet("QProgressBar::chunk:horizontal {background: qlineargradient(x1: .99, y1: 0.5, x2: 1, y2: 0.5, stop: 0 green, stop: 1 transparent);}"
                                      "QProgressBar::horizontal {border: 1px solid gray; border-radius: 3px; background: transparent; padding: 0px; margin-right: 4px;}");
    } else if ((utilisation >= 80) && (utilisation < 100)){
        mp_progressbar->setStyleSheet("QProgressBar::chunk:horizontal {background: qlineargradient(x1: .99, y1: 0.5, x2: 1, y2: 0.5, stop: 0 orange, stop: 1 transparent);}"
                                      "QProgressBar::horizontal {border: 1px solid gray; border-radius: 3px; background: transparent; padding: 0px; margin-right: 4px;}");

    } else {
        utilisation = 100;
        mp_progressbar->setStyleSheet("QProgressBar::chunk:horizontal {background: qlineargradient(x1: .99, y1: 0.5, x2: 1, y2: 0.5, stop: 0 red, stop: 1 transparent);}"
                                      "QProgressBar::horizontal {border: 1px solid gray; border-radius: 3px; background: transparent; padding: 0px; margin-right: 4px;}");
    }
    mp_progressbar->setValue(utilisation);
    mp_RessourceText->setText(tr("%1%").arg(utilisation));
}



