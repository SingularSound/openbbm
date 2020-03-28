#ifndef DRUMSETNAMEDIALOG_H
#define DRUMSETNAMEDIALOG_H

#include <QDialog>
#include <QDialogButtonBox>
#include <QComboBox>
#include <QGridLayout>
#include <QLabel>
#include <QLineEdit>
#include <QSlider>

class DrumsetNameDialog : public QDialog
{
    Q_OBJECT
public:
    explicit DrumsetNameDialog(QString name, int volume, QWidget *parent = nullptr);

    // Getters
    QString name() const { return lineEditDrumsetName->text(); }
    int volume() const { return sliderVolume->value(); }

private:
    QDialogButtonBox *buttonBox;
    QGridLayout *gridLayout;
    QLineEdit *lineEditDrumsetName;
    QSlider *sliderVolume;
};

#endif // DRUMSETNAMEDIALOG_H
