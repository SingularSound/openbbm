/*
    This software and the content provided for use with it is \n Copyright © 2014-2020 Singular Sound
    BeatBuddy Manager is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2 as published by
    the Free Software Foundation.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/
#include <QAction>
#include <QDebug>
#include <QDialog>
#include <QFile>
#include <QFileDialog>
#include <QGridLayout>
#include <QItemSelectionModel>
#include <QMenuBar>
#include <QMessageBox>
#include <QObject>
#include <QPainter>
#include <QSplitter>
#include <QVBoxLayout>

#include "beatspanel/beatspanel.h"
#include "beatspanel/songfolderview.h"
#include "debug.h"
#include "dialogs/aboutdialog.h"
#include "dialogs/optionsdialog.h"
#include "dialogs/supportdialog.h"
#include "drmmaker/DrumsetPanel.h"
#include "mainwindow.h"
#include "model/beatsmodelfiles.h"
#include "model/selectionlinker.h"
#include "model/tree/abstracttreeitem.h"
#include "model/tree/project/beatsprojectmodel.h"
#include "model/tree/project/songfolderproxymodel.h"
#include "model/tree/project/drmfoldertreeitem.h"
#include "model/tree/project/effectfoldertreeitem.h"
#include "model/tree/project/paramsfoldertreemodel.h"
#include "model/tree/project/foldertreeitem.h"
#include "model/tree/song/songfileitem.h"
#include "model/tree/song/songfoldertreeitem.h"
#include "newprojectdialog.h"
#include "pexpanel/drmlistmodel.h"
#include "pexpanel/projectexplorerpanel.h"
#include "platform/platform.h"
#include "playbackpanel.h"
#include "quazip.h"
#include "quazipdir.h"
#include "quazipfile.h"
#include "update/update.h"
#include "utils/dirlistallsubfilesmodal.h"
#include "utils/dircleanupmodal.h"
#include "utils/dircopymodal.h"
#include "utils/extractzipmodal.h"
#include "versioninfo.h"
#include "vm/virtualmachinepanel.h"
#include "workspace/contentlibrary.h"
#include "workspace/libcontent.h"
#include "workspace/settings.h"
#include "workspace/workspace.h"

#include <midi_editor/editordata.h>
#include <midi_editor/editorhorizontalheader.h>
#include <midi_editor/editorverticalheader.h>
#include <midi_editor/undoable_commands.h>

MainWindow::MainWindow(QString default_path, QWidget *parent)
    : QMainWindow(parent)
    , m_playing(false)
    , m_beats_inactive(false)
    , mp_beatsModel(nullptr)
    , mp_beatsProxyModel(nullptr)
    , m_drag_all(QString("^.*\\.(")+BMFILES_PROJECT_EXTENSION+"|"+BMFILES_SINGLE_FILE_EXTENSION+"|"+BMFILES_PORTABLE_SONG_EXTENSION+"|"+BMFILES_PORTABLE_FOLDER_EXTENSION+"|"+BMFILES_DRUMSET_EXTENSION+")$")
    , m_drag_project(QString("^.*\\.(")+BMFILES_PROJECT_EXTENSION+"|"+BMFILES_SINGLE_FILE_EXTENSION+")$")
    , mp_CheatShitDialog(nullptr)
    , m_done(true)
    , update(this)
{
    initUI();
    createTempProjectFolder(default_path);
    // according to inherited comment, this should be done after menus are created.
    initWorkspace();

    createDrmListModel(); // TODO - needs master workspace in order to refresh drumset list
    if (default_path.endsWith("." BMFILES_PROJECT_EXTENSION)){
        createBeatsModels(default_path);
    } else if (default_path.endsWith("." BMFILES_SINGLE_FILE_EXTENSION)){
        slotImportPrjArchive(default_path);
    } // Otherwise, do not load anything. TODO manage drumset opening
    refreshMenus();
}

void MainWindow::initUI()
{
    setWindowTitle(VersionInfo().productName());
    createCentralWidget();
    createActions();
    createMenus();
    createStateSaver();
    setGeometry(Settings::getWindowX(), Settings::getWindowY(), Settings::getWindowW(), Settings::getWindowH());
    if(Settings::getWindowMaximized()) {
        showMaximized();
    }

}

void MainWindow::initWorkspace()
{
    // Make sure workspace folders and keys exist
    mp_MasterWorkspace = new Workspace(this);
    if(!mp_MasterWorkspace->isValid()){
        QTimer::singleShot(1000, this, SLOT(slotChangeWorkspaceLocation()));
    }
}

void MainWindow::createTempProjectFolder(QString default_path){
    QDir tmp = QDir::temp();
    if(!tmp.exists()){
        qCritical()<< "Couldn't create temporary folder! Path: " <<  default_path;
        QMessageBox::critical(this, windowTitle(), tr("Unable to create temporary folder.\nAborting..."));
        close();
        return;
    }
    
    qDebug() << "temp dir:" << tmp.absolutePath() << "default_path:" << default_path;
    
    m_tempDirFI = QFileInfo(tmp.absoluteFilePath(QUuid::createUuid().toString()));
    
    if(!tmp.mkdir(m_tempDirFI.absoluteFilePath())){
        qCritical()<<"Couldn't create temporary _project_ folder, inside the temporary folder.";
        QMessageBox::critical(this, windowTitle(), tr("Unable to create temporary project folder.\nAborting..."));
        close();
        return;
    }
}

/**
 * @brief MainWindow::createBeatsModels
 * @param projectFilePath
 *
 * requires refresh menu to be called after call
 */
void MainWindow::createBeatsModels(const QString &projectFilePath, bool create)
{
   // 1 - Start by deleting any previous model
   deleteModel();

   if(create){
      if(projectFilePath.isEmpty()) {
         qWarning() << "ERROR 1 - cannot create model projectFilePath.isEmpty()";
         return;
      }

      QFileInfo fi(projectFilePath);

      if(fi.exists()){
         qWarning() << "ERROR 2 - Overwriting project file not handled, file exists" << "(" << fi.absoluteFilePath() << ")";
         return;
      }

      mp_beatsModel = new BeatsProjectModel(fi.absoluteFilePath(), this, m_tempDirFI.absoluteFilePath());

   } else if(!projectFilePath.isEmpty()) {
      mp_beatsModel = new BeatsProjectModel(projectFilePath, this, m_tempDirFI.absoluteFilePath());

   } else {
      qWarning() << "ERROR 9 - Model not created";
      return;
   }

   // memorize last opened project;
   if(mp_beatsModel){
      Settings::setLastProject(mp_beatsModel->projectFileFI().absoluteFilePath());
   }

   mp_drmListModel->setBeatsModel(mp_beatsModel);


   mp_beatsProxyModel = new SongFolderProxyModel;
   mp_beatsProxyModel->setSourceModel(mp_beatsModel);
   mp_beatsProxyModel->setMaxDepth(3);
   mp_BeatsPanel->setModel(mp_beatsModel);

   QItemSelectionModel *pexSelectionModel = new SelectionLinker(mp_beatsProxyModel, mp_BeatsPanel->selectionModel());

   mp_ProjectExplorerPanel->setProxyBeatsModel(mp_beatsProxyModel);
   mp_ProjectExplorerPanel->setProxyBeatsRootIndex(mp_beatsProxyModel->mapFromSource(mp_beatsModel->songsFolderIndex()));
   mp_ProjectExplorerPanel->setProxyBeatsSelectionModel(pexSelectionModel);

   mp_ProjectExplorerPanel->setBeatsModel(mp_beatsModel, mp_UndoRedo);
   mp_ProjectExplorerPanel->setBeatsDebugSelectionModel(mp_BeatsPanel->selectionModel());

   mp_PlaybackPanel->setModel(mp_beatsModel);

   connect(mp_BeatsPanel->selectionModel(), SIGNAL(currentChanged(QModelIndex,QModelIndex)), this, SLOT(slotOnBeatsCurrentOrSelChanged()));
   connect(mp_BeatsPanel->selectionModel(), SIGNAL(currentChanged(const QModelIndex&,const QModelIndex&)), mp_beatsModel, SLOT(selectionChanged(const QModelIndex&,const QModelIndex&)));
   connect(mp_beatsModel, SIGNAL(changeSelection(const QModelIndex&)), mp_BeatsPanel, SLOT(changeSelection(const QModelIndex&)));
   connect(mp_BeatsPanel->selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)), this, SLOT(slotOnBeatsCurrentOrSelChanged()));
   connect(mp_ProjectExplorerPanel->beatsSelectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)), this, SLOT(slotOnBeatsCurrentOrSelChanged()));
   connect(mp_ProjectExplorerPanel->beatsSelectionModel(), SIGNAL(currentChanged(QModelIndex,QModelIndex)), this, SLOT(slotOnBeatsCurrentOrSelChanged()));
   connect(mp_ProjectExplorerPanel, SIGNAL(sigCurrentItemChanged(QListWidgetItem*,QListWidgetItem*)), mp_DrumsetPanel, SLOT(slotCurrentItemChanged(QListWidgetItem*,QListWidgetItem*)));
   connect(mp_beatsModel, SIGNAL(dataChanged(QModelIndex,QModelIndex)), this, SLOT(slotModelDataChanged(QModelIndex,QModelIndex)));

   connect(mp_beatsModel, SIGNAL(rowsInserted(QModelIndex,int,int)), this, SLOT(slotOrderChanged()));
   connect(mp_beatsModel, SIGNAL(rowsRemoved(QModelIndex,int,int)), this, SLOT(slotOrderChanged()));
   connect(mp_beatsModel, SIGNAL(rowsMoved(QModelIndex,int,int,QModelIndex,int)), this, SLOT(slotOrderChanged()));

   connect(mp_beatsModel, &BeatsProjectModel::beginEditMidi, this, &MainWindow::slotOnBeginEditSongPart);

   connect(mp_VMPanel, SIGNAL(sigPedalPress()), mp_PlaybackPanel, SLOT(slotPedalPress()));
   connect(mp_VMPanel, SIGNAL(sigPedalRelease()), mp_PlaybackPanel, SLOT(slotPedalRelease()));
   connect(mp_VMPanel, SIGNAL(sigPedalLongPress()), mp_PlaybackPanel, SLOT(slotPedalLongPress()));
   connect(mp_VMPanel, SIGNAL(sigPedalDoubleTap()), mp_PlaybackPanel, SLOT(slotPedalDoubleTap()));
   connect(mp_VMPanel, SIGNAL(sigEffect()), mp_PlaybackPanel, SLOT(slotEffect()));
   connect(mp_PlaybackPanel, SIGNAL(signalIsPlaying(bool)),this,SLOT(slotActivePlayer(bool)));

   connect(mp_BeatsPanel, &BeatsPanel::sigSelectTrack, mp_PlaybackPanel, &PlaybackPanel::slotSelectTrack);
   connect(mp_PlaybackPanel, SIGNAL(sigPlayerEnabled(bool)), mp_BeatsPanel, SLOT(slotSetPlayerEnabled(bool)));

   mp_beatsModel->songsFolder()->manageParsingErrors(this);

    if (auto stack = mp_beatsModel ? mp_beatsModel->undoStack() : nullptr) {
        mp_UndoRedo->addStack(stack);
    }

    auto ix = mp_beatsModel->songsFolderIndex().child(0,0);
    { auto ix0 = ix.child(0,0); if (ix0 != QModelIndex()) { ix = ix0; } }
    BlockUndoForSelectionChangesFurtherInThisScope _;
    mp_BeatsPanel->selectionModel()->select(ix, QItemSelectionModel::SelectCurrent);
    mp_BeatsPanel->selectionModel()->setCurrentIndex(ix, QItemSelectionModel::SelectCurrent);
}

void MainWindow::createDrmListModel()
{
    mp_drmListModel = new DrmListModel(this, mp_MasterWorkspace);
    mp_drmListModel->setDrmMakerModel(mp_DrumsetPanel->drmMakerModel());
    mp_ProjectExplorerPanel->setDrmListModel(mp_drmListModel);


    // signals/slots related to Drm

    connect(mp_ProjectExplorerPanel, SIGNAL(sigOpenDrm(const QString&,const QString&)), mp_DrumsetPanel,  SLOT(slotOpenDrm(const QString&,const QString&)));
    connect(mp_DrumsetPanel, SIGNAL(sigOpenedDrm(QString)), mp_drmListModel, SLOT(slotOnDrmFileOpened(QString)));
    connect(mp_DrumsetPanel, SIGNAL(sigClosedDrm(QString)), mp_drmListModel, SLOT(slotOnDrmFileClosed(QString)));
    connect(mp_DrumsetPanel, SIGNAL(sigOpenedDrm(QString)), this, SLOT(slotOnDrmOpened()));
    connect(mp_DrumsetPanel, SIGNAL(sigClosedDrm(QString)), this, SLOT(slotOnDrmClosed()));
    connect(mp_drmListModel, SIGNAL(sigOpenedDrmNameChanged(QString, QString)), mp_DrumsetPanel, SLOT(slotOpenedDrmNameChanged(QString, QString)));

    // Actions that may require refreshing menu
    connect(mp_BeatsDrmStackedWidget, SIGNAL(currentChanged(int)),   this, SLOT(refreshMenus()));
    connect(mp_DrumsetPanel,          SIGNAL(sigOpenedDrm(QString)), this, SLOT(refreshMenus()));
    connect(mp_DrumsetPanel,          SIGNAL(sigClosedDrm(QString)), this, SLOT(refreshMenus()));
    connect(mp_ProjectExplorerPanel->drmListSelectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)), this, SLOT(refreshMenus()));
    connect(mp_ProjectExplorerPanel->drmListSelectionModel(), SIGNAL(currentChanged(QModelIndex,QModelIndex)), this, SLOT(refreshMenus()));

}

/**
 * @brief MainWindow::deleteModel
 *
 * requires refresh menu to be called after call
 */
void MainWindow::deleteModel()
{
   // 1 - Stop Player
   if (mp_PlaybackPanel) {
      mp_PlaybackPanel->stop();
   }

   // 1.3 - Drop old undo stack
   if (auto stack = mp_beatsModel ? mp_beatsModel->undoStack() : nullptr) {
       mp_UndoRedo->removeStack(stack);
   }

   // 1.5 - Disconnect old signals
   if(mp_beatsModel){

      disconnect(mp_BeatsPanel->selectionModel(), SIGNAL(currentChanged(QModelIndex,QModelIndex)), this, SLOT(slotOnBeatsCurrentOrSelChanged()));
      disconnect(mp_BeatsPanel->selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)), this, SLOT(slotOnBeatsCurrentOrSelChanged()));
      disconnect(mp_ProjectExplorerPanel->beatsSelectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)), this, SLOT(slotOnBeatsCurrentOrSelChanged()));
      disconnect(mp_ProjectExplorerPanel->beatsSelectionModel(), SIGNAL(currentChanged(QModelIndex,QModelIndex)), this, SLOT(slotOnBeatsCurrentOrSelChanged()));
      disconnect(mp_VMPanel, SIGNAL(sigPedalPress()), mp_PlaybackPanel, SLOT(slotPedalPress()));
      disconnect(mp_VMPanel, SIGNAL(sigPedalRelease()), mp_PlaybackPanel, SLOT(slotPedalRelease()));
      disconnect(mp_VMPanel, SIGNAL(sigPedalLongPress()), mp_PlaybackPanel, SLOT(slotPedalLongPress()));
      disconnect(mp_VMPanel, SIGNAL(sigPedalDoubleTap()), mp_PlaybackPanel, SLOT(slotPedalDoubleTap()));
      disconnect(mp_VMPanel, SIGNAL(sigEffect()), mp_PlaybackPanel, SLOT(slotEffect()));
      disconnect(mp_PlaybackPanel, SIGNAL(signalIsPlaying(bool)),this,SLOT(slotActivePlayer(bool)));

      disconnect(mp_BeatsPanel, &BeatsPanel::sigSelectTrack, mp_PlaybackPanel, &PlaybackPanel::slotSelectTrack);
      disconnect(mp_PlaybackPanel, SIGNAL(sigPlayerEnabled(bool)), mp_BeatsPanel, SLOT(slotSetPlayerEnabled(bool)));
   }

   // 2 - memorize old model
   BeatsProjectModel    *p_oldModel      = mp_beatsModel;
   SongFolderProxyModel *p_oldProxyModel = mp_beatsProxyModel;

   mp_beatsModel = nullptr;
   mp_beatsProxyModel = nullptr;

   mp_BeatsPanel->setModel(mp_beatsModel);
   mp_ProjectExplorerPanel->setProxyBeatsModel(mp_beatsProxyModel);
   mp_ProjectExplorerPanel->setBeatsModel(mp_beatsModel, mp_UndoRedo);
   mp_PlaybackPanel->setModel(mp_beatsModel);
   mp_drmListModel->setBeatsModel(mp_beatsModel);

   // Delete former model if it is not null
   if(p_oldModel){
      delete p_oldModel;
   }

   if(p_oldProxyModel){
      delete p_oldProxyModel;
   }
}
QAction* MainWindow::buildAction(const QString &title,const QString &statusTip,const QString &toolTip){
    auto action = new QAction(title,this);
    action->setStatusTip(statusTip);
    action->setToolTip(toolTip);
    return action;
}

QAction* MainWindow::buildAction(const QString &title, const QString &statusTip, const QString &toolTip,const QKeySequence &qks){
    auto action = this->buildAction(title,statusTip,toolTip);
    action->setShortcut(qks);
    return action;
}

void MainWindow::createActions()
{
#ifdef Q_OS_MAC
    auto KEY_INSERT = Qt::MetaModifier | Qt::Key_Return;
    auto KEY_DELETE = Qt::MetaModifier | Qt::Key_Backspace;
#else
    auto KEY_INSERT = Qt::Key_Insert;
    auto KEY_DELETE = Qt::Key_Delete;
#endif

    mp_newSong = this->buildAction(tr("New Song"), tr("Create a new song file"), tr("Create a new song file"), QKeySequence(KEY_INSERT));
    connect(mp_newSong, SIGNAL(triggered()), this, SLOT(slotNewSong()));

    QList<QKeySequence> sc;
    sc << (QKeySequence(KEY_DELETE)) << (QKeySequence(Qt::ShiftModifier | KEY_DELETE));

    mp_delete = this->buildAction(tr("Delete Song"), tr("Delete selected song file or directory"), tr("Delete selected song file or directory"));
    mp_delete->setShortcuts(sc);
    connect(mp_delete, SIGNAL(triggered()), this, SLOT(slotDelete()));

    mp_prev = this->buildAction(tr("Go To Previous"),tr("Go To Previous"), tr("Select Previous"), QKeySequence(Qt::Key_Up));
    connect(mp_prev, SIGNAL(triggered()), this, SLOT(slotPrev()));

    mp_next = this->buildAction(tr("Go To Next"), tr("Go To Next"), tr("Select Next"), QKeySequence(Qt::Key_Down));
    connect(mp_next, SIGNAL(triggered()), this, SLOT(slotNext()));

    mp_child = this->buildAction(tr("Go Inside Folder"), tr("Go Inside Folder"), tr("Select First Folder Item"), QKeySequence(Qt::Key_Right));
    connect(mp_child, SIGNAL(triggered()), this, SLOT(slotChild()));

    mp_parent = this->buildAction(tr("Go To Folder"), tr("Go To Parent Folder"), tr("Select Parent Folder"), QKeySequence(Qt::Key_Left));
    connect(mp_parent, SIGNAL(triggered()), this, SLOT(slotParent()));

    mp_exportSong = this->buildAction(tr("&Song"), tr("Export Current Song in a portable song format"), tr("Export Current Song in a portable song format"));
    connect(mp_exportSong, SIGNAL(triggered()), this, SLOT(slotExport()));

    mp_importSong = this->buildAction(tr("&Song"), tr("Import a portable song to current folder"), tr("Import a portable song to current folder"), QKeySequence(Qt::AltModifier | KEY_INSERT));
    connect(mp_importSong, &QAction::triggered, [this]() { slotImport(QStringList(), -1); });

    mp_exportFolder = this->buildAction(tr("&Folder"),  tr("Export Current Folder in a portable folder format"), tr("Export Current Folder in a portable folder format"));
    connect(mp_exportFolder, SIGNAL(triggered()), this, SLOT(slotExportFolder()));

    mp_importFolder = this->buildAction(tr("&Folder"), tr("Import a portable folder to current project"), tr("Import a portable folder to current project"), QKeySequence(Qt::AltModifier | Qt::ControlModifier | KEY_INSERT));
    connect(mp_importFolder, &QAction::triggered, [this]() { slotImport(QStringList(), 1); });

    mp_importDrm = this->buildAction(tr("&Drumset"), tr("Import a Drumset to the workspace"), tr("Import a Drumset to the workspace"));
    connect(mp_importDrm, SIGNAL(triggered()), this, SLOT(slotImportDrm()));

    mp_newPrj = this->buildAction(tr("&New Project"), tr("Create a new empty project. Current project will be closed"), tr("Create a new empty project. Current project will be closed"), QKeySequence::New);
    connect(mp_newPrj, SIGNAL(triggered()), this, SLOT(slotNewPrj()));

    mp_openPrj = this->buildAction(tr("&Open Project"), tr("Open an existing project. Current project will be closed"), tr("Open an existing project. Current project will be closed"), QKeySequence::Open);
    connect(mp_openPrj, SIGNAL(triggered()), this, SLOT(slotOpenPrj()));

    mp_savePrj = this->buildAction(tr("Save &Project"), tr("Save project file"), tr("Save project file"), QKeySequence::Save);
    connect(mp_savePrj, SIGNAL(triggered()), this, SLOT(slotSavePrj()));

#ifdef Q_OS_WIN
    auto KEY_SAVE_AS = QKeySequence(Qt::ControlModifier | Qt::ShiftModifier | Qt::Key_S);
#else
    auto KEY_SAVE_AS = QKeySequence::SaveAs;
#endif
    mp_savePrjAs = this->buildAction(tr("Save Project &As"), tr("Save current project under a new name and/or location"), tr("Save current project under a new name and/or location"), KEY_SAVE_AS);
    connect(mp_savePrjAs, SIGNAL(triggered()), this, SLOT(slotSavePrjAs()));

    mp_syncPrj = this->buildAction(tr("&Synchronize Project"), tr("Synchronize a Pedal / SD card content with local project"), tr("Synchronize a Pedal / SD card content with local project"));
    connect(mp_syncPrj, SIGNAL(triggered()), this, SLOT(slotSyncPrj()));

    mp_exportPrj = this->buildAction(tr("&Project to SD card"), tr("Export the whole local project to a Pedal / SD card"), tr("Export the whole local project to a Pedal / SD card"));
    connect(mp_exportPrj, SIGNAL(triggered()), this, SLOT(slotExportPrj()));

    mp_moveFolderUp = this->buildAction(tr("Move Folder Up"), tr("Move current Folder Up one position"), tr("Move current Folder Up one position"), QKeySequence(Qt::ControlModifier | Qt::ShiftModifier | Qt::Key_Up));
    connect(mp_moveFolderUp, SIGNAL(triggered()), this, SLOT(slotMoveFolderUp()));

    mp_moveFolderDown = this->buildAction(tr("Move Folder Down"), tr("Move current Folder Down one position"), tr("Move current Folder Down one position"), QKeySequence(Qt::ControlModifier | Qt::ShiftModifier | Qt::Key_Down));
    connect(mp_moveFolderDown, SIGNAL(triggered()), this, SLOT(slotMoveFolderDown()));

    if (Settings::midiIdEnabled()) {
        mp_toggleMidiId = this->buildAction(tr("Disable Song MIDI ID"), tr("Disable Song MIDI ID"), tr("Disable Song MIDI ID"));
    } else {
        mp_toggleMidiId = this->buildAction(tr("Enable Song MIDI ID"), tr("Enable Song MIDI ID"), tr("Enable Song MIDI ID"));
    }
    connect(mp_toggleMidiId, SIGNAL(triggered()), this, SLOT(slotToggleMidiId()));

    mp_moveUp = this->buildAction(tr("Move Song Up"), tr("Move current Song Up one position"), tr("Move current Song Up one position"), QKeySequence(Qt::ShiftModifier | Qt::Key_Up));
    connect(mp_moveUp, SIGNAL(triggered()), this, SLOT(slotMoveUp()));

    mp_moveDown = this->buildAction(tr("Move Song Down"), tr("Move current Song Down one position"), tr("Move current Song Down one position"), QKeySequence(Qt::ShiftModifier | Qt::Key_Down));
    connect(mp_moveDown, SIGNAL(triggered()), this, SLOT(slotMoveDown()));

    mp_newFolder = this->buildAction(tr("New Folder"), tr("Create a new Song Folder"), tr("Create a new Song Folder"), QKeySequence(Qt::ControlModifier | KEY_INSERT));
    connect(mp_newFolder, SIGNAL(triggered()), this, SLOT(slotNewFolder()));

    sc.clear();
    sc << (QKeySequence(Qt::ControlModifier | KEY_DELETE)) << (QKeySequence(Qt::ControlModifier | Qt::ShiftModifier | KEY_DELETE));
    mp_deleteFolder = this->buildAction(tr("Delete Folder"), tr("Delete current song folder and all of its content"), tr("Delete current song folder and all of its content"));
    mp_deleteFolder->setShortcuts(sc);
    connect(mp_deleteFolder, SIGNAL(triggered()), this, SLOT(slotDeleteFolder()));

    mp_exit = this->buildAction(tr("&Quit"), tr("Quit BeatBuddy Manager"), tr("Quit BeatBuddy Manager"), QKeySequence::Quit);
    connect(mp_exit, SIGNAL(triggered()), this, SLOT(close()));

    mp_newDrm = this->buildAction(tr("&New Drumset"), tr("Create a new drumset"), tr("Create a new drumset"));
    connect(mp_newDrm, SIGNAL(triggered()), mp_DrumsetPanel,SLOT(on_actionNew_triggered()));

    mp_openDrm = this->buildAction(tr("&Open Drumset"), tr("Open a drumset"), tr("Open a drumset"));
    connect(mp_openDrm, SIGNAL(triggered()), mp_DrumsetPanel,SLOT(on_actionOpen_triggered()));

    mp_saveDrm = this->buildAction(tr("&Save Drumset"), tr("Save current drumset"), tr("Save current drumset"));
    connect(mp_saveDrm, SIGNAL(triggered()), mp_DrumsetPanel, SLOT(on_actionSave_triggered()));

    mp_saveDrmAs = this->buildAction(tr("Save Drumset &As"), tr("Save current drumset under a new name and/or location"), tr("Save current drumset under a new name and/or location"));
    connect(mp_saveDrmAs, SIGNAL(triggered()), mp_DrumsetPanel, SLOT(on_actionSaveAs_triggered()));

    mp_closeDrm = this->buildAction(tr("&Close Drumset"), tr("Close the current drumset"), tr("Close the current drumset"));
    connect(mp_closeDrm, SIGNAL(triggered()), mp_DrumsetPanel, SLOT(on_actionClose_triggered()));

    mp_addDrmInstrument = this->buildAction(tr("Add &Instrument"), tr("Add an instrument to currently opened Drum Set"), tr("Add an instrument to currently opened Drum Set"));
    connect(mp_addDrmInstrument, SIGNAL(triggered()), mp_DrumsetPanel, SLOT(on_actionAdd_instrument_triggered()));

    mp_sortDrmInstrument = this->buildAction(tr("Sor&t Instruments"), tr("Sort the instruments in the currently opened Drum Set"), tr("Sort the instruments in the currently opened Drum Set"));
    connect(mp_sortDrmInstrument, SIGNAL(triggered()), mp_DrumsetPanel, SLOT(on_actionSort_Instruments_triggered()));

    mp_deleteDrm = this->buildAction(tr("&Delete Drumset"), tr("Delete Drumset from workspace and/or project"), tr("Delete Drumset from workspace and/or project"));
    connect(mp_deleteDrm, SIGNAL(triggered()), mp_ProjectExplorerPanel, SLOT(slotDrmDeleteRequest()));

    mp_zoomIn = this->buildAction(tr("Zoom &In"), tr("Zoom editor in"), tr("Increase editor zoom level"), QKeySequence(Qt::ControlModifier | Qt::Key_Plus));
    connect(mp_zoomIn, &QAction::triggered, [this] { mp_TableEditor->zoom(1); });

    mp_zoomOut = this->buildAction(tr("Zoom &Out"), tr("Zoom editor out"), tr("Decrease editor zoom level"), QKeySequence(Qt::ControlModifier | Qt::Key_Minus));
    connect(mp_zoomOut, &QAction::triggered, [this] { mp_TableEditor->zoom(-1); });

    mp_zoomReset = this->buildAction(tr("&Zoom Reset"), tr("Default editor zoom"), tr("Reset editor zoom level"), QKeySequence(Qt::ControlModifier | Qt::Key_0));
    connect(mp_zoomReset, &QAction::triggered, [this] { mp_TableEditor->zoom(0); });

    mp_UpdateFirmware = this->buildAction(tr("Update Firmware"), tr("Update firmware of the BeatBuddy"), tr("Update firmware of the BeatBuddy"));
    connect(mp_UpdateFirmware, SIGNAL(triggered()), this, SLOT(slotInstallNewFirmware()));

    mp_ChangeWorkspaceLocation = this->buildAction(tr("Set Workspace Location"), tr("Change the BeatBuddy Manager Workspace location"), tr("Change the BeatBuddy Manager Workspace location"), QKeySequence(Qt::AltModifier | Qt::Key_F2));
    connect(mp_ChangeWorkspaceLocation, SIGNAL(triggered()), this, SLOT(slotChangeWorkspaceLocation()));

    mp_ShowOptionsDialog = this->buildAction(tr("Buffering time"), tr("Show Options Dialog"), tr("Show Options Dialog"), QKeySequence(Qt::AltModifier | Qt::Key_F3));
    connect(mp_ShowOptionsDialog, SIGNAL(triggered()), this, SLOT(slotShowOptionsDialog()));

    mp_ShowAboutDialog = this->buildAction(tr("&About BBManager"), tr("Show About Dialog"), tr("Show About Dialog"), QKeySequence(Qt::AltModifier | Qt::Key_F1));
    connect(mp_ShowAboutDialog, SIGNAL(triggered()), this, SLOT(slotShowAboutDialog()));

    mp_linkToPdfManual = this->buildAction(tr("&Tutorial"), tr("Open BeatBuddy Manager Tutorial"), tr("Open BeatBuddy Manager Tutorial"), QKeySequence::HelpContents);
    connect(mp_linkToPdfManual, SIGNAL(triggered()), this, SLOT(slotOpenUrlManual()));

    mp_linkToForum = this->buildAction(tr("&Forum"), tr("Open BeatBuddy Online Forum"), tr("Open BeatBuddy Online Forum"));
    connect(mp_linkToForum, SIGNAL(triggered()), this, SLOT(slotOpenUrlForum()));

    mp_ShowCheatShitDialog = this->buildAction(tr("Getting Started"), tr("Show Cheat Sheet Help Dialog"), tr("Show Cheat Sheet Help Dialog"));
    connect(mp_ShowCheatShitDialog, SIGNAL(triggered()), this, SLOT(slotShowCheatShit()));

    mp_ShowSupportDialog = this->buildAction(tr("&Support"), tr("Show Support Dialog"), tr("Show Support Dialog"));
    connect(mp_ShowSupportDialog, SIGNAL(triggered()), this, SLOT(slotShowSupportDialog()));

    mp_ShowUpdateDialog = this->buildAction(tr("Update"), tr("Show Update Dialog"), tr("Show Update Dialog"), QKeySequence(Qt::AltModifier | Qt::Key_F5));
    
    connect(mp_ShowUpdateDialog, &QAction::triggered,&update, &Update::showUpdateDialog);

    mp_ColorOptions = this->buildAction(tr("Color Options"), tr("Setup BBManager colors"), tr("Setup BBManager colors"), QKeySequence(Qt::AltModifier | Qt::Key_F6));
    connect(mp_ColorOptions, SIGNAL(triggered()), this, SLOT(slotShowColorOptions()));

    mp_UndoRedo = new QUndoGroup(this);
    mp_UndoRedo->addStack(mp_TableEditor->undoStack());
    connect(mp_UndoRedo, SIGNAL(cleanChanged(bool)), mp_ProjectExplorerPanel, SLOT(slotCleanChanged(bool)));

    mp_Undo = mp_UndoRedo->createUndoAction(this);
    mp_Undo->setShortcut(QKeySequence::Undo);

    mp_Redo = mp_UndoRedo->createRedoAction(this);
    mp_Redo->setShortcut(QKeySequence::Redo);

    mp_Copy = this->buildAction(tr("&Copy"), tr("Copy Selection"), tr("Copy Selection to Clipboard"), QKeySequence::Copy);
    connect(mp_Copy, SIGNAL(triggered()), this, SLOT(slotCopy()));

    mp_Paste = this->buildAction(tr("&Paste"), tr("Paste Data"), tr("Paste Data from Clipboard"), QKeySequence::Paste);
    connect(mp_Paste, SIGNAL(triggered()), this, SLOT(slotPaste()));

    mp_AcceptEditor = this->buildAction(tr("&Apply editor data"), tr("Copy Selection"), tr("Copy Selection to Clipboard"), QKeySequence(Qt::ShiftModifier | Qt::Key_Enter));
    connect(mp_AcceptEditor, &QAction::triggered, mp_TableEditor, &EditorTableWidget::accept);

    mp_CancelEditor = this->buildAction(tr("Cancel editor &data"), tr("Paste Data"), tr("Paste Data from Clipboard"), QKeySequence(Qt::ShiftModifier | Qt::Key_Escape));
    connect(mp_CancelEditor, &QAction::triggered, mp_TableEditor, &EditorTableWidget::cancel);

    mp_RevealLogs = this->buildAction(tr("Show Log File"),tr("Show Log File"),tr("Show Log File"));
    connect(mp_RevealLogs, SIGNAL(triggered()), this, SLOT(slotRevealLogs()));
}

void MainWindow::createStateSaver()
{
    mp_window_state_saver = new QTimer(this);
    mp_window_state_saver->setSingleShot(true);
    mp_window_state_saver->setInterval(250);
    connect(mp_window_state_saver, SIGNAL(timeout()), this, SLOT(slotSaveWindowState()));
}

void MainWindow::createMenus()
{
    mp_fileMenu = menuBar()->addMenu(tr("&File"));
    mp_fileMenu->addAction(mp_newPrj);
    mp_fileMenu->addAction(mp_openPrj);
    mp_fileMenu->addAction(mp_savePrj);
    mp_fileMenu->addAction(mp_savePrjAs);
    mp_fileMenu->addSeparator();
    mp_import = mp_fileMenu->addMenu(tr("&Import"));
    mp_import->addAction(mp_importSong);
    mp_import->addAction(mp_importFolder);
    mp_import->addSeparator();
    mp_import->addAction(mp_importDrm);
    mp_export = mp_fileMenu->addMenu(tr("&Export"));
    mp_export->addAction(mp_exportSong);
    mp_export->addAction(mp_exportFolder);
    mp_export->addSeparator();
    mp_export->addAction(mp_exportPrj);
    mp_fileMenu->addSeparator();
    mp_fileMenu->addAction(mp_syncPrj);
    mp_fileMenu->addSeparator();
    mp_fileMenu->addAction(mp_exit);

    mp_edit = menuBar()->addMenu(tr("&Edit"));
    mp_edit->setVisible(false);
    mp_edit->addAction(mp_Undo);
    mp_edit->addAction(mp_Redo);
    mp_edit->addSeparator();
    mp_edit->addAction(mp_Copy);
    mp_edit->addAction(mp_Paste);

    mp_songsMenu = menuBar()->addMenu(tr("&Songs"));
    mp_songsMenu->addAction(mp_newSong);
    mp_songsMenu->addAction(mp_newFolder);
    mp_songsMenu->addAction(mp_delete);
    mp_songsMenu->addAction(mp_deleteFolder);
    mp_songsMenu->addSeparator();
    mp_songsMenu->addAction(mp_parent);
    mp_songsMenu->addAction(mp_prev);
    mp_songsMenu->addAction(mp_next);
    mp_songsMenu->addAction(mp_child);
    mp_songsMenu->addSeparator();
    mp_songsMenu->addAction(mp_moveUp);
    mp_songsMenu->addAction(mp_moveDown);
    mp_songsMenu->addAction(mp_moveFolderUp);
    mp_songsMenu->addAction(mp_moveFolderDown);
    mp_songsMenu->addSeparator();
    mp_songsMenu->addAction(mp_toggleMidiId);

    mp_drumsMenu = menuBar()->addMenu(tr("&Drumsets"));
    mp_drumsMenu->addAction(mp_newDrm);
    mp_drumsMenu->addAction(mp_openDrm);
    mp_drumsMenu->addAction(mp_saveDrm);
    mp_drumsMenu->addAction(mp_saveDrmAs);
    mp_drumsMenu->addAction(mp_closeDrm);
    mp_drumsMenu->addAction(mp_deleteDrm);
    mp_drumsMenu->addSeparator();
    mp_drumsMenu->addAction(mp_addDrmInstrument);
    mp_drumsMenu->addAction(mp_sortDrmInstrument);

    mp_editorMenu = menuBar()->addMenu(tr("&MIDI Editor"));
    mp_editorMenu->addAction(mp_AcceptEditor);
    mp_editorMenu->addAction(mp_CancelEditor);
    mp_editorMenu->addSeparator();
    mp_editorMenu->addAction(mp_zoomIn);
    mp_editorMenu->addAction(mp_zoomOut);
    mp_editorMenu->addAction(mp_zoomReset);

    mp_toolsMenu = menuBar()->addMenu(tr("&Tools"));
    // DISABLED FOR NOW until properly implemented
    //mp_toolsMenu->addAction(mp_UpdateFirmware);
    //mp_toolsMenu->addSeparator();
    mp_toolsMenu->addAction(mp_ChangeWorkspaceLocation);
    mp_toolsMenu->addAction(mp_ShowOptionsDialog);
    mp_toolsMenu->addAction(mp_ShowUpdateDialog);
    //TODO: check this easter egg created by Daefecator.
    if (QFileInfo(QDir(QApplication::applicationDirPath()).absoluteFilePath("Daefecator")).exists()) {
        mp_toolsMenu->addSeparator();
        mp_toolsMenu->addAction(mp_ColorOptions);
    }

    auto links = menuBar()->addMenu(tr("Useful &Links"));
    links->addAction(mp_linkToPdfManual);
    links->addAction(mp_linkToForum);
    links->addSeparator();
    links->addAction("Premium Content &Library", this, SLOT(slotOpenUrlLibrary()));

    mp_helpMenu = menuBar()->addMenu(tr("&Help"));
    mp_helpMenu->addAction(mp_ShowCheatShitDialog);
    mp_helpMenu->addAction(mp_ShowSupportDialog);
    mp_helpMenu->addAction(mp_ShowAboutDialog);
    mp_helpMenu->addSeparator();
    mp_helpMenu->addAction(mp_RevealLogs);
    // Disable functionalities that are not implemented yet
    mp_syncPrj->setEnabled(false);

    // Disable functionalities that require a model
    mp_newSong->setEnabled(false);
    mp_savePrj->setEnabled(false);
    mp_exportSong->setEnabled(false);
    mp_importSong->setEnabled(false);
    mp_exportFolder->setEnabled(false);
    mp_importFolder->setEnabled(false);
    mp_importDrm->setEnabled(false);
    mp_savePrjAs->setEnabled(false);
    mp_syncPrj->setEnabled(false);
    mp_exportPrj->setEnabled(false);
    mp_moveFolderUp->setEnabled(false);
    mp_moveFolderDown->setEnabled(false);
    mp_moveUp->setEnabled(false);
    mp_moveDown->setEnabled(false);
    mp_newFolder->setEnabled(false);
    mp_deleteFolder->setEnabled(false);

    // disable functionalities that require an opened drumset
    mp_saveDrm->setEnabled(false);
    mp_saveDrmAs->setEnabled(false);
    mp_closeDrm->setEnabled(false);
    mp_addDrmInstrument->setEnabled(false);
    mp_sortDrmInstrument->setEnabled(false);

    // disable functionalities that require DrmList model
    mp_deleteDrm->setEnabled(false);

}

QUndoStack* MainWindow::selectUndoStack(){
    QUndoStack* stack = nullptr;
    if(!mp_BeatsDrmStackedWidget->currentIndex()){
        stack = mp_TableEditor->undoStack();
    }else if (mp_BeatsDrmStackedWidget->currentIndex()-1){
        if(mp_beatsModel){
            stack = mp_beatsModel->undoStack();
        }
    }else if(mp_drmListModel){
        stack = mp_drmListModel->undoStack();
    }
    return stack;
}

void MainWindow::refreshMenus()
{
    // activate necessary undo stack
    QUndoStack* stack = selectUndoStack();
    if (stack) {
        mp_UndoRedo->setActiveStack(stack);
    }

    auto editor = !mp_BeatsDrmStackedWidget->currentIndex();
    mp_newPrj->setDisabled(editor);
    mp_openPrj->setDisabled(editor);
    mp_savePrj->setDisabled(editor);
    mp_savePrjAs->setDisabled(editor);
    mp_import->setDisabled(editor);
    mp_export->setDisabled(editor);
    mp_import->setDisabled(editor);
    mp_exportSong->setDisabled(editor);
    mp_importSong->setDisabled(editor);
    mp_exportFolder->setDisabled(editor);
    mp_importFolder->setDisabled(editor);
    mp_importDrm->setDisabled(editor);
    mp_syncPrj->setDisabled(editor);
    mp_exportPrj->setDisabled(editor);
    mp_Copy->setDisabled(editor);
    mp_Paste->setDisabled(editor);
    mp_newSong->setDisabled(editor);
    mp_delete->setDisabled(editor);
    mp_newFolder->setDisabled(editor);
    mp_deleteFolder->setDisabled(editor);
    mp_parent->setDisabled(editor);
    mp_prev->setDisabled(editor);
    mp_next->setDisabled(editor);
    mp_child->setDisabled(editor);
    mp_moveUp->setDisabled(editor);
    mp_moveDown->setDisabled(editor);
    mp_moveFolderUp->setDisabled(editor);
    mp_moveFolderDown->setDisabled(editor);
    mp_newDrm->setDisabled(editor);
    mp_openDrm->setDisabled(editor);
    mp_saveDrm->setDisabled(editor);
    mp_saveDrmAs->setDisabled(editor);
    mp_closeDrm->setDisabled(editor);
    mp_deleteDrm->setDisabled(editor);
    mp_addDrmInstrument->setDisabled(editor);
    mp_sortDrmInstrument->setDisabled(editor);
    mp_AcceptEditor->setEnabled(editor);
    mp_CancelEditor->setEnabled(editor);
    mp_zoomIn->setEnabled(editor);
    mp_zoomOut->setEnabled(editor);
    mp_zoomReset->setEnabled(editor);
    if (editor){
        return;
    }
    if (!mp_beatsModel) {
        mp_newSong->setEnabled(false);
        mp_savePrj->setEnabled(false);
        mp_exportSong->setEnabled(false);
        mp_importSong->setEnabled(false);
        mp_exportFolder->setEnabled(false);
        mp_importFolder->setEnabled(false);
        mp_importDrm->setEnabled(false);
        mp_savePrjAs->setEnabled(false);
        mp_syncPrj->setEnabled(false);
        mp_exportPrj->setEnabled(false);
        mp_moveFolderUp->setEnabled(false);
        mp_moveFolderDown->setEnabled(false);
        mp_moveUp->setEnabled(false);
        mp_moveDown->setEnabled(false);
        mp_newFolder->setEnabled(false);
        mp_deleteFolder->setEnabled(false);

        mp_prev->setEnabled(false);
        mp_next->setEnabled(false);
        mp_child->setEnabled(false);
        mp_parent->setEnabled(false);
    } else {
        mp_importDrm->setEnabled(true);
        mp_savePrjAs->setEnabled(true);
        mp_exportSong->setEnabled(true);
        mp_exportFolder->setEnabled(true);
        mp_exportPrj->setEnabled(true);
        mp_savePrj->setEnabled(true);

        mp_syncPrj->setEnabled(mp_beatsModel->isProject2WayLinked());

        mp_newSong->setDisabled(m_playing);
        mp_newFolder->setDisabled(m_playing);
        mp_importSong->setDisabled(m_playing);
        mp_importFolder->setDisabled(m_playing);

        // Song count related
        QModelIndex songFolderIndex = mp_BeatsPanel->rootIndex();
        if (songFolderIndex.isValid()) {
           int maxChildrenCnt = songFolderIndex.sibling(songFolderIndex.row(), AbstractTreeItem::MAX_CHILD_CNT).data().toInt();
           if (mp_beatsModel->rowCount(songFolderIndex) >= maxChildrenCnt) {
              mp_importSong->setEnabled(false);
              mp_newSong->setEnabled(false);
           }
        } else {
           mp_importSong->setEnabled(false);
           mp_newSong->setEnabled(false);
        }

        // Current and selected related
        QModelIndex current = mp_BeatsPanel->selectionModel()->currentIndex();
        QItemSelection selection = mp_BeatsPanel->selectionModel()->selection();

        bool currentIsFolder = false;
        bool currentIsSong = false;
        if (!selection.isEmpty() && current.isValid() && current.parent().isValid()) {
            // Retrieve parent's file children file type
            QModelIndex parentChildrenTypes = mp_beatsModel->index(current.parent().row(), AbstractTreeItem::CHILDREN_TYPE, current.parent().parent());
            if (parentChildrenTypes.data().isValid()) {
                QStringList types =  parentChildrenTypes.data().toString().split(",");
                currentIsSong = types.contains(BMFILES_MIDI_BASED_SONG_EXTENSION, Qt::CaseInsensitive) || types.contains(BMFILES_WAVE_BASED_SONG_EXTENSION, Qt::CaseInsensitive);
                currentIsFolder = !currentIsSong; // TODO : make sure `current' is really a folder under this broad conditions
            } else {
                currentIsFolder = true; // TODO : make sure `current' is really a folder under this broad conditions
            }
        }

        if (!m_playing && currentIsSong) {
            // song selected
            mp_prev->setDisabled(m_beats_inactive);
            auto last = current.sibling(current.row()+1, 0) == QModelIndex();
            auto folder = current.parent();
            auto last_folder = folder.sibling(folder.row()+1, 0) == QModelIndex();
            mp_next->setDisabled(m_beats_inactive || (last && last_folder));
            mp_child->setDisabled(true);
            mp_parent->setDisabled(m_beats_inactive);
            mp_moveUp->setDisabled(!current.row());
            mp_moveDown->setDisabled(last);
            mp_moveFolderUp->setDisabled(!folder.row());
            mp_moveFolderDown->setDisabled(last_folder);
        } else if (!m_playing && currentIsFolder) {
            // folder selected
            mp_prev->setDisabled(m_beats_inactive || !current.row());
            auto last = current.sibling(current.row()+1, 0) == QModelIndex();
            mp_next->setDisabled(m_beats_inactive || (last && current.child(0, 0) == QModelIndex()));
            mp_child->setDisabled(m_beats_inactive || current.child(0, 0) == QModelIndex());
            mp_parent->setDisabled(true);
            mp_moveUp->setDisabled(!current.row());
            mp_moveDown->setDisabled(last);
            mp_moveFolderUp->setDisabled(!current.row());
            mp_moveFolderDown->setDisabled(last);
            mp_exportSong->setDisabled(true);
        } else {
            mp_prev->setDisabled(true);
            mp_next->setDisabled(true);
            mp_child->setDisabled(true);
            mp_parent->setDisabled(true);
            mp_moveUp->setDisabled(true);
            mp_moveDown->setDisabled(true);
            mp_moveFolderUp->setDisabled(true);
            mp_moveFolderDown->setDisabled(true);
            mp_importSong->setDisabled(true);
            mp_importFolder->setDisabled(true);
            mp_exportSong->setDisabled(true);
            mp_exportFolder->setDisabled(true);
        }

        mp_deleteFolder->setDisabled(m_playing);
    }

    if (!mp_drmListModel) {
       mp_deleteDrm->setEnabled(false);
    } else {
       QModelIndex current = mp_ProjectExplorerPanel->drmListSelectionModel()->currentIndex();
       QItemSelection selection = mp_ProjectExplorerPanel->drmListSelectionModel()->selection();
       // If current is valid and selected pex tab is drumset
       if(!selection.isEmpty() && current.isValid() && mp_BeatsDrmStackedWidget->currentIndex() == 1){
          // Built in drumsets or opened drumset cannot be edited
          if(current.sibling(current.row(), DrmListModel::TYPE).data().toString() != "default" &&
                !current.sibling(current.row(), DrmListModel::OPENED).data().toBool()){
             mp_deleteDrm->setEnabled(true);
          } else {
             mp_deleteDrm->setEnabled(false);
          }
       } else {
          mp_deleteDrm->setEnabled(false);
       }
    }
}

void MainWindow::createCentralWidget()
{
    setCentralWidget(new QWidget(this));
    centralWidget()->setLayout(new QGridLayout());
    
    QSplitter *p_GlobalSplitter = new QSplitter(centralWidget());
    p_GlobalSplitter->setOrientation(Qt::Horizontal);
    centralWidget()->layout()->addWidget(p_GlobalSplitter);
    
    QWidget *p_LeftPanel = new QWidget(p_GlobalSplitter);
    QWidget *p_RightPanel = new QWidget(p_GlobalSplitter);
    p_GlobalSplitter->addWidget(p_LeftPanel);
    p_GlobalSplitter->setStretchFactor(0,1);
    p_GlobalSplitter->addWidget(p_RightPanel);
    p_GlobalSplitter->setStretchFactor(1,3);
    
    // Left Side
    p_LeftPanel->setLayout(new QGridLayout());
    p_LeftPanel->layout()->setContentsMargins(0,0,0,0);
    QSplitter *p_LeftSplitter = new QSplitter(p_LeftPanel);
    p_LeftSplitter->setOrientation(Qt::Vertical);
    p_LeftPanel->layout()->addWidget(p_LeftSplitter);
    
    // Top Left
    mp_VMPanel = new VirtualMachinePanel(p_LeftSplitter);
    p_LeftSplitter->addWidget(mp_VMPanel);
    
    // Bottom Left
    mp_ProjectExplorerPanel = new ProjectExplorerPanel(p_LeftSplitter);
    p_LeftSplitter->addWidget(mp_ProjectExplorerPanel);
    
    //p_LeftSplitter->setStretchFactor(0,0);
    p_LeftSplitter->setStretchFactor(1,1);
    
    // Right Side
    QVBoxLayout *p_RightLayout = new QVBoxLayout();
    p_RightLayout->setContentsMargins(0,0,0,0);
    p_RightPanel->setLayout(p_RightLayout);
    
    // Top Right
    mp_PlaybackPanel = new PlaybackPanel(p_RightPanel);
    p_RightLayout->addWidget(mp_PlaybackPanel, 0);
    
    // Bottom Right
    mp_BeatsDrmStackedWidget = new QStackedWidget(p_RightPanel);
    p_RightLayout->addWidget(mp_BeatsDrmStackedWidget, 1);
    
    // MIDI Editor
    mp_TableEditor = new EditorTableWidget(mp_BeatsDrmStackedWidget);
    connect(mp_TableEditor, &EditorTableWidget::changed, [this] {
        mp_TableEditor->dropUnusedUnsupportedRows();
        if (mp_beatsModel) {
            emit mp_beatsModel->editMidi(mp_TableEditor->collect());
        }
    });
    mp_BeatsDrmStackedWidget->addWidget(mp_TableEditor);
    
    mp_DrumsetPanel = new DrumsetPanel(mp_BeatsDrmStackedWidget);
    mp_BeatsDrmStackedWidget->addWidget(mp_DrumsetPanel);
    
    mp_BeatsPanel = new BeatsPanel(mp_BeatsDrmStackedWidget);
    mp_BeatsDrmStackedWidget->addWidget(mp_BeatsPanel);
    
    mp_BeatsDrmStackedWidget->setCurrentIndex(2);
    
    // Static connections
    connect(mp_ProjectExplorerPanel, SIGNAL(sigCurrentTabChanged(int)), this, SLOT(setBeatsDrmStackedWidgetCurrentIndex(int)));
    
    // Initialize VMPanel Settings
    mp_VMPanel->getVMScreen()->slotSetStarted(mp_PlaybackPanel->mp_Player->started());
    mp_VMPanel->getVMScreen()->slotSetPart(mp_PlaybackPanel->mp_Player->part());
    mp_VMPanel->getVMScreen()->slotTimeSigNumChanged(mp_PlaybackPanel->mp_Player->sigNum());
    mp_VMPanel->getVMScreen()->slotBeatInBarChanged(mp_PlaybackPanel->mp_Player->beatInBar());
    
    connect(mp_PlaybackPanel, &PlaybackPanel::drumsetChanged, this, &MainWindow::slotDrumsetChanged);
    
    // Queued connections since player is on another thread
    connect(mp_PlaybackPanel->mp_Player, SIGNAL(sigStartedChanged(bool)), mp_VMPanel->getVMScreen(), SLOT(slotSetStarted(bool)), Qt::QueuedConnection);
    connect(mp_PlaybackPanel->mp_Player, SIGNAL(sigPartChanged(int)), mp_VMPanel->getVMScreen(), SLOT(slotSetPart(int)), Qt::QueuedConnection);
    connect(mp_PlaybackPanel->mp_Player, SIGNAL(sigSigNumChanged(int)), mp_VMPanel->getVMScreen(), SLOT(slotTimeSigNumChanged(int)), Qt::QueuedConnection);
    connect(mp_PlaybackPanel->mp_Player, SIGNAL(sigBeatInBarChanged(int)), mp_VMPanel->getVMScreen(), SLOT(slotBeatInBarChanged(int)), Qt::QueuedConnection);

}

#include "player/soundManager.h"
// FIXME copy-paste from player/soundmanager.c
PACK typedef struct {
    char     fileType[4];
    uint8_t  version;
    uint8_t  revision;
    uint16_t build;
    uint32_t fileCRC;
} PACKED DRUMSETFILE_HeaderStruct;
//!FIXME copy-paste from player/soundmanager.c

void MainWindow::slotDrumsetChanged(const QString& drm_path)
{
    auto m = mp_TableEditor->model();
    if (m->property(EditorData::Drumset).toString() == drm_path)
        return;
    QString name;
    QMap<int, QString> notes; {
        QByteArray drm; {
            QFile file(drm_path);
            if (!file.open(QIODevice::ReadOnly))
                return;
            drm = file.readAll();
        }
        auto data = drm.data();
        auto hdr = (DRUMSETFILE_HeaderStruct*)data; hdr;
        auto inst = (Instrument_t*)(data + sizeof(DRUMSETFILE_HeaderStruct));
        for (int i = 0; i < MIDIPARSER_NUMBER_OF_INSTRUMENTS; ++i)
            if (inst[i].nVel)
                notes[i] = QString::null;
        QDataStream meta(drm.mid(*(int*)&inst[MIDIPARSER_NUMBER_OF_INSTRUMENTS]));
        meta >> name;
        foreach (auto x, notes.keys())
            meta >> notes[x];
    }
    mp_TableEditor->clearSelection();
    auto v = (EditorVerticalHeader*)mp_TableEditor->verticalHeader();
    for (int i = mp_TableEditor->rowCount()-1; i+1; --i) {
        auto note = m->headerData(i, Qt::Vertical, EditorData::V::Id).toInt();
        auto used = m->headerData(i, Qt::Vertical, EditorData::V::Count).toInt();
        auto supported = notes.find(note);
        if (supported != notes.end()) {
            m->setHeaderData(i, Qt::Vertical, notes[note], EditorData::V::Name);
            m->setHeaderData(i, Qt::Vertical, QVariant(), EditorData::V::Unsupported);
            m->setHeaderData(i, Qt::Vertical, QVariant(), Qt::ForegroundRole);
            v->decorate(i);
            notes.erase(supported);
        } else if (used) {
            m->setHeaderData(i, Qt::Vertical, true, EditorData::V::Unsupported);
            m->setHeaderData(i, Qt::Vertical, QBrush(Qt::red), Qt::ForegroundRole);
            v->decorate(i);
        } else {
            mp_TableEditor->removeRow(i);
        }
    }
    for (int i = mp_TableEditor->rowCount()-1; !notes.empty(); --i) {
        auto note = notes.firstKey();
        if (i < 0 || m->headerData(i, Qt::Vertical, EditorData::V::Id).toInt() > note) {
            mp_TableEditor->insertRow(++i);
            m->setHeaderData(i, Qt::Vertical, note, EditorData::V::Id);
            m->setHeaderData(i, Qt::Vertical, notes[note], EditorData::V::Name);
            v->decorate(i);
        } else {
            continue;
        }
        notes.remove(note);
    }
    mp_TableEditor->resizeRowsToContents();
    ((EditorHorizontalHeader*)mp_TableEditor->horizontalHeader())->colorize();
    m->setProperty(EditorData::Drumset, drm_path);
}

void MainWindow::slotOnBeginEditSongPart(const QString& name, const QByteArray& d)
{
    auto x = new SongPart(name, d);
    auto& data = x->data();
    if (!data.barLength) {
        QMessageBox::critical(this, tr("MIDI Editor"), tr("File has zero bar length!"));
        return slotOnEndEditSongPart(x, false);
    }
    // fix for bad MIDI files
    if (auto diff = data.nTick % data.barLength)
        data.nTick = (1+data.nTick/data.barLength)*data.barLength;
    connect(mp_TableEditor, &EditorTableWidget::accept, x, &SongPart::accept);
    connect(mp_TableEditor, &EditorTableWidget::cancel, x, &SongPart::cancel);
    connect(x, &SongPart::done, this, &MainWindow::slotOnEndEditSongPart);
    auto m = mp_TableEditor->model();
    m->setProperty(EditorData::BarLength, data.barLength);
    m->setProperty(EditorData::TickCount, data.nTick);
    m->setProperty(EditorData::TimeSignature, QPoint(data.timeSigNum, data.timeSigDen));
    m->setProperty(EditorData::Quantized, QVariant());
    m->setProperty(EditorData::Tempo, data.bpm);
    mp_TableEditor->changeLayout(x);
    connect(x, &SongPart::colorSchemeChanged, [this](bool color) {
        QList<QRgb> colors;
        if (color) {
            colors.append(qRgb(0, 127, 255)); colors.append(qRgb(127, 192, 255));
            colors.append(qRgb(127, 0, 255)); colors.append(qRgb(192, 127, 255));
        }
        ((EditorHorizontalHeader*)mp_TableEditor->horizontalHeader())->colorize(colors);
    });
    connect(x, &SongPart::timeSignatureChanged, [this, x](int num, int den) {
        auto& data = x->data();
        mp_TableEditor->model()->setProperty(EditorData::TimeSignature, QPoint(data.timeSigNum = num, data.timeSigDen = den));
        ((EditorHorizontalHeader*)mp_TableEditor->horizontalHeader())->colorize();
        emit mp_TableEditor->changed();
    });
    connect(x, &SongPart::visualizerSchemeChanged, [this](int scheme) {
        mp_TableEditor->model()->setProperty(EditorData::VisualizerScheme, 3-scheme);
    });
    connect(x, &SongPart::buttonStyleChanged, [this](bool button) {
        auto m = mp_TableEditor->model();
        auto b = button ? QVariant(true) : QVariant();
        m->setProperty(EditorData::ButtonStyle, b);
        for (int i = m->rowCount()-1; i+1; --i)
            for (int j = m->columnCount()-1; j+1; --j)
                m->setData(m->index(i, j), b, EditorData::X::DrawBorder);
    });
    connect(x, &SongPart::showVelocityChanged, [this](bool show) {
        auto m = mp_TableEditor->model();
        auto s = show ? QVariant(true) : QVariant();
        m->setProperty(EditorData::ShowVelocity, s);
        qDebug() << "setting data";
        for (int i = m->rowCount()-1; i+1; --i)
            for (int j = m->columnCount()-1; j+1; --j)
                m->setData(m->index(i, j), s, EditorData::X::DrawNumber);
        qDebug() << "finished setting data";
    });
    connect(x, &SongPart::barCountChanged, [this, x](int bars) {
        const auto& data = x->data();
        auto m = mp_TableEditor->model();
        auto nTick = m->property(EditorData::TickCount).toInt();
        if (bars == nTick/data.barLength) return;
        const auto& qq = x->qq();
        std::set<int> grid;
        auto c = m->columnCount();
        auto end = bars*data.barLength;
        for (int i = 0; i < c; ++i) {
            auto t = m->headerData(i, Qt::Horizontal, EditorData::H::Tick).toInt();
            if (t > end) { grid.insert(end); end = 0; break; }
            grid.insert(t);
        }
        auto merged = mp_TableEditor->collect();
        if (end) {
            std::set<int> new_grid;
            new_grid.insert(0);
            for (int i = 1; i < 16; ++i)
                new_grid.insert(data.barLength*i/16.+.5);
            new_grid.insert(data.barLength);
            auto pos = data.barLength*(1+int(m->headerData(m->columnCount()-1, Qt::Horizontal, EditorData::H::Tick).toDouble()-.5)/data.barLength);
            for (auto i = 0u; i < data.event.size(); ++i) {
                if (data.event[i].tick >= pos) {
                    pos = data.event[i].tick;
                    if (pos > end)
                        break;
                    auto q = qq.find(pos);
                    grid.insert(q == qq.end() ? pos : q->second);
                }
            }
            for (pos = data.barLength*(1+int(pos-.5)/data.barLength); pos < end; pos += data.barLength)
                for (auto it = new_grid.begin(); it != new_grid.end(); ++it)
                    grid.insert(pos +* it);
        }
        for (auto i = 0u; i < data.event.size(); ++i)
            if (data.event[i].tick >= merged.nTick)
                merged.event.push_back(data.event[i]);
        merged.nTick = bars*data.barLength;
        CmdEditorGrid::queue(mp_TableEditor, x, merged, grid);
    });
    connect(x, &SongPart::tempoChanged, [this, x](int bpm) {
        const auto& data = x->data();
        qDebug() << "bpm" << data.bpm << bpm;
        if (0 == bpm) {
            // use song tempo
        } else if (bpm<MIN_BPM) {
            bpm = MIN_BPM;
        } else if (bpm>MAX_BPM) {
            bpm = MAX_BPM;
        }
        auto m = mp_TableEditor->model();
        m->setProperty(EditorData::Tempo, bpm);
        qDebug() << "updated tempo in editorData to" << bpm;
    });;
    connect(x, &SongPart::quantize, [this, x](bool original) {
        const auto& data = original ? x->data() : mp_TableEditor->collect();
        std::map<int, std::set<int>> layout;
        for (auto it = data.event.begin(); it != data.event.end(); ++it) {
            layout[-1].insert(it->tick);
            layout[it->note].insert(it->tick);
        }
        auto nparts = data.nTick/data.barLength*data.timeSigNum; if (!nparts) nparts = 1;
        auto len = double(data.nTick)/nparts;
        auto eps = .05*len; // coeff must be less than .5
        std::vector<int> split(nparts); for (int k = 0; k < nparts; ++k) split[k] = (k+1)*len;
        std::vector<std::map<int, std::set<int>>> parts(2*nparts); {
            auto d = data.event; std::sort(d.begin(), d.end(), [](const MIDIPARSER_MidiEvent& a, const MIDIPARSER_MidiEvent& b) { return a.tick < b.tick; });
            auto k = 0;
            for (auto n = d.begin(); n != d.end(); ++n) {
                while (k < 2*nparts-1 && n->tick > split[k/2]+(k&1?1:-1)*eps) ++k;
                parts[k][n->note].insert(n->tick);
            }
        }
        for (auto k = 1u; k < parts.size(); k += 2) {
            const auto& end = parts[k].end();
            for (auto i = parts[k].begin(); i != end; ++i) {
                while (i->second.size() > 1) {
                    if (k == parts.size()-1) {
                        auto c = i->second.begin();
                        parts[k-1][i->first].insert(*c);
                        parts[k][i->first].erase(c);
                    } else {
                        auto a = *i->second.begin(), b = *i->second.rbegin(), l = split[k>>1], c = abs(l - a) < abs(l - b) ? b : a;
                        parts[k+(a==c?-1:1)][i->first].insert(c);
                        parts[k][i->first].erase(c);
                    }
                }
            }
        }
        // TODO : analyze all split options in `parts'
        // ...but disregard this for now...
        for (auto k = 1u; k < parts.size()-1; k += 2)
            for (auto i = parts[k].begin(); i != parts[k].end(); ++i)
                for (auto n = i->second.begin(); n != i->second.end(); ++n)
                    parts[k+1][i->first].insert(*n);
        for (auto i = 1u; i < parts.size()/2; ++i) parts[2*i].swap(parts[i]);
        parts.back().swap(parts[parts.size()/2]); parts.resize(parts.size()/2+1);
        // prepare possible quantized grids
        std::map<int, std::set<int>> qq;
        // 1
        if (data.timeSigDen > 4) { std::set<int> grid; grid.insert(0); qq[1].swap(grid); }
        // (2 3) 4 6 8 12 16 24 32 48 64
        for (int i = 1+(data.timeSigDen < 5); i < 64/data.timeSigDen; i <<= 1) {
            std::set<int> grid;
            for (int k = 0; k < i*2; ++k)
                grid.insert(len*k/i/2);
            qq[grid.size()].swap(grid);
            for (int k = 0; k < i*3; ++k)
                grid.insert(len*k/i/3);
            qq[grid.size()].swap(grid);
        }
        // find appropriate grid
        std::set<int> grid;
        std::map<int, int> fix;
        for (auto k = 0u; k < parts.size(); ++k) {
            auto s = parts[k];
            int start = len*k;
            int density = 0;
            for (auto i = s.begin(); i != s.end(); ++i)
                if (density < int(i->second.size()))
                    density = i->second.size();
            auto min = 0x7FFFFFFF, best = -1;
            std::map<int, int> best_fix;
            for (auto q = qq.begin(); q != qq.end(); ++q) {
                if (q->first < density) continue;
                int err = 0;
                std::map<int, int> fix;
                auto begin = q->second.begin(), end = q->second.end();
                for (auto i = s.begin(); i != s.end(); ++i) {
                    std::map<int, std::set<int>> used;
                    for (auto n = i->second.begin(); n != i->second.end(); ++n) {
                        auto pos = *n - start, fix = *begin;
                        if (auto local = pos - fix)
                            for (auto qi = begin; ++qi != end;) {
                                if (auto err = abs(pos -* qi))
                                    if (err < local) { local = err; fix = *qi; }
                                    else break;
                                else { fix = *qi; break; }
                            }
                        used[fix].insert(pos);
                    }
                    for (auto u = used.begin(); u != used.end(); ++u) {
                        auto s = u->second.size();
                        if (s < 2) continue;
                        auto shl = s >> 2, shr = shl;
                        if (s & 1); else {
                            auto sum = 0;
                            for (auto p = u->second.begin(); p != u->second.end(); ++p){
                                sum += *p;
                            }
                            ++(sum < int(s*u->first) ? shl : shr);
                        }
                        auto pos = q->second.find(u->first);
                        for (auto p = pos; shr && p != end; ++p) {
                            if (used.find(*p) == used.end()) {
                                used[*p].insert(-1);
                                --shr;
                            }
                        }
                        shl += shr; shr = 0;
                        for (auto p = pos; shl; --p) {
                            if (used.find(*p) == used.end()) {
                                used[*p].insert(-1);
                                --shl;
                            }
                            if (p == begin)
                                break;
                        }
                        shr += shl; shl = 0;
                        for (auto p = pos; shr; ++p) {
                            if (used.find(*p) == used.end()) {
                                used[*p].insert(-1);
                                --shr;
                            }
                        }
                    }
                    auto m = i->second.begin();
                    auto avg = 0;
                    for (auto u = used.begin(); u != used.end(); ++u, ++m) {
                        avg += abs(u->first -* m + start);
                        fix[*m] = u->first + start;
                    }
                    avg /= i->second.size();
                    err += avg*avg;
                }
                err *= q->first;
                if (err < min) {
                    min = err;
                    best = q->first;
                    best_fix.swap(fix);
                }
            }
            if (best > 0 && int(k) < nparts) {
                for (auto i = qq[best].begin(); i != qq[best].end(); ++i) grid.insert(start +* i); grid.insert(data.nTick);
                for (auto i = best_fix.begin(); i != best_fix.end(); ++i) if (i->first != i->second) fix[i->first] = i->second;
            }
        }
        CmdEditorGrid::queue(mp_TableEditor, x, data, grid, &fix);
    });
    auto plr0 = connect(mp_PlaybackPanel->mp_Player, &Player::sigPlayerStarted, this, [this] {
        auto tick = 0;
        auto m = mp_TableEditor->model();
        auto col = -1;
        for (int i = 0; i < m->columnCount(); ++i) {
            if (m->headerData(i, Qt::Horizontal, EditorData::H::Tick) >= tick) { col = i+1; break; }
        }
        m->setProperty(EditorData::PlayerPosition, col);
        auto scheme = 4-m->property(EditorData::VisualizerScheme).toInt();
        if (!-- scheme) return;
        if (col --> 0) {
            for (auto row = m->rowCount(); row --> 0 ;)
                m->setData(m->index(row, col), true, EditorData::X::Played);
            m->setHeaderData(col, Qt::Horizontal, true, EditorData::H::Played);
        }
        if (!-- scheme) return;
        mp_TableEditor->scrollTo(m->index(0, col), !-- scheme ? QAbstractItemView::PositionAtCenter : QAbstractItemView::EnsureVisible);
    }, Qt::QueuedConnection);
    auto plr1 = connect(mp_PlaybackPanel->mp_Player, &Player::sigPlayerPosition, this, [this](int tick) {
        auto m = mp_TableEditor->model();
        auto prev = m->property(EditorData::PlayerPosition).toInt(), col = prev;
        auto t = m->headerData(col, Qt::Horizontal, EditorData::H::Tick);
        for (; t > tick; t = m->headerData(--col, Qt::Horizontal, EditorData::H::Tick)) {
            // rewind position (safety code)
            if (!col) {
                col = m->columnCount()+1;
                qDebug() << "Editor : Wrong player position (" << tick << ") before start!";
                break;
            }
        }
        for (; t <= tick && col < m->columnCount(); t = m->headerData(++col, Qt::Horizontal, EditorData::H::Tick)) {
            // fast forward position
        }
        if (col > m->columnCount()) {
            m->setProperty(EditorData::PlayerPosition, QVariant());
            col = 0;
        } else if (col != prev || prev < 0) {
            m->setProperty(EditorData::PlayerPosition, col);
        } else {
            return;
        }
        if (prev --> 0) {
            for (auto row = m->rowCount(); row --> 0 ;)
                m->setData(m->index(row, prev), QVariant(), EditorData::X::Played);
            m->setHeaderData(prev, Qt::Horizontal, QVariant(), EditorData::H::Played);
        }
        auto scheme = 4-m->property(EditorData::VisualizerScheme).toInt();
        if (!-- scheme) return;
        if (col --> 0) {
            for (auto row = m->rowCount(); row --> 0 ;)
                m->setData(m->index(row, col), true, EditorData::X::Played);
            m->setHeaderData(col, Qt::Horizontal, true, EditorData::H::Played);
        }
        if (!-- scheme) return;
        mp_TableEditor->scrollTo(m->index(0, col), !-- scheme ? QAbstractItemView::EnsureVisible : QAbstractItemView::PositionAtCenter);
    }, Qt::QueuedConnection);
    auto plr2 = connect(mp_PlaybackPanel->mp_Player, &Player::sigPlayerStopped, this, [this, x] {
        auto m = mp_TableEditor->model();
        if (auto prev = m->property(EditorData::PlayerPosition).toInt())
            if (prev --> 0) {
                for (auto row = m->rowCount(); row --> 0 ;)
                    m->setData(m->index(row, prev), QVariant(), EditorData::X::Played);
                m->setHeaderData(prev, Qt::Horizontal, QVariant(), EditorData::H::Played);
            }
        m->setProperty(EditorData::PlayerPosition, QVariant());
    }, Qt::QueuedConnection);
    x->OnDestroy([this, plr0, plr1, plr2] { disconnect(plr0); disconnect(plr1); disconnect(plr2); });
    mp_ProjectExplorerPanel->slotBeginEdit(x);
    // make default grid for already quantized patterns
    if (m->property(EditorData::Quantized).toBool()) {
        // TODO : re-implement this as a proper logic in changeLayout()
        emit x->quantizeChanged(true);
        emit x->quantize(true);
    }
    if (!m->property(EditorData::Quantized).toBool()) QTimer::singleShot(0, [this, x] {
        if (QMessageBox::Yes == QMessageBox::question(this, tr("Quantize MIDI?"), tr(
            "WARNING! This MIDI pattern was likely recorded from live performance and is not quantized. "
            "To simplify the editing process, we highly recommend to quantize it now.\n\n"
            "Quantized patterns will be shown in a music notation form in the editor header.\n"
            "Right now you can only see note offsets in logical ticks instead.\n\n"
            "TIP: You can always quantize the pattern later via \"Quantize\" button!\n\n"
            "Quantize MIDI pattern now?"),
            QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes)
            ) {
                emit x->quantize(true);
        }
    });
    mp_BeatsDrmStackedWidget->setCurrentIndex(0);
    refreshMenus();
}

void MainWindow::slotOnEndEditSongPart(SongPart* x, bool accept)
{
    emit mp_beatsModel->endEditMidi(accept ? QByteArray(mp_TableEditor->collect()) : QByteArray());
    mp_TableEditor->undoStack()->clear();
    delete x;
    mp_BeatsDrmStackedWidget->setCurrentIndex(2);
    refreshMenus();
}

void MainWindow::slotTempoChangedBySong(int bpm)
{
    qDebug() << "slotTempoChangedBySong" << bpm;

}

MainWindow::~MainWindow(){
   if(mp_beatsModel){
      delete mp_beatsModel;
   }
   if(mp_beatsProxyModel){
      delete mp_beatsProxyModel;
   }
   if (mp_CheatShitDialog) {
      delete mp_CheatShitDialog;
   }
   //selection model deleted by model
   //delete mp_SelectionModel;
}

// Required to apply stylesheet
void MainWindow::paintEvent(QPaintEvent *)
{
    QStyleOption opt;
    opt.init(this);
    QPainter p(this);
    style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);
}

/**
 * @brief MainWindow::browseForPedal
 * @return
 *
 * Utility function used in Sync/Export/Import prj
 */
QString MainWindow::browseForPedal()
{
    // 1 - Default drive is c
    QDir destDir("c:/");

    // 2 - Select the first drive that contains a BeatBuddy project
    QProgressDialog progress(tr("Exploring Drives..."), tr("Abort"), 0, 1, this);
    progress.setWindowModality(Qt::WindowModal);
    progress.setMinimumDuration(0);

    QFileInfoList drives = QDir::drives();
    progress.setMaximum(1 + drives.count());

    for(int i = 0; i < drives.count(); i++){
        if(progress.wasCanceled()){
            return nullptr;
        }

        if(BeatsProjectModel::isProjectFolder(drives.at(i).absoluteFilePath(), false)){
            destDir = drives.at(i).absoluteDir();
            progress.setMaximum(1 + drives.count());
            break;
        }
        progress.setMaximum(1 + i);
    }

    // 3 - File dialog to determine where to export
    return QFileDialog::getExistingDirectory(
                this,
                tr("Browse for the Pedal SD location"),
                destDir.absolutePath());
}

/**
 * @brief MainWindow::promptSaveUnsavedChanges
 * @param allowDiscard
 * @return continue
 *
 * Discard can only be allowed on closing the project for now
 */
bool MainWindow::promptSaveUnsavedChanges(bool allowDiscard)
{
    if (mp_beatsModel && mp_beatsModel->isSongsFolderDirty()) {
        auto userResponse = QMessageBox::question(this, tr("Save Project"), tr("There are unsaved changes in the project.\n\nDo you want to save your changes?"), QMessageBox::Save | QMessageBox::Cancel | (allowDiscard ? QMessageBox::Discard : QMessageBox::NoButton), QMessageBox::Save);
        switch (userResponse) {
        case QMessageBox::Save:
            mp_beatsModel->saveModal();
            return true;
        case QMessageBox::Discard:
            mp_beatsModel->effectFolder()->discardAllUseChanges();
            return true;
        default:
            return false;
        }
    }
    return true;
}

/**
 * @brief MainWindow::slotNewSong
 *
 * Create new empty song in current folder
 */
void MainWindow::slotNewSong()
{
    if(!mp_beatsModel){
        return;
    }

    auto songFolderIndex = mp_BeatsPanel->rootIndex();

    int maxChildrenCnt = songFolderIndex.sibling(songFolderIndex.row(), AbstractTreeItem::MAX_CHILD_CNT).data().toInt();

    if(mp_beatsModel->rowCount(songFolderIndex) >= maxChildrenCnt){
       QMessageBox::warning(this, tr("Max Song count reached"), tr("The maximum number of songs per folder is %1").arg(maxChildrenCnt));
       return;
    }

    if(songFolderIndex.isValid()){
        auto current = mp_BeatsPanel->selectionModel()->currentIndex();
        int row = current.parent().parent() == mp_beatsModel->songsFolderIndex() ? current.row() : mp_beatsModel->rowCount(songFolderIndex);
        mp_beatsModel->createNewSong(songFolderIndex, row);
    }
}

/**
 * @brief MainWindow::slotDelete
 *
 * Deletes current song in current folder
 */
void MainWindow::slotDelete()
{
    if(!mp_beatsModel){
        return;
    }
    auto current = mp_BeatsPanel->selectionModel()->currentIndex();
    if (current.parent() == mp_beatsModel->songsFolderIndex()) {
        return slotDeleteFolder();
    } else {
        if (current.parent().parent() == mp_beatsModel->songsFolderIndex()) {
            if (!(QApplication::keyboardModifiers() & Qt::ShiftModifier) && QMessageBox::Yes != QMessageBox::question(this, tr("Delete Song"), tr("Are you sure you want to delete the song?\nNOTE You can undo the deletion up until the software is closed!")
                                                                                                          #ifndef Q_OS_MAC
                                                                                                                      + tr("\n\nTip: Hold Shift to hide this confirmation box")
                                                                                                          #endif
                                                                                                                      , QMessageBox::Yes|QMessageBox::Cancel, QMessageBox::Yes)) {
                return;
            }
            mp_beatsModel->deleteSong(current);
        } else {
            QMessageBox::warning(this, tr("Delete Song"), tr("Current selected item is neither a song nor a folder"));
        }
    }
}

/**
 * @brief MainWindow::slotImportDrm
 *
 * Import a drumset to the workspace
 */
void MainWindow::slotImportDrm(QString drm)
{
    // 1 - Browse for source file
    QString fileName = !drm.isEmpty() ? drm : QFileDialog::getOpenFileName(
                this,
                tr("Import Drum Set"),
                mp_MasterWorkspace->userLibrary()->libDrumSets()->currentPath(),
                BMFILES_DRUMSET_DIALOG_FILTER);

    if(fileName.isEmpty()){
        return;
    }

    // Remember the path for drumsets
    QFileInfo newPath(fileName);
    mp_MasterWorkspace->userLibrary()->libDrumSets()->setCurrentPath(newPath.absolutePath());

    // 2 - validate that file exists
    QFileInfo drmInfo(fileName);
    if(!drmInfo.exists()){
        QMessageBox::warning(this, tr("Import Drumset"), tr("Source file %1 does not exist").arg(drmInfo.absoluteFilePath()));
        return;
    }

    // 3 - Perform import operation in drm list model
    mp_drmListModel->importDrm(drmInfo.absoluteFilePath());



}

/**
 * @brief MainWindow::slotExportPrj
 *
 * Export to pedal
 */
void MainWindow::slotExportPrj()
{
    QDir dstPrjDir;

    if(!mp_beatsModel){
        return;
    }

    // 1 - Save unsaved changes
    if(!promptSaveUnsavedChanges(false)){
        return;
    }

    // 2 - make sure all songs are valid
    auto invalid = mp_beatsModel->songsFolder()->data(AbstractTreeItem::INVALID).toString();
    if (!invalid.isEmpty()) {
        auto parsed = QRegularExpression("^([^|]+).*?(?:|[^\\d]*(\\d+)[^|]*)?(?:|[^\\d]*(\\d+)[^|]*)?$").match(invalid);
        auto song = parsed.captured(2);
        auto folder = parsed.captured(3);
        auto faulty_song = !song.isEmpty() && !folder.isEmpty() ? mp_beatsModel->songsFolderIndex().child(QVariant(folder).toInt()-1, 0).child(QVariant(song).toInt()-1, 0) : QModelIndex();
        if (QMessageBox::Yes == QMessageBox::warning(this, tr("Export Project"),
            tr("%1:(\nAll project songs should be valid before continuing!\n---\nTo fix this error, please do one of the following:"
            "\n\tAdd missing song part"
            "\n\tUse %2 > %3 to make sure song part was not deleted by mistake"
            "\n\tRemove song part without a Main Drum Loop"
            "\n\tRemove the invalid song altogether").arg(parsed.captured(1)).arg(mp_edit->title().replace("&", "")).arg(mp_Undo->text().replace("&", ""))
            + (faulty_song == QModelIndex() ? "" : tr("\n\nShow the faulty song now?")),
            faulty_song == QModelIndex() ? QMessageBox::Ok : QMessageBox::Yes|QMessageBox::Cancel)) {
                mp_BeatsPanel->selectionModel()->select(faulty_song, QItemSelectionModel::Clear | QItemSelectionModel::SelectCurrent);
                mp_BeatsPanel->selectionModel()->setCurrentIndex(faulty_song, QItemSelectionModel::SelectCurrent);
        }
        return;
    }

    // 3 - Determine if it is posible to export to last location
    bool useLastLocation = false;
    if (Settings::lastSdCardDirExists()){
        dstPrjDir = QDir(Settings::getLastSdCardDir());
        if(dstPrjDir.exists()){
            if(BeatsProjectModel::isProjectFolder(dstPrjDir.absolutePath(), false)){
                if (QMessageBox::Yes == QMessageBox::question(this, tr("Export Project"), tr("Use saved location to write project to\n%1?").arg(dstPrjDir.absolutePath()))){
                    useLastLocation = true;
                }
            }
        }
    }

    // 4 - Determine export dst location if required
    if(!useLastLocation){
        QString dstProjectPath = browseForPedal();
        if(dstProjectPath.isEmpty()){
            return;
        }
        dstPrjDir = QDir(dstProjectPath);

        // go to the root
#ifdef Q_OS_MAC
        for (auto parent = dstPrjDir; parent.absolutePath().compare("/Volumes", Qt::CaseInsensitive);){
            dstPrjDir = parent;
            if (!parent.cdUp()) {
                dstPrjDir = QDir(dstProjectPath);
                break;
            }
        }
#else
        while (dstPrjDir.cdUp());
#endif
        if (dstPrjDir.absolutePath() != dstProjectPath){
            auto choice = QMessageBox::question(this, tr("Export Project"), tr("Project should be exported to the top of SD card in order for your pedal to work.\n\nChoose No  to export to %1 at your own risk.\nChoose Yes to export to %2 instead.").arg(dstProjectPath).arg(dstPrjDir.absolutePath()), QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);
            switch (choice) {
            case QMessageBox::Cancel:
                return;
            case QMessageBox::No:
                dstPrjDir = QDir(dstProjectPath);
                break;
            default:
                break;
            }
        }

        if(!dstPrjDir.exists()){
            QMessageBox::critical(this, tr("Export Project"), tr("Destination %1 not found").arg(dstPrjDir.absolutePath()));
            return;
        }
        const QString sdCardDirKey = mp_MasterWorkspace->userLibrary()->libProjects()->key() + "/sd_card_dir";
        Settings::setLastSdCardDir(dstPrjDir.absolutePath());

    }

    // 5 - Verify if clean up is necessary and warn user if so
    bool containsProjectEntry = BeatsProjectModel::containsAnyProjectEntry(dstPrjDir.absolutePath());
    if(containsProjectEntry){
        if (QMessageBox::Cancel == QMessageBox::question(this, tr("Export Project"), tr("Chosen destination already contains a project.\n\nDo you want to overwrite it?"), QMessageBox::Yes | QMessageBox::Cancel)) {
            return;
        }
    }

    // 6 - List files to clean up
    QStringList pathToCleanUp;
    if (containsProjectEntry){

        qDebug() << "BeatsProjectModel::existingProjectEntriesForPath(dstPrjDir.absolutePath()) = ";
        foreach(const QString & path, BeatsProjectModel::existingProjectEntriesForPath(dstPrjDir.absolutePath(), true, true)){
            qDebug() << "   " << path;
        }


        DirListAllSubFilesModal listCleanUpFiles(BeatsProjectModel::existingProjectEntriesForPath(dstPrjDir.absolutePath(), true, true), DirListAllSubFilesModal::rootLast, this);
        bool success = listCleanUpFiles.run();
        if(!success){
            QMessageBox::information(this, tr("Export Project"), tr("Aborting by user request"));
            return;
        }

        pathToCleanUp = listCleanUpFiles.result();


        qDebug() << "pathToCleanUp = ";
        foreach(const QString & path, pathToCleanUp){
            qDebug() << "   " << path;
        }
    }

    // 7 - Delete files to clean up
    if(!pathToCleanUp.isEmpty()){
        DirCleanUpModal cleanUpFiles(pathToCleanUp, this);
        bool success = cleanUpFiles.run();
        if(!success){
            QMessageBox::information(this, tr("Export Project"), tr("Aborting by user request"));
            return;
        }
    }

    // 8 - List Files to copy
    QStringList pathToCopy;
    qint64 fileSizeToCopy = 0;
    foreach(QFileInfo pathToCopyFI, mp_beatsModel->projectEntriesInfo()){
        pathToCopy.append(pathToCopyFI.absoluteFilePath());
    }

    if (!pathToCopy.isEmpty()){
        DirListAllSubFilesModal listCopyFiles(pathToCopy, DirListAllSubFilesModal::rootFirst, this);
        bool success = listCopyFiles.run(true); //true is set to ignore FOOTSW.INI file
        if(!success){
            QMessageBox::information(this, tr("Export Project"), tr("Aborting by user request"));
            return;
        }

        pathToCopy = listCopyFiles.result();


        qDebug() << "pathToCopy = ";
        foreach(const QString & path, pathToCopy){
            qDebug() << "   " << path;
        }
        fileSizeToCopy = listCopyFiles.totalFileSize();
    }

    // 9 - Copy files
    if(!pathToCopy.isEmpty()){
        DirCopyModal copyFiles(dstPrjDir.absolutePath(), mp_beatsModel->projectDirFI().absoluteFilePath(), pathToCopy, this);
        copyFiles.setProgressType(DirCopyModal::ProgressFileSize);
        copyFiles.setTotalSize(fileSizeToCopy);
        bool success = copyFiles.run();
        if(!success){
            QMessageBox::information(this, tr("Export Project"), tr("Aborting by user request"));
            return;
        }
    }

    // 10 - Link
    if (QMessageBox::Yes != QMessageBox::question(this, tr("Export Project"), tr("Do you want to link the exported project for future synchronization?"))) {
        return;
    }

    if(!mp_beatsModel->linkProject(dstPrjDir.absolutePath())){
        QMessageBox::warning(this, tr("Export Project"), tr("Unexpected error occured while linking project.\nProject synchronization will not be available yet.\nUse %1 > %2 > %3 instead")
            .arg(mp_fileMenu->title().replace("&", ""), mp_export->title().replace("&", ""), mp_exportPrj->text().replace("&", "")));
    }
    // 11 - Enable Sync operation if linked
    refreshMenus();
}

/**
 * @brief MainWindow::slotNewPrj
 *
 * Create new project
 */
void MainWindow::slotNewPrj()
{
    // 1 - Save unsaved changes
    if(!promptSaveUnsavedChanges(true)){
        return;
    }

    // 2 - Browse to find destination
 /*   Workspace w;
    NewProjectDialog dialog(this,w.userLibrary()->libProjects()->currentPath());
    dialog.exec();
    if (dialog.result() != dialog.Accepted){
        return;
    }
    */
    qDebug() << "new project dir:" << mp_MasterWorkspace->userLibrary()->libProjects()->currentPath()
             << "wkspdir" << mp_MasterWorkspace->dir().absolutePath() << "userlib" << mp_MasterWorkspace->userLibrary()->dir().absolutePath()
             << "libproj" << mp_MasterWorkspace->userLibrary()->libProjects()->dir().absolutePath();
    // 2 - Browse to find destination
    QString destFileName = QFileDialog::getSaveFileName(
             this,
             tr("Create new project - set project name"),
             //mp_MasterWorkspace->userLibrary()->libProjects()->currentPath(), lets put them in the workspace!
             mp_MasterWorkspace->userLibrary()->libProjects()->dir().absolutePath(),
             BMFILES_PROJECT_DIALOG_FILTER);

    qDebug() << "creating project here:" << destFileName;

    if (destFileName.isEmpty()){
        return;
    }

    QFileInfo selectedProjectFileFI(destFileName);

    // 4 - Create project folder from the file name specified
    QFileInfo realProjectFileFI = BeatsProjectModel::createProjectFolderForProjectFile(selectedProjectFileFI);
    QFileInfo realProjectFolderFI(realProjectFileFI.absolutePath());
    // Set to the folder's parent
    mp_MasterWorkspace->userLibrary()->libProjects()->setCurrentPath(realProjectFolderFI.absolutePath());

    createBeatsModels(realProjectFileFI.absoluteFilePath(), true);

    refreshMenus();

}

class FileDialog : public QFileDialog
{
public:
    explicit FileDialog(QWidget* parent = nullptr, const QString& caption = QString(), const QString& directory = QString(), const QString& filter = QString())
        : QFileDialog(parent, caption, directory, filter)
    {
        connect(this, &QFileDialog::directoryEntered, this, [this](const QString& dir) { m_entered = dir; });
    }

    QStringList selectedFiles() const
    {
        auto ret = QFileDialog::selectedFiles();
        QDir entered(m_entered);
        for (auto it = ret.begin(); it != ret.end(); ++it) {
            QFileInfo cur(*it);
            if (cur.absoluteDir().absolutePath() != entered.absolutePath()) {
                *it = entered.absoluteFilePath(cur.fileName());
            }
        }
        return ret;
    }

private:
    QString m_entered;
};

/**
 * @brief MainWindow::slotOpenPrj
 *
 * Open existing project
 */
bool MainWindow::slotOpenPrj(const QString& default_path)
{
    m_done = false;
    ScopeGuard _([this]() { m_done ? QApplication::quit() : (void)(m_done = true); });
    // 1 - Save unsaved changes
    if(!promptSaveUnsavedChanges(true)){
        return false;
    }
    
    // 2 - Browse to find destination
    QString projectFileName;
    if (!default_path.isEmpty()) {
        projectFileName = default_path;
    } else {
        QString file;
        QFileDialog dlg(
                    this,
                    tr("Open Project"),
                    nullptr,
                    BMFILES_OPEN_PROJECT_DIALOG_FILTER);
        dlg.setAcceptMode(QFileDialog::AcceptOpen);
        QTimer t;
        connect(&dlg, &FileDialog::directoryEntered, [&t]() {
            t.start();
        });
        t.setSingleShot(true);
        t.setInterval(200);
        connect(&t, &QTimer::timeout, [&dlg, &file](){
            auto dir = dlg.directory();
            auto name = dir.absolutePath();
            if (!BeatsProjectModel::isProjectFolder(name)) {
                return;
            }

#ifdef Q_OS_OSX // FIXME Mac hangs without this dummy messagebox
            QMessageBox mac_sucks_ass_hard(QMessageBox::Icon::Information, tr("Loading Project"), tr("BBManager is preparing your Mac to load the project...\n\nPress Ok to continue"), QMessageBox::Ok, &dlg);
            QTimer t;
            t.setSingleShot(true);
            mac_sucks_ass_hard.connect(&t, &QTimer::timeout, &mac_sucks_ass_hard, &QMessageBox::close);
            t.start(200);
            mac_sucks_ass_hard.exec();
#endif
            // pick any BBP file, or come up with a fake one
            foreach (auto x, dir.entryList(QDir::nameFiltersFromString(BMFILES_PROJECT_FILTER))) {
                file = dir.absoluteFilePath(x);
                dlg.selectUrl(QUrl::fromLocalFile(x));
                dlg.close();
                return;
            }
            auto bbp = (dir.isRoot() ? "project" : dir.dirName()) + "." BMFILES_PROJECT_EXTENSION;
            file = dir.absoluteFilePath(bbp);
            dlg.setAcceptMode(QFileDialog::AcceptSave);
            dlg.selectUrl(QUrl::fromLocalFile(bbp));
            dlg.close();
        });
        dlg.setDirectory(mp_MasterWorkspace->userLibrary()->libProjects()->currentPath());
        auto res = dlg.exec();
        t.stop();
        if (!file.isEmpty()) {
            projectFileName = file;
        } else if (res){
            foreach (auto file, dlg.selectedFiles()) {
                projectFileName = file;
                break;
            }
        }
    }
    
    if(projectFileName.isEmpty()){
        return false;
    } else if (projectFileName.endsWith("." BMFILES_SINGLE_FILE_EXTENSION)) {
        return slotImportPrjArchive(projectFileName);
    }

    QFileInfo projectFileFI(projectFileName);
    if (projectFileFI.isDir()) { // if you select dir, and click "Open" we need to search for BBP file
        QDir dir(projectFileName);
        QStringList projectFileFilters;
        projectFileFilters << BMFILES_PROJECT_FILTER;
        QStringList projectFiles = dir.entryList(projectFileFilters, QDir::Files | QDir::Hidden);
        if (projectFiles.count() != 1){
            qDebug() << "Project folder appears to have no files - or more than one.";
            qDebug() << "Path: " << projectFileName;
            qDebug() << "projectFiles.count() = " << projectFiles.count();
            return false;
        }
        projectFileFI = QFileInfo(dir.absoluteFilePath(projectFiles.first()));
        projectFileName = projectFileFI.absoluteFilePath();
    }
    QFileInfo projectFolderFI(projectFileFI.absolutePath());

#ifdef Q_OS_MAC
    auto save = !QFileInfo(projectFolderFI).dir().absolutePath().compare("/Volumes", Qt::CaseInsensitive);
#else
    auto save = QDir::drives().indexOf(projectFolderFI)+1;
#endif
    if (!save) {
        // Set to the folder's parent
        mp_MasterWorkspace->userLibrary()->libProjects()->setCurrentPath(projectFolderFI.absolutePath());
    }

    // 3 - Validate that the folder contains the base project directories
    if(projectFileFI.suffix().compare(BMFILES_PROJECT_EXTENSION, Qt::CaseInsensitive) != 0 || (projectFileFI.exists() && !projectFileFI.isFile())){
        QMessageBox::critical(this, tr("Open Project"), tr("%1 is not a valid project file").arg(projectFileFI.absoluteFilePath()));
        return false;
    }

    if(!BeatsProjectModel::isProjectFolder(projectFileFI.absoluteDir().absolutePath())){
        //QMessageBox::warning(this, tr("Open Project"), tr("Invalid project file location:\n%1 is not in a proper location.\nMissing required files and/or folders").arg(projectFileFI.absoluteFilePath()));
        // Last project file is no longer there--moved or deleted?  Don't just give an error, let them create a new one.
        qDebug() << tr("Invalid project file location:\n%1 is not in a proper location.\nMissing required files and/or folders").arg(projectFileFI.absoluteFilePath());
        //return false;
        slotNewPrj();
        return true;
    }

    // 4 - refresh change model
    createBeatsModels(projectFileName);
    refreshMenus();
    qDebug() <<mp_MasterWorkspace->userLibrary()->libProjects()->dir().absolutePath()<<mp_MasterWorkspace->userLibrary()->libProjects()->currentPath();
    if (save && QMessageBox::Yes == QMessageBox::question(this, tr("Open Project"), tr("It looks like you are loading a project directly from your SD card.\n\n"
                                                                                       "We highly recommend to save a copy of it on your computer - as this will not only serve as backup in case your SD card gets lost or damaged, "
                                                                                       "but having a copy will also help you work more efficiently in the future, and you will be able to create backup SD cards to keep with you when travelling.\n\n"
                                                                                       "Save a copy of your project now?")/*.arg(mp_fileMenu->title().replace("&", "")).arg(mp_savePrjAs->text().replace("&", ""))*/)) {
        slotSavePrjAs();
    }else if(mp_MasterWorkspace->userLibrary()->libProjects()->dir().absolutePath() != mp_MasterWorkspace->userLibrary()->libProjects()->currentPath() && QMessageBox::Yes == QMessageBox::question(this, tr("Open Project"), tr("It looks like you are loading a project from a personal directory.\n\n"
                                                                                       "We highly recommend to save a copy of it in the workspace (default) directory."
                                                                                       "It will help you work more efficiently in the future.\n\n"
                                                                                       "Save a copy of your project now?"))){
        slotSavePrjAs();
    }
    return true;
}

/**
 * @brief MainWindow::slotSavePrj
 * Save project under existing name
 */
void MainWindow::slotSavePrj()
{
    if(!mp_beatsModel){
        return;
    }
    mp_beatsModel->saveModal();
    setClean();

    refreshMenus();
}

/**
 * @brief MainWindow::slotSavePrjAs
 *
 * Save project under new name
 */
void MainWindow::slotSavePrjAs()
{
   if(!mp_beatsModel){
      return;
   }

   // 1 - Browse to find destination

   // Replace inappropriate symbols with a sad smilie in hope for happy future
   QString defaultFileName = mp_beatsModel->projectFileFI().fileName().replace(QRegularExpression("[ \\\"/<|>:*_?]+"), "_");
   // remove extension
   defaultFileName = QFileInfo(defaultFileName).baseName();
   QDir defaultDir(mp_MasterWorkspace->userLibrary()->libProjects()->dir().absolutePath());
   QFileInfo defaultFI(defaultDir.absoluteFilePath(defaultFileName));

    QFileDialog dlg(
        this,
        tr("Save Project As"),
        nullptr,
        BMFILES_SAVE_PROJECT_DIALOG_FILTER);
    dlg.setAcceptMode(QFileDialog::AcceptSave);
    dlg.setDirectory(mp_MasterWorkspace->userLibrary()->libProjects()->dir().absolutePath());
    QString last_dir = defaultFI.absolutePath();
    connect(&dlg, &FileDialog::directoryEntered, [&dlg, &last_dir](const QString& name) {
        if (!BeatsProjectModel::isProjectFolder(name)) {
            last_dir = name;
        } else {
            dlg.setDirectory(last_dir);
            QToolTip::showText(QCursor::pos(), tr("Chosen location already contains a BeatBuddy project:\n%1\n\nOne does not simply save a project inside of a project!\n\nPlease select another location.").arg(name), &dlg);
        }
    });
    QString destFileName;
    if (dlg.exec()) foreach (auto file, dlg.selectedFiles()) {
        destFileName = file;
        break;
    }

   if(destFileName.isEmpty()){
      return;
   } else if (destFileName.endsWith("." BMFILES_SINGLE_FILE_EXTENSION)) {
       return slotExportPrjArchive(destFileName);
   }


   QFileInfo selectedDstPrjFileFI(destFileName);

   // 4 - Create project folder from the file name specified
   QFileInfo realDstPrjFileFI = BeatsProjectModel::createProjectFolderForProjectFile(selectedDstPrjFileFI);
   QFileInfo realDstPrjFolderFI(realDstPrjFileFI.absolutePath());
   // Set to the folder's parent
   mp_MasterWorkspace->userLibrary()->libProjects()->setCurrentPath(realDstPrjFolderFI.absolutePath());

   // if saveAsModal succeed
   if (!mp_beatsModel->saveAsModal(realDstPrjFileFI, this)){
      qWarning() << "MainWindow::slotSavePrjAs - ERROR - saveAsModal failed/aborted";
      return;
   }

   // 5 - Open new project
   auto srcPrjDir = mp_beatsModel->projectFileFI().absolutePath();
   createBeatsModels(realDstPrjFileFI.absoluteFilePath());

    // 6 - Set clean mark
    setClean();

    // 12 - Link
    if (QMessageBox::Yes != QMessageBox::question(this, tr("Save Project As"), tr("Do you want to link the new project for future synchronization?"))) {
        return;
    }

    if(!mp_beatsModel->linkProject(srcPrjDir)){
        QMessageBox::warning(this, tr("Save Project As"), tr("Unexpected error occured while linking project.\nProject synchronization will not be available yet.\nUse %1 > %2 > %3 instead")
            .arg(mp_fileMenu->title().replace("&", ""), mp_export->title().replace("&", ""), mp_exportPrj->text().replace("&", "")));
    }
    // 13 - Enable Sync operation if linked
    refreshMenus();
}

/**
 * @brief MainWindow::slotImportPrjArchive
 * @param default_path Optional
 *
 * When called with `path', open specified single file project. Called by constructor
 * Otherwise, browse for path
 */
bool MainWindow::slotImportPrjArchive(const QString& path){
   // 1 - Save unsaved changes
   if(!promptSaveUnsavedChanges(true)){
      return false;
   }

   QString archiveFileName;

   if (path.isEmpty()){

      // 2 - Browse to find destination
      archiveFileName = QFileDialog::getOpenFileName(
               this,
               tr("Select Single File Project to Open"),
               mp_MasterWorkspace->userLibrary()->libProjects()->currentPath(),
               BMFILES_SINGLE_FILE_DIALOG_FILTER);
      if(archiveFileName.isEmpty()){
         return false;
      }
   } else {
      // Dialog to explain what is happening on original project importation
      if (Settings::lastProjectExists() && path == Settings::getLastProject() &&
              QMessageBox::No == QMessageBox::question(this, tr("Import Single File Project"), tr("Do you want to import default project?"))) {
          return false;
      }
      archiveFileName = path;
   }

   QFileInfo archiveFI(archiveFileName);

   if(!archiveFI.exists()){
      QMessageBox::critical(this, tr("Import Single File Project"), tr("An error occured while opening %1").arg(archiveFI.absoluteFilePath()));
   }

   // 3 - Validate that the folder contains the base project directories
   if(archiveFI.suffix().compare(BMFILES_SINGLE_FILE_EXTENSION, Qt::CaseInsensitive) != 0
           || !archiveFI.isFile()
           || !BeatsProjectModel::isProjectArchiveFile(archiveFI.absoluteFilePath())){
      QMessageBox::critical(this, tr("Import Single File Project"), tr("%1 is not a valid project file").arg(archiveFI.absoluteFilePath()));
      return false;
   }

   // 4 - Browse to select where imported project is going to be saved
   QString exportProjectFilePath = QFileDialog::getSaveFileName(this, tr("Select where to save imported project"),
            QDir(mp_MasterWorkspace->userLibrary()->libProjects()->currentPath()).absoluteFilePath(archiveFI.completeBaseName()),
            BMFILES_PROJECT_DIALOG_FILTER);

   if (exportProjectFilePath.isEmpty()){
      // Empty means operation was cancelled
      return false;
   }

   QFileInfo selectedProjectFileFI(exportProjectFilePath);

   // 5 - Create project folder from the file name specified
   QFileInfo realProjectFileFI = BeatsProjectModel::createProjectFolderForProjectFile(selectedProjectFileFI);
   QFileInfo realProjectFolderFI(realProjectFileFI.absolutePath());
   // Set to the folder's parent
   mp_MasterWorkspace->userLibrary()->libProjects()->setCurrentPath(realProjectFolderFI.absolutePath());

   // 6 - Extract the zip archive
   QuaZip archive(archiveFI.absoluteFilePath());
   QStringList files;
   if(archive.open(QuaZip::mdUnzip)){
      files = archive.getFileNameList();
      archive.close();
   }

   ExtractZipModal extractor(realProjectFolderFI.absoluteFilePath(), archiveFI.absoluteFilePath(), files, this);
   extractor.setProgressType(ExtractZipModal::ProgressFileSize);
   if(!extractor.run()){
      qWarning() << "MainWindow::slotImportPrjArchive - WARNING - !extractor.run()";
      return false;
   }

   // 7 - make sure the project ID is changed to avoid multiple links (use parent folder of infoMapFile) and archive
   if(!BeatsProjectModel::resetProjectInfo(realProjectFolderFI.absoluteFilePath())){
      QMessageBox::warning(this, tr("Import Single File Project"), tr("An error occured while resetting synchronization link info for the project"));
   }

   // 8 - Rename project file / update info or create
   QStringList projectFileFilter;
   projectFileFilter << BMFILES_PROJECT_FILTER;
   QStringList extractedProjectFiles = QDir(realProjectFolderFI.absoluteFilePath()).entryList(projectFileFilter, QDir::Files);
   // If a project file was copied
   if(!extractedProjectFiles.isEmpty()){
      QFile extractedProjectFile(QDir(realProjectFolderFI.absoluteFilePath()).absoluteFilePath(extractedProjectFiles.first()));
      if (extractedProjectFile.fileName() == realProjectFileFI.absoluteFilePath()); // WTF - error while renaming file to itself?!
      // try more than once in case os has a lock on file
      else while(!extractedProjectFile.rename(realProjectFileFI.absoluteFilePath())){
          if (QMessageBox::Abort == QMessageBox::warning(this, tr("Import Single File Project"), tr("Unable to rename project file\n%1\nto\n%2").arg(QFileInfo(extractedProjectFile).absoluteFilePath()).arg(realProjectFileFI.absoluteFilePath()), QMessageBox::Retry | QMessageBox::Abort, QMessageBox::Retry)) {
              break;
          }
      }
      // Refresh the file with new info...
      BeatsProjectModel::saveProjectFile(realProjectFileFI.absoluteFilePath());
   } else {
      // create from scratch
      BeatsProjectModel::saveProjectFile(realProjectFileFI.absoluteFilePath());
   }
   // 9 - refresh change model
   createBeatsModels(realProjectFileFI.absoluteFilePath());
   refreshMenus();
   return true;
}

/**
 * @brief MainWindow::slotSavePrj
 * Save project under existing name
 */
void MainWindow::slotExportPrjArchive(const QString& path)
{
    if(!mp_beatsModel){
        return;
    }

    // 1 - Browse to find destination

    // Replace inappropriate symbols with a sad smilie in hope for happy future
    QString defaultFileName = mp_beatsModel->projectFileFI().fileName().replace(QRegularExpression("[ \\\"/<|>:*_?]+"), "_");
    // remove extension
    defaultFileName = QFileInfo(defaultFileName).baseName();
    QDir defaultDir(mp_MasterWorkspace->userLibrary()->libProjects()->currentPath());
    QFileInfo defaultFI(defaultDir.absoluteFilePath(defaultFileName));

    QString destFileName = !path.isEmpty() ? path : QFileDialog::getSaveFileName(
             this,
             tr("Select Single File Project Destination"),
             defaultFI.absoluteFilePath(),
             BMFILES_SINGLE_FILE_DIALOG_FILTER);

    if(destFileName.isEmpty()){
       return;
    }
    QFileInfo destPrjFI(destFileName);

    // 2 - Perform save as operation
    mp_beatsModel->saveProjectArchive(destPrjFI.absoluteFilePath(), this);

    // 3 - refreshMenus
    refreshMenus();

    // 4 - Set clean mark
    setClean();
}

void MainWindow::slotSyncPrj()
{
    QDir dstPrjDir;

    if(!mp_beatsModel){
        qWarning() << "Tried to sync with no BeatsProjectModel";
        return;
    }

    // 1 - Save unsaved changes
    if(!promptSaveUnsavedChanges(false)){
        return;
    }

    // 2 - make sure all songs are valid
    auto invalid = mp_beatsModel->songsFolder()->data(AbstractTreeItem::INVALID).toString();
    if (!invalid.isEmpty()) {
        auto parsed = QRegularExpression("^([^|]+).*?(?:|[^\\d]*(\\d+)[^|]*)?(?:|[^\\d]*(\\d+)[^|]*)?$").match(invalid);
        auto song = parsed.captured(2);
        auto folder = parsed.captured(3);
        auto faulty_song = !song.isEmpty() && !folder.isEmpty() ? mp_beatsModel->songsFolderIndex().child(QVariant(folder).toInt()-1, 0).child(QVariant(song).toInt()-1, 0) : QModelIndex();
        if (QMessageBox::Yes == QMessageBox::warning(this, tr("Synchronize Project"),
            tr("%1:(\nAll project songs should be valid before continuing!\n---\nTo fix this error, please do one of the following:"
            "\n\tAdd missing song part"
            "\n\tUse %2 > %3 to make sure song part was not deleted by mistake"
            "\n\tRemove song part without a Main Drum Loop"
            "\n\tRemove the invalid song altogether").arg(parsed.captured(1)).arg(mp_edit->title().replace("&", "")).arg(mp_Undo->text().replace("&", ""))
            + (faulty_song == QModelIndex() ? "" : tr("\n\nShow the faulty song now?")),
            faulty_song == QModelIndex() ? QMessageBox::Ok : QMessageBox::Yes|QMessageBox::Cancel)) {
                mp_BeatsPanel->selectionModel()->select(faulty_song, QItemSelectionModel::Clear | QItemSelectionModel::SelectCurrent);
                mp_BeatsPanel->selectionModel()->setCurrentIndex(faulty_song, QItemSelectionModel::SelectCurrent);
        }
        qDebug() << "At least one song is invalid.";
        return;
    }
    // 3 - Determine if it is possible to import from last location
    bool useLastLocation = false;
    if (Settings::lastSdCardDirExists()){
        auto lastSdCardDir = Settings::getLastSdCardDir();
        dstPrjDir = QDir(lastSdCardDir);
        qDebug() << "Last SD Card Dir: " << lastSdCardDir;
        if(dstPrjDir.exists()){
            qDebug() << "Last SD Card Dir exists";
            if(BeatsProjectModel::isProjectFolder(dstPrjDir.absolutePath(), false)){
                if (mp_beatsModel->isProject3WayLinked(dstPrjDir.absolutePath()) &&
                    QMessageBox::Yes == QMessageBox::question(this, tr("Synchronize Project"), tr("Synchronize project to %1?").arg(dstPrjDir.absolutePath()))) {
                        qDebug() << "Project is 3 way linked.";
                        useLastLocation = true;
                }
            }
        }
    }

    // 4 - Determine sync dst location if required
    if(!useLastLocation){
        qDebug() << "Can't use a 'last location'.";
        QString dstProjectPath = browseForPedal();
        if(dstProjectPath.isEmpty()){
            return;
        }
        dstPrjDir = QDir(dstProjectPath);

        if(!dstPrjDir.exists()){
            QMessageBox::critical(this, tr("Synchronize Project"), tr("Destination %1 not found").arg(dstPrjDir.absolutePath()));
            return;
        }

        if(!mp_beatsModel->isProject3WayLinked(dstPrjDir.absolutePath())){
            QMessageBox::warning(this, tr("Synchronize Project"), tr("Destination project at %1 is not linked.\n\nUse %1 > %2 > %3 instead")
                .arg(mp_fileMenu->title().replace("&", ""), mp_export->title().replace("&", ""), mp_exportPrj->text().replace("&", "")));
            return;
        }
        const QString sdCardDirKey = mp_MasterWorkspace->userLibrary()->libProjects()->key() + "/sd_card_dir";

        Settings::setLastSdCardDir(dstPrjDir.absolutePath());
    }

    // 5 - Perform operation in project
    mp_beatsModel->synchronizeModal(dstPrjDir.absolutePath(), this);

}

/**
 * @brief MainWindow::slotPrev
 *
 * Selects previous song in a folder
 */
void MainWindow::slotPrev()
{
    auto sel = mp_BeatsPanel->selectionModel()->currentIndex();
    if (!sel.row() && sel.parent().parent() == mp_beatsModel->songsFolderIndex()) {
        sel = sel.parent();
    } else {
        sel = sel.sibling(sel.row()-1, 0);
    }
    mp_BeatsPanel->selectionModel()->select(sel, QItemSelectionModel::Clear | QItemSelectionModel::SelectCurrent);
    mp_BeatsPanel->selectionModel()->setCurrentIndex(sel, QItemSelectionModel::SelectCurrent);
}

/**
 * @brief MainWindow::slotNext
 *
 * Selects next song in a folder
 */
void MainWindow::slotNext()
{
    auto sel = mp_BeatsPanel->selectionModel()->currentIndex();
    auto nx = sel.sibling(sel.row()+1, 0);
    if (nx == QModelIndex() && sel.parent().parent() == mp_beatsModel->songsFolderIndex()) {
        nx = sel.parent();
        nx = nx.sibling(nx.row()+1, 0);
    }
    sel = nx;
    if (sel == QModelIndex()) {
        return;
    }
    mp_BeatsPanel->selectionModel()->select(sel, QItemSelectionModel::Clear | QItemSelectionModel::SelectCurrent);
    mp_BeatsPanel->selectionModel()->setCurrentIndex(sel, QItemSelectionModel::SelectCurrent);
}

/**
 * @brief MainWindow::slotParent
 *
 * Selects a folder by file
 */
void MainWindow::slotParent()
{
    auto sel = mp_BeatsPanel->selectionModel()->currentIndex();
    if (sel.parent().parent() != mp_beatsModel->songsFolderIndex()) {
        return;
    }
    sel = sel.parent();
    mp_BeatsPanel->selectionModel()->select(sel, QItemSelectionModel::Clear | QItemSelectionModel::SelectCurrent);
    mp_BeatsPanel->selectionModel()->setCurrentIndex(sel, QItemSelectionModel::SelectCurrent);
}

/**
 * @brief MainWindow::slotPrev
 *
 * Selects previous song in a folder
 */
void MainWindow::slotChild()
{
    auto sel = mp_BeatsPanel->selectionModel()->currentIndex();
    if (sel.parent() != mp_beatsModel->songsFolderIndex()) {
        return;
    }
    sel = sel.child(0,0);
    if (sel == QModelIndex()) {
        return;
    }
    mp_BeatsPanel->selectionModel()->select(sel, QItemSelectionModel::Clear | QItemSelectionModel::SelectCurrent);
    mp_BeatsPanel->selectionModel()->setCurrentIndex(sel, QItemSelectionModel::SelectCurrent);
}


/**
 * @brief MainWindow::slotMoveUp
 *
 * Move song/folder up
 */
void MainWindow::slotMoveUp()
{
    auto current = mp_BeatsPanel->selectionModel()->currentIndex();
    if (current.parent() != mp_beatsModel->songsFolderIndex() && current.parent().parent() != mp_beatsModel->songsFolderIndex()) {
        QMessageBox::warning(this, tr("Not a song/folder"), tr("Current selected item is neither a song nor a folder"));
    }
    mp_beatsModel->moveItem(current, -1);
}

/**
 * @brief MainWindow::slotMoveDown
 *
 * Move song/folder down
 */
void MainWindow::slotMoveDown()
{
    auto current = mp_BeatsPanel->selectionModel()->currentIndex();
    if (current.parent() != mp_beatsModel->songsFolderIndex() && current.parent().parent() != mp_beatsModel->songsFolderIndex()) {
        QMessageBox::warning(this, tr("Not a song/folder"), tr("Current selected item is neither a song nor a folder"));
    }
    mp_beatsModel->moveItem(current, 1);
}

/**
 * @brief MainWindow::slotMoveFolderUp
 *
 * Move song folder up in Songs folder
 */
void MainWindow::slotMoveFolderUp()
{
    auto current = mp_BeatsPanel->selectionModel()->currentIndex();
    if (current.parent() == mp_beatsModel->songsFolderIndex()) {
    } else if (current.parent().parent() == mp_beatsModel->songsFolderIndex()) {
        current = current.parent();
    } else {
        QMessageBox::warning(this, tr("Not a song/folder"), tr("Current selected item is neither a song nor a folder"));
    }
    mp_beatsModel->moveItem(current, -1);
}

/**
 * @brief MainWindow::slotMoveFolderDown
 *
 * Move song folder up in Songs folder
 */
void MainWindow::slotMoveFolderDown()
{
    auto current = mp_BeatsPanel->selectionModel()->currentIndex();
    if (current.parent() == mp_beatsModel->songsFolderIndex()) {
    } else if (current.parent().parent() == mp_beatsModel->songsFolderIndex()) {
        current = current.parent();
    } else {
        QMessageBox::warning(this, tr("Not a song/folder"), tr("Current selected item is neither a song nor a folder"));
    }
    mp_beatsModel->moveItem(current, 1);
}

/**
 * @brief MainWindow::slotToggleMidiId
 *
 * Toggle whether MIDI ID is seen
 */
void MainWindow::slotToggleMidiId()
{
    Settings::toggleMidiId();
    qDebug() << "use midi id:" << Settings::midiIdEnabled();
    if (Settings::midiIdEnabled()) {
        mp_toggleMidiId->setText(tr("Disable Song MIDI ID"));
        mp_toggleMidiId->setStatusTip(tr("Disable Song MIDI ID"));
        mp_toggleMidiId->setToolTip(tr("Disable Song MIDI ID"));
    } else {
        mp_toggleMidiId->setText(tr("Enable Song MIDI ID"));
        mp_toggleMidiId->setStatusTip(tr("Enable Song MIDI ID"));
        mp_toggleMidiId->setToolTip(tr("Enable Song MIDI ID"));

        // The situation now is that the default for the repurposed field (loopSong) was 1.
        // We should detect this situation at least once, and change them all to 0
        // Set all midi ids to 0.  The old default is 1, which would be awkward.
        SongsFolderTreeItem *songsFolder = mp_beatsModel->songsFolder();
        songsFolder->initMidiIds();
    }
    qDebug() << QModelIndex() << QModelIndexList();

    mp_ProjectExplorerPanel->setMidiColumn();

    slotParent(); // This causes the beat panel to be redrawn with the new setting.
    slotNext();   // If the selection is the first folder, or IN the first folder, no redraw happens
    slotPrev();   // so this forces that to happen


}

/**
 * @brief MainWindow::slotNewFolder
 *
 * Create new song folder in SongS folder
 */
void MainWindow::slotNewFolder()
{
    auto current = mp_BeatsPanel->selectionModel()->currentIndex();
    int row = current.parent().parent() == mp_beatsModel->songsFolderIndex() ? current.parent().row() :
        current.parent() == mp_beatsModel->songsFolderIndex() ? current.row() : mp_beatsModel->rowCount(mp_beatsModel->songsFolderIndex());

    QModelIndex index = mp_beatsModel->createNewSongFolder(row);
    if(index.isValid()){
        if(!mp_BeatsPanel->rootIndex().isValid()) {
            mp_BeatsPanel->setRootIndex(index);
        }
    } else {
        qWarning() << "MainWindow::slotNewFolder - ERROR 1 - index invalid";
    }

}

/**
 * @brief MainWindow::slotDeleteFolder
 *
 * Delete song folder and all its content
 */
void MainWindow::slotDeleteFolder()
{
    QPersistentModelIndex currentFolder = mp_BeatsPanel->rootIndex();
    if(!currentFolder.isValid() || !currentFolder.parent().isValid() || currentFolder.parent() != mp_beatsModel->songsFolderIndex()){
        QMessageBox::warning(
                    this,
                    tr("No folder selected"),
                    tr("No folder selected that can be deleted"));
        return;
    }

    // if rootIndex contains songs AND no Shift is held, ask for confirmation
    if (mp_beatsModel->rowCount(currentFolder) && !(QApplication::keyboardModifiers() & Qt::ShiftModifier) &&
        QMessageBox::Yes != QMessageBox::question(this, tr("Delete Folder"), tr("Are you sure you want to delete the folder with all of its content?\nNOTE You can undo the deletion up until the software is closed!")
#ifndef Q_OS_MAC
            + tr("\n\nTip: Hold Shift to hide this confirmation box")
#endif
        , QMessageBox::Yes|QMessageBox::Cancel, QMessageBox::Yes)) {
            return;
    }

    // Change the selection so that the display won't crash
    if(currentFolder.row() > 0){
        // Select folder above
        QModelIndex newSelection = mp_beatsModel->index(currentFolder.row() - 1, 0, currentFolder.parent());
        mp_BeatsPanel->selectionModel()->select(newSelection, QItemSelectionModel::Clear | QItemSelectionModel::SelectCurrent);
        mp_BeatsPanel->selectionModel()->setCurrentIndex(newSelection, QItemSelectionModel::SelectCurrent);
    } else if (mp_beatsModel->rowCount(currentFolder.parent()) > 1){
        // Select folder below
        QModelIndex newSelection = mp_beatsModel->index(1, 0, currentFolder.parent());
        mp_BeatsPanel->selectionModel()->select(newSelection, QItemSelectionModel::Clear | QItemSelectionModel::SelectCurrent);
        mp_BeatsPanel->selectionModel()->setCurrentIndex(newSelection, QItemSelectionModel::SelectCurrent);
    } else {
        // Clear selection
        QModelIndex newSelection;
        mp_BeatsPanel->selectionModel()->select(newSelection, QItemSelectionModel::Clear | QItemSelectionModel::SelectCurrent);
        mp_BeatsPanel->selectionModel()->setCurrentIndex(newSelection, QItemSelectionModel::SelectCurrent);

        // Clear display (since there will not be any data to display after removal
        mp_BeatsPanel->setRootIndex(QModelIndex()); // Set to invalid
    }

    mp_beatsModel->deleteSongFolder(currentFolder);
}

/**
 * @brief MainWindow::closeEvent
 *
 * re-implementation
 */
void MainWindow::closeEvent(QCloseEvent *event)
{

    if(promptSaveUnsavedChanges(true)){
        event->accept();

        // Delete model first
        deleteModel();

        // Then clean up temp dir
        cleanUp();
    } else {
        event->ignore();
    }
}

void MainWindow::dragEnterEvent(QDragEnterEvent *event)
{
    QList<QUrl> x = event->mimeData()->urls();
    QRegularExpression y = mp_beatsModel ? m_drag_all : m_drag_project;
    if (!x.size())
        return;
    foreach (QUrl i, x) {
       if (!i.isLocalFile() || (!i.fileName().isEmpty() && !y.match(i.fileName()).hasMatch())){
           return;
       }
    }
    event->setDropAction(Qt::LinkAction);
    event->accept();
}

void MainWindow::dropEvent(QDropEvent *event)
{
    if (event->isAccepted()) return;
    QList<QUrl> x = event->mimeData()->urls();
    QRegularExpression y = mp_beatsModel ? m_drag_all : m_drag_project;
    QMap<QString, QStringList> u = QMap<QString, QStringList>();
    if (!x.size())
        return;
    foreach (QUrl i, x)
        u[y.match(i.fileName()).captured(1).toLower()].append(i.toLocalFile());
    if ((u.contains(BMFILES_PROJECT_EXTENSION) ? u[BMFILES_PROJECT_EXTENSION].size() : 0)
        + (u.contains(BMFILES_SINGLE_FILE_EXTENSION) ? u[BMFILES_SINGLE_FILE_EXTENSION].size() : 0)
        + (u.contains("") ? u[""].size() : 0)
        > 1)
    {
        QMessageBox::critical(this, "Handling Drag'n'Drop", "Only one project file can be opened at a time!");
        return;
    }
    // 56047 TODO : make atomic drag'n'drop handling - whenever anything fails, rollback everything.
    if (u.contains(BMFILES_PROJECT_EXTENSION) ? slotOpenPrj(u[BMFILES_PROJECT_EXTENSION].first()) :
        u.contains(BMFILES_SINGLE_FILE_EXTENSION) ? slotImportPrjArchive(u[BMFILES_SINGLE_FILE_EXTENSION].first()) :
        u.contains("") ? slotOpenPrj(u[""].first()) :
        true);
    else
        return;
    if (u.contains(BMFILES_DRUMSET_EXTENSION))
        foreach (QString i, u[BMFILES_DRUMSET_EXTENSION])
            slotImportDrm(i);
    if (u.contains(BMFILES_PORTABLE_SONG_EXTENSION))
        slotImport(u[BMFILES_PORTABLE_SONG_EXTENSION]);
    if (u.contains(BMFILES_PORTABLE_FOLDER_EXTENSION))
        slotImport(u[BMFILES_PORTABLE_FOLDER_EXTENSION]);
    // TODO copy directly .bbs file will corrupt things
    //      need to at least clean up accent hit and replace UUID when doing this
    if (u.contains(BMFILES_MIDI_BASED_SONG_EXTENSION))
        for (QModelIndex i = mp_BeatsPanel->selectionModel()->currentIndex(); i != QModelIndex(); i = i.parent())
            if (mp_beatsModel->data(i.parent()) == "SONGS")
                if (ContentFolderTreeItem* folder = qobject_cast<ContentFolderTreeItem*>((QObject*)i.internalPointer())) {
                    QString path = mp_beatsModel->data(i.sibling(i.row(), AbstractTreeItem::ABSOLUTE_PATH)).toString()+"/";
                    foreach (QString file, u[BMFILES_MIDI_BASED_SONG_EXTENSION]) {
                        QString name = file.mid(file.lastIndexOf('/')+1);
                        if (!file.startsWith(path) || file.indexOf('/', path.size()) != -1){
                            QFile(file).copy(path+name);
                        }
                        folder->createFileWithData(mp_beatsModel->rowCount(i), "dragged", name);
                    }
                }
    event->accept();
}

void MainWindow::moveEvent(QMoveEvent*)
{
    mp_window_state_saver->start();
}

void MainWindow::resizeEvent(QResizeEvent*)
{
    mp_window_state_saver->start();
}

void MainWindow::setClean()
{
    foreach (auto stack, mp_UndoRedo->stacks()) {
        stack->setClean();
    }
}

void MainWindow::cleanUp()
{
   // Clean up tmp dir
   DirListAllSubFilesModal listFilesOp(m_tempDirFI.absoluteFilePath(),DirListAllSubFilesModal::rootLast, this);
   if(!listFilesOp.run()){
      qWarning() << "MainWindow::cleanUp - ERROR 1 - unable to clean up temp dir";
      return;
   }
   DirCleanUpModal cleanUpOp(listFilesOp.result(), this);
   if(!cleanUpOp.run()){
      qWarning() << "MainWindow::cleanUp - ERROR 2 - unable to clean up temp dir";
      return;
   }
}



void MainWindow::slotOnBeatsCurrentOrSelChanged()
{
    // 1 - refresh menu
    refreshMenus();

    // 2 - refresh sub panels
    mp_PlaybackPanel->onBeatsCurOrSelChange(mp_BeatsPanel->selectionModel());
}

void MainWindow::slotModelDataChanged(const QModelIndex &left, const QModelIndex &right)
{
    for(int column = left.column(); column <= right.column(); column++){
        if(column == AbstractTreeItem::SAVE){
            // make sure enabled menus items correspond to Save Status
            refreshMenus();
            break;
        }
    }
}

void MainWindow::slotOrderChanged()
{
    refreshMenus();
}

void MainWindow::slotShowOptionsDialog()
{
   OptionsDialog options(mp_PlaybackPanel->bufferTime_ms(), this);
   int ret = options.exec();
   if(ret == QDialog::Accepted){
      mp_PlaybackPanel->slotSetBufferTime_ms(options.bufferingTime_ms());
      // Save the setting by using the value actually stored in player
      Settings::setBufferingTime_ms(mp_PlaybackPanel->bufferTime_ms());
   }
}

void MainWindow::slotChangeWorkspaceLocation()
{
    // Browse for location
    QString directoryName = QFileDialog::getExistingDirectory(this, tr("Choose BeatBuddy Manager Workspace Location"),
                mp_MasterWorkspace->defaultPath()); // will default to default user directory if none exist

    if(!directoryName.isEmpty()){
        QFileInfo directoryFI(directoryName);
        mp_MasterWorkspace->create(directoryFI, this);
        if (mp_MasterWorkspace->isValid()){
            if(QMessageBox::Yes == QMessageBox::question(this, tr("BeatBuddy Manager Workspace"), tr("Do you want to reset the last Navigation location? Last location would be set to default location in Workspace"))) {
                mp_MasterWorkspace->resetCurrentPath();
            }
        }
    }

    if(!mp_MasterWorkspace->isValid()){
        QMessageBox::warning(this, tr("BeatBuddy Manager Workspace"),
                             tr("No valid folder was chosen as the BeatBuddy Manager Workspace.\nSome functions may not work properly.\n\nPlease set a workspace location via main menu: %1 > %2")
                                .arg(mp_toolsMenu->title().replace("&", ""))
                                .arg(mp_ChangeWorkspaceLocation->text().replace("&", "")));
    }
}


void MainWindow::slotShowAboutDialog()
{
   AboutDialog about(this);
   about.exec();
}

void MainWindow::slotShowSupportDialog()
{
   SupportDialog support(this);
   support.exec();
}

void MainWindow::slotOpenUrlManual()
{
    QDesktopServices::openUrl(QUrl(PDF_MANUAL_URL));
}

void MainWindow::slotOpenUrlForum()
{
    QDesktopServices::openUrl(QUrl(BEATBUDDY_FORUM_URL));
}

void MainWindow::slotOpenUrlLibrary()
{
    QDesktopServices::openUrl(QUrl(BEATBUDDY_LIBRARY_URL));
}

void MainWindow::slotShowColorOptions()
{
    auto dlg = new QDialog(this);
    dlg->setWindowTitle(tr("Color Options"));
    auto lo = new QGridLayout(dlg);
    auto cpk = new QColorDialog(dlg);
    const auto& color = [&cpk](QRgb c) -> QRgb {
        cpk->setCurrentColor(c);
        return cpk->exec() ? cpk->currentColor().rgb() : c;
    };
    const auto& button = [&dlg, &color](QRgb& c) -> QPushButton* {
        auto ret = new QPushButton(dlg);
        auto icon = [ret](QRgb c) { QPixmap pm(16, 16); pm.fill(c); ret->setIcon(pm); };
        icon(c);
        ret->setSizePolicy(QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed));
        ret->setFocusPolicy(Qt::NoFocus);
        ret->connect(ret, &QPushButton::clicked, [icon, &c, &color]() { icon(c = color(c)); });
        return ret;
    };
    QRgb w = Settings::getColorDnDWithdraw();
    QRgb c = Settings::getColorDnDCopy();
    QRgb t = Settings::getColorDnDTarget();
    QRgb d = Settings::getColorDnDAppend();
    lo->addWidget(button(w), 0, 0);
    lo->addWidget(new QLabel(tr("Drag'n'Drop Withdraw color"), dlg), 0, 1);
    lo->addWidget(button(c), 1, 0);
    lo->addWidget(new QLabel(tr("Drag'n'Drop Copy color"), dlg), 1, 1);
    lo->addWidget(button(t), 2, 0);
    lo->addWidget(new QLabel(tr("Drag'n'Drop Target color"), dlg), 2, 1);
    lo->addWidget(button(d), 3, 0);
    lo->addWidget(new QLabel(tr("Drag'n'Drop Append color"), dlg), 3, 1);
    QDialogButtonBox* bb;
    lo->addWidget(bb = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Open | QDialogButtonBox::Ok | QDialogButtonBox::Cancel, dlg), 4, 1, 1, 2);
    connect(bb->button(QDialogButtonBox::Ok), &QPushButton::clicked, dlg, &QDialog::accept);
    connect(bb->button(QDialogButtonBox::Cancel), &QPushButton::clicked, dlg, &QDialog::reject);
    bb->button(QDialogButtonBox::Save)->setText(tr("Make Shiny"));
    connect(bb->button(QDialogButtonBox::Save), &QPushButton::clicked, [&dlg] { dlg->done(1337); });
    bb->button(QDialogButtonBox::Open)->setText(tr("Make Gloomy"));
    connect(bb->button(QDialogButtonBox::Open), &QPushButton::clicked, [&dlg] { dlg->done(56047); });
    dlg->setLayout(lo);
    dlg->resize(dlg->size());
    switch (auto i = dlg->exec())
    {
    case true:
        Settings::setColorDnDWithdraw(w);
        Settings::setColorDnDCopy(c);
        Settings::setColorDnDTarget(t);
        Settings::setColorDnDAppend(d);

        break;
    case 1337:
        Settings::setColorDnDWithdraw(0xffff0000);
        Settings::setColorDnDCopy(0xffffff00);
        Settings::setColorDnDTarget(0xff00ffff);
        Settings::setColorDnDAppend(0xff00ff00);
        break;
    case 56047:
        Settings::setColorDnDWithdraw(0xffad3e3e);
        Settings::setColorDnDCopy(0xff83caca);
        Settings::setColorDnDTarget(0xfffffc96);
        Settings::setColorDnDAppend(0xff76a875);
        break;
    }
    delete dlg;
}

void MainWindow::slotShowCheatShit()
{
    if (!mp_CheatShitDialog) {
        auto dlg = new QDialog(this);
        dlg->setWindowTitle(tr("BBManager Cheat Sheet"));
        dlg->setMinimumSize(QSize(500, 500));
        dlg->resize(dlg->size());
        QFile noProjectFile(":/BeatBuddyManager-NoProject-Help.html");
        if(!noProjectFile.open(QFile::ReadOnly | QFile::Text)){
            delete dlg;
            qWarning() << "MainWindow::slotShowCheatShit - ERROR - unable to cheat shit";
            return;
        }
        mp_CheatShitDialog = dlg;
    }
    mp_CheatShitDialog->show();
}

void MainWindow::slotCopy()
{
    for (QWidget * w = childAt(mapFromGlobal(QCursor::pos())); w; w = w->parentWidget()){
        if (CopyPaste::Copyable* c = dynamic_cast<CopyPaste::Copyable*>(w)){
            if (c->copy()){
                return;
            }
        }
    }
}

void MainWindow::slotPaste()
{
    for (QWidget * w = childAt(mapFromGlobal(QCursor::pos())); w; w = w->parentWidget()){
        if (CopyPaste::Pastable* c = dynamic_cast<CopyPaste::Pastable*>(w)){
            if (c->paste()){
                return;
            }
        }
    }
}

void MainWindow::slotSaveWindowState()
{
    if (isMaximized()){
        Settings::setWindowMaximized(true);
    } else {
        Settings::setWindowMaximized(false);
        Settings::setWindowX(x());
        Settings::setWindowY(y());
        Settings::setWindowW(width());
        Settings::setWindowH(height());
    }
}

void MainWindow::setBeatsDrmStackedWidgetCurrentIndex(int ix)
{
    mp_BeatsDrmStackedWidget->setCurrentIndex(!(m_beats_inactive = ix)+1);
}


/**
 * @brief MainWindow::slotExportSong
 *
 * Export song/folder with dependencies as a single file in a portable format
 */
void MainWindow::slotExport(bool parent)
{
    if(!mp_beatsModel){
        return;
    }


    // 1 - Validate current, export folder if it is a folder, give user a choice otherwise
    QModelIndex current = mp_BeatsPanel->selectionModel()->currentIndex();

    auto cur = static_cast<AbstractTreeItem*>(current.internalPointer());
    auto song = qobject_cast<SongFileItem*>(cur);
    auto folder = qobject_cast<SongFolderTreeItem*>(song ? song->parent() : cur);

    if (!folder) {
        QMessageBox::warning(this, tr("Export Song or Folder"), tr("No valid song or folder is selected"));
        return;
    }

    if (parent) {
        song = nullptr;
        cur = folder;
    }

    // 2 - Select location where to export
    QDir dir(mp_MasterWorkspace->userLibrary()->libSongs()->currentPath());
    QRegularExpression re("[ \\\"/<|>:*_?]+");
    QString song_name = !song ? "" : song->data(AbstractTreeItem::NAME).toString().replace(re, "_"); // we replace inappropriate symbols with a sad smilie in hopes for happy future
    QString folder_name = folder->data(AbstractTreeItem::NAME).toString().replace(re, "_"); // we replace inappropriate symbols with a sad smilie in hopes for happy future

    QFileDialog dlg(
        this,
        tr("Export Song or Folder"),
        dir.absoluteFilePath(song ? song_name : folder_name),
        song ? BMFILES_PORTABLE_SONG_DIALOG_FILTER : BMFILES_PORTABLE_FOLDER_DIALOG_FILTER);
    dlg.setAcceptMode(QFileDialog::AcceptSave);
    connect(&dlg, &QFileDialog::filterSelected, this, [&dlg, &song_name, &folder_name](const QString& filter) {
        if (filter == BMFILES_PORTABLE_SONG) {
            dlg.setWindowTitle(tr("Export Song")); // TODO : Investigate why caption isn't changed on Windows
            dlg.selectUrl(QUrl::fromLocalFile(song_name));
        } else if (filter == BMFILES_PORTABLE_FOLDER) {
            dlg.setWindowTitle(tr("Export Folder")); // TODO : Investigate why caption isn't changed on Windows
            dlg.selectUrl(QUrl::fromLocalFile(folder_name));
        } else {
            dlg.setWindowTitle(tr("Export Song or Folder")); // TODO : Investigate why caption isn't changed on Windows
        }
    });
    dlg.exec();
    QString dstFileName;
    auto x = dlg.selectedFiles();
    foreach (auto s, dlg.selectedFiles()) { dstFileName = s; break; }

    if (dstFileName.isEmpty()) {
        return;
    } else if (dstFileName.endsWith("." BMFILES_PROJECT_EXTENSION, Qt::CaseInsensitive)) {
        song = nullptr;
        cur = folder;
    }

    // 3 - Make sure there are no unsaved changes
    if (cur->data(AbstractTreeItem::SAVE).toBool() && !promptSaveUnsavedChanges(false)) {
        return;
    }

    // 4 - make sure item is valid
    auto invalid = cur->data(AbstractTreeItem::INVALID).toString();
    if (!invalid.isEmpty()) {
        auto parsed = QRegularExpression("^([^|]+).*?(?:|[^\\d]*(\\d+)[^|]*)?(?:|[^\\d]*(\\d+)[^|]*)?$").match(invalid);
        auto song = parsed.captured(2);
        auto folder = parsed.captured(3);
        auto faulty_song = !song.isEmpty() && !folder.isEmpty() ? mp_beatsModel->songsFolderIndex().child(QVariant(folder).toInt()-1, 0).child(QVariant(song).toInt()-1, 0) : QModelIndex();
        if (QMessageBox::Yes == QMessageBox::warning(this, tr("Export Song or Folder"),
            tr("%1:(\nAll project songs should be valid before continuing!\n---\nTo fix this error, please do one of the following:"
            "\n\tAdd missing song part"
            "\n\tUse %2 > %3 to make sure song part was not deleted by mistake"
            "\n\tRemove song part without a Main Drum Loop"
            "\n\tRemove the invalid song altogether").arg(parsed.captured(1)).arg(mp_edit->title().replace("&", "")).arg(mp_Undo->text().replace("&", ""))
            + (faulty_song == QModelIndex() ? "" : tr("\n\nShow the faulty song now?")),
            faulty_song == QModelIndex() ? QMessageBox::Ok : QMessageBox::Yes|QMessageBox::Cancel)) {
                mp_BeatsPanel->selectionModel()->select(faulty_song, QItemSelectionModel::Clear | QItemSelectionModel::SelectCurrent);
                mp_BeatsPanel->selectionModel()->setCurrentIndex(faulty_song, QItemSelectionModel::SelectCurrent);
        }
        return;
    }

    mp_MasterWorkspace->userLibrary()->libSongs()->setCurrentPath(QFileInfo(dstFileName).absolutePath());
    // 5 - Perform operation
    if (song) {
        song->exportModal(this, dstFileName);
    } else {
        folder->exportModal(this, dstFileName);
    }
}

/**
 * @brief MainWindow::slotImport
 *
 * Import songs/folders with dependencies from a portable format
 */
void MainWindow::slotImport(const QStringList& files, int preferred)
{
    if(mp_beatsModel){
        // Delegate operation to SongFolderView
        mp_BeatsPanel->songFolderView()->import(mp_BeatsPanel->selectionModel()->currentIndex(), preferred, files);
    }
}

void MainWindow::slotOnDrmOpened()
{
    mp_saveDrm->setEnabled(true);
    mp_saveDrmAs->setEnabled(true);
    mp_closeDrm->setEnabled(true);
    mp_addDrmInstrument->setEnabled(true);
    mp_sortDrmInstrument->setEnabled(true);
}

void MainWindow::slotOnDrmClosed()
{
    mp_saveDrm->setEnabled(false);
    mp_saveDrmAs->setEnabled(false);
    mp_closeDrm->setEnabled(false);
    mp_addDrmInstrument->setEnabled(false);
    mp_sortDrmInstrument->setEnabled(false);
}

/**
 * @brief MainWindow::slotActivePlayer
 *       Enables or disables the user interaction with the BeatsPanel.
 *       This method (slot) is mainly used during the playback to stop
 *       any user interaction with the beatspanel.
 * @param status
 */
void MainWindow::slotActivePlayer(bool status)
{
   m_playing = status;

   mp_BeatsPanel->setSongsEnabled(true);
   mp_beatsModel->setEditingDisabled(false);
   mp_beatsModel->setSelectionDisabled(false);

    refreshMenus();
}

void MainWindow::slotRevealLogs()
{
    QDesktopServices::openUrl(QUrl::fromLocalFile(LOGGING_DIR));
}

/**
 * @brief process open file event sent by BBManagerApplication, from OS
 * @param filePath
 */
void MainWindow::slotProcessOpenFileEvent(const QString &filePath)
{
    if (filePath.endsWith("." BMFILES_PROJECT_EXTENSION)){
        // Open project while doing required verifications (like saving changes)
        slotOpenPrj(filePath);
    } else if (filePath.endsWith("." BMFILES_SINGLE_FILE_EXTENSION)){
        slotImportPrjArchive(filePath);
    } // Otherwise, do not load anything - TODO handle .drm
}
