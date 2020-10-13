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
#include "model/tree/abstracttreeitem.h"
#include "beatfilewidget.h"
#include "newsongwidget.h"
#include "utils/wavfile.h"
#include "workspace/settings.h"
#include "workspace/workspace.h"
#include "workspace/contentlibrary.h"
#include "workspace/libcontent.h"
#include "model/filegraph/songfile.h"
#include "model/beatsmodelfiles.h"
#include "model/tree/project/beatsprojectmodel.h"
#include "dialogs/autopilotsettingsdialog.h"

#include <QSound>
#include <QDebug>

/*
 * INTERNAL CLASS DragButton
 */
DragButton::DragButton(QWidget* parent)
    : QPushButton(parent)
{
    // Timer used to differentiate Drag event from click event
    t.setSingleShot(true);
    connect(&t, SIGNAL(timeout()), this, SIGNAL(drag()));
}

void DragButton::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::RightButton) {
        showMenu();
        return;
    } else if (event->button() != Qt::LeftButton)
        return QPushButton::mousePressEvent(event);
    // Start timer in order to differentiate Drag event from click event
    t.start(200);
}

void DragButton::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() == Qt::RightButton) {
        menu()->hide();
        return;
    }
    // If button pressed for more than timeout, considered as dragged and not clicked
    if (!t.isActive())
        return;
    t.stop();
    emit clicked();
    //return QPushButton::mousePressEvent(event);
}

/*
 * MAIN CLASS BeatFileWidget
 */
 
void BeatFileWidget::drag()
{
    QModelIndex exportDirIndex = modelIndex().sibling(modelIndex().row(), AbstractTreeItem::EXPORT_DIR);
    QString path = dragClipboardDir().absoluteFilePath(exportDirIndex.data().toString());
    QFile file(path);
    file.remove();
    model()->clearAPSettingQueue();

    // Export to temp dir
    model()->setData(exportDirIndex, QVariant(dragClipboardDir().absolutePath()));

    if (!file.exists()){
        qWarning() << "BeatFileWidget::drag - ERROR 1 - unable to export file.";
        return;
    }

    QList<int> settings = model()->data(model()->index(modelIndex().row(), AbstractTreeItem::PLAY_AT_FOR
                                                       ,modelIndex().parent())).value<QList<int>>();
    MIDIPARSER_TrackType type = (MIDIPARSER_TrackType)model()->index(modelIndex().row(), AbstractTreeItem::TRACK_TYPE, modelIndex().parent()).data().toInt();
    if(type != INTRO_FILL && type != OUTRO_FILL)
    {
        if(settings.size() < 1){
            settings.push_back(m_PlayAt);
            if(type != TRANS_FILL){
                MIDIPARSER_MidiTrack data(modelIndex().sibling(modelIndex().row(), AbstractTreeItem::RAW_DATA).data().toByteArray());
                settings.push_back(data.nTick/data.barLength);//saves bar length in case of trans fill drop
                m_PlayFor  = data.nTick/data.barLength;
                QList<QVariant> set2 = QList<QVariant>() << m_PlayAt << m_PlayFor;
                model()->setData(model()->index(modelIndex().row(), AbstractTreeItem::PLAY_AT_FOR, modelIndex().parent())
                                 ,QVariant(set2),Qt::EditRole);
            }else{
                settings.push_back(m_PlayFor);
            }

        }
    }else{
        if(settings.size() < 1){
            settings.push_back(0);
            settings.push_back(0);
        }
    }

    model()->addAPSettingInQueue(settings);

    QDrag* drag = new QDrag(this);
    connect(drag, SIGNAL(actionChanged(Qt::DropAction)), this, SLOT(dragActionChanged(Qt::DropAction)));
    grabKeyboard();
    LambdaFilter f(this, [this](QObject*, QEvent* event) {
        switch (event->type()) {
        case QEvent::KeyPress :
        case QEvent::KeyRelease :
            dragActionChanged(QApplication::keyboardModifiers() & Qt::ControlModifier || QApplication::keyboardModifiers() & Qt::AltModifier ? Qt::CopyAction : Qt::TargetMoveAction);
            break;
        default:
            break;
        }
        return false;
    });
    drag->setPixmap(grab());
    drag->setMimeData(new QMimeData());
    QList<QUrl> urls;
    urls.append(QUrl::fromLocalFile(path));
    drag->mimeData()->setUrls(urls);
    drag->mimeData()->setParent(this);
    setAcceptDrops(false);
    m_dragging = true;
    dragActionChanged(Qt::IgnoreAction);
    auto _ = model()->undoMacro(tr("Dragging Song Part", "Undo Commands"));
    Qt::DropAction dropped = drag->exec(Qt::TargetMoveAction | Qt::CopyAction, Qt::TargetMoveAction);
    releaseKeyboard();
    m_dragging = false;
    setAcceptDrops(true);
    setCopyPasteState();
    if (!(dropped & Qt::MoveAction));
    else if (path == drag->mimeData()->urls().first().toLocalFile())
        deleteButtonClicked();
    else
    {   //swap with another fill
        path = drag->mimeData()->urls().first().toLocalFile();
        trackButtonClicked(path);
        AdjustAPText();
    }
    // remove exported file at the end of operation
    if (file.exists()){
        file.remove();
    }
}

/**
 * @brief BeatFileWidget::dragActionChanged
 * @param action
 *
 * Update the background of source button
 * cyan - file is being copied
 * red  - file is being moved / exchanged
 */
void BeatFileWidget::dragActionChanged(Qt::DropAction action)
{
    setStyleSheet(QString().sprintf("background-color:#%06X",
        ((action ? action : (QApplication::keyboardModifiers() & Qt::ControlModifier
        || QApplication::keyboardModifiers() & Qt::AltModifier)) & Qt::CopyAction)
        ? Settings::getColorDnDCopy()&0xffffff : Settings::getColorDnDWithdraw()&0xffffff)); // ? light cyan : light red
}

/**
 * @brief BeatFileWidget::dragEnterEvent
 * @param event
 *
 * Update background of destination button
 * red     - cannot exchange because destination file does not exist - N/A
 * yellow  - can exchange or replace
 */
void BeatFileWidget::dragEnterEvent(QDragEnterEvent *event)
{
    QList<QUrl> draggedUrls = event->mimeData()->urls();
    if (draggedUrls.size() != 1)
        return;

    QUrl draggedUrl = draggedUrls.first();
    if (!draggedUrl.isLocalFile()){
        return;
    }

    QFileInfo draggedFI(draggedUrl.toLocalFile());

    // make sure the extension is accepted
    QStringList acceptedExtensions = modelIndex().parent().sibling(modelIndex().parent().row(), AbstractTreeItem::CHILDREN_TYPE).data().toString().toUpper().split(",");
    if (!acceptedExtensions.contains(draggedFI.suffix().toUpper())){
        return;
    }

    event->setDropAction(Qt::LinkAction);
    event->accept();
    setStyleSheet(QString().sprintf("background-color:#%06X", (Settings::getColorDnDTarget()&0xffffff)));
}

void BeatFileWidget::dragLeaveEvent(QDragLeaveEvent *)
{
    setCopyPasteState();
}

void BeatFileWidget::dropEvent(QDropEvent *event)
{
    setCopyPasteState();

    if (event->isAccepted()){
        return;
    }
    QList<QUrl> dropUrls = event->mimeData()->urls();

    if(dropUrls.size() != 1){
        return;
    }
    QUrl dropUrl = dropUrls.first();
    if (!dropUrl.isLocalFile()){
        return;
    }

    QModelIndex exportDirIndex = modelIndex().sibling(modelIndex().row(), AbstractTreeItem::EXPORT_DIR);
    QString path = dropClipboardDir().absoluteFilePath(exportDirIndex.data().toString());
    QFile file(path);
    file.remove();

    // Export to temp dir
    model()->setData(exportDirIndex, QVariant(dropClipboardDir().absolutePath()));

    if (!file.exists()){
        qWarning() << "BeatFileWidget::dropEvent - ERROR 1 - unable to export file.";
        return;
    }
    MIDIPARSER_TrackType type = (MIDIPARSER_TrackType)model()->index(modelIndex().row(), AbstractTreeItem::TRACK_TYPE, modelIndex().parent()).data().toInt();
    QList<int> settings = model()->data(model()->index(modelIndex().row(), AbstractTreeItem::PLAY_AT_FOR
                                                   ,modelIndex().parent())).value<QList<int>>();
    if(type != INTRO_FILL && type != OUTRO_FILL)
    {
        if(settings.size() < 1){
            settings.push_back(m_PlayAt);
            settings.push_back(m_PlayFor);
        }
    }else{
        if(settings.size() < 1){
            settings.push_back(0);
            settings.push_back(0);
        }
    }
    model()->addAPSettingInQueue(settings);

    if (trackButtonClicked(dropUrl.toLocalFile()))
    {
        QList<QUrl> urls;
        urls.append(QUrl::fromLocalFile(path));
        const_cast<QMimeData*>(event->mimeData())->setUrls(urls);
        event->accept();
        AdjustAPText();
    } 
}

void BeatFileWidget::enterEvent(QEvent*)
{
    // Retrieve path used to copy
    QModelIndex exportDirIndex = modelIndex().sibling(modelIndex().row(), AbstractTreeItem::EXPORT_DIR);
    QString path = copyClipboardDir().absoluteFilePath(exportDirIndex.data().toString());

    QList<QUrl> pasteUrls = QApplication::clipboard()->mimeData()->urls();
    setCopyPasteState(path, pasteUrls.size() == 1 ? pasteUrls.first().toLocalFile() : nullptr);
}

void BeatFileWidget::leaveEvent(QEvent*)
{
    if (!m_dragging)
        setCopyPasteState();
}

bool BeatFileWidget::copy()
{
    QModelIndex exportDirIndex = modelIndex().sibling(modelIndex().row(), AbstractTreeItem::EXPORT_DIR);
    QString path = copyClipboardDir().absoluteFilePath(exportDirIndex.data().toString());
    QFile file(path);
    file.remove();

    // Export to temp dir
    model()->setData(exportDirIndex, QVariant(copyClipboardDir().absolutePath()));

    if (!file.exists()){
        qWarning() << "BeatFileWidget::copy - ERROR 1 - Unable to export " + path;
        return false;
    }
    QList<QUrl> urls;
    urls.append(QUrl::fromLocalFile(path));
    QMimeData* m = new QMimeData();
    m->setUrls(urls);
    QClipboard * c = QApplication::clipboard();
    c->clear();
    c->setMimeData(m);
    auto v = false;
    auto pos = mapFromGlobal(QCursor::pos());
    foreach (auto rc, visibleRegion().rects()) {
        v |= rc.contains(pos);
        if (v){
            break;
        }
    }
    if (v){
        setCopyPasteState(path, path);
    }
    return true;
}

bool BeatFileWidget::paste()
{
    QList<QUrl> pasteUrls = QApplication::clipboard()->mimeData()->urls();
    if(pasteUrls.size() != 1){
        return false;
    }
    QUrl pasteUrl = pasteUrls.first();
    if (!pasteUrl.isLocalFile()){
        return false;
    }

    QFileInfo pasteFI(pasteUrl.toLocalFile());

    // make sure the extension is accepted
    QStringList acceptedExtensions = modelIndex().parent().sibling(modelIndex().parent().row(), AbstractTreeItem::CHILDREN_TYPE).data().toString().toUpper().split(",");
    if (!acceptedExtensions.contains(pasteFI.suffix().toUpper())){
        return false;
    }

    // Export original file for setCopyPasteState() that requires files to exist
    QModelIndex exportDirIndex = modelIndex().sibling(modelIndex().row(), AbstractTreeItem::EXPORT_DIR);
    QString path = pasteClipboardDir().absoluteFilePath(exportDirIndex.data().toString());
    QFile file(path);
    file.remove();

    // Export to temp dir
    model()->setData(exportDirIndex, QVariant(pasteClipboardDir().absolutePath()));

    if (!file.exists()){
        qWarning() << "BeatFileWidget::paste - ERROR 1 - Unable to export " + path;
        return false;
    }

    // perform paste operation
    if (!trackButtonClicked(pasteFI.absoluteFilePath())){
        qWarning() << "BeatFileWidget::paste - ERROR 2 - unable to paste " << pasteFI.absoluteFilePath();
        return false;
    }

    auto v = false;
    auto pos = mapFromGlobal(QCursor::pos());
    foreach (auto rc, visibleRegion().rects()) {
        v |= rc.contains(pos);
        if (v){
            break;
        }
    }
    if (v){
        setCopyPasteState(path, path);
    }
    return true;
}

void BeatFileWidget::exportMIDI(const QString& destination)
{
    emit sigSubWidgetClicked(modelIndex());

    auto ix = modelIndex();
    // 1 - select location where to export
    auto exported = ix.sibling(ix.row(), AbstractTreeItem::EXPORT_DIR).data().toString();
    auto filename = exported;
    if (!filename.right(4).compare("." BMFILES_SONG_TRACK_EXTENSION)) {
        filename = filename.left(filename.size()-3) + BMFILES_MIDI_EXTENSION;
    } else {
        filename += "." BMFILES_MIDI_EXTENSION;
    }
    filename = !destination.isEmpty() ? destination : QFileDialog::getSaveFileName(
                this,
                tr("Export MIDI"),
                QDir(Workspace().userLibrary()->libMidiSources()->currentPath()).absoluteFilePath(filename),
                BMFILES_MIDI_DIALOG_FILTER);

    if (filename.isEmpty()) {
        return;
    }

    // 2 - export MIDI
    MIDIPARSER_MidiTrack data(ix.sibling(ix.row(), AbstractTreeItem::RAW_DATA).data().toByteArray());
    if (!data.event.size())
        return;
    data.write_file(filename.toStdString());
}

void BeatFileWidget::openAutopilotSettings()
{
    int type = model()->index(modelIndex().row(),
                              AbstractTreeItem::TRACK_TYPE,
                              modelIndex().parent()).data().toInt();

    MIDIPARSER_MidiTrack data(modelIndex().sibling(modelIndex().row(), AbstractTreeItem::RAW_DATA).data().toByteArray());
    int sigNum = data.timeSigNum;


    if(!sigNum){
        qWarning() << "Warning - Time signature is Zero.";
        return;
    }

    QList<QVariant> settings = model()->data(model()->index(modelIndex().row(), AbstractTreeItem::PLAY_AT_FOR,modelIndex().parent())).toList();

    if(settings.size() >= 2)
    {
        m_PlayAt = settings.at(0).toInt();
        m_PlayFor = settings.at(1).toInt();
    }


    AutoPilotSettingsDialog *apDialog;
    apDialog = new AutoPilotSettingsDialog((MIDIPARSER_TrackType)type, sigNum, m_PlayFor, m_PlayAt);

    if(apDialog->exec()){
        m_PlayFor = apDialog->playFor();
        m_PlayAt  = apDialog->playAt();
    }

    settings = QList<QVariant>() << m_PlayAt << m_PlayFor;
    model()->setData(model()->index(modelIndex().row(), AbstractTreeItem::PLAY_AT_FOR, modelIndex().parent())
                     ,QVariant(settings),Qt::EditRole);
}

/**
 * @brief BeatFileWidget::setCopyPasteState
 * @param file
 * @param paste
 *
 * Updates the tooltip
 * Updates the border style according to the current file and content of the clipboard
 * red    - source file does not exist                     - cannot copy - cannot paste (because cannot swap files)
 * white  - Clipboard empty and source file exists         - may copy    - cannot paste
 * gray   - TBD
 * orange - TBD
 *
 */
void BeatFileWidget::setCopyPasteState(const QString& file, const QString& paste)
{
    auto can_copy = true;
    auto can_paste = false;
    if (file.isEmpty() && paste.isEmpty())
    {
        setStyleSheet("QPushButton::menu-indicator{image:url(:/drawable/1x1.png)}");
        mp_FileButton->setToolTip(tr("Click to replace the file"));
        return;
    }
    bool bp = QFile(paste).exists();
    bool eq = paste == file;

    // verify if extension accepted
    QStringList acceptedExtensions = modelIndex().parent().sibling(modelIndex().parent().row(), AbstractTreeItem::CHILDREN_TYPE).data().toString().toUpper().split(",");
    bool ok = !bp || acceptedExtensions.contains(QFileInfo(paste).suffix().toUpper());

    // Create tooltip
    // If there is a modifier, display the full path to the file in the library
    // Otherwise, simply display the temp file name
    QString fileName;
    if(QApplication::keyboardModifiers() == Qt::NoModifier){
       fileName = QFileInfo(file).fileName();
    } else {
       fileName = modelIndex().sibling(modelIndex().row(), AbstractTreeItem::ABSOLUTE_PATH).data().toString();
    }

    QString midiInfo = "";
    QString cpp = modelIndex().sibling(modelIndex().row(), AbstractTreeItem::CPP_TYPE).data().toString();
    if (!(0 == cpp.compare("EffectPtrItem"))) {
        MIDIPARSER_MidiTrack data(modelIndex().sibling(modelIndex().row(), AbstractTreeItem::RAW_DATA).data().toByteArray());
        midiInfo = "Time Sig: "+QString::number(data.timeSigNum)+"/"+QString::number(data.timeSigDen)+
                ", Length:"+QString::number(int(double(data.nTick)/data.barLength+.5))+" m, Tempo:"+QString::number(data.bpm);
    }
    QString tt = fileName +"\n***\n"+midiInfo+"\n("+tr("Click to replace the file")+")";
    if (bp && eq) {
        tt += "\n("+tr("Already copied")+")";
        can_copy = false;
    }
    if (!ok)
        tt += "\n("+tr("Incompatible copied")+")";
    if (!eq)
    {
#ifdef Q_OS_WIN
        auto mod = "Ctrl+";
#else
        auto mod = "⌘";
#endif
        tt += "\n---"+tr("or")+"---";
        tt += "\n" + tr("Press %1C to Copy").arg(mod);
        if (bp && ok) {
            tt += "\n" + tr("Press %1V to Paste").arg(mod);
            can_paste = true;
        }
    }
    mp_FileButton->setToolTip(tt);
    setStyleSheet("border-color:"+QString(bp ? eq ? "cyan" : ok ? "yellow" : "gray" : "white"));

    if (mp_acCopy)
        mp_acCopy->setDisabled(can_copy?false:true);
    if (mp_acPaste)
        mp_acPaste->setDisabled(can_paste?false:true);
}

BeatFileWidget::BeatFileWidget(BeatsProjectModel* p_Model, QWidget* parent)
    : SongFolderViewItem(p_Model, parent)
    , m_dragging(false)
{
   setAcceptDrops(true);
   m_selectedStyleSheet = STYLE_NONE;

   setStyleSheet("QPushButton::menu-indicator{image:url(:/drawable/1x1.png)}");

   mp_FileButton = new DragButton(this);
   mp_FileButton->setObjectName(QStringLiteral("beatFileButton"));
   mp_FileButton->setText(tr("Filename"));
   mp_FileButton->setToolTip(tr("Click to replace the file"));

   mp_DeleteButton = new QPushButton(this);
   mp_DeleteButton->setObjectName(QStringLiteral("deleteButton"));
   mp_DeleteButton->setToolTip(tr("Remove the file"));

   mp_PlayButton = new QPushButton(this);
   mp_PlayButton->setObjectName(QStringLiteral("playButton"));
   mp_PlayButton->setToolTip(tr("Play this track"));

   mp_APBox = new QCheckBox("AP",this);
   mp_APBox->setChecked(false);
   mp_APBox->setObjectName(QStringLiteral("APBox"));
   mp_APBox->setToolTip(tr("Turn On or Off the AutoPilot for this track"));
   mp_APBox->setLayoutDirection(Qt::RightToLeft);
   rightl->addWidget(mp_APBox);

   APBar = new QLineEdit(this);
   APBar->setValidator( new QIntValidator(1, 99, this) );
   APText = new QLabel(this);
   APText->setObjectName(QStringLiteral("APText1"));
   APBar->setText("1");
   PostText = new QLabel(this);
   PostText->setText("Bars");
   PostText->setObjectName(QStringLiteral("APText2"));
   APText->setText("Trigger at bar");
   leftl->addWidget(APText);
   leftl->addWidget(APBar);
   leftl->addWidget(PostText);
   leftl->addStretch();//to group bar information to the left and the checkbox to the right
   leftl->addLayout(rightl);
   mp_FileButton->setLayout(leftl);
   mp_APBox->hide();
   APText->hide();
   APBar->hide();
   PostText->hide();


   connect(mp_DeleteButton, SIGNAL(clicked()), this, SLOT(deleteButtonClicked()));
   connect(mp_FileButton  , SIGNAL(clicked()), this, SLOT( trackButtonClicked()));
   connect(mp_FileButton  , SIGNAL(   drag()), this, SLOT(               drag()));
   connect(mp_PlayButton  , SIGNAL(clicked()), this, SLOT(  playButtonClicked()));
   connect(mp_APBox  , SIGNAL(clicked()), this, SLOT(  APBoxStatusChanged()));
   connect(APBar, SIGNAL(textChanged(const QString &)), this, SLOT(ApValueChanged()));

   connect(p_Model, &BeatsProjectModel::endEditMidi, this, &BeatFileWidget::endEditMidi);
}

void BeatFileWidget::endEditMidi(const QByteArray& data)
{
    if (m_editingTrackData.isEmpty())
        return;
    QByteArray prev;
    prev.swap(m_editingTrackData);
    if (data.isEmpty() || prev == data)
        return;
    auto ix = modelIndex();
    qDebug() << "end edit midi:" << data.length();
    model()->changeSongFile(ix.sibling(ix.row(), AbstractTreeItem::RAW_DATA), data);
}

void BeatFileWidget::setLabel(QString const& label)
{
   // Configure widgets
   mp_FileButton->setText( label );
}

void BeatFileWidget::populate(QModelIndex const& modelIndex)
{
   // Delete all previous children widgets (if any)
   for(int i = 0; i < mp_ChildrenItems->size(); i++){
      delete mp_ChildrenItems->at(i);
   }

   mp_ChildrenItems->clear();

   // copy new model index
   setModelIndex(modelIndex);

   // Populate self's data
   QVariant labelVatiant = modelIndex.data();
   QString label = labelVatiant.toString();
   MIDIPARSER_TrackType type = (MIDIPARSER_TrackType)model()->index(modelIndex.row(), AbstractTreeItem::TRACK_TYPE, modelIndex.parent()).data().toInt();
   if(type != INTRO_FILL && type != OUTRO_FILL)
   {
       mp_FileButton->setText( label +"\n\n");
   }else{
       mp_FileButton->setText( label);
   }



   if (modelIndex.sibling(modelIndex.row(), AbstractTreeItem::PLAYING).data().toBool() == true) {

      // watch out for bug https://bugreports.qt-project.org/browse/QTBUG-20292
      if(m_selectedStyleSheet != STYLE_PLAYING){
         m_selectedStyleSheet = STYLE_PLAYING;
         mp_FileButton->setStyleSheet("#beatFileButton{"
                                      "color: white;}"
                                      "#beatFileButton:pressed{"
                                      "color: black;}");
      }
   } else {

      // Avoid setting the stylesheet twice the same
      // watch out for bug https://bugreports.qt-project.org/browse/QTBUG-20292
      if(m_selectedStyleSheet != STYLE_NONE){
         m_selectedStyleSheet = STYLE_NONE;
         mp_FileButton->setStyleSheet("QPushButton::menu-indicator{image:url(:/drawable/1x1.png)}");
      }
   }

   QList<QVariant> apSettings = model()->index(modelIndex.row(), AbstractTreeItem::PLAY_AT_FOR, modelIndex.parent()).data().toList();
   if(apSettings.size() >= 2){
       m_PlayAt = apSettings.at(0).toInt();
       m_PlayFor = apSettings.at(1).toInt();
       isfiniteMain = (m_PlayAt > 0)?true:false;
   }

    QString cpp = model()->index(modelIndex.row(), AbstractTreeItem::CPP_TYPE, modelIndex.parent()).data().toString();
    auto m = new TemporaryMenu(mp_FileButton);
#ifdef Q_OS_WIN
    QString mod = "Ctrl+";
#else
    QString mod = "⌘";
#endif

    m->addAction(tr("Play"), this, SLOT(playButtonClicked()));
    if (!(0 == cpp.compare("EffectPtrItem"))) { // we don't want this for WAV columns (acc hits)
        m->addSeparator();
        m->addAction(tr("Edit..."), this, SLOT(edit()))->setVisible(modelIndex.parent().row() != 3);
        m->addSeparator();
        mp_acCopy = m->addAction(tr("Copy") + "\t"+mod+"C", this, SLOT(copy()));
        mp_acPaste = m->addAction(tr("Paste") + "\t"+mod+"V", this, SLOT(paste()));
        m->addSeparator();

        m->addAction(tr("Export MIDI file..."), this, SLOT(exportMIDI()));
    } else {
        mp_acCopy = new QAction(m);
        mp_acPaste = new QAction(m);
    }
    mp_FileButton->setMenu(m);

}

void BeatFileWidget::updateMinimumSize()
{
   // Recursively update all children first
   for(int i = 0; i < mp_ChildrenItems->size(); i++) {
      mp_ChildrenItems->at(i)->updateMinimumSize();
   }

   setMinimumWidth(50);
}

void BeatFileWidget::updateLayout()
{

   mp_FileButton->setGeometry( rect().x(), rect().y()+10, rect().width(),  rect().height()-10 );
   // 5 px from upper right corner, 12 px size.
   mp_DeleteButton->setGeometry( rect().width()-17, 12 , 12, 12);
   // Relative to parent
   // Nothing to do since contained in Layout

   // 5 px from upper left corner, 15 px size.
   mp_PlayButton->setGeometry( 4, 12 , 15, 15);

   APBar->setFixedSize(24,17);
   APBar->setAlignment(Qt::AlignCenter);
   leftl->setAlignment(Qt::AlignBottom);
}

void BeatFileWidget::dataChanged(const QModelIndex &left, const QModelIndex &right)
{

   for(int column = left.column(); column <= right.column(); column++){
      QModelIndex index = left.sibling(left.row(), column);
      if(!index.isValid()){
         continue;
      }

      QString label;
      if(column == AbstractTreeItem::NAME){
          MIDIPARSER_TrackType type = (MIDIPARSER_TrackType)model()->index(index.row(), AbstractTreeItem::TRACK_TYPE, index.parent()).data().toInt();
          if(type != INTRO_FILL && type != OUTRO_FILL)
          {
              label = index.data().toString()+"\n\n";
          }else{
             label = index.data().toString();
          }
      }

      switch(column){
         case AbstractTreeItem::NAME:
                mp_FileButton->setText( label);
            break;
         case AbstractTreeItem::PLAYING:
            if (index.data().toBool() == true) {
               // Avoid setting the stylesheet twice the same
               // watch out for bug https://bugreports.qt-project.org/browse/QTBUG-20292
               if(m_selectedStyleSheet != STYLE_PLAYING){
                  m_selectedStyleSheet = STYLE_PLAYING;
                  mp_FileButton->setStyleSheet("#beatFileButton{"
                                               "color: white;}"
                                               "#beatFileButton:pressed{"
                                               "color: white;}");
               }
            } else {
               // Avoid setting the stylesheet twice the same
               // watch out for bug https://bugreports.qt-project.org/browse/QTBUG-20292
               if(m_selectedStyleSheet != STYLE_NONE){
                  m_selectedStyleSheet = STYLE_NONE;
                  mp_FileButton->setStyleSheet("QPushButton::menu-indicator{image:url(:/drawable/1x1.png)}"
                                               "#APText1{"
                                               " color : gray;}"
                                               "#APText2{"
                                               " color : gray;}");
               }
            }
            break;
         default:
            // Do nothing
            break;
      }
   }
}

int BeatFileWidget::headerColumnWidth(int /*columnIndex*/)
{
   return 0;
}

void BeatFileWidget::deleteButtonClicked()
{
   emit sigSubWidgetClicked(modelIndex());
   model()->deleteSongFile(modelIndex());
}


/**
 * @brief BeatFileWidget::trackButtonClicked
 * @param dropFileName
 * @return
 *
 * trackButtonClicked can be used for 3 reasons
 * (a) Drop/file in case of drag and drop
 *     in this case, file name is passed as parameter
 * (b) Fix content in case track does not point to valid file (when key modifier + clicked)(only applies to midi files)
 *     in this case, file name is using last browsed location
 * (c) Browse to select new file (when clicked)
 *     in this case, user is asked to browse to locate file
 */
bool BeatFileWidget::trackButtonClicked(const QString& dropFileName)
{
    Workspace w;
    emit sigSubWidgetClicked(modelIndex());

    // 1 - Determine fileName

    QString fileName;

    if(!dropFileName.isEmpty()){
        // 1.1 (a) Drop/file in case of drag and drop
        fileName = dropFileName;
    } else if (model()->index(modelIndex().parent().row(), AbstractTreeItem::CHILDREN_TYPE, modelIndex().parent().parent()).data().toString().compare(BMFILES_WAVE_EXTENSION, Qt::CaseInsensitive) == 0){
        // 1.2 (c) Browse to select new file (wave)
        fileName = QFileDialog::getOpenFileName(
                    this,
                    tr("Open Wave"),
                    w.userLibrary()->libWaveSources()->currentPath(),
                    BMFILES_WAVE_DIALOG_FILTER);

        // Memorize last browse location
        if(!fileName.isEmpty()){
            QFileInfo newPath(fileName);
            w.userLibrary()->libWaveSources()->setCurrentPath(newPath.absolutePath());
        }
    } else {

        // Retrieve location of current midi file
        QModelIndex curFilePathIndex = modelIndex().sibling(modelIndex().row(), AbstractTreeItem::ABSOLUTE_PATH);
        QFileInfo curFileFI(curFilePathIndex.data().toString());

        // 1.4 (b) Try fixing content from various locations
        if (QApplication::keyboardModifiers()){
            if(curFileFI.isFile() && curFileFI.exists()){
                // Use Workspace location first
                fileName = curFileFI.absoluteFilePath();
            } else {
                // Otherwise, try in last browsed path
                QDir borwseDir(w.userLibrary()->libMidiSources()->currentPath());
                QFileInfo browseFI(borwseDir.absoluteFilePath(curFileFI.fileName()));
                if(browseFI.isFile() && browseFI.exists()){
                    fileName = browseFI.absoluteFilePath();
                }
            }
        }

        if(fileName.isEmpty()){
            // 1.5 (c) Browse to select new file

            // If current file does not exist, use last browse location as default location
            if(!curFileFI.isFile() || !curFileFI.exists()){
                // If file does not exist verify if file exists in last browsed location
                curFileFI = QFileInfo(QDir(w.userLibrary()->libMidiSources()->currentPath()).absoluteFilePath(curFileFI.fileName()));
                if(!curFileFI.isFile() || !curFileFI.exists()){
                    // If not Use last browsed location without any file name
                    curFileFI = QFileInfo(curFileFI.absolutePath());
                }
            }
            fileName = QFileDialog::getOpenFileName(this, tr("Open Midi"), curFileFI.absoluteFilePath(), BMFILES_MIDI_TRACK_DIALOG_FILTER);


            // Memorize last browse location
            if(!fileName.isEmpty()){
                QFileInfo newPath(fileName);
                w.userLibrary()->libMidiSources()->setCurrentPath(newPath.absolutePath());
            }
        }
    }


    // 2 - Validate and apply selected file if valid
    if(!fileName.isEmpty()){
        // 2.1 Validate Wave File Format
        QFileInfo fi(fileName);
        if(fi.completeSuffix().compare(BMFILES_WAVE_EXTENSION,Qt::CaseInsensitive) == 0){
            // Limit the wave file size
            if(fi.size() > MAX_ACCENT_HIT_FILE_SIZE){
                QMessageBox::critical(this, tr("Replacing file"), tr("Accent hit file exceeds file size limit of %1 kB (%2 kB)")
                            .arg(((double) MAX_ACCENT_HIT_FILE_SIZE) / 1024.0, 0, 'f', 1)
                            .arg(((double) fi.size()) / 1024.0, 0, 'f', 1));
                return false;
            }

            WavFile wavFile;
            if(!wavFile.open(fileName)){
                QMessageBox::critical(this, tr("Replacing file"), tr("Unable to open or parse file %1\nWave file either cannot be read or has an unsupported format\nMust be 16-24 bits/sample, 44.1 kHz, mono/stereo standard WAVE").arg(fi.absoluteFilePath()));
                return false;
            }
            if(!wavFile.isFormatSupported()){
                QMessageBox::critical(this, tr("Replacing file"), tr("Unsupported Wave File format for %1\nMust be 16-24 bits/sample, 44.1 kHz, mono/stereo standard wave").arg(fi.absoluteFilePath()));
                wavFile.close();
                return false;
            }
            wavFile.close();
        }

        // 2.2 Try applying file
        QModelIndex newFileIndex = model()->index(modelIndex().row(), AbstractTreeItem::NAME, modelIndex().parent());
        model()->changeSongFile(newFileIndex, fileName);

        // 2.3 Verify if there was a parsing error
        QStringList errorList = newFileIndex.sibling(newFileIndex.row(), AbstractTreeItem::ERROR_MSG).data().toStringList();
        if(!errorList.isEmpty()){
            // 2.3.1. Create Error Log
            QString errorLog;
            for(int k = 0; k < errorList.count(); k++){
                errorLog += QString("%1 - %2\n").arg(k+1).arg(errorList.at(k));
            }

            // 2.3.2. Report Error
            QMessageBox::critical(this, tr("Replacing file"), tr("Error while parsing midi file %1\nThe error log is:\n\n%2").arg(fileName).arg(errorLog));
            return false;
        }
    }

    return true;
}
void BeatFileWidget::ApValueChanged(bool off){

    MIDIPARSER_TrackType trackType = (MIDIPARSER_TrackType)model()->index(modelIndex().row(), AbstractTreeItem::TRACK_TYPE, modelIndex().parent()).data().toInt();
    if(APBar->text().toInt() < 1 && !APBar->text().isEmpty()){
        if(m_dropped){
            mp_APBox->setChecked(false);
            APBoxStatusChanged();
        }else{
            APBar->setText("1");
        }
    }else if(trackType == TRANS_FILL){
        if(m_dropped){
            mp_APBox->setChecked(true);
            APBoxStatusChanged();
        }
         m_PlayFor = (!off)?APBar->text().toInt():0;
         QList<QVariant> settings = QList<QVariant>() << m_PlayAt << m_PlayFor;
         model()->setData(model()->index(modelIndex().row(), AbstractTreeItem::PLAY_AT_FOR, modelIndex().parent())
                          ,QVariant(settings),Qt::EditRole);
    }else if(!newFill){
        if(m_dropped){
            mp_APBox->setChecked(true);
            APBoxStatusChanged();
        }
        //set new ap value
        MIDIPARSER_MidiTrack data(modelIndex().sibling(modelIndex().row(), AbstractTreeItem::RAW_DATA).data().toByteArray());
        m_PlayAt = (!off)?(APBar->text().toInt()-1)*data.timeSigNum+1:0;


        QList<QVariant> settings = QList<QVariant>() << m_PlayAt << m_PlayFor;
        model()->setData(model()->index(modelIndex().row(), AbstractTreeItem::PLAY_AT_FOR, modelIndex().parent())
                         ,QVariant(settings),Qt::EditRole);
    }
}
void BeatFileWidget::parentAPBoxStatusChanged(int sigNum, bool hasMain){

    updateAPText(false, hasMain, false,sigNum,false);
}

void BeatFileWidget::APBoxStatusChanged(){

    MIDIPARSER_TrackType trackType = (MIDIPARSER_TrackType)model()->index(modelIndex().row(), AbstractTreeItem::TRACK_TYPE, modelIndex().parent()).data().toInt();
    if(mp_APBox->isChecked()){
        APText->show();
        APBar->show();
        if(!m_dropped){
            ApValueChanged(false);
        }

        if (trackType == MAIN_DRUM_LOOP || trackType == TRANS_FILL){
            if(trackType == MAIN_DRUM_LOOP){
                isfiniteMain = true;
            }
            emit sigPartEmpty(false);
            emit sigMainAPUpdated();
        }
    }else{
        if(!m_dropped){
            ApValueChanged(true);
        }
        if(trackType == MAIN_DRUM_LOOP || trackType == TRANS_FILL){
            if(trackType == MAIN_DRUM_LOOP){
                isfiniteMain = false;
            }
            emit sigPartEmpty(true);
            emit sigMainAPUpdated();
        }else{
            APBar->hide();
            APText->hide();
        }
    }
    model()->setProjectDirty();
}

void BeatFileWidget::playButtonClicked()
{
   if (model()->index(modelIndex().parent().row(), AbstractTreeItem::CHILDREN_TYPE, modelIndex().parent().parent()).data().toString().compare(BMFILES_WAVE_EXTENSION, Qt::CaseInsensitive) == 0){

      // Retrieve Wave file path
      QString waveFilePath = modelIndex().sibling(modelIndex().row(), AbstractTreeItem::ABSOLUTE_PATH).data().toString();
      QSound::play(waveFilePath);

      // Emit signal after sound is played in order to be more responsive on Audio
      emit sigSubWidgetClicked(modelIndex());

   } else {
      // Emit first before track gets disabled due to audio playing
      emit sigSubWidgetClicked(modelIndex());

      // Retrieve Track information
      QByteArray trackData = modelIndex().sibling(modelIndex().row(), AbstractTreeItem::RAW_DATA).data().toByteArray();

      // Notify track index in case this is a drum fill
      emit sigSelectTrack(trackData, modelIndex().row());

   }
}

void BeatFileWidget::slotSetPlayerEnabled(bool enabled)
{
   mp_PlayButton->setEnabled(enabled);
}

void BeatFileWidget::edit()
{
    auto m = model();
    auto ix = modelIndex(); ix = ix.sibling(ix.row(), AbstractTreeItem::RAW_DATA);
    // Retrieve Track information
    auto trackData = ix.data().toByteArray();

    // Move selection (e.g. to set correct BPM for player)
    auto song = ix.parent().parent().parent();
    m->moveSelection(song);

    // Begin edit
    qDebug() << "begin edit midi" << trackData.length();
    emit m->beginEditMidi(tr("Editing %2 of %1", "Context: Editing '*Song Name* - *Part Name*'")
        .arg(song.data().toString()).arg(m->songFileName(ix)), m_editingTrackData = trackData);
}

void BeatFileWidget::updateAPText(bool hasTrans, bool hasMain, bool hasOutro, int sigNum, bool lastpart){

    MIDIPARSER_TrackType trackType = (MIDIPARSER_TrackType)model()->index(modelIndex().row(), AbstractTreeItem::TRACK_TYPE, modelIndex().parent()).data().toInt();
    bool songapOn = model()->data(modelIndex().parent().parent().parent().sibling(modelIndex().parent().parent().parent().row(), AbstractTreeItem::AUTOPILOT_ON)).toBool();

    if(trackType != INTRO_FILL && trackType != OUTRO_FILL && songapOn){
        mp_APBox->show();
        APBar->show();
        APText->show();
        PostText->hide();
        if(trackType == MAIN_DRUM_LOOP){
            if(m_PlayAt > 0 ||mp_APBox->isChecked()){
              mp_APBox->setChecked(true);
              if(!newFill){
                  APBar->setText(QString::number((m_PlayAt-1)/sigNum+1));
                 // ApValueChanged() not necessary here because this fires a signal
              }
              isfiniteMain = true;
             }
            if(!hasMain && !isfiniteMain){
                APText->setText("Loop infinitely");
                APText->show();
                APBar->hide();
            }else if(hasTrans){
                APText->setText("Transition Fill at Bar");
                APText->show();
                APBar->show();
            }else{
                if(lastpart){
                    if(hasOutro){
                        APText->setText("Play Outro Fill At Bar");
                        APText->show();
                        APBar->show();
                    }else{
                        APText->setText("End After");
                        PostText->show();
                        APText->show();
                        APBar->show();
                    }
                }else{
                    APText->setText("Play For");
                    PostText->show();
                    APText->show();
                    APBar->show();
                }
            }
         TransFill = hasTrans;
        }else if(trackType == TRANS_FILL){
            if(m_PlayFor > 0 || mp_APBox->isChecked()){
                if(!newFill && !hasMain){
                    APBar->hide();
                    mp_APBox->hide();
                    APText->setText("Manual Trigger Only");
                    isfiniteMain = false;
                }else if (hasMain){
                    mp_APBox->setChecked(true);
                    APText->setText("Play For");
                    PostText->show();
                    if(m_PlayFor > 0){
                        APBar->setText(QString::number(m_PlayFor));
                    }
                    mp_APBox->show();
                    APBar->show();
                    APText->show();
                    isfiniteMain = true;
                }
            }else{
                if(hasMain){
                    isfiniteMain = true;
                    mp_APBox->show();
                    APText->setText("Manual Trigger Only");
                    APBar->hide();
                    emit sigPartEmpty(true);
                }else{
                    if(!newFill){
                       mp_APBox->hide();
                    }
                    APBar->hide();
                    APText->show();
                    APText->setText("Manual Trigger Only");
                    isfiniteMain = false;
                }
            }
        }else{
            if(!newFill && m_PlayAt > 0){
                     mp_APBox->setChecked(true);
                     if(sigNum == 0){//when there is no main fill loaded
                         MIDIPARSER_MidiTrack data(modelIndex().sibling(modelIndex().row(), AbstractTreeItem::RAW_DATA).data().toByteArray());
                         sigNum = data.timeSigNum;
                     }
                        APBar->setText(QString::number((m_PlayAt-1)/sigNum+1));

                 }else{
                     APBar->hide();
                     APText->hide();
                 }
        }
    }else{
        mp_APBox->hide();
        APBar->hide();
        APText->hide();
        PostText->hide();
    }

    newFill =false;
}
bool BeatFileWidget::finiteMain(){    
 return isfiniteMain;
}

void BeatFileWidget::setAsNew(bool value){
        newFill = value;
}

bool BeatFileWidget::isNew(){
    return newFill;
}

void BeatFileWidget::AdjustAPText()
{
    QList<int> settings = model()->getAPSettings(modelIndex());
    MIDIPARSER_TrackType trackType = (MIDIPARSER_TrackType)model()->index(modelIndex().row(), AbstractTreeItem::TRACK_TYPE, modelIndex().parent()).data().toInt();
    bool off = !mp_APBox->isChecked();
    if(settings.size() > 0 && trackType != INTRO_FILL && trackType!= OUTRO_FILL){
        MIDIPARSER_MidiTrack data(modelIndex().sibling(modelIndex().row(), AbstractTreeItem::RAW_DATA).data().toByteArray());
        int sigNum = data.timeSigNum;
        m_PlayAt = settings.at(0);
        m_PlayFor = settings.at(1);

        QString textvalue = (m_PlayAt > 0)?QString::number((m_PlayAt-1)/sigNum+1):QString::number(m_PlayFor);
        m_dropped = true;//this flag is for the slot signaled on the next line
        if(APBar->text() != textvalue){
            APBar->setText(textvalue);
        }else if(off && m_PlayAt > 0){//if the incomming On fill had the default value
            mp_APBox->setChecked(true);
            APBoxStatusChanged();
        }
        m_dropped = false;

    }else{
        m_PlayAt = 0;
        m_PlayFor = 0;
    }
}
