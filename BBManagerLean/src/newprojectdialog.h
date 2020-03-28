#ifndef NEWPROJECTDIALOG_H
#define NEWPROJECTDIALOG_H

#include <QDialog>
#include <QString>

namespace Ui {
class NewProjectDialog;
}

class NewProjectDialog : public QDialog
{
    Q_OBJECT

public:
    explicit NewProjectDialog(QWidget *parent, QString default_path);
    QString getProjectName();
    QString getSelectedFolder();
    ~NewProjectDialog();

private slots:
    void on_pushButton_clicked();

    void on_lineEdit_textChanged(const QString &arg1);

    void on_buttonBox_accepted();

    void on_buttonBox_rejected();

private:
    Ui::NewProjectDialog *ui;
    void refreshButton();
    QString m_defaultPath;
};

#endif // NEWPROJECTDIALOG_H
