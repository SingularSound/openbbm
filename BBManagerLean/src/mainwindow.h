#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#define BEATBUDDY_FORUM_URL "https://forum.singularsound.com"
#define BEATBUDDY_LIBRARY_URL "https://library.mybeatbuddy.com/?utm_source=bbmanager&utm_medium=software&utm_campaign=users"
#define PDF_MANUAL_URL  "http://forum.mybeatbuddy.com/index.php?threads/bbmanager-1-64-video-tutorials.6431/"

#include <QAbstractItemModel>
#include <QCloseEvent>
#include <QFileInfo>
#include <QItemSelectionModel>
#include <QMainWindow>
#include <QStackedWidget>
#include <QTableWidget>
#include <QRegularExpression>

#include <beatspanel/beatfilewidget.h>
#include <beatspanel/beatspanel.h>
#include <drmmaker/DrumsetPanel.h>
#include <midi_editor/editor.h>
#include <model/tree/project/songfolderproxymodel.h>
#include <model/tree/project/beatsprojectmodel.h>
#include <pexpanel/drmlistmodel.h>
#include <pexpanel/projectexplorerpanel.h>
#include <playbackpanel.h>
#include <update/update.h>
#include <vm/virtualmachinepanel.h>
#include <workspace/workspace.h>

class MainWindow : public QMainWindow
{
   Q_OBJECT
public:
    explicit MainWindow(QString default_path = nullptr, QWidget* parent = nullptr);
    ~MainWindow();

    inline bool hasOpenedProject(){return (mp_beatsModel != nullptr);}

public slots:
    void slotActivePlayer(bool status);
    void slotProcessOpenFileEvent(const QString &filePath);

private slots:
    void slotNewSong();
    void slotDelete();
    void slotSavePrj();
    void slotExport(bool folder = false);
    inline void slotExportFolder() { return slotExport(true); }
    void slotImport(const QStringList& files = QStringList(), int preferred = 0); // 0 - no matter, > 0 - dirs, < 0 - files
    void slotImportDrm(QString drm = nullptr);
    void slotNewPrj();
    bool slotOpenPrj(const QString& path = nullptr);
    void slotSavePrjAs();
    void slotExportPrjArchive(const QString& path = nullptr);
    bool slotImportPrjArchive(const QString& path = nullptr);
    void slotSyncPrj();
    void slotExportPrj();

    void slotPrev();
    void slotNext();
    void slotParent();
    void slotChild();
    void slotMoveUp();
    void slotMoveDown();
    void slotMoveFolderUp();
    void slotMoveFolderDown();
    void slotNewFolder();
    void slotDeleteFolder();
    void slotToggleMidiId();

    void slotOnBeatsCurrentOrSelChanged();
    void slotModelDataChanged(const QModelIndex &left, const QModelIndex &right);

    void slotOrderChanged();

    void slotOnDrmOpened();
    void slotOnDrmClosed();

    void slotShowOptionsDialog();
    void slotChangeWorkspaceLocation();
    void slotShowAboutDialog();
    void slotOpenUrlManual();
    void slotOpenUrlForum();
    void slotShowSupportDialog();
    void slotShowColorOptions();
    void slotShowCheatShit();

    void slotOpenUrlLibrary();

    void slotCopy();
    void slotPaste();
    void slotSaveWindowState();
    void setBeatsDrmStackedWidgetCurrentIndex(int);

    void slotDrumsetChanged(const QString& drm);

    void slotOnBeginEditSongPart(const QString& name, const QByteArray& data);
    void slotOnEndEditSongPart(SongPart*, bool accept);

    void slotTempoChangedBySong(int);

    void slotRevealLogs();

    void refreshMenus();

protected:
    void paintEvent(QPaintEvent* event);
    void closeEvent(QCloseEvent* event);
    void dragEnterEvent(QDragEnterEvent* event);
    void dropEvent(QDropEvent* event);
    void moveEvent(QMoveEvent* event);
    void resizeEvent(QResizeEvent* event);

private:
    void createTempProjectFolder(QString default_path);
    void createCentralWidget();
    void createActions();
    void createMenus();
    void createStateSaver();
    void createBeatsModels(const QString &projectFilePath, bool create = false);
    void createDrmListModel();
    void deleteModel();
    void initWorkspace();
    void initUI();
    QString browseForPedal();
    bool promptSaveUnsavedChanges(bool allowDiscard);
    QAction* buildAction(const QString &title, const QString &statusTip, const QString &toolTip);
    QAction* buildAction(const QString &title, const QString &statusTip, const QString &toolTip,const QKeySequence &qks);
    QUndoStack* selectUndoStack();
    void setClean();
    void cleanUp();

    bool m_playing;
    bool m_beats_inactive;

public:
    QMenu* mp_fileMenu;
    QAction* mp_savePrj;
    QAction* mp_newPrj;
    QAction* mp_openPrj;
    QAction* mp_savePrjAs;

    QMenu* mp_drumsMenu;
    QAction* mp_newDrm;
    QAction* mp_openDrm;
    QAction* mp_saveDrm;
    QAction* mp_saveDrmAs;
    QAction* mp_closeDrm;
    QAction* mp_exit;

    QMenu* mp_songsMenu;
    QAction* mp_newSong;
    QAction* mp_delete;
    QAction* mp_moveUp;
    QAction* mp_moveDown;
    QAction* mp_moveFolderUp;
    QAction* mp_moveFolderDown;
    QAction* mp_toggleMidiId;
    QAction* mp_newFolder;
    QAction* mp_deleteFolder;
    QAction* mp_addDrmInstrument;
    QAction* mp_sortDrmInstrument;
    QAction* mp_deleteDrm;

    QMenu* mp_export;
    QMenu* mp_import;
    QAction* mp_exportSong;
    QAction* mp_importSong;
    QAction* mp_exportFolder;
    QAction* mp_importFolder;
    QAction* mp_importDrm;
    QAction* mp_syncPrj;
    QAction* mp_exportPrj;

    QAction* mp_prev;
    QAction* mp_next;
    QAction* mp_parent;
    QAction* mp_child;

    QMenu* mp_editorMenu;
    QAction* mp_zoomIn;
    QAction* mp_zoomOut;
    QAction* mp_zoomReset;

    QMenu* mp_toolsMenu;
    QAction* mp_UpdateFirmware;
    QAction* mp_ShowUpdateDialog;
    QAction* mp_ColorOptions;
    QAction* mp_ListUsb;
    QAction* mp_ShowOptionsDialog;
    QAction* mp_ChangeWorkspaceLocation;

    QMenu* mp_helpMenu;
    QAction* mp_ShowAboutDialog;
    QAction* mp_ShowSupportDialog;
    QAction* mp_ShowCheatShitDialog;
    QAction* mp_linkToPdfManual;
    QAction* mp_linkToForum;

    QAction* mp_libraryOpen;

    class QUndoGroup* mp_UndoRedo;
    QMenu* mp_edit;
    QAction* mp_Undo;
    QAction* mp_Redo;

    QAction* mp_Copy;
    QAction* mp_Paste;

    QAction* mp_AcceptEditor;
    QAction* mp_CancelEditor;

    QAction* mp_RevealLogs;

    QTimer* mp_window_state_saver;

    VirtualMachinePanel* mp_VMPanel;
    ProjectExplorerPanel* mp_ProjectExplorerPanel;
    PlaybackPanel* mp_PlaybackPanel;
    BeatsPanel* mp_BeatsPanel;
    EditorTableWidget* mp_TableEditor;
    DrumsetPanel* mp_DrumsetPanel;
    QStackedWidget*  mp_BeatsDrmStackedWidget;

    BeatsProjectModel* mp_beatsModel;
    SongFolderProxyModel* mp_beatsProxyModel;
    DrmListModel* mp_drmListModel;

    QFileInfo m_tempDirFI;

    QRegularExpression m_drag_all;
    QRegularExpression m_drag_project;

    Workspace* mp_MasterWorkspace;

    QDialog* mp_CheatShitDialog;
    bool m_done;

private:
    Update update;
};

#endif // MAINWINDOW_H
