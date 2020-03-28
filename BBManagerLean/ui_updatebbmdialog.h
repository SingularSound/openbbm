/********************************************************************************
** Form generated from reading UI file 'updatebbmdialog.ui'
**
** Created by: Qt User Interface Compiler version 5.12.2
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_UPDATEBBMDIALOG_H
#define UI_UPDATEBBMDIALOG_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QDialog>
#include <QtWidgets/QFrame>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QProgressBar>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QStackedWidget>
#include <QtWidgets/QTextEdit>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_UpdateBBMDialog
{
public:
    QVBoxLayout *verticalLayout;
    QHBoxLayout *refreshInfoLayout;
    QLabel *curVersionLabel;
    QSpacerItem *horizontalSpacer_2;
    QPushButton *forumPageButton;
    QPushButton *refreshInfoButton;
    QFrame *line;
    QTextEdit *releaseInfoEdit;
    QFrame *line_2;
    QStackedWidget *stackedUpdate;
    QWidget *page0;
    QGridLayout *gridLayout;
    QCheckBox *doNotShowCheckbox;
    QSpacerItem *horizontalSpacer;
    QPushButton *updateButton;
    QWidget *page1;
    QGridLayout *gridLayout_4;
    QLabel *labelDownloading;
    QWidget *page2;
    QGridLayout *gridLayout_2;
    QProgressBar *progressDownloading;
    QWidget *page3;
    QGridLayout *gridLayout_3;
    QLabel *label;

    void setupUi(QDialog *UpdateBBMDialog)
    {
        if (UpdateBBMDialog->objectName().isEmpty())
            UpdateBBMDialog->setObjectName(QString::fromUtf8("UpdateBBMDialog"));
        UpdateBBMDialog->resize(628, 409);
        UpdateBBMDialog->setMinimumSize(QSize(400, 302));
        verticalLayout = new QVBoxLayout(UpdateBBMDialog);
        verticalLayout->setObjectName(QString::fromUtf8("verticalLayout"));
        refreshInfoLayout = new QHBoxLayout();
        refreshInfoLayout->setObjectName(QString::fromUtf8("refreshInfoLayout"));
        curVersionLabel = new QLabel(UpdateBBMDialog);
        curVersionLabel->setObjectName(QString::fromUtf8("curVersionLabel"));

        refreshInfoLayout->addWidget(curVersionLabel);

        horizontalSpacer_2 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        refreshInfoLayout->addItem(horizontalSpacer_2);

        forumPageButton = new QPushButton(UpdateBBMDialog);
        forumPageButton->setObjectName(QString::fromUtf8("forumPageButton"));

        refreshInfoLayout->addWidget(forumPageButton);

        refreshInfoButton = new QPushButton(UpdateBBMDialog);
        refreshInfoButton->setObjectName(QString::fromUtf8("refreshInfoButton"));

        refreshInfoLayout->addWidget(refreshInfoButton);


        verticalLayout->addLayout(refreshInfoLayout);

        line = new QFrame(UpdateBBMDialog);
        line->setObjectName(QString::fromUtf8("line"));
        line->setFrameShape(QFrame::HLine);
        line->setFrameShadow(QFrame::Sunken);

        verticalLayout->addWidget(line);

        releaseInfoEdit = new QTextEdit(UpdateBBMDialog);
        releaseInfoEdit->setObjectName(QString::fromUtf8("releaseInfoEdit"));
        releaseInfoEdit->setEnabled(true);
        releaseInfoEdit->setAcceptDrops(false);
        releaseInfoEdit->setReadOnly(true);

        verticalLayout->addWidget(releaseInfoEdit);

        line_2 = new QFrame(UpdateBBMDialog);
        line_2->setObjectName(QString::fromUtf8("line_2"));
        line_2->setFrameShape(QFrame::HLine);
        line_2->setFrameShadow(QFrame::Sunken);

        verticalLayout->addWidget(line_2);

        stackedUpdate = new QStackedWidget(UpdateBBMDialog);
        stackedUpdate->setObjectName(QString::fromUtf8("stackedUpdate"));
        stackedUpdate->setMaximumSize(QSize(16777215, 27));
        page0 = new QWidget();
        page0->setObjectName(QString::fromUtf8("page0"));
        gridLayout = new QGridLayout(page0);
        gridLayout->setSpacing(0);
        gridLayout->setObjectName(QString::fromUtf8("gridLayout"));
        gridLayout->setContentsMargins(0, 0, 0, 0);
        doNotShowCheckbox = new QCheckBox(page0);
        doNotShowCheckbox->setObjectName(QString::fromUtf8("doNotShowCheckbox"));

        gridLayout->addWidget(doNotShowCheckbox, 0, 0, 1, 1);

        horizontalSpacer = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        gridLayout->addItem(horizontalSpacer, 0, 1, 1, 1);

        updateButton = new QPushButton(page0);
        updateButton->setObjectName(QString::fromUtf8("updateButton"));

        gridLayout->addWidget(updateButton, 0, 2, 1, 1);

        stackedUpdate->addWidget(page0);
        page1 = new QWidget();
        page1->setObjectName(QString::fromUtf8("page1"));
        gridLayout_4 = new QGridLayout(page1);
        gridLayout_4->setSpacing(0);
        gridLayout_4->setObjectName(QString::fromUtf8("gridLayout_4"));
        gridLayout_4->setContentsMargins(0, 0, 0, 0);
        labelDownloading = new QLabel(page1);
        labelDownloading->setObjectName(QString::fromUtf8("labelDownloading"));

        gridLayout_4->addWidget(labelDownloading, 0, 0, 1, 1);

        stackedUpdate->addWidget(page1);
        page2 = new QWidget();
        page2->setObjectName(QString::fromUtf8("page2"));
        gridLayout_2 = new QGridLayout(page2);
        gridLayout_2->setSpacing(0);
        gridLayout_2->setObjectName(QString::fromUtf8("gridLayout_2"));
        gridLayout_2->setContentsMargins(0, 0, 0, 0);
        progressDownloading = new QProgressBar(page2);
        progressDownloading->setObjectName(QString::fromUtf8("progressDownloading"));
        progressDownloading->setMinimumSize(QSize(0, 21));
        progressDownloading->setValue(24);
        progressDownloading->setAlignment(Qt::AlignCenter);
        progressDownloading->setInvertedAppearance(false);

        gridLayout_2->addWidget(progressDownloading, 0, 0, 1, 1);

        stackedUpdate->addWidget(page2);
        page3 = new QWidget();
        page3->setObjectName(QString::fromUtf8("page3"));
        gridLayout_3 = new QGridLayout(page3);
        gridLayout_3->setSpacing(0);
        gridLayout_3->setObjectName(QString::fromUtf8("gridLayout_3"));
        gridLayout_3->setContentsMargins(0, 0, 0, 0);
        label = new QLabel(page3);
        label->setObjectName(QString::fromUtf8("label"));

        gridLayout_3->addWidget(label, 0, 0, 1, 1);

        stackedUpdate->addWidget(page3);

        verticalLayout->addWidget(stackedUpdate);


        retranslateUi(UpdateBBMDialog);

        stackedUpdate->setCurrentIndex(0);
        updateButton->setDefault(true);


        QMetaObject::connectSlotsByName(UpdateBBMDialog);
    } // setupUi

    void retranslateUi(QDialog *UpdateBBMDialog)
    {
        UpdateBBMDialog->setWindowTitle(QApplication::translate("UpdateBBMDialog", "BeatBuddy Manager Update", nullptr));
        curVersionLabel->setText(QApplication::translate("UpdateBBMDialog", "TextLabel", nullptr));
        forumPageButton->setText(QApplication::translate("UpdateBBMDialog", "Visit Forum Page...", nullptr));
        refreshInfoButton->setText(QApplication::translate("UpdateBBMDialog", "Refresh Info", nullptr));
        doNotShowCheckbox->setText(QApplication::translate("UpdateBBMDialog", "Do not show notifications for this update again", nullptr));
        updateButton->setText(QApplication::translate("UpdateBBMDialog", "Update", nullptr));
        labelDownloading->setText(QApplication::translate("UpdateBBMDialog", "Preparing to download...", nullptr));
        label->setText(QApplication::translate("UpdateBBMDialog", "Your BBManager will now close to apply an update...", nullptr));
    } // retranslateUi

};

namespace Ui {
    class UpdateBBMDialog: public Ui_UpdateBBMDialog {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_UPDATEBBMDIALOG_H
