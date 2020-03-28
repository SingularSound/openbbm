#ifndef DIALOGFIRMWARE_H
#define DIALOGFIRMWARE_H

#include <QDialog>

namespace Ui {
class DialogFirmware;
}

class DialogFirmware : public QDialog
{
   Q_OBJECT

public:
   explicit DialogFirmware(QWidget *parent = 0);
   ~DialogFirmware();

private slots:
   void on_buttonBox_accepted();

   void on_lineEdit_editingFinished();

private:
   Ui::DialogFirmware *ui;
};

#endif // DIALOGFIRMWARE_H
