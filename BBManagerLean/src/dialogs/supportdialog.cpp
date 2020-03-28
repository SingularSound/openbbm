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
