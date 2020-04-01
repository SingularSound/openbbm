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
#include "newprojectdialog.h"
#include "ui_newprojectdialog.h"
#include "QFileDialog"
#include <QDebug>

NewProjectDialog::NewProjectDialog(QWidget *parent, QString default_path) :
    QDialog(parent),
    ui(new Ui::NewProjectDialog)
{

    ui->setupUi(this);
    this->setFixedSize(this->size());
    this->setWindowTitle(tr("Create a new BeatBuddy Project Folder"));

    // Set default values
    m_defaultPath = default_path;
    ui->lineEdit_2->setText(default_path);

    refreshButton();

    // Initial result is Rejected in case the user close the Dialog
    this->setResult(Rejected);
}



QString NewProjectDialog::getProjectName()
{
    return ui->lineEdit->text();
}

QString NewProjectDialog::getSelectedFolder()
{
    return ui->lineEdit_2->text();
}

NewProjectDialog::~NewProjectDialog()
{
    delete ui;
}

void NewProjectDialog::on_pushButton_clicked()
{
    QString directory = QFileDialog::getExistingDirectory(
                this,
                tr("Select a folder"),
                m_defaultPath,
                QFileDialog::ShowDirsOnly);

    if (!directory.isEmpty()){
        ui->lineEdit_2->setText(directory);
    }
    refreshButton();
}


void NewProjectDialog::refreshButton(void){

    // If there is no path selected or no project name
    if (ui->lineEdit->text().isEmpty() || ui->lineEdit_2->text().isEmpty()){
        ui->buttonBox->button( QDialogButtonBox::Ok )->setEnabled( false );
    } else {
        ui->buttonBox->button( QDialogButtonBox::Ok )->setEnabled( true );
    }

}



void NewProjectDialog::on_lineEdit_textChanged(const QString &arg1)
{
    (void) arg1;

    refreshButton();
}

void NewProjectDialog::on_buttonBox_accepted()
{
    this->setResult(Accepted);
}



void NewProjectDialog::on_buttonBox_rejected()
{
    this->setResult(Rejected);
}
