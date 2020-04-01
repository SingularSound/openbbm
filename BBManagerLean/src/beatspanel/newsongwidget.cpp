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
#include <QHBoxLayout>
#include <QMenu>

#include "newsongwidget.h"

NewSongWidget::NewSongWidget(QWidget *parent) :
    QFrame(parent)
{
    setLayout(new QHBoxLayout);
    layout()->setContentsMargins(5,5,5,5);

    auto butt = new MenuPushButton(this);
    mp_NewButton = butt;
    mp_NewButton->setText(tr("+ Song"));
    mp_NewButton->setToolTip(tr("Add a new song in the current folder"));
    layout()->addItem(new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum));
    layout()->addWidget(mp_NewButton);
    layout()->addItem(new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum));

    setMinimumHeight(38); // 28 for button + 5 padding up and down
    setMaximumHeight(38); // 28 for button + 5 padding up and down

    connect(mp_NewButton, SIGNAL(clicked()), this, SLOT(slotButtonClicked()));

    auto menu = butt->addMenu();
    menu->addAction(tr("Add new song"), this, SLOT(slotButtonClicked()));
    menu->addAction(tr("Import song"), this, SLOT(slotContextMenuImport()));
}

void NewSongWidget::slotButtonClicked()
{
    emit sigAddSongToRow(this, false);
}

void NewSongWidget::slotContextMenuImport()
{
    emit sigAddSongToRow(this, true);
}
