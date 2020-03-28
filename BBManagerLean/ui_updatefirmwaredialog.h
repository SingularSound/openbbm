/********************************************************************************
** Form generated from reading UI file 'updatefirmwaredialog.ui'
**
** Created by: Qt User Interface Compiler version 5.12.2
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_UPDATEFIRMWAREDIALOG_H
#define UI_UPDATEFIRMWAREDIALOG_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QDialog>
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QProgressBar>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_UpdateFirmwareDialog
{
public:
    QWidget *gridLayoutWidget;
    QGridLayout *gridLayout_3;
    QDialogButtonBox *buttonBox;
    QProgressBar *progressBar_2;
    QPushButton *UpdateFirmware;
    QPushButton *checkupdate;
    QPushButton *pushButton_3;
    QLabel *label_2;
    QPushButton *pushButton;
    QLineEdit *lineEdit_2;
    QProgressBar *progressBar;
    QLineEdit *lineEdit;
    QLabel *label;
    QPushButton *pushButton_2;
    QSpacerItem *verticalSpacer;
    QSpacerItem *verticalSpacer_3;

    void setupUi(QDialog *UpdateFirmwareDialog)
    {
        if (UpdateFirmwareDialog->objectName().isEmpty())
            UpdateFirmwareDialog->setObjectName(QString::fromUtf8("UpdateFirmwareDialog"));
        UpdateFirmwareDialog->resize(349, 317);
        QSizePolicy sizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(UpdateFirmwareDialog->sizePolicy().hasHeightForWidth());
        UpdateFirmwareDialog->setSizePolicy(sizePolicy);
        gridLayoutWidget = new QWidget(UpdateFirmwareDialog);
        gridLayoutWidget->setObjectName(QString::fromUtf8("gridLayoutWidget"));
        gridLayoutWidget->setGeometry(QRect(10, 10, 331, 307));
        gridLayout_3 = new QGridLayout(gridLayoutWidget);
        gridLayout_3->setObjectName(QString::fromUtf8("gridLayout_3"));
        gridLayout_3->setContentsMargins(0, 0, 0, 0);
        buttonBox = new QDialogButtonBox(gridLayoutWidget);
        buttonBox->setObjectName(QString::fromUtf8("buttonBox"));
        buttonBox->setOrientation(Qt::Horizontal);
        buttonBox->setStandardButtons(QDialogButtonBox::Close);

        gridLayout_3->addWidget(buttonBox, 14, 0, 1, 2);

        progressBar_2 = new QProgressBar(gridLayoutWidget);
        progressBar_2->setObjectName(QString::fromUtf8("progressBar_2"));
        progressBar_2->setEnabled(true);
        progressBar_2->setCursor(QCursor(Qt::IBeamCursor));
        progressBar_2->setValue(0);
        progressBar_2->setInvertedAppearance(false);
        progressBar_2->setTextDirection(QProgressBar::TopToBottom);

        gridLayout_3->addWidget(progressBar_2, 13, 0, 1, 2);

        UpdateFirmware = new QPushButton(gridLayoutWidget);
        UpdateFirmware->setObjectName(QString::fromUtf8("UpdateFirmware"));

        gridLayout_3->addWidget(UpdateFirmware, 12, 0, 1, 2);

        checkupdate = new QPushButton(gridLayoutWidget);
        checkupdate->setObjectName(QString::fromUtf8("checkupdate"));

        gridLayout_3->addWidget(checkupdate, 3, 0, 1, 1);

        pushButton_3 = new QPushButton(gridLayoutWidget);
        pushButton_3->setObjectName(QString::fromUtf8("pushButton_3"));
        QSizePolicy sizePolicy1(QSizePolicy::Minimum, QSizePolicy::Fixed);
        sizePolicy1.setHorizontalStretch(1);
        sizePolicy1.setVerticalStretch(0);
        sizePolicy1.setHeightForWidth(pushButton_3->sizePolicy().hasHeightForWidth());
        pushButton_3->setSizePolicy(sizePolicy1);

        gridLayout_3->addWidget(pushButton_3, 9, 0, 1, 1);

        label_2 = new QLabel(gridLayoutWidget);
        label_2->setObjectName(QString::fromUtf8("label_2"));
        QFont font;
        font.setPointSize(11);
        label_2->setFont(font);
        label_2->setScaledContents(false);

        gridLayout_3->addWidget(label_2, 7, 0, 1, 2);

        pushButton = new QPushButton(gridLayoutWidget);
        pushButton->setObjectName(QString::fromUtf8("pushButton"));
        QSizePolicy sizePolicy2(QSizePolicy::Minimum, QSizePolicy::Fixed);
        sizePolicy2.setHorizontalStretch(0);
        sizePolicy2.setVerticalStretch(0);
        sizePolicy2.setHeightForWidth(pushButton->sizePolicy().hasHeightForWidth());
        pushButton->setSizePolicy(sizePolicy2);

        gridLayout_3->addWidget(pushButton, 9, 1, 1, 1);

        lineEdit_2 = new QLineEdit(gridLayoutWidget);
        lineEdit_2->setObjectName(QString::fromUtf8("lineEdit_2"));

        gridLayout_3->addWidget(lineEdit_2, 8, 0, 1, 2);

        progressBar = new QProgressBar(gridLayoutWidget);
        progressBar->setObjectName(QString::fromUtf8("progressBar"));
        progressBar->setMinimum(0);
        progressBar->setMaximum(0);
        progressBar->setValue(0);
        progressBar->setTextVisible(false);

        gridLayout_3->addWidget(progressBar, 4, 0, 1, 2);

        lineEdit = new QLineEdit(gridLayoutWidget);
        lineEdit->setObjectName(QString::fromUtf8("lineEdit"));

        gridLayout_3->addWidget(lineEdit, 1, 0, 1, 2);

        label = new QLabel(gridLayoutWidget);
        label->setObjectName(QString::fromUtf8("label"));
        label->setFont(font);
        label->setScaledContents(false);

        gridLayout_3->addWidget(label, 0, 0, 1, 2);

        pushButton_2 = new QPushButton(gridLayoutWidget);
        pushButton_2->setObjectName(QString::fromUtf8("pushButton_2"));
        sizePolicy2.setHeightForWidth(pushButton_2->sizePolicy().hasHeightForWidth());
        pushButton_2->setSizePolicy(sizePolicy2);

        gridLayout_3->addWidget(pushButton_2, 3, 1, 1, 1);

        verticalSpacer = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);

        gridLayout_3->addItem(verticalSpacer, 11, 0, 1, 2);

        verticalSpacer_3 = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);

        gridLayout_3->addItem(verticalSpacer_3, 5, 0, 1, 2);


        retranslateUi(UpdateFirmwareDialog);
        QObject::connect(buttonBox, SIGNAL(accepted()), UpdateFirmwareDialog, SLOT(accept()));
        QObject::connect(buttonBox, SIGNAL(rejected()), UpdateFirmwareDialog, SLOT(reject()));

        QMetaObject::connectSlotsByName(UpdateFirmwareDialog);
    } // setupUi

    void retranslateUi(QDialog *UpdateFirmwareDialog)
    {
        UpdateFirmwareDialog->setWindowTitle(QApplication::translate("UpdateFirmwareDialog", "Dialog", nullptr));
        progressBar_2->setFormat(QApplication::translate("UpdateFirmwareDialog", "%p%", nullptr));
        UpdateFirmware->setText(QApplication::translate("UpdateFirmwareDialog", "Update Firmware", nullptr));
        checkupdate->setText(QApplication::translate("UpdateFirmwareDialog", "Check for update online", nullptr));
        pushButton_3->setText(QApplication::translate("UpdateFirmwareDialog", "Detect Pedal", nullptr));
        label_2->setText(QApplication::translate("UpdateFirmwareDialog", "BeatBuddy selection", nullptr));
        pushButton->setText(QApplication::translate("UpdateFirmwareDialog", "Select Pedal Path", nullptr));
        progressBar->setFormat(QString());
        label->setText(QApplication::translate("UpdateFirmwareDialog", "Firmware source selection", nullptr));
        pushButton_2->setText(QApplication::translate("UpdateFirmwareDialog", "Select Firmware Path", nullptr));
    } // retranslateUi

};

namespace Ui {
    class UpdateFirmwareDialog: public Ui_UpdateFirmwareDialog {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_UPDATEFIRMWAREDIALOG_H
