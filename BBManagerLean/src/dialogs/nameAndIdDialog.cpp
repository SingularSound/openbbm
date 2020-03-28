
#include <QtWidgets>

#include "nameAndIdDialog.h"

NameAndIdDialog::NameAndIdDialog(QWidget *parent, QString name, int id)
    : QDialog(parent)
{
    itemName = name;
    midiId = id;

    qDebug() << "nameAndId" << name << midiId;

    QGridLayout *mainLayout = new QGridLayout(this);
    mainLayout->setContentsMargins(10,10,10,10);

    QLabel *itemNameLabel = new QLabel("Item Name");
    mainLayout->addWidget(itemNameLabel,0,0);
    itemNameEdit = new QLineEdit;
    QRegExp re("[+-_a-zA-Z0-9 ]+");
    QRegExpValidator *v = new QRegExpValidator(re);
    itemNameEdit->setValidator(v);
    itemNameEdit->setText(itemName);

    mainLayout->addWidget(itemNameEdit,0,1);
    QLabel *midiIdLabel = new QLabel("MIDI ID");
    mainLayout->addWidget(midiIdLabel,1,0);
    midiIdEdit = new QSpinBox;
    midiIdEdit->setMinimum(0);
    midiIdEdit->setMaximum(128);
    midiIdEdit->setValue(midiId);
    mainLayout->addWidget(midiIdEdit,1,1);

    QDialogButtonBox *buttonBox = new QDialogButtonBox(this);
    buttonBox->setObjectName(QStringLiteral("buttonBox"));
    buttonBox->setGeometry(QRect(20, 220, 291, 32));
    buttonBox->setOrientation(Qt::Horizontal);
    buttonBox->setStandardButtons(QDialogButtonBox::Cancel|QDialogButtonBox::Ok);
    mainLayout->addWidget(buttonBox,2,1);

    QObject::connect(buttonBox, SIGNAL(accepted()), this, SLOT(pre_accept()));
    QObject::connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
}

void NameAndIdDialog::pre_accept() {

    itemName = itemNameEdit->text();
    midiId = midiIdEdit->text().toInt();

    qDebug() << "accepting" << itemName << midiId;
    emit itemNameAndMidiIdChanged(itemName, midiId);
    accept();
    hide();
}

void NameAndIdDialog::setName(QString name) {
    itemName = name;
    itemNameEdit->setText(name);
}

void NameAndIdDialog::setMidiId(int id) {
    midiId = id;
    midiIdEdit->setValue(id);
}
