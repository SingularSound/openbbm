#ifndef OPTIONSDIALOG_H
#define OPTIONSDIALOG_H

#include <QDialog>

namespace Ui {
class OptionsDialog;
}

class OptionsDialog : public QDialog
{
   Q_OBJECT

public:
   explicit OptionsDialog(int bufferingTime_ms, QWidget *parent = 0);
   ~OptionsDialog();
   inline int bufferingTime_ms(){return m_bufferingTime_ms; }

private slots:

   void on_bufferingTimeSlider_valueChanged(int value);

private:
   Ui::OptionsDialog *ui;
   int m_bufferingTime_ms;
};

#endif // OPTIONSDIALOG_H
