#ifndef DRUMSETPANEL_H
#define DRUMSETPANEL_H

#include <QWidget>
#include <QMessageBox>
#include <QAbstractItemModel>
#include <QItemSelectionModel>
#include <QListWidgetItem>
#include <QProgressBar>

#include "Model/drmmakermodel.h"
#include "DrumSetMaker.h"
#include "DrumSetExtractor.h"
#include "UI_Elements/Labels/DrumsetLabel.h"
#include "UI_Elements/Dialogs/DrumsetNameDialog.h"
#include "UI_Elements/drumsetsizeprogressbar.h"

//#define DEBUG

#ifdef DEBUG
#include <QDebug>
#endif

//class DrumsetFolderView;

class DrumsetPanel : public QWidget
{
    Q_OBJECT
    
public:
    DrumsetPanel(QWidget *parent = nullptr);
    ~DrumsetPanel();


    QGridLayout *mp_InstrumentsGridLayout;

    inline DrmMakerModel *drmMakerModel(){ return mp_drmMakerModel;}


signals:
   void sigOpenedDrm(const QString &filePath);
   void sigClosedDrm(const QString &filePath);

public slots:
    void slotCurrentItemChanged(QListWidgetItem *current, QListWidgetItem *previous);
    void slotOpenDrm(const QString &name, const QString &path);
    void slotOpenedDrmNameChanged(const QString & originalName, const QString & newName);

    // Menu actions
    void on_actionNew_triggered();
    void on_actionSave_triggered();
    void on_actionSaveAs_triggered();
    void on_actionClose_triggered();
    void on_actionOpen_triggered();
    void on_actionAdd_instrument_triggered();
    void on_instrument_deleteButt_onPressed(Instrument* instr);
    void on_actionSort_Instruments_triggered();

    void drumsetNameChanged(const QString& name);
    void drumsetSizeChanged(quint32 size);

    void instrumentAdded(int index);
    void instrumentRemoved(int index);
    void instrumentSorted();

private slots:
    // Menu actions
    void on_pushButton_clicked();

    void changeDrumsetName(QMouseEvent*);

private:
    // Members
    DrumsetLabel *mDrumsetName;
    DrmMakerModel *mp_drmMakerModel;
    DrumSetMaker *mDrumSetMaker;
    DrumsetSizeProgressBar *mProgressBar;

    // Methods
    void createLayout();
    void createNewDrumset();
    bool promptSaveUnsavedChanges();
    bool saveCurrentDrumset();
    bool saveCurrentDrumsetAs();
    void closeDrmInternal();
    bool openDrmInternal(const QString &path);

protected:
    virtual void paintEvent(QPaintEvent * event);

    friend class CmdOpenDrumset;
    friend class CmdAddRemoveInstrument;
};

#endif // DRUMSETPANEL_H
