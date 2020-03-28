#include "aboutdialog.h"
#include "ui_aboutdialog.h"
#include <QCoreApplication>
#include "../versioninfo.h"
#include <QFile>
#include <QDebug>
#include <QTextStream>
#include <QDir>
#include "../workspace/workspace.h"
#include "mainwindow.h"

AboutDialog::AboutDialog(QWidget *parent) :
   QDialog(parent),
   ui(new Ui::AboutDialog)
{
    setWindowTitle(tr("About"));
    ui->setupUi(this);
    QPalette pal = ui->aboutDataTextEdit->palette();
    pal.setColor(QPalette::Base, pal.color(QPalette::Window));
    ui->aboutDataTextEdit->setPalette(pal);
    ui->appIconFrame->setStyleSheet("background-image: url(:/images/images/App_Icon_64_2.png) 0 0 0 0 stretch stretch");
    VersionInfo vi = VersionInfo::RunningAppVersionInfo();
    ui->appInfoDataLabel->setText(vi.productName());
    ui->authorDataLabel->setText(vi.companyName());
    ui->versionDataLabel->setText(vi.toQString());

    Workspace *wksp = static_cast<MainWindow *>(parent)->mp_MasterWorkspace;
    QString dir = QString("No workspace");
    if (wksp && wksp->isValid()) {
        QDir wkspDir = wksp->dir();
        if (wkspDir.exists()) {
            dir = QString(wkspDir.absolutePath());
        }
    }
    ui->wkspDataLabel->setText(dir);

    // Load About file
    QFile aboutContentFile(":/BeatBuddyManager-About.html");
    if(!aboutContentFile.open(QFile::ReadOnly | QFile::Text)){
        qWarning() << "AboutDialog::AboutDialog - ERROR - unable to open about content";
        return;
    }
    QTextStream in(&aboutContentFile);

    ui->aboutDataTextEdit->setHtml(in.readAll());
}

AboutDialog::~AboutDialog()
{
    delete ui;
}
