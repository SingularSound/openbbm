#ifndef PROJECTEXPLORERPANEL_H
#define PROJECTEXPLORERPANEL_H

#include <QWidget>
#include <QLabel>
#include <QAbstractItemModel>
#include <QItemSelectionModel>
#include <QTreeView>
#include <QListWidget>
#include <QFileSystemWatcher>
#include <QFileInfoList>
#include <QAction>
#include <QStandardItem>
#include <QGridLayout>
#include <QTimer>
#include <QStackedWidget>
#include <QSpinBox>
#include "../dialogs/nameAndIdDialog.h"
#include "../model/tree/project/songfolderproxymodel.h"

class DrmListModel;
class BeatsProjectModel;
class QUndoView;

#if !defined(DEBUG_TAB_ENABLED) && defined(QT_DEBUG)
#define DEBUG_TAB_ENABLED
#endif

class NoInputSpinBox : public QSpinBox
{
public:
    NoInputSpinBox(QWidget *parent = nullptr) : QSpinBox(parent) {}
 
protected:
    void keyPressEvent(QKeyEvent* event);
};

class ProjectExplorerPanel : public QWidget
{
   Q_OBJECT
public:
   explicit ProjectExplorerPanel(QWidget *parent = nullptr);
    ~ProjectExplorerPanel();

   // Beats Model Getters/Setters
   void setProxyBeatsModel(SongFolderProxyModel * p_model);
   void setProxyBeatsRootIndex(QModelIndex rootIndex);
   void setProxyBeatsSelectionModel(QItemSelectionModel *beatsSelectionModel);
   QItemSelectionModel * beatsSelectionModel();
   void setMidiColumn();

   // Debug Beats Model Getters/Setters
   void setBeatsModel(BeatsProjectModel * p_model, class QUndoGroup* undo);
   void setBeatsDebugSelectionModel(QItemSelectionModel *beatsSelectionModel);

   // Drm List Model Getters/Setters
   void setDrmListModel(QAbstractItemModel * p_model);
   DrmListModel *drmListModel();
   QItemSelectionModel * drmListSelectionModel();

signals:
   void sigCurrentTabChanged(int index);
   void sigCurrentItemChanged(QListWidgetItem *current, QListWidgetItem *previous);
   void sigOpenDrm(const QString& name, const QString &filePath);

public slots:
   void slotCleanChanged(bool clean);
   void slotCurrentItemChanged(QListWidgetItem *current, QListWidgetItem *previous);
   void slotOnCurrentTabChanged(int index);

   // Used for debug only
   void slotDebugOnCurrentChanged(const QModelIndex & current, const QModelIndex & previous);
   void slotDebugOnSelectionChanged(const QItemSelection & selected, const QItemSelection & deselected);

    void slotBeginEdit(class SongPart*);

private slots:
   void slotOnDoubleClick(const QModelIndex &index);
   void slotDrumListContextMenu(QPoint pnt);
   void slotDrmDeleteRequestShortcut();
   void slotDrmDeleteRequest();
   void slotTempoChanged(int i);

protected:
   virtual void paintEvent(QPaintEvent * event);

private:
   void createLayout();
   bool doesUserConsentToImportDrumset();
private:
   QString  m_currentContextSelectionPath;
   QAction *mp_ContextDeleteDrumsetAction;
   QAction *mp_ContextCloseDrumsetAction;
   QStackedWidget* mp_MainContainer;
   QTreeView *mp_beatsTreeView;
   QListView *mp_drmListView;
   QLabel * mp_Title;
   NameAndIdDialog *nameAndIdDialog;
   bool m_clean;
#ifdef DEBUG_TAB_ENABLED
   QTreeView *mp_beatsDebugTreeView;
#else
   BeatsProjectModel* mp_beatsModel;
#endif
   QUndoView* mp_viewUndoRedo;

   QLabel *mp_TempoLabel;
   QSpinBox *mp_TempoText;

};

#endif // PROJECTEXPLORERPANEL_H
