#ifndef SUPPORTDIALOG_H
#define SUPPORTDIALOG_H

#include <QDialog>
#include <QDir>

namespace Ui {
class SupportDialog;
}

class SupportDialog : public QDialog
{
   Q_OBJECT

public:
   explicit SupportDialog(QWidget *parent = 0);
   ~SupportDialog();

private slots:
   void on_linkClicked(const QUrl &arg1);

private:
   Ui::SupportDialog *ui;
};

#endif // SUPPORTDIALOG_H
