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
#include "movehandlewidget.h"
#include "songwidget.h"

#include <QPainter>
#include <QStyleOption>
#include <QVBoxLayout>
#include <QPainter>

const int MoveHandleWidget::PREFERRED_WIDTH = 24;

MoveHandleWidget::MoveHandleWidget(QWidget *parent) :
   QWidget(parent)
{
   setLayout(new QVBoxLayout());
   layout()->setContentsMargins(2,3,2,3);

   mp_UpButton = new QPushButton(this);
   mp_UpButton->setObjectName(QStringLiteral("upButton"));
   layout()->addWidget(mp_UpButton);

   layout()->addItem(new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding));

   mp_Label = new QLabel(this);
   mp_Label->setAlignment( Qt::AlignCenter );
   layout()->addWidget(mp_Label);

   layout()->addItem(new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding));

   mp_DownButton = new QPushButton(this);
   mp_DownButton->setObjectName("downButton");
   layout()->addWidget(mp_DownButton);

   // Tool tip according to the nature of the parent
   if(qobject_cast<SongWidget *>(parent)){
      mp_UpButton->setToolTip(tr("Move the song up"));
      mp_DownButton->setToolTip(tr("Move the song down"));
   } else {
      mp_UpButton->setToolTip(tr("Move the part up"));
      mp_DownButton->setToolTip(tr("Move the part down"));
   }

   // connect slots to signals
   connect(mp_UpButton,   SIGNAL(clicked()), this, SLOT(  upButtonClicked()));
   connect(mp_DownButton, SIGNAL(clicked()), this, SLOT(downButtonClicked()));
   connect(this, SIGNAL(sigIsFirst(bool)), mp_UpButton  , SLOT(setDisabled(bool)));
   connect(this, SIGNAL(sigIsLast (bool)), mp_DownButton, SLOT(setDisabled(bool)));

}

// Required to apply stylesheet
void MoveHandleWidget::paintEvent(QPaintEvent * /*event*/)
{
   QStyleOption opt;
   opt.init(this);
   QPainter p(this);
   style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);
}

void MoveHandleWidget::hideArrows()
{
   mp_UpButton->hide();
   mp_DownButton->hide();
}


void MoveHandleWidget::slotIsFirst(bool first)
{
   emit sigIsFirst(first);
}

void MoveHandleWidget::slotIsLast(bool last)
{
   emit sigIsLast(last);
}

void MoveHandleWidget::setLabelText(const QString & text)
{
   mp_Label->setText(text);
}

void MoveHandleWidget::upButtonClicked()
{
   emit sigSubWidgetClicked();
   emit sigUpClicked();
}

void MoveHandleWidget::downButtonClicked()
{
   emit sigSubWidgetClicked();
   emit sigDownClicked();
}
