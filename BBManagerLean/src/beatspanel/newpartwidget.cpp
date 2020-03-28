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
