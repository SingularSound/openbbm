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
