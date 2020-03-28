#ifndef AUTOPILOTSETTINGSDIALOG_H
#define AUTOPILOTSETTINGSDIALOG_H

#include <QDialog>
#include "model/filegraph/midiParser.h"

namespace Ui {
class AutoPilotSettingsDialog;
}

class AutoPilotSettingsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AutoPilotSettingsDialog(MIDIPARSER_TrackType type, int sigNum, int playFor, int playAt, QWidget *parent = nullptr);
    ~AutoPilotSettingsDialog();

    int playAt();
    int playFor();

    void setPlayAt(int playAt);
    void setPlayFor(int playFor);

    void deletePlayForUi();
    void deletePlayAtUi();
    void hidePlayAtBeatUi();
    void hidePlayForBeatUi();

private slots:
    void on_playForCheckBox_toggled(bool checked);
    void on_playAtCheckBox_toggled(bool checked);

private:
    Ui::AutoPilotSettingsDialog *ui;
    MIDIPARSER_TrackType         m_type;
    int                          m_sigNum;
    bool                         m_playAtEnabled;
    bool                         m_playForEnabled;
};

#endif // AUTOPILOTSETTINGSDIALOG_H
