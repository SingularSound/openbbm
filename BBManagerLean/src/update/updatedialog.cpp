#include "updatedialog.h"
#include "ui_updatedialog.h"

UpdateDialog::UpdateDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::UpdateDialog)
{
    ui->setupUi(this);
    ui->label->setOpenExternalLinks(true);
}

UpdateDialog::~UpdateDialog()
{
    delete ui;
}

void UpdateDialog::setInformationLabelText(QString content){
    this->ui->label->setText(content);
}
