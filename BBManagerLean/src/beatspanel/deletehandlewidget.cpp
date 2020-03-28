#include "deletehandlewidget.h"

#include <QPainter>
#include <QStyleOption>
#include <QVBoxLayout>
#include <QPainter>

const int DeleteHandleWidget::PREFERRED_WIDTH = 32;

/**
 * @brief DeleteHandleWidget::DeleteHandleWidget
 * @param parent
 * @param song    Flag to set the DeleteHandleWidget as a delete part if it's a part (=0) or
 *                delete the song if (=1)
 */
DeleteHandleWidget::DeleteHandleWidget(QWidget *parent, bool song) :
    QWidget(parent)
{
    setLayout(new QVBoxLayout());

    mp_DeleteButton = new QPushButton(this);
    mp_DeleteButton->setObjectName(QStringLiteral("deleteButton"));
    m_type = song;

    layout()->addWidget(mp_DeleteButton);
    layout()->addItem(new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding));

    if (song){
        mp_DeleteButton->setToolTip(tr("Delete this song"));
    } else {
        mp_DeleteButton->setToolTip(tr("Delete this part"));
    }

    // connect slots to signals
    connect(mp_DeleteButton, SIGNAL(clicked()), this, SLOT(slotDeleteClicked()));
}




// Required to apply stylesheet
void DeleteHandleWidget::paintEvent(QPaintEvent * /*event*/)
{
    QStyleOption opt;
    opt.init(this);
    QPainter p(this);
    style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);
}

/**
 * @brief DeleteHandleWidget::hideButton
 *       Hides the Delete button (like if there is nothing to delete)
 */
void DeleteHandleWidget::hideButton()
{
    mp_DeleteButton->hide();
}

void DeleteHandleWidget::slotDeleteClicked()
{
    emit sigSubWidgetClicked();
    emit sigDeleteClicked();
}


void DeleteHandleWidget::slotIsDisabled(bool disabled)
{
    if (m_type){
        if (disabled){
            mp_DeleteButton->setToolTip(tr("Cannot delete playing song"));
        } else {
            mp_DeleteButton->setToolTip(tr("Delete this song"));
        }
    } else {
        if (disabled){
            mp_DeleteButton->setToolTip(tr("Cannot delete last part"));
        } else {
            mp_DeleteButton->setToolTip(tr("Delete this part"));
        }
    }

    // Disable the button from the GUI
    mp_DeleteButton->setDisabled(disabled);
}
