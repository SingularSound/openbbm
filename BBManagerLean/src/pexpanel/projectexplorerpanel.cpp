/*
  	This software and the content provided for use with it is Copyright © 2014-2020 Singular Sound 
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
#include <QLabel>
#include <QVBoxLayout>
#include <QStyleOption>
#include <QPainter>
#include <QHeaderView>
#include <QDebug>
#include <QMenu>
#include <QMessageBox>
#include <QShortcut>
#include <QUndoView>
#include <QDrag>
#include <QTreeWidget>

#include "projectexplorerpanel.h"
#include "../workspace/workspace.h"
#include "../workspace/contentlibrary.h"
#include "../workspace/libcontent.h"
#include "drmmaker/Model/drmmakermodel.h"
#include "drmlistmodel.h"
#include "../model/tree/project/beatsprojectmodel.h"
#include "drmmaker/UI_Elements/Labels/ClickableLabel.h"
#include "../model/tree/abstracttreeitem.h"
#include "../mainwindow.h" 
#include "../model/filegraph/song.h"  // to get MAX_BPM
#include "../workspace/settings.h"

class QTreeViewDragFixed : public QTreeView
{
protected:
    void startDrag(Qt::DropActions supportedActions)
    {
        QModelIndexList indexes = selectedIndexes();
        QList<QPersistentModelIndex> persistentIndexes;

        if (indexes.count() > 0) {
            QMimeData *data = model()->mimeData(indexes);
            if (!data)
                return;
            for (int i = 0; i<indexes.count(); i++){
                QModelIndex idx = indexes.at(i);
                qDebug() << "\tDragged item to delete" << i << " is: \"" << idx.data().toString() << "\"";
                qDebug() << "Row is: " << idx.row();
                persistentIndexes.append(QPersistentModelIndex(idx));
            }

            QPixmap pixmap = indexes.first().data(Qt::DecorationRole).value<QPixmap>();
            QDrag *drag = new QDrag(this);
            drag->setPixmap(pixmap);
            drag->setMimeData(data);
            drag->setHotSpot(QPoint(pixmap.width()/2, pixmap.height()/2));

            Qt::DropAction defaultDropAction = Qt::IgnoreAction;
            if (supportedActions & Qt::MoveAction && dragDropMode() != QAbstractItemView::InternalMove)
                defaultDropAction = Qt::MoveAction; 

            if ( drag->exec(supportedActions, defaultDropAction) & Qt::MoveAction ){
                //when we get here any copying done in dropMimeData has messed up our selected indexes
                //that's why we use persistent indexes
                for (int i = 0; i<indexes.count(); i++){
                    QPersistentModelIndex idx = persistentIndexes.at(i);
                    qDebug() << "\tDragged item to delete" << i << " is: " << idx.data().toString();
                    qDebug() << "Row is: " << idx.row();
                    if (idx.isValid()){ //the item is not top level
                        model()->removeRow(idx.row(), idx.parent());
                    }
                    else{
                        model()->removeRow(idx.row(), QModelIndex());
                    }
                }
            }
        }
    }

public:
    explicit QTreeViewDragFixed(QWidget* parent = nullptr) : QTreeView(parent) {}
};

void NoInputSpinBox::keyPressEvent(QKeyEvent* event)
{
    if (event->key() == Qt::Key_Up || event->key() == Qt::Key_Down)
        return QSpinBox::keyPressEvent(event);
    event->ignore();
}


ProjectExplorerPanel::ProjectExplorerPanel(QWidget *parent)
    : QWidget(parent)
    , m_clean(true)
#ifndef DEBUG_TAB_ENABLED
    , mp_beatsModel(nullptr)
#endif
{
   createLayout();
}

ProjectExplorerPanel::~ProjectExplorerPanel() {

}

void showInGraphicalShell(const QString &path)
{
    // Mac, Windows support folder or file.
#if defined(Q_OS_WIN)
    QProcess::startDetached(QString("explorer /select, \"%1\"").arg(QDir::toNativeSeparators(path)));
#elif defined(Q_OS_MAC)
    QProcess::execute("/usr/bin/osascript", QStringList() << QString::fromLatin1("-e") << QString::fromLatin1("tell application \"Finder\" to reveal POSIX file \"%1\"").arg(path));
    QProcess::execute("/usr/bin/osascript", QStringList() << QString::fromLatin1("-e") << QString::fromLatin1("tell application \"Finder\" to activate"));
#endif
}

void ProjectExplorerPanel::createLayout()
{
   QVBoxLayout * p_VBoxLayout = new QVBoxLayout();
   setLayout(p_VBoxLayout);
   p_VBoxLayout->setContentsMargins(0,0,0,0);

    auto title = new ClickableLabel(tr("Project Explorer%1%2").arg("").arg(m_clean ? "" : "*"), this);
    connect(title, &ClickableLabel::clicked, [title] {
        auto tt = title->toolTip();
        if (tt.isEmpty())
            return;
        showInGraphicalShell(tt);
    });
    mp_Title = title;
    mp_Title->setToolTip(nullptr);
    mp_Title->setObjectName(QStringLiteral("titleBar"));

   p_VBoxLayout->addWidget(mp_Title, 0);

   mp_MainContainer = new QStackedWidget(this);
   p_VBoxLayout->addWidget(mp_MainContainer, 1);
   mp_MainContainer->setLayout(new QVBoxLayout());

   auto p_tabWidget = new QTabWidget(mp_MainContainer);
   mp_MainContainer->layout()->addWidget(p_tabWidget);

   mp_beatsTreeView = new QTreeView(this);
   mp_beatsTreeView->setAnimated(true);
   mp_beatsTreeView->setExpandsOnDoubleClick(false);
   mp_beatsTreeView->setSelectionMode(QAbstractItemView::SingleSelection);
   mp_beatsTreeView->setSelectionBehavior(QAbstractItemView::SelectRows);
   mp_beatsTreeView->setEditTriggers(QAbstractItemView::NoEditTriggers);
   mp_beatsTreeView->setDragEnabled(true);
   mp_beatsTreeView->setAcceptDrops(true);
   p_tabWidget->addTab(mp_beatsTreeView, tr("Songs"));
   connect(mp_beatsTreeView, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(slotOnDoubleClick(QModelIndex)));

   mp_drmListView = new QListView();
   mp_drmListView->setEditTriggers(QAbstractItemView::SelectedClicked);

   p_tabWidget->addTab(mp_drmListView, tr("Drum Sets"));
   //TODO: MS CONNECT BUG
   connect(mp_drmListView->selectionModel(), SIGNAL(currentItemChanged(QListWidgetItem*,QListWidgetItem*)), this, SLOT(slotCurrentItemChanged(QListWidgetItem*,QListWidgetItem*)));
   connect(mp_drmListView, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(slotOnDoubleClick(QModelIndex)));

#ifdef DEBUG_TAB_ENABLED
   mp_beatsDebugTreeView = new QTreeView(this);
   p_tabWidget->addTab(mp_beatsDebugTreeView, tr("Debug"));
   mp_beatsDebugTreeView->setAlternatingRowColors(true);
#endif
   mp_viewUndoRedo = new QUndoView(this);
   p_tabWidget->addTab(mp_viewUndoRedo, tr("Undo/Redo"));

   connect(p_tabWidget, SIGNAL(currentChanged(int)), this, SLOT(slotOnCurrentTabChanged(int)));

   // Delete Key will delete drumset if allowed
   connect(new QShortcut(QKeySequence(Qt::Key_Delete), mp_drmListView), SIGNAL(activated()), this, SLOT(slotDrmDeleteRequestShortcut()));
}

bool ProjectExplorerPanel::doesUserConsentToImportDrumset()
{
    return QMessageBox::Yes == QMessageBox::question(this, tr("Edit Drumset"), tr("Project drum sets must be copied to workspace.\n\nContinue?"));
}




// Required to apply stylesheet
void ProjectExplorerPanel::paintEvent(QPaintEvent *)
{
   QStyleOption opt;
   opt.init(this);
   QPainter p(this);
   style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);
}

void ProjectExplorerPanel::slotOnCurrentTabChanged(int index)
{
   if(index <= 1){
      emit sigCurrentTabChanged(index);
   }
}

// Used for debug only
void ProjectExplorerPanel::slotDebugOnCurrentChanged(const QModelIndex & current, const QModelIndex & previous)
{
   qDebug() << "ProjectExplorerPanel::slotDebugOnCurrentChanged from " << previous.data().toString() << " to " << current.data().toString();
}

// Used for debug only
void ProjectExplorerPanel::slotDebugOnSelectionChanged(const QItemSelection & selected, const QItemSelection & deselected)
{
   qDebug() << "ProjectExplorerPanel::slotDebugOnSelectionChanged deselected: ";
   foreach (const QItemSelectionRange & range, deselected) {
      qDebug() << "   topLeft = " << range.topLeft().data().toString();
      qDebug() << "   bottomRight = " << range.bottomRight().data().toString();
   }
   qDebug() << "ProjectExplorerPanel::slotDebugOnSelectionChanged selected: ";
   foreach (const QItemSelectionRange & range, selected) {
      qDebug() << "   topLeft = " << range.topLeft().data().toString();
      qDebug() << "   bottomRight = " << range.bottomRight().data().toString();
   }
}

void ProjectExplorerPanel::slotBeginEdit(SongPart* sp)
{
    auto w = new QWidget(mp_MainContainer);
    sp->OnDestroy([this, w] { mp_MainContainer->setCurrentIndex(0); delete w; });
    mp_MainContainer->addWidget(w);
    auto lo = new QVBoxLayout();
    w->setLayout(lo);
    lo->addWidget(new QLabel(tr("Now editing:"), w));
    auto line = new QLineEdit(sp->name(), w);
    line->setReadOnly(true);
    lo->addWidget(line);
    auto l1 = new QGridLayout(w);
    lo->addLayout(l1);
    auto& data = sp->data();
    auto ln = 0;
    {
        l1->addWidget(new QLabel(tr("Time Signature:"), w), ln, 0);
        auto l = new QHBoxLayout(w);
        l1->addLayout(l, ln, 1);
        auto num = new QComboBox(w);
        for (int i = 1; i < 33; ++i)
            num->addItem(QVariant(i).toString(), i);
        num->setInsertPolicy(QComboBox::NoInsert);
        num->setCurrentIndex(data.timeSigNum-1);
        l->addWidget(num, 1);
        l->addWidget(new QLabel("/", w));
        auto den = new QComboBox(w);
        for (int i = 0; i < 6; ++i)
            den->addItem(QVariant(1 << i).toString(), 1 << i);
        int ix = -1; for (auto d = data.timeSigDen; d; d >>= 1, ++ix);
        den->setCurrentIndex(ix);
        den->setInsertPolicy(QComboBox::NoInsert);
        auto changed = [sp, num, den] { emit sp->timeSignatureChanged(num->currentIndex()+1, 1 << den->currentIndex()); };
        connect(num, (void(QComboBox::*)(int))&QComboBox::currentIndexChanged, changed);
        connect(den, (void(QComboBox::*)(int))&QComboBox::currentIndexChanged, changed);
        l->addWidget(den, 1);
        ++ln;
    }
    {
        l1->addWidget(new QLabel(tr("Total Bars Count:"), w), ln, 0);
        auto x = new NoInputSpinBox(w);
        x->setMinimum(1);
        x->setMaximum(0x7FFFFFFF);
        x->setSingleStep(1);
        x->setValue(int(double(data.nTick)/data.barLength+.5));
        connect(x, (void(QSpinBox::*)(int))&QSpinBox::valueChanged, sp, &SongPart::barCountChanged);
        connect(sp, &SongPart::nTickChanged, [sp, x](int nTick) { auto bars = nTick/sp->data().barLength; if (bars != x->value()) x->setValue(bars); });
        l1->addWidget(x, ln, 1);
        ++ln;
    }
    {
        mp_TempoLabel = new QLabel(this);
        mp_TempoLabel->setText(tr("Default Tempo:"));
        mp_TempoLabel->setObjectName(QStringLiteral("tempoLabel"));

        l1->addWidget(mp_TempoLabel, ln, 0);
        mp_TempoText = new QSpinBox(this);
        qDebug() << "bpm:" << data.bpm;
        mp_TempoText->setMaximum(MAX_BPM);
        mp_TempoText->setKeyboardTracking(false);
        mp_TempoText->setSpecialValueText("Song Default");
        mp_TempoText->setObjectName(QStringLiteral("tempoSpin"));
        mp_TempoText->setFocusPolicy( Qt::StrongFocus );
        mp_TempoText->setToolTip(tr("Change the default tempo of this part"));
        mp_TempoText->setValue(data.bpm);

        connect(mp_TempoText, SIGNAL(valueChanged(int)), this, SLOT(slotTempoChanged(int)));
        connect(mp_TempoText, (void(QSpinBox::*)(int))&QSpinBox::valueChanged, sp, &SongPart::tempoChanged);
        l1->addWidget(mp_TempoText, ln, 1);
        ++ln;
    }
    {
        auto txq = tr("Quantize"), txnq = tr("*** Quantize ***");
        auto x = new QPushButton(txnq, w);
#ifdef Q_OS_WIN
        auto key = Qt::ControlModifier;
        auto mod = "Ctrl";
#else
        auto key = Qt::MetaModifier;
        auto mod = "⌘";
#endif
        connect(x, &QCheckBox::clicked, [sp, key] { emit sp->quantize(QApplication::keyboardModifiers() & key); });
        connect(sp, &SongPart::quantizeChanged, [x, txq, txnq](bool q) { x->setText(q ? txq : txnq); });
        x->setToolTip(tr("Quantize (or re-quantize) pattern using current time signature\n\n"
            "Displayed (edited) pattern will be used.\nHold %1 to use original (unmodified) pattern").arg(mod));
        l1->addWidget(x, ln, 0, 1, 2);
        ++ln;
    }
    {
        l1->addWidget(new QLabel(tr("Visual style:"), w), ln, 0);
        auto x = new QCheckBox(tr("Draw borders"), w);
        connect(x, &QCheckBox::toggled, sp, &SongPart::buttonStyleChanged);
        x->setStyleSheet("color : gray");
        connect(x, &QCheckBox::toggled, sp, [x](bool checked) { x->setStyleSheet(checked ? "color : black" : "color : gray"); });
        x->setChecked(true);
        l1->addWidget(x, ln, 1);
        ++ln;
    }
    {
        l1->addWidget(new QLabel(tr("Velocity display:"), w), ln, 0);
        auto x = new QCheckBox(tr("Show values (0-127)"), w);
        connect(x, &QCheckBox::toggled, sp, &SongPart::showVelocityChanged);
        x->setStyleSheet("color : gray");
        connect(x, &QCheckBox::toggled, sp, [x](bool checked) { x->setStyleSheet(checked ? "color : black" : "color : gray"); });
        x->setChecked(true);
        l1->addWidget(x, ln, 1);
        ++ln;
    }
    {
        l1->addWidget(new QLabel(tr("Color scheme:"), w), ln, 0);
        auto x = new QCheckBox(tr("Use colors"), w);
        connect(x, &QCheckBox::toggled, sp, &SongPart::colorSchemeChanged);
        x->setStyleSheet("color : gray");
        connect(x, &QCheckBox::toggled, sp, [x](bool checked) { x->setStyleSheet(checked ? "color : cyan" : "color : gray"); });
        l1->addWidget(x, ln, 1);
        ++ln;
    }
    {
        l1->addWidget(new QLabel(tr("Player indicator:"), w), ln, 0);
        auto x = new QComboBox(w);
        x->addItem(tr("Disabled"), 0);
        x->addItem(tr("Don't scroll"), 1);
        x->addItem(tr("Ensure visible"), 2);
        x->addItem(tr("In the center"), 3);
        connect(x, (void(QComboBox::*)(int))&QComboBox::currentIndexChanged, sp, &SongPart::visualizerSchemeChanged);
        x->setCurrentIndex(3);
        l1->addWidget(x, ln, 1);
        ++ln;
    }
    {
        auto l = new QLabel(tr("Total Ticks:"), w);
        l1->addWidget(l, ln, 0);
        auto x = new QLineEdit(QVariant(data.nTick).toString(), w);
        connect(sp, &SongPart::nTickChanged, [x](int nTick) { x->setText(QVariant(nTick).toString()); });
        connect(sp, &SongPart::quantizeChanged, [l, x](bool q) { l->setVisible(!q); x->setVisible(!q); });
        x->setReadOnly(true);
        l1->addWidget(x, ln, 1);
        ++ln;
    }
    {
        auto l = new QLabel(tr("Bar Length (ticks):"), w);
        l1->addWidget(l, ln, 0);
        auto x = new QLineEdit(QVariant(data.barLength).toString(), w);
        connect(sp, &SongPart::quantizeChanged, [l, x](bool q) { l->setVisible(!q); x->setVisible(!q); });
        x->setReadOnly(true);
        l1->addWidget(x, ln, 1);
        ++ln;
    }
    {
        auto l = new QLabel(tr("Number of notes:"), w);
        l1->addWidget(l, ln, 0);
        int sz = data.event.size();
        auto x = new QLineEdit(QVariant(sz).toString(), w);
        connect(sp, &SongPart::quantizeChanged, [l, x](bool q) { l->setVisible(!q); x->setVisible(!q); });
        x->setReadOnly(true);
        l1->addWidget(x, ln, 1);
        ++ln;
    }
    lo->addStretch();
    auto bb = new QDialogButtonBox(QDialogButtonBox::Apply | QDialogButtonBox::Cancel, w);
    lo->addWidget(bb);
    connect(bb->button(QDialogButtonBox::Apply), &QPushButton::clicked, [sp] { sp->accept(); });
    connect(bb->button(QDialogButtonBox::Cancel), &QPushButton::clicked, [sp] { sp->cancel(); });
    w->setStyleSheet("QLabel { color: white }");
    mp_MainContainer->setCurrentIndex(1);
}

void ProjectExplorerPanel::slotCleanChanged(bool clean)
{
    
    m_clean = clean;
    QFileInfo project;
#ifdef DEBUG_TAB_ENABLED
    if (mp_beatsDebugTreeView->model()) project = ((BeatsProjectModel*)mp_beatsDebugTreeView->model())->projectFileFI();
#else
    if (mp_beatsModel) project = mp_beatsModel->projectFileFI();
#endif
    mp_Title->setText(tr("Project Explorer%1%2").arg(project.exists() ? " - " + project.fileName() : "").arg(m_clean ? "" : "*"));
}

void ProjectExplorerPanel::slotCurrentItemChanged(QListWidgetItem *current, QListWidgetItem *previous)
{
    emit sigCurrentItemChanged(current, previous);
}

void ProjectExplorerPanel::setProxyBeatsModel(SongFolderProxyModel * p_model)
{
    mp_beatsTreeView->setModel(p_model);
    if (p_model) {
        for (int i=1;i<=p_model->columnCount(); i++) { // hide all the columns but midi id
            if (AbstractTreeItem::LOOP_COUNT != i)
                mp_beatsTreeView->setColumnHidden(i,true);
        }
        mp_beatsTreeView->header()->resizeSection(0,200); // set up some reasonable sizes
        mp_beatsTreeView->header()->resizeSection(1,40);
    }
}

void ProjectExplorerPanel::setMidiColumn() {
    mp_beatsTreeView->setColumnHidden(AbstractTreeItem::LOOP_COUNT,!Settings::midiIdEnabled());
    if (Settings::midiIdEnabled()) {
        mp_beatsTreeView->header()->resizeSection(0,200);
        mp_beatsTreeView->header()->resizeSection(1,40);
    }
}

void ProjectExplorerPanel::setProxyBeatsRootIndex(QModelIndex rootIndex)
{
   mp_beatsTreeView->setRootIndex(rootIndex);
}

void ProjectExplorerPanel::setProxyBeatsSelectionModel(QItemSelectionModel *selectionModel)
{
   mp_beatsTreeView->setSelectionModel(selectionModel);
}

QItemSelectionModel * ProjectExplorerPanel::beatsSelectionModel()
{
   return mp_beatsTreeView->selectionModel();
}


void ProjectExplorerPanel::setDrmListModel(QAbstractItemModel * p_model)
{
   mp_drmListView->setModel(p_model);
}

QItemSelectionModel * ProjectExplorerPanel::drmListSelectionModel()
{
   return mp_drmListView->selectionModel();
}


void ProjectExplorerPanel::setBeatsModel(BeatsProjectModel *p_model, QUndoGroup* undo)
{
#ifdef DEBUG_TAB_ENABLED
   mp_beatsDebugTreeView->setModel(p_model);
#else
    mp_beatsModel = p_model;
#endif
    mp_viewUndoRedo->setGroup(undo);

   slotCleanChanged(m_clean);
}

void ProjectExplorerPanel::setBeatsDebugSelectionModel(QItemSelectionModel *selectionModel)
{
#ifdef DEBUG_TAB_ENABLED
   mp_beatsDebugTreeView->setSelectionModel(selectionModel);
#elif _MSC_VER
    selectionModel; // warning C4100: 'selectionModel' : unreferenced formal parameter
#endif
}

DrmListModel *ProjectExplorerPanel::drmListModel()
{
   return static_cast<DrmListModel *>(mp_drmListView->model());
}

void ProjectExplorerPanel::slotOnDoubleClick(const QModelIndex &index){

   Workspace w;
   if(!w.isValid()){
       qWarning() << "ProjectExplorerPanel::slotOnDoubleClick - ERROR 8 - There should not be any drumsets while workspace is invalid " << index.data().toString();
       return;
   }

   QString type = index.sibling(index.row(), DrmListModel::TYPE).data().toString();
   if(type == "user"){
       qDebug() << "ProjectExplorerPanel::slotOnDoubleClick - TODO - open" << index.data().toString();
       emit sigOpenDrm(index.data().toString(), index.sibling(index.row(), DrmListModel::ABSOLUTE_PATH).data().toString());

   } else if(type == "project"){
       qDebug() << "ProjectExplorerPanel::slotOnDoubleClick - TODO - Copy, then open" << index.data().toString();
       if (doesUserConsentToImportDrumset()){
           // 1 - import drumset
           QString dstPath = drmListModel()->importDrmFromPrj(index.data().toString());
           // 2 - create list entry immediately
           QFileInfo dstFI(dstPath);
           if(!dstFI.exists()){
               qWarning() << "ProjectExplorerPanel::slotOnDoubleClick - ERROR 1 - !dstFI.exists" << dstFI.absoluteFilePath();
               return;
           }
           if(!drmListModel()->addDrmToList(dstFI, "user")){
               qWarning() << "ProjectExplorerPanel::slotOnDoubleClick - ERROR 2 - addDrmToList returned false";
               return;
           }

           bool found = false;
           for(int row = 0; row < drmListModel()->rowCount() && !found; row++){
               found = drmListModel()->item(row, DrmListModel::ABSOLUTE_PATH)->data(Qt::DisplayRole).toString().toUpper() == dstFI.absoluteFilePath().toUpper();
               if(found){
                   auto ix = drmListModel()->index(row, DrmListModel::NAME);
                   mp_drmListView->selectionModel()->select(ix, QItemSelectionModel::Clear | QItemSelectionModel::SelectCurrent);
                   mp_drmListView->selectionModel()->setCurrentIndex(ix, QItemSelectionModel::SelectCurrent);

                   emit sigOpenDrm(ix.data().toString(), dstFI.absoluteFilePath());
               }
           }
           if(!found){
               qWarning() << "ProjectExplorerPanel::slotOnDoubleClick - ERROR 3 - inserted row not found";
           }

       }

   } else if(type == "default"){
       if (doesUserConsentToImportDrumset()){
           // 1 - import drumset
           QString dstPath = drmListModel()->importDrm(index.sibling(index.row(), DrmListModel::ABSOLUTE_PATH).data().toString(), true);
           // 2 - create list entry immediately
           QFileInfo dstFI(dstPath);
           if(!dstFI.exists()){
               qWarning() << "ProjectExplorerPanel::slotOnDoubleClick - ERROR 4 - !dstFI.exists" << dstFI.absoluteFilePath();
               return;
           }
           if(!drmListModel()->addDrmToList(dstFI, "user")){
               qWarning() << "ProjectExplorerPanel::slotOnDoubleClick - ERROR 5 - addDrmToList returned false";
               return;
           }

           // 3 - find the index of the added item
           for(int row = 0; row < drmListModel()->rowCount(); row++){
               if(drmListModel()->item(row, DrmListModel::ABSOLUTE_PATH)->data(Qt::DisplayRole).toString().toUpper() == dstFI.absoluteFilePath().toUpper()){
                   if(row >= drmListModel()->rowCount()){
                       qWarning() << "ProjectExplorerPanel::slotOnDoubleClick - ERROR 6 - inserted row not found";
                       return;
                   }

                   auto ix = drmListModel()->index(row, DrmListModel::NAME);
                   mp_drmListView->selectionModel()->select(ix, QItemSelectionModel::Clear | QItemSelectionModel::SelectCurrent);
                   mp_drmListView->selectionModel()->setCurrentIndex(ix, QItemSelectionModel::SelectCurrent);

                   emit sigOpenDrm(ix.data().toString(), dstFI.absoluteFilePath());

                   qDebug() << "ProjectExplorerPanel::slotOnDoubleClick - Success!";
                   break;
               }
           }

       }
   } else {
       const QString type2 = index.sibling(index.row(), AbstractTreeItem::Column::CPP_TYPE).data().toString();

       if (type2 == "SongFolderTreeItem" || type2 == "SongFileItem") {

           const QString name = index.sibling(index.row(), AbstractTreeItem::Column::NAME).data().toString();
           const int midiId = index.sibling(index.row(), AbstractTreeItem::Column::LOOP_COUNT).data().toInt();

           qDebug() << "ProjectExplorerPanel::slotOnDoubleClick" << type2 << name << midiId;
           nameAndIdDialog = new NameAndIdDialog(this, name, midiId);

           const QModelIndex nameIndex = index.sibling(index.row(), AbstractTreeItem::Column::NAME);
           const QModelIndex midiIdIndex = index.sibling(index.row(), AbstractTreeItem::Column::LOOP_COUNT);
           QAbstractItemModel *treeModel = mp_beatsTreeView->model();
           connect(nameAndIdDialog,&NameAndIdDialog::itemNameAndMidiIdChanged, [nameIndex,midiIdIndex,treeModel](QString name, int midiId){
               treeModel->setData(nameIndex, name);
               treeModel->setData(midiIdIndex, midiId);
           });

           nameAndIdDialog->exec();
           m_clean = false;
       } else {
           qWarning() << "ProjectExplorerPanel::slotOnDoubleClick - ERROR 7 - unknown type for " << index.data().toString() << "(" << type << ") or (" << type2 << ")";
       }
   }
}


void ProjectExplorerPanel::slotDrumListContextMenu(QPoint pnt)
{

    // index of the selected item for context menu
    QModelIndex index = mp_drmListView->indexAt(pnt);

    QMenu contextMenu(this);

    // drumsetListview model contaning the drumsets
    // verify that the position of the right click is on a drumset
    if (!index.isValid()) return;

    // If the selected drumset is opened
    bool isDrumSetOpen = drmListModel()->item(index.row(), DrmListModel::OPENED)->data(Qt::DisplayRole).toBool();
    mp_ContextCloseDrumsetAction->setEnabled(isDrumSetOpen);

    // Get the Path of the drumset and save it for when the action will be triggered
    m_currentContextSelectionPath =  drmListModel()->item(index.row(), DrmListModel::ABSOLUTE_PATH)->data(Qt::DisplayRole).toString();

    mp_ContextDeleteDrumsetAction->setEnabled(true);

    QList<QAction *> actionList;
    actionList.append(mp_ContextDeleteDrumsetAction);
    actionList.append(mp_ContextCloseDrumsetAction);
    QMenu::exec(actionList,mp_drmListView->mapToGlobal(pnt));
}



/**
 * @brief ProjectExplorerPanel::slotDrmDeleteRequestShortcut
 *
 * Makes sure the mp_drmListView has the focus before qctually deleting
 * This is done in order for the delete key being pressed somewhere else not to trigger a deletion
 */
void ProjectExplorerPanel::slotDrmDeleteRequestShortcut()
{
   // Make sure mp_drmListView has focus when delete is being performed through shortcut
   if(!mp_drmListView->hasFocus()){
      return;
   }
   slotDrmDeleteRequest();

}

/**
 * @brief ProjectExplorerPanel::slotDrmDeleteRequest
 *
 * Slot that validates that indexes are valid, then calls for the
 * deletion on the current drumset
 */
void ProjectExplorerPanel::slotDrmDeleteRequest()
{

   // if no project
   if(!drmListModel() || !drmListSelectionModel()){
      qWarning() << "ProjectExplorerPanel::slotDrmDeleteRequest - ERROR 1 - drmListModel or drmListSelectionModel don't exist";
      return;
   }

   // Make sure there is a project selected
   QModelIndex currentIndex = mp_drmListView->selectionModel()->currentIndex();
   QItemSelection selection = mp_drmListView->selectionModel()->selection();
   if(selection.isEmpty() || !currentIndex.isValid()){
      return;
   }

   drmListModel()->deleteDrmModal(currentIndex);
}

void ProjectExplorerPanel::slotTempoChanged(int i) {
    qDebug() << "slot tempo changed:" << i;

    slotCleanChanged(false);

}



