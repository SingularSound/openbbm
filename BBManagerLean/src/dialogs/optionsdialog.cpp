#include "optionsdialog.h"
#include "ui_optionsdialog.h"
#include "../player/mixer.h"

OptionsDialog::OptionsDialog(int bufferingTime_ms, QWidget *parent) :
   QDialog(parent),
   ui(new Ui::OptionsDialog)
{
   m_bufferingTime_ms = bufferingTime_ms;
   ui->setupUi(this);
   ui->bufferingTimeSlider->setMaximum(MIXER_MAX_BUFFERRING_TIME_MS);
   ui->bufferingTimeSlider->setMinimum(MIXER_MIN_BUFFERRING_TIME_MS);
   ui->bufferingTimeSlider->setSingleStep(10);
   ui->bufferingTimeSlider->setTickInterval(10);
   ui->bufferingTimeSlider->setValue(bufferingTime_ms);

   ui->bufferingTimeLabel->setText(tr("Buffering Time : %1 ms").arg(m_bufferingTime_ms, 3, 10, QChar(' ')));
}

OptionsDialog::~OptionsDialog()
{
   delete ui;
}

void OptionsDialog::on_bufferingTimeSlider_valueChanged(int value)
{
   if(value > MIXER_MAX_BUFFERRING_TIME_MS){
      m_bufferingTime_ms = MIXER_MAX_BUFFERRING_TIME_MS;
   } else if (value < MIXER_MIN_BUFFERRING_TIME_MS){
      m_bufferingTime_ms = MIXER_MIN_BUFFERRING_TIME_MS;
   } else {
      m_bufferingTime_ms = value;
   }

   ui->bufferingTimeLabel->setText(tr("Buffering Time : %1 ms").arg(m_bufferingTime_ms, 3, 10, QChar(' ')));
}
