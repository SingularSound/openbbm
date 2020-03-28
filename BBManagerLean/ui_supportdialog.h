/********************************************************************************
** Form generated from reading UI file 'supportdialog.ui'
**
** Created by: Qt User Interface Compiler version 5.12.2
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_SUPPORTDIALOG_H
#define UI_SUPPORTDIALOG_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QDialog>
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QTextBrowser>

QT_BEGIN_NAMESPACE

class Ui_SupportDialog
{
public:
    QGridLayout *gridLayout;
    QDialogButtonBox *buttonBox;
    QTextBrowser *textBrowser;

    void setupUi(QDialog *SupportDialog)
    {
        if (SupportDialog->objectName().isEmpty())
            SupportDialog->setObjectName(QString::fromUtf8("SupportDialog"));
        SupportDialog->resize(633, 241);
        gridLayout = new QGridLayout(SupportDialog);
        gridLayout->setObjectName(QString::fromUtf8("gridLayout"));
        buttonBox = new QDialogButtonBox(SupportDialog);
        buttonBox->setObjectName(QString::fromUtf8("buttonBox"));
        buttonBox->setOrientation(Qt::Horizontal);
        buttonBox->setStandardButtons(QDialogButtonBox::Ok);
        buttonBox->setCenterButtons(true);

        gridLayout->addWidget(buttonBox, 1, 0, 1, 1);

        textBrowser = new QTextBrowser(SupportDialog);
        textBrowser->setObjectName(QString::fromUtf8("textBrowser"));

        gridLayout->addWidget(textBrowser, 0, 0, 1, 1);


        retranslateUi(SupportDialog);
        QObject::connect(buttonBox, SIGNAL(accepted()), SupportDialog, SLOT(accept()));
        QObject::connect(buttonBox, SIGNAL(rejected()), SupportDialog, SLOT(reject()));

        QMetaObject::connectSlotsByName(SupportDialog);
    } // setupUi

    void retranslateUi(QDialog *SupportDialog)
    {
        SupportDialog->setWindowTitle(QApplication::translate("SupportDialog", "Support", nullptr));
    } // retranslateUi

};

namespace Ui {
    class SupportDialog: public Ui_SupportDialog {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_SUPPORTDIALOG_H
