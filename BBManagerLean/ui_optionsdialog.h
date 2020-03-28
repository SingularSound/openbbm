/********************************************************************************
** Form generated from reading UI file 'optionsdialog.ui'
**
** Created by: Qt User Interface Compiler version 5.12.2
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_OPTIONSDIALOG_H
#define UI_OPTIONSDIALOG_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QDialog>
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QLabel>
#include <QtWidgets/QSlider>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QVBoxLayout>

QT_BEGIN_NAMESPACE

class Ui_OptionsDialog
{
public:
    QVBoxLayout *verticalLayout_2;
    QGroupBox *bufferingOptionsGB;
    QGridLayout *gridLayout;
    QSlider *bufferingTimeSlider;
    QLabel *bufferingTimeLabel;
    QLabel *label;
    QSpacerItem *verticalSpacer;
    QDialogButtonBox *buttonBox;

    void setupUi(QDialog *OptionsDialog)
    {
        if (OptionsDialog->objectName().isEmpty())
            OptionsDialog->setObjectName(QString::fromUtf8("OptionsDialog"));
        OptionsDialog->resize(400, 300);
        verticalLayout_2 = new QVBoxLayout(OptionsDialog);
        verticalLayout_2->setObjectName(QString::fromUtf8("verticalLayout_2"));
        bufferingOptionsGB = new QGroupBox(OptionsDialog);
        bufferingOptionsGB->setObjectName(QString::fromUtf8("bufferingOptionsGB"));
        gridLayout = new QGridLayout(bufferingOptionsGB);
        gridLayout->setObjectName(QString::fromUtf8("gridLayout"));
        bufferingTimeSlider = new QSlider(bufferingOptionsGB);
        bufferingTimeSlider->setObjectName(QString::fromUtf8("bufferingTimeSlider"));
        bufferingTimeSlider->setOrientation(Qt::Horizontal);

        gridLayout->addWidget(bufferingTimeSlider, 0, 1, 1, 1);

        bufferingTimeLabel = new QLabel(bufferingOptionsGB);
        bufferingTimeLabel->setObjectName(QString::fromUtf8("bufferingTimeLabel"));
        QSizePolicy sizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(bufferingTimeLabel->sizePolicy().hasHeightForWidth());
        bufferingTimeLabel->setSizePolicy(sizePolicy);

        gridLayout->addWidget(bufferingTimeLabel, 0, 0, 1, 1);

        label = new QLabel(bufferingOptionsGB);
        label->setObjectName(QString::fromUtf8("label"));
        label->setScaledContents(false);
        label->setWordWrap(true);

        gridLayout->addWidget(label, 1, 0, 1, 2);


        verticalLayout_2->addWidget(bufferingOptionsGB);

        verticalSpacer = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);

        verticalLayout_2->addItem(verticalSpacer);

        buttonBox = new QDialogButtonBox(OptionsDialog);
        buttonBox->setObjectName(QString::fromUtf8("buttonBox"));
        buttonBox->setOrientation(Qt::Horizontal);
        buttonBox->setStandardButtons(QDialogButtonBox::Cancel|QDialogButtonBox::Ok);

        verticalLayout_2->addWidget(buttonBox);


        retranslateUi(OptionsDialog);
        QObject::connect(buttonBox, SIGNAL(accepted()), OptionsDialog, SLOT(accept()));
        QObject::connect(buttonBox, SIGNAL(rejected()), OptionsDialog, SLOT(reject()));

        QMetaObject::connectSlotsByName(OptionsDialog);
    } // setupUi

    void retranslateUi(QDialog *OptionsDialog)
    {
        OptionsDialog->setWindowTitle(QApplication::translate("OptionsDialog", "Buffering Options", nullptr));
        bufferingOptionsGB->setTitle(QApplication::translate("OptionsDialog", "Buffering Options", nullptr));
        bufferingTimeLabel->setText(QApplication::translate("OptionsDialog", "Buffering Time : 100 ms", nullptr));
        label->setText(QApplication::translate("OptionsDialog", "<html><head/><body><p>If you are experiencing distorted sound from the BeatBuddy Manager, please adjust the Buffering Time. Some experimentation may be needed to find right level of buffering time for your computer system to play non-distorted sound.</p></body></html>", nullptr));
    } // retranslateUi

};

namespace Ui {
    class OptionsDialog: public Ui_OptionsDialog {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_OPTIONSDIALOG_H
