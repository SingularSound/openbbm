/********************************************************************************
** Form generated from reading UI file 'aboutdialog.ui'
**
** Created by: Qt User Interface Compiler version 5.12.2
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_ABOUTDIALOG_H
#define UI_ABOUTDIALOG_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QDialog>
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QFrame>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QTextEdit>

QT_BEGIN_NAMESPACE

class Ui_AboutDialog
{
public:
    QGridLayout *gridLayout;
    QLabel *authorLabel;
    QFrame *appIconFrame;
    QLabel *appInfoDataLabel;
    QTextEdit *aboutDataTextEdit;
    QDialogButtonBox *buttonBox;
    QLabel *authorDataLabel;
    QLabel *versionLabel;
    QLabel *versionDataLabel;
    QLabel *aboutLabel;
    QLabel *wkspLabel;
    QLabel *wkspDataLabel;

    void setupUi(QDialog *AboutDialog)
    {
        if (AboutDialog->objectName().isEmpty())
            AboutDialog->setObjectName(QString::fromUtf8("AboutDialog"));
        AboutDialog->resize(400, 489);
        gridLayout = new QGridLayout(AboutDialog);
        gridLayout->setObjectName(QString::fromUtf8("gridLayout"));
        authorLabel = new QLabel(AboutDialog);
        authorLabel->setObjectName(QString::fromUtf8("authorLabel"));

        gridLayout->addWidget(authorLabel, 1, 0, 1, 1);

        appIconFrame = new QFrame(AboutDialog);
        appIconFrame->setObjectName(QString::fromUtf8("appIconFrame"));
        appIconFrame->setMinimumSize(QSize(64, 64));
        appIconFrame->setMaximumSize(QSize(64, 64));
        appIconFrame->setFrameShape(QFrame::StyledPanel);
        appIconFrame->setFrameShadow(QFrame::Raised);

        gridLayout->addWidget(appIconFrame, 0, 0, 1, 1);

        appInfoDataLabel = new QLabel(AboutDialog);
        appInfoDataLabel->setObjectName(QString::fromUtf8("appInfoDataLabel"));

        gridLayout->addWidget(appInfoDataLabel, 0, 1, 1, 1);

        aboutDataTextEdit = new QTextEdit(AboutDialog);
        aboutDataTextEdit->setObjectName(QString::fromUtf8("aboutDataTextEdit"));
        aboutDataTextEdit->setReadOnly(true);

        gridLayout->addWidget(aboutDataTextEdit, 6, 0, 1, 2);

        buttonBox = new QDialogButtonBox(AboutDialog);
        buttonBox->setObjectName(QString::fromUtf8("buttonBox"));
        buttonBox->setOrientation(Qt::Horizontal);
        buttonBox->setStandardButtons(QDialogButtonBox::Ok);
        buttonBox->setCenterButtons(true);

        gridLayout->addWidget(buttonBox, 7, 0, 1, 2);

        authorDataLabel = new QLabel(AboutDialog);
        authorDataLabel->setObjectName(QString::fromUtf8("authorDataLabel"));

        gridLayout->addWidget(authorDataLabel, 1, 1, 1, 1);

        versionLabel = new QLabel(AboutDialog);
        versionLabel->setObjectName(QString::fromUtf8("versionLabel"));

        gridLayout->addWidget(versionLabel, 2, 0, 1, 1);

        versionDataLabel = new QLabel(AboutDialog);
        versionDataLabel->setObjectName(QString::fromUtf8("versionDataLabel"));

        gridLayout->addWidget(versionDataLabel, 2, 1, 1, 1);

        aboutLabel = new QLabel(AboutDialog);
        aboutLabel->setObjectName(QString::fromUtf8("aboutLabel"));

        gridLayout->addWidget(aboutLabel, 4, 0, 1, 1);

        wkspLabel = new QLabel(AboutDialog);
        wkspLabel->setObjectName(QString::fromUtf8("wkspLabel"));

        gridLayout->addWidget(wkspLabel, 3, 0, 1, 1);

        wkspDataLabel = new QLabel(AboutDialog);
        wkspDataLabel->setObjectName(QString::fromUtf8("wkspDataLabel"));

        gridLayout->addWidget(wkspDataLabel, 3, 1, 1, 1);


        retranslateUi(AboutDialog);
        QObject::connect(buttonBox, SIGNAL(accepted()), AboutDialog, SLOT(accept()));
        QObject::connect(buttonBox, SIGNAL(rejected()), AboutDialog, SLOT(reject()));

        QMetaObject::connectSlotsByName(AboutDialog);
    } // setupUi

    void retranslateUi(QDialog *AboutDialog)
    {
        AboutDialog->setWindowTitle(QApplication::translate("AboutDialog", "About BeatBuddy Manager", nullptr));
        authorLabel->setText(QApplication::translate("AboutDialog", "Author:", nullptr));
        appInfoDataLabel->setText(QApplication::translate("AboutDialog", "TODO", nullptr));
        authorDataLabel->setText(QApplication::translate("AboutDialog", "TODO", nullptr));
        versionLabel->setText(QApplication::translate("AboutDialog", "Version:", nullptr));
        versionDataLabel->setText(QApplication::translate("AboutDialog", "TODO", nullptr));
        aboutLabel->setText(QApplication::translate("AboutDialog", "About:", nullptr));
        wkspLabel->setText(QApplication::translate("AboutDialog", "Workspace:", nullptr));
        wkspDataLabel->setText(QApplication::translate("AboutDialog", "TODO", nullptr));
    } // retranslateUi

};

namespace Ui {
    class AboutDialog: public Ui_AboutDialog {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_ABOUTDIALOG_H
