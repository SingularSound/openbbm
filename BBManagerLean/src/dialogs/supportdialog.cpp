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
#include "supportdialog.h"
#include "ui_supportdialog.h"
#include "../workspace/settings.h"

#include <QFile>
#include <QDir>
#include <QMessageBox>
#include <QTextStream>
#include <QUrl>
#include <QUuid>
#include <QDesktopServices>

SupportDialog::SupportDialog(QWidget *parent) :
   QDialog(parent),
   ui(new Ui::SupportDialog)
{
    setWindowTitle(tr("Help"));
    ui->setupUi(this);
    ui->textBrowser->setSource(QUrl("qrc:/helpcontent/Index.html"));
    ui->textBrowser->setOpenExternalLinks(false);
    ui->textBrowser->setOpenLinks(false);
    connect(ui->textBrowser, &QTextBrowser::anchorClicked, this, &SupportDialog::on_linkClicked);
}

SupportDialog::~SupportDialog()
{
}

void SupportDialog::on_linkClicked(const QUrl &arg1)
{
    QDesktopServices::openUrl(arg1);
}
