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
#include "newpartwidget.h"

NewPartWidget::NewPartWidget(QWidget *parent, int partRow) :
   QFrame(parent)
 , m_PartRow(partRow)
{

   setMinimumSize(24,24);
   setMaximumSize(24,24);

   mp_NewButton = new QPushButton(this);
   mp_NewButton->setText(tr("+"));
   mp_NewButton->setMinimumSize(24,24);
   mp_NewButton->setMaximumSize(24,24);
   mp_NewButton->setGeometry(0,0,24,24); 
   mp_NewButton->setToolTip(tr("Add a new part to the song"));
   connect(mp_NewButton, SIGNAL(clicked()), this, SLOT(slotButtonClicked()));
}


void NewPartWidget::slotButtonClicked()
{
   emit sigSubWidgetClicked();
   emit sigAddPartToRow(m_PartRow);
}
