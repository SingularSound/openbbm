#include "dialogfirmware.h"
#include "ui_dialogfirmware.h"

DialogFirmware::DialogFirmware(QWidget *parent) :
   QDialog(parent),
   ui(new Ui::DialogFirmware)
{
   ui->setupUi(this);
}

DialogFirmware::~DialogFirmware()
{
   delete ui;
}

void DialogFirmware::on_buttonBox_accepted()
{

}

void DialogFirmware::on_lineEdit_editingFinished()
{

}
