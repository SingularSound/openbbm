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
#include "instrumentconfigdialog.h"
#include "../../Model/drmmakermodel.h"
#include "math.h"

InstrumentConfigDialog::InstrumentConfigDialog(DrmMakerModel *model, QString name, int midiId, int chokeGroup, int polyPhony, int volume, int fillChokeGroup, int fillChokeDelay, int nonPercussion, QWidget *parent) :
    QDialog(parent)
{
    mp_Model = model;

    // Auto generated from Designer
    if (this->objectName().isEmpty())
        this->setObjectName(QStringLiteral("Intrument_change_dialog"));
    this->setFixedSize(332, 250);
    this->setWindowTitle(tr("Instrument details"));
    buttonBox = new QDialogButtonBox(this);
    buttonBox->setObjectName(QStringLiteral("buttonBox"));
    buttonBox->setGeometry(QRect(20, 220, 291, 32));
    buttonBox->setOrientation(Qt::Horizontal);
    buttonBox->setStandardButtons(QDialogButtonBox::Cancel|QDialogButtonBox::Ok);
    gridLayoutWidget = new QWidget(this);
    gridLayoutWidget->setObjectName(QStringLiteral("gridLayoutWidget"));
    gridLayoutWidget->setGeometry(QRect(10, 10, 311, 201));
    gridLayout = new QGridLayout(gridLayoutWidget);
    gridLayout->setObjectName(QStringLiteral("gridLayout"));
    gridLayout->setContentsMargins(0, 0, 0, 0);
    lineEditInstrumentName = new QLineEdit();
    lineEditInstrumentName->setObjectName(QStringLiteral("lineEditInstrumentName"));



    label = new QLabel(gridLayoutWidget);
    label->setObjectName(QStringLiteral("label"));
    label->setText(tr("Instrument Name"));


    label_2 = new QLabel(gridLayoutWidget);
    label_2->setObjectName(QStringLiteral("label_2"));
    label_2->setText(tr("Midi ID"));


    /* Fill choke group widget */
    label_fillChokeGroup = new QLabel(gridLayoutWidget);
    label_fillChokeGroup->setObjectName(QStringLiteral("label_fillchokeGroup"));
    label_fillChokeGroup->setText(tr("Fill Choke Group"));

    spinBoxFillChokeGroup = new QSpinBox(gridLayoutWidget);
    spinBoxFillChokeGroup->setObjectName((QStringLiteral("spinBoxFillChokeGroup")));
    spinBoxFillChokeGroup->setMinimum(0);
    spinBoxFillChokeGroup->setMaximum(15);
    spinBoxFillChokeGroup->setValue(fillChokeGroup);

    connect(spinBoxFillChokeGroup, SIGNAL(valueChanged(int)),this,SLOT(onFillChokeGroupSpinBoxChanged(int)));

    comboBoxFillChokeDelay = new QComboBox(gridLayoutWidget);
    comboBoxFillChokeDelay->addItem(tr("1/4th"));
    comboBoxFillChokeDelay->addItem(tr("1/8th"));
    comboBoxFillChokeDelay->addItem(tr("1/16th"));
    comboBoxFillChokeDelay->setCurrentIndex(fillChokeDelay); //todo remove of not accepted
    // called to update state
    onFillChokeGroupSpinBoxChanged(fillChokeGroup);

    label_3 = new QLabel(gridLayoutWidget);
    label_3->setObjectName(QStringLiteral("label_3"));
    label_3->setText(tr("Choke Group"));


    spinBoxChokeGroup = new QSpinBox(gridLayoutWidget);
    spinBoxChokeGroup->setMinimum(0);
    spinBoxChokeGroup->setMaximum(15);
    spinBoxChokeGroup->setObjectName(QStringLiteral("spinBoxChokeGroup"));


    /* Volume of of the instrument in % */
    horizontalVolumeSlider =  new QSlider(Qt::Horizontal);
    horizontalVolumeSlider->setMinimum(1);
    horizontalVolumeSlider->setMaximum(100);
    horizontalVolumeSlider->setValue(volume);
    horizontalVolumeSlider->setToolTip(tr("Select the volume of the instrument in %"));
    horizontalVolumeSlider->setObjectName((QStringLiteral("horizontalVolumeSlider")));



    label_5 = new QLabel(gridLayoutWidget);
    label_5->setObjectName(QStringLiteral("label_5"));
    label_5->setText(tr("Volume (%1 dB)").arg(QString::number(- 20.0 * log10(100.0/volume),'g',2)));


    connect(horizontalVolumeSlider, SIGNAL(valueChanged(int)), this, SLOT(volSliderValueChanged(int)));


    comboBoxMidiId = new QComboBox(gridLayoutWidget);
    comboBoxMidiId->setObjectName(QStringLiteral("comboBoxMidiId"));

    // Populate combo box
    // Add current ID
    comboBoxMidiId->addItem(QString::number(midiId),midiId);
    // Add all available IDs
    QList<int>* idList = mp_Model->getAvailableMidiIds();
    for(int i=0; i< idList->size(); i++){
        comboBoxMidiId->addItem(QString::number(idList->at(i)),idList->at(i));
    }
    delete idList;



    QObject::connect(buttonBox, SIGNAL(accepted()), this, SLOT(pre_accept()));
    QObject::connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));

    QMetaObject::connectSlotsByName(this);

    // Manually added
    lineEditInstrumentName->setText(name);
    spinBoxChokeGroup->setValue(chokeGroup);

    label_4 = new QLabel(gridLayoutWidget);
    label_4->setObjectName(QStringLiteral("label_2"));


    horizontalSlider = new QSlider(Qt::Horizontal);
    horizontalSlider->setObjectName(QStringLiteral("horizontalSlider"));
    horizontalSlider->setMaximum(10);

    if(polyPhony){
        label_4->setText(tr("Polyphony (%1)").arg(polyPhony));
        horizontalSlider->setValue(polyPhony);
    } else {
        label_4->setText(tr("Polyphony (%1)").arg("+inf"));
    }

    connect(horizontalSlider, SIGNAL(valueChanged(int)), this, SLOT(polySliderValueChanged(int)));

    qDebug() << "setting up non percussion" << nonPercussion;
    comboBoxNonPercussion = new QComboBox(gridLayoutWidget);
    comboBoxNonPercussion->addItem(tr("Percussion"),0);
    comboBoxNonPercussion->addItem(tr("Non-percussion"),1);
    comboBoxNonPercussion->setCurrentIndex(nonPercussion);

    label_6 = new QLabel(gridLayoutWidget);
    label_6->setObjectName(QStringLiteral("label_6"));
    label_6->setText(tr("Instrument Type"));

    gridLayout->addWidget(label, 0, 0, 1, 1);
    gridLayout->addWidget(lineEditInstrumentName, 0, 1, 1, 2);

    gridLayout->addWidget(comboBoxMidiId, 1, 1, 1, 2);
    gridLayout->addWidget(label_2, 1, 0, 1, 1);

    gridLayout->addWidget(label_3, 2, 0, 1, 1);
    gridLayout->addWidget(spinBoxChokeGroup, 2, 1, 1, 2);

    gridLayout->addWidget(label_fillChokeGroup,3,0,1,1);
    gridLayout->addWidget(spinBoxFillChokeGroup,3,1,1,1);
    gridLayout->addWidget(comboBoxFillChokeDelay,3,2,1,1);

    gridLayout->addWidget(horizontalVolumeSlider, 4, 1, 1, 2);
    gridLayout->addWidget(label_5, 4, 0, 1, 1);

    gridLayout->addWidget(horizontalSlider,5, 1, 1, 2);
    gridLayout->addWidget(label_4, 5, 0, 1, 1);

    gridLayout->addWidget(comboBoxNonPercussion,6, 1, 1, 2);
    gridLayout->addWidget(label_6, 6, 0, 1, 1);

}



void InstrumentConfigDialog::pre_accept(){

    mName = lineEditInstrumentName->text();
    mMidiId = *((int*)comboBoxMidiId->itemData(comboBoxMidiId->currentIndex()).data());
    mChokeGroup = spinBoxChokeGroup->value();
    mPolyPhony = horizontalSlider->value();
    mVolume = horizontalVolumeSlider->value();

    mFillChokeGroup = spinBoxFillChokeGroup->value();
    mFillChokeDelay = comboBoxFillChokeDelay->currentIndex();
    mNonPercussion = *((int*)comboBoxNonPercussion->itemData(comboBoxNonPercussion->currentIndex()).data());

    accept();
}

void InstrumentConfigDialog::polySliderValueChanged(int value){
    if(value){
        label_4->setText(tr("Polyphony (%1)").arg(value));
    } else {
        label_4->setText(tr("Polyphony (%1)").arg("+inf"));
    }
}

void InstrumentConfigDialog::volSliderValueChanged(int value){
    if (value){
        label_5->setText(tr("Volume (%1 dB)").arg(QString::number(- 20.0 * log10(100.0/value),'g',2)));
    } else {
        label_5->setText(tr("Volume (%1 dB)").arg("-inf"));
    }
}

void InstrumentConfigDialog::onFillChokeGroupSpinBoxChanged(int value)
{
    // desactivate the selector if no choke group selected
    if (value){
        comboBoxFillChokeDelay->setEnabled(true);
    } else {
        comboBoxFillChokeDelay->setEnabled(false);
    }
}

void InstrumentConfigDialog::onComboBoxNonPercussionChanged(int value)
{
    qDebug() << "InstrumentConfigDialog::onComboBoxNonPercussionChanged:" << value;
    mNonPercussion = value;
}


// Getter functions
QString InstrumentConfigDialog::getName(){
    return mName;
}

int InstrumentConfigDialog::getMidiId(){
    return mMidiId;
}

int InstrumentConfigDialog::getChokeGroup(){
    return mChokeGroup;
}

uint InstrumentConfigDialog::getPolyPhony(){
    return mPolyPhony;
}

uint InstrumentConfigDialog::getNonPercussion(){
    return mNonPercussion;
}

uint InstrumentConfigDialog::getVolume(){
    return mVolume;
}

uint InstrumentConfigDialog::getFillChokeGroup()
{
    return mFillChokeGroup;
}

uint InstrumentConfigDialog::getFillChokeDelay()
{
    return mFillChokeDelay;
}

