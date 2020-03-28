/********************************************************************************
** Form generated from reading UI file 'autopilotsettingsdialog.ui'
**
** Created by: Qt User Interface Compiler version 5.12.2
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_AUTOPILOTSETTINGSDIALOG_H
#define UI_AUTOPILOTSETTINGSDIALOG_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QDialog>
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QSpinBox>

QT_BEGIN_NAMESPACE

class Ui_AutoPilotSettingsDialog
{
public:
    QGridLayout *gridLayout;
    QDialogButtonBox *buttonBox;
    QSpinBox *playForMeasureSpinBox;
    QLabel *playForLabel;
    QLabel *playAtBeatLabel;
    QSpinBox *playAtBeatSpinBox;
    QLabel *playAtMeasureLabel;
    QSpinBox *playAtMeasureSpinBox;
    QLabel *playAtLabel;
    QSpacerItem *horizontalSpacer;
    QSpacerItem *horizontalSpacer_2;
    QLabel *playForMeasureLabel;
    QSpinBox *playForBeatSpinBox;
    QLabel *playForBeatLabel;
    QCheckBox *playForCheckBox;
    QCheckBox *playAtCheckBox;

    void setupUi(QDialog *AutoPilotSettingsDialog)
    {
        if (AutoPilotSettingsDialog->objectName().isEmpty())
            AutoPilotSettingsDialog->setObjectName(QString::fromUtf8("AutoPilotSettingsDialog"));
        AutoPilotSettingsDialog->setEnabled(true);
        AutoPilotSettingsDialog->resize(431, 97);
        QSizePolicy sizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(AutoPilotSettingsDialog->sizePolicy().hasHeightForWidth());
        AutoPilotSettingsDialog->setSizePolicy(sizePolicy);
        gridLayout = new QGridLayout(AutoPilotSettingsDialog);
        gridLayout->setObjectName(QString::fromUtf8("gridLayout"));
        buttonBox = new QDialogButtonBox(AutoPilotSettingsDialog);
        buttonBox->setObjectName(QString::fromUtf8("buttonBox"));
        buttonBox->setOrientation(Qt::Horizontal);
        buttonBox->setStandardButtons(QDialogButtonBox::Cancel|QDialogButtonBox::Ok);

        gridLayout->addWidget(buttonBox, 2, 1, 1, 7);

        playForMeasureSpinBox = new QSpinBox(AutoPilotSettingsDialog);
        playForMeasureSpinBox->setObjectName(QString::fromUtf8("playForMeasureSpinBox"));
        playForMeasureSpinBox->setMaximum(999999999);

        gridLayout->addWidget(playForMeasureSpinBox, 1, 3, 1, 1);

        playForLabel = new QLabel(AutoPilotSettingsDialog);
        playForLabel->setObjectName(QString::fromUtf8("playForLabel"));
        playForLabel->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);

        gridLayout->addWidget(playForLabel, 1, 1, 1, 1);

        playAtBeatLabel = new QLabel(AutoPilotSettingsDialog);
        playAtBeatLabel->setObjectName(QString::fromUtf8("playAtBeatLabel"));

        gridLayout->addWidget(playAtBeatLabel, 0, 7, 1, 1);

        playAtBeatSpinBox = new QSpinBox(AutoPilotSettingsDialog);
        playAtBeatSpinBox->setObjectName(QString::fromUtf8("playAtBeatSpinBox"));
        playAtBeatSpinBox->setMaximum(999999999);

        gridLayout->addWidget(playAtBeatSpinBox, 0, 6, 1, 1);

        playAtMeasureLabel = new QLabel(AutoPilotSettingsDialog);
        playAtMeasureLabel->setObjectName(QString::fromUtf8("playAtMeasureLabel"));

        gridLayout->addWidget(playAtMeasureLabel, 0, 4, 1, 1);

        playAtMeasureSpinBox = new QSpinBox(AutoPilotSettingsDialog);
        playAtMeasureSpinBox->setObjectName(QString::fromUtf8("playAtMeasureSpinBox"));
        playAtMeasureSpinBox->setMaximum(999999999);

        gridLayout->addWidget(playAtMeasureSpinBox, 0, 3, 1, 1);

        playAtLabel = new QLabel(AutoPilotSettingsDialog);
        playAtLabel->setObjectName(QString::fromUtf8("playAtLabel"));
        playAtLabel->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);

        gridLayout->addWidget(playAtLabel, 0, 1, 1, 1);

        horizontalSpacer = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        gridLayout->addItem(horizontalSpacer, 0, 0, 3, 1);

        horizontalSpacer_2 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        gridLayout->addItem(horizontalSpacer_2, 0, 9, 3, 1);

        playForMeasureLabel = new QLabel(AutoPilotSettingsDialog);
        playForMeasureLabel->setObjectName(QString::fromUtf8("playForMeasureLabel"));

        gridLayout->addWidget(playForMeasureLabel, 1, 4, 1, 1);

        playForBeatSpinBox = new QSpinBox(AutoPilotSettingsDialog);
        playForBeatSpinBox->setObjectName(QString::fromUtf8("playForBeatSpinBox"));
        playForBeatSpinBox->setMaximum(999999999);

        gridLayout->addWidget(playForBeatSpinBox, 1, 6, 1, 1);

        playForBeatLabel = new QLabel(AutoPilotSettingsDialog);
        playForBeatLabel->setObjectName(QString::fromUtf8("playForBeatLabel"));

        gridLayout->addWidget(playForBeatLabel, 1, 7, 1, 1);

        playForCheckBox = new QCheckBox(AutoPilotSettingsDialog);
        playForCheckBox->setObjectName(QString::fromUtf8("playForCheckBox"));

        gridLayout->addWidget(playForCheckBox, 1, 8, 1, 1);

        playAtCheckBox = new QCheckBox(AutoPilotSettingsDialog);
        playAtCheckBox->setObjectName(QString::fromUtf8("playAtCheckBox"));

        gridLayout->addWidget(playAtCheckBox, 0, 8, 1, 1);


        retranslateUi(AutoPilotSettingsDialog);
        QObject::connect(buttonBox, SIGNAL(accepted()), AutoPilotSettingsDialog, SLOT(accept()));
        QObject::connect(buttonBox, SIGNAL(rejected()), AutoPilotSettingsDialog, SLOT(reject()));

        QMetaObject::connectSlotsByName(AutoPilotSettingsDialog);
    } // setupUi

    void retranslateUi(QDialog *AutoPilotSettingsDialog)
    {
        AutoPilotSettingsDialog->setWindowTitle(QApplication::translate("AutoPilotSettingsDialog", "Dialog", nullptr));
        playForLabel->setText(QApplication::translate("AutoPilotSettingsDialog", "Play for:", nullptr));
        playAtBeatLabel->setText(QApplication::translate("AutoPilotSettingsDialog", "Beat(s)", nullptr));
        playAtMeasureLabel->setText(QApplication::translate("AutoPilotSettingsDialog", "Measure(s)", nullptr));
        playAtLabel->setText(QApplication::translate("AutoPilotSettingsDialog", "Play at:", nullptr));
        playForMeasureLabel->setText(QApplication::translate("AutoPilotSettingsDialog", "Measure(s)", nullptr));
        playForBeatLabel->setText(QApplication::translate("AutoPilotSettingsDialog", "Beat(s)", nullptr));
        playForCheckBox->setText(QApplication::translate("AutoPilotSettingsDialog", "Infinite", nullptr));
        playAtCheckBox->setText(QApplication::translate("AutoPilotSettingsDialog", "Infinite", nullptr));
    } // retranslateUi

};

namespace Ui {
    class AutoPilotSettingsDialog: public Ui_AutoPilotSettingsDialog {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_AUTOPILOTSETTINGSDIALOG_H
