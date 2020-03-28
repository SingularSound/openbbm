#include "autopilotsettingsdialog.h"
#include "ui_autopilotsettingsdialog.h"

AutoPilotSettingsDialog::AutoPilotSettingsDialog(MIDIPARSER_TrackType type, int sigNum, int playFor, int playAt, QWidget *parent) :
    QDialog(parent)
    ,ui(new Ui::AutoPilotSettingsDialog)
    ,m_type(type)
    ,m_sigNum(sigNum)
    ,m_playAtEnabled(true)
    ,m_playForEnabled(true)
{
    ui->setupUi(this);

    setPlayAt(playAt);
    setPlayFor(playFor);

    delete ui->playForCheckBox; //Unused for now.

    setWindowTitle("AutoPilot Settings");
    ui->playAtBeatSpinBox->setMaximum(m_sigNum);
    ui->playAtBeatSpinBox->setMinimum(1);
    ui->playAtMeasureSpinBox->setMinimum(1);


    //Dialog Setup
    ui->playForBeatSpinBox->setMaximum(m_sigNum-1);
    switch(m_type)
    {
    case(MAIN_DRUM_LOOP):
        ui->playForLabel->setText(tr("Reset Drum Fills after:"));
        ui->playAtLabel->setText(tr("Trigger Transition Fill at:"));
        hidePlayForBeatUi();
        break;
    case(DRUM_FILL):
        ui->playAtLabel->setText(tr("Trigger Drum Fill At (relative to Main Drum beginning):"));
        ui->playAtCheckBox->hide();
        deletePlayForUi();
        break;
    case(TRANS_FILL):
        deletePlayAtUi();
        ui->playForLabel->setText(tr("Additionnal Transition Fill Measure(s):"));
        hidePlayForBeatUi();
        break;
    case(INTRO_FILL):
        deletePlayAtUi();
        deletePlayForUi();
        ///Should Not Happen
        break;
    case(OUTRO_FILL):
        deletePlayAtUi();
        deletePlayForUi();
        ///Should Not Happen
        break;
    }
}

AutoPilotSettingsDialog::~AutoPilotSettingsDialog()
{
    delete ui;
}

int AutoPilotSettingsDialog::playAt()
{
    if(ui->playAtCheckBox->isChecked() || !m_playAtEnabled ){
        return 0;
    }
    return (ui->playAtMeasureSpinBox->value()-1)*m_sigNum+ui->playAtBeatSpinBox->value();

}

int AutoPilotSettingsDialog::playFor()
{
    if(!m_playForEnabled ){
        return 0;
    }
    return ui->playForMeasureSpinBox->value()*m_sigNum+ui->playForBeatSpinBox->value();
}

void AutoPilotSettingsDialog::setPlayAt(int playAt)
{
    if(m_playAtEnabled){
        if(playAt > m_sigNum){
            ui->playAtBeatSpinBox->setValue((playAt-1)%m_sigNum+1);
            ui->playAtMeasureSpinBox->setValue((playAt-1)/m_sigNum+1);
        }else{
            ui->playAtBeatSpinBox->setValue(playAt);
        }
    }
    if(playAt == 0 && m_type == MAIN_DRUM_LOOP){
        ui->playAtCheckBox->setChecked(true);
    }
}

void AutoPilotSettingsDialog::setPlayFor(int playFor)
{
    if(m_playForEnabled){
        if(playFor > m_sigNum){
            ui->playForBeatSpinBox->setValue((playFor)%m_sigNum);
            ui->playForMeasureSpinBox->setValue((playFor)/m_sigNum);
        }else{
            ui->playForBeatSpinBox->setValue(playFor);
        }
    }
}


void AutoPilotSettingsDialog::deletePlayForUi()
{
    m_playForEnabled = false;
    delete ui->playForLabel;
    delete ui->playForMeasureLabel;
    delete ui->playForMeasureSpinBox;
    delete ui->playForBeatLabel;
    delete ui->playForBeatSpinBox;
    QSize size = sizeHint();
    resize(size);
}

void AutoPilotSettingsDialog::deletePlayAtUi()
{
    m_playAtEnabled = false;
    delete ui->playAtLabel;
    delete ui->playAtMeasureLabel;
    delete ui->playAtMeasureSpinBox;
    delete ui->playAtBeatLabel;
    delete ui->playAtBeatSpinBox;
    delete ui->playAtCheckBox;
    QSize size = sizeHint();
    resize(size);
}

void AutoPilotSettingsDialog::hidePlayAtBeatUi()
{
    ui->playAtBeatLabel->hide();
    ui->playAtBeatSpinBox->hide();

    ui->playAtBeatSpinBox->setMinimum(0);
    ui->playAtBeatSpinBox->setValue(0);
}

void AutoPilotSettingsDialog::hidePlayForBeatUi()
{
    ui->playForBeatLabel->hide();
    ui->playForBeatSpinBox->hide();

    ui->playForBeatSpinBox->setMinimum(0);
    ui->playForBeatSpinBox->setValue(0);
}


void AutoPilotSettingsDialog::on_playForCheckBox_toggled(bool checked)
{
    if(checked){
        ui->playForMeasureSpinBox->setEnabled(false);
        ui->playForBeatSpinBox->setEnabled(false);
    } else {
        ui->playForMeasureSpinBox->setEnabled(true);
        ui->playForBeatSpinBox->setEnabled(true);
    }
}

void AutoPilotSettingsDialog::on_playAtCheckBox_toggled(bool checked)
{
    if(checked){
        ui->playAtMeasureSpinBox->setEnabled(false);
        ui->playAtBeatSpinBox->setEnabled(false);
    } else {
        ui->playAtMeasureSpinBox->setEnabled(true);
        ui->playAtBeatSpinBox->setEnabled(true);
    }
}
