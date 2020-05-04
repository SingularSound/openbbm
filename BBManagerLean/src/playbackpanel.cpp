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
#include <QStyleOption>
#include <QVBoxLayout>
#include <QPainter>
#include <QComboBox>
#include <QLineEdit>
#include <QPushButton>
#include <QModelIndex>
#include <QDebug>
#include <QMessageBox>
#include <QMenu>

#include "playbackpanel.h"

#include "model/tree/abstracttreeitem.h"
#include "model/tree/project/beatsprojectmodel.h"
#include "model/tree/project/drmfoldertreeitem.h"
#include "mainwindow.h"

PlaybackPanel::PlaybackPanel(QWidget *parent)
    : QWidget(parent)
    , m_editorMode(false)
{
    QVBoxLayout *p_VBoxLayout = new QVBoxLayout(this);
    mp_Title = new QLabel(this);
    QWidget *p_MainContainer = new QWidget(this);

    mp_Title->setText(tr("Playback"));
    mp_Title->setObjectName(QStringLiteral("titleBar"));

    p_VBoxLayout->setContentsMargins(0,0,0,0);
    p_VBoxLayout->addWidget(mp_Title, 0);
    p_VBoxLayout->addWidget(p_MainContainer, 1);

    setLayout(p_VBoxLayout);

    QGridLayout *p_GridLayout = new QGridLayout(p_MainContainer);

    QHBoxLayout *p_LayoutDrumset = new QHBoxLayout(p_MainContainer);
    QLabel *p_LabelDrumset = new QLabel(p_MainContainer);
    mp_ComboBoxDrumset = new QComboBox(p_MainContainer);

    QHBoxLayout *p_LayoutPlayer = new QHBoxLayout(p_MainContainer);
    QLabel *p_LabelPlayer = new QLabel(p_MainContainer);
    mp_LabelTempo = new QLabel(p_MainContainer);
    QVBoxLayout *p_LayoutTempo = new QVBoxLayout(p_MainContainer);
    mp_SliderTempo = new QSlider(Qt::Horizontal, p_MainContainer);
    QLabel *p_LabelTempo = new QLabel(p_MainContainer);

    QHBoxLayout *p_LayoutControls = new QHBoxLayout(p_MainContainer);
    mp_ButtonPlay = new QPushButton(p_MainContainer);
    mp_ButtonStop = new QPushButton(p_MainContainer);

    p_LabelDrumset->setText(tr("Drumset"));
    p_LayoutDrumset->addWidget(p_LabelDrumset);

    mp_ComboBoxDrumset->clear();
    mp_ComboBoxDrumset->setToolTip(tr("Select a drumset from project"));
    mp_ComboBoxDrumset->setSizeAdjustPolicy(QComboBox::AdjustToContents);
    connect(mp_ComboBoxDrumset, SIGNAL(currentIndexChanged(QString)), this, SLOT(onDrumsetChanged(QString)));
    p_LayoutDrumset->addWidget(mp_ComboBoxDrumset);

    p_LabelPlayer->setText(tr("Player:"));
    p_LayoutPlayer->addWidget(p_LabelPlayer);

    mp_LabelTempo->setText(tr("%1 BPM").arg(""));
    mp_LabelTempo->setFixedWidth(60);
    mp_LabelTempo->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    p_LayoutPlayer->addWidget(mp_LabelTempo);

    mp_SliderTempo->setMinimum(40);
    mp_SliderTempo->setMaximum(300);
    mp_SliderTempo->setToolTip(tr("Control the tempo of the song"));
    mp_SliderTempo->setSingleStep(1);
    p_LayoutTempo->addWidget(mp_SliderTempo);
    connect(mp_SliderTempo, SIGNAL(valueChanged(int)), this, SLOT(slotChangeTempo(int)));

    p_LabelTempo->setText(tr("Tempo"));
    p_LabelTempo->setAlignment(Qt::AlignHCenter);
    p_LayoutTempo->addWidget(p_LabelTempo);

    p_LayoutPlayer->addLayout(p_LayoutTempo);

    mp_ButtonPlay->setText(tr("Play"));
    mp_ButtonPlay->setToolTip(tr("Start the playback of the song"));
    p_LayoutControls->addWidget(mp_ButtonPlay);
    mp_ButtonStop->setText(tr("Stop"));
    mp_ButtonStop->setToolTip(tr("Stop the playback of the song"));
    p_LayoutControls->addWidget(mp_ButtonStop);

    p_GridLayout->addLayout(p_LayoutControls,0,0);
    p_GridLayout->addLayout(p_LayoutDrumset,0,1);
    p_GridLayout->addLayout(p_LayoutPlayer,0,2);

    p_MainContainer->setLayout(p_GridLayout);

    mp_Player = new Player(this);
    connect(mp_ButtonPlay, SIGNAL(clicked()), this, SLOT(play()));
    connect(mp_ButtonStop, SIGNAL(clicked()), this, SLOT(stop()));

    // player signals are emited in a different thread
    connect(mp_Player, SIGNAL(sigPlayerStarted()), this, SLOT(playerStarted()), Qt::QueuedConnection);
    connect(mp_Player, SIGNAL(sigPlayerStopped()), this, SLOT(playerStopped()), Qt::QueuedConnection);
    connect(mp_Player, SIGNAL(sigPlayerError(QString)), this, SLOT(slotPlayerError(QString)), Qt::QueuedConnection);

    connect(mp_Player, SIGNAL(sigPlayingIntro()), this, SLOT(playingIntro()), Qt::QueuedConnection);
    connect(mp_Player, SIGNAL(sigPlayingMainTrack(unsigned int)), this, SLOT(playingMainTrack(unsigned int)), Qt::QueuedConnection);
    connect(mp_Player, SIGNAL(sigPlayingOutro()), this, SLOT(playingOutro()), Qt::QueuedConnection);
    connect(mp_Player, SIGNAL(sigPlayingTranfill(unsigned int)), this, SLOT(playingTranfill(unsigned int)), Qt::QueuedConnection);
    connect(mp_Player, SIGNAL(sigPlayingDrumfill(unsigned int, unsigned int)), this, SLOT(playingDrumfill(unsigned int, unsigned int)), Qt::QueuedConnection);

    connect(mp_Player, SIGNAL(sigTempoChangedBySong(int)), this, SLOT(slotChangeTempo(int)), Qt::QueuedConnection);

    m_hashDrumset.clear();
    mp_beatsModel = nullptr;
    disable();
}

PlaybackPanel::~PlaybackPanel() {

}

// Required to apply stylesheet
void PlaybackPanel::paintEvent(QPaintEvent *)
{
    QStyleOption opt;
    opt.init(this);
    QPainter p(this);
    style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);
}

void PlaybackPanel::setModel(BeatsProjectModel *model)
{
    // Remove anything related to previous model
    if (mp_beatsModel) {
        disconnect(mp_beatsModel, SIGNAL(dataChanged(QModelIndex,QModelIndex)), this, SLOT(dataChanged(QModelIndex,QModelIndex)));
        disconnect(mp_beatsModel, SIGNAL(rowsAboutToBeRemoved(QModelIndex,int,int)), this, SLOT(slotOnRowRemoved(QModelIndex,int,int)));
        disconnect(mp_beatsModel, SIGNAL(rowsInserted(QModelIndex,int,int)), this, SLOT(slotOnRowInserted(QModelIndex,int,int)));
        disconnect(mp_beatsModel, &BeatsProjectModel::beginEditMidi, this, &PlaybackPanel::slotOnStartEditing);
        disconnect(mp_beatsModel, &BeatsProjectModel::editMidi, this, &PlaybackPanel::slotOnTrackEditing);
        disconnect(mp_beatsModel, &BeatsProjectModel::endEditMidi, this, &PlaybackPanel::slotOnTrackEdited);


        if (mp_Player->isRunning()) {
            mp_Player->stop();
            mp_Player->wait();
        }
        clearDrumsets();
        disable();
    }


    // Set new model (may be null)
    mp_beatsModel = model;

    // Add stuff related to new model
    if (mp_beatsModel) {
        // Set EFFECTS folder absolute path
        QModelIndex efxIndex = mp_beatsModel->effectFolderIndex();
        setEffectsPath(efxIndex.sibling(efxIndex.row(), AbstractTreeItem::ABSOLUTE_PATH).data().toString());

        // fill drumsets
        DrmFolderTreeItem *p_drmFolder = mp_beatsModel->drmFolder();
        for (int i = 0; i < p_drmFolder->childCount(); i++) {
            addDrumset(p_drmFolder->child(i)->data(AbstractTreeItem::NAME).toString(),
                       p_drmFolder->child(i)->data(AbstractTreeItem::ABSOLUTE_PATH).toString());
        }
        connect(mp_beatsModel, SIGNAL(dataChanged(QModelIndex,QModelIndex)), this, SLOT(dataChanged(QModelIndex,QModelIndex)));
        connect(mp_beatsModel, SIGNAL(rowsAboutToBeRemoved(QModelIndex,int,int)), this, SLOT(slotOnRowRemoved(QModelIndex,int,int)));
        connect(mp_beatsModel, SIGNAL(rowsInserted(QModelIndex,int,int)), this, SLOT(slotOnRowInserted(QModelIndex,int,int)));
        connect(mp_beatsModel, &BeatsProjectModel::beginEditMidi, this, &PlaybackPanel::slotOnStartEditing);
        connect(mp_beatsModel, &BeatsProjectModel::editMidi, this, &PlaybackPanel::slotOnTrackEditing);
        connect(mp_beatsModel, &BeatsProjectModel::endEditMidi, this, &PlaybackPanel::slotOnTrackEdited);
    }
}

void PlaybackPanel::onBeatsCurOrSelChange(QItemSelectionModel *p_selectionModel)
{
    // 1 - determine if model is valid
    if(!mp_beatsModel){
        qWarning() << "PlaybackPanel::onBeatsCurOrSelChange - ERROR 1 - called when beats model null";
        return;
    }

    // 2 - determine if p_selectionModel is valid
    if(!p_selectionModel){
        qWarning() << "PlaybackPanel::onBeatsCurOrSelChange - ERROR 2 - called with p_selectionModel null";
        return;
    }


    // 3 - Determine if Current is a song of folder
    QModelIndex current = p_selectionModel->currentIndex();
    QItemSelection selection = p_selectionModel->selection();

    bool currentIsSong = false;

    if(!selection.isEmpty() && current.isValid() && current.parent().isValid()){
        // Retrieve parent's file children file type
        QModelIndex parentChildrenTypes = mp_beatsModel->index(current.parent().row(), AbstractTreeItem::CHILDREN_TYPE, current.parent().parent());
        auto BBS = QString(BMFILES_MIDI_BASED_SONG_EXTENSION).toUpper(), BWS = QString(BMFILES_WAVE_BASED_SONG_EXTENSION).toUpper();
        if(parentChildrenTypes.data().isValid()){
            QStringList types =  parentChildrenTypes.data().toString().toUpper().split(",");
            if(types.contains(BBS) || types.contains(BWS)){
                currentIsSong = true;
            }
        }
    }

    // 4 - Perform operations related to nature of current
    if(currentIsSong){
        setTempo(mp_beatsModel->index(current.row(), AbstractTreeItem::TEMPO, current.parent()).data().toInt());

        // Set drumset to song's default drumset if it exists
        // CSV: "FILE_NAME, NAME", file name always upper case
        QString drmDataCsv = mp_beatsModel->index(current.row(), AbstractTreeItem::DEFAULT_DRM, current.parent()).data().toString();
        QStringList drmData = drmDataCsv.split(",");

        if(drmData.count() == 2){
           if(!drmData.at(0).isEmpty() && drmData.at(0).compare("00000000." BMFILES_DRUMSET_EXTENSION, Qt::CaseInsensitive)){
              setSelectedDrm(drmData.at(1));
           }
        } else {
           qWarning() << "PlaybackPanel::onBeatsCurOrSelChange - ERROR 2 - drmData.count() != 2";
        }

        setSong(current);
        enable();
    } else {
        setSong(QModelIndex());
        disable();
    }

}

void PlaybackPanel::dataChanged(const QModelIndex &left, const QModelIndex &right)
{
   // NOTE: should not need to manage drumset name change or path change. A change in drumset should mean a complete removal and re-adding.

   QString drmDataCsv;
   QStringList drmData;

   QModelIndex modelIndex(left.sibling(left.row(),0));
   if (modelIndex.internalPointer() != m_SongIndex.internalPointer()) {
      return;
   }

   for(int column = left.column(); column <= right.column(); column++){
      switch (column){
         case AbstractTreeItem::NAME:
            if (m_LastPlayingPart != NO_SONG_LOADED) {
               if(m_enabled){
                  mp_Title->setText(tr("Playback - %1").arg(mp_beatsModel->index(m_SongIndex.row(), AbstractTreeItem::NAME, m_SongIndex.parent()).data().toString()));
               }
            }
            break;

         case AbstractTreeItem::ABSOLUTE_PATH:
            // Update path for player
            if (!mp_Player->isRunning()) {
                mp_Player->setSong(mp_beatsModel->index(modelIndex.row(), column, modelIndex.parent()).data().toString());
            }
            break;

         case AbstractTreeItem::SAVE:
            // Stop updating playing parts when song is modified
            m_LastPlayingPart = NO_SONG_LOADED;
            break;

         case AbstractTreeItem::TEMPO:
            // Stop updating playing parts when song is modified
            setTempo(left.sibling(left.row(), column).data().toInt());
            break;

         case AbstractTreeItem::DEFAULT_DRM:
            // CSV: "FILE_NAME, NAME", file name always upper case
            drmDataCsv = left.sibling(left.row(), column).data().toString();
            drmData = drmDataCsv.split(",");
            if(drmData.count() == 2){
               QString drmFileName = drmData.at(0);
               if(drmFileName.compare("00000000." BMFILES_DRUMSET_EXTENSION, Qt::CaseInsensitive)){
                  setSelectedDrm(drmData.at(1));
               }

            } else {
               qWarning() << "PlaybackPanel::onBeatsCurOrSelChange - ERROR 2 - drmData.count() != 2";
            }
            break;
      }
   }
}

void PlaybackPanel::slotOnRowRemoved(const QModelIndex &parent, int start, int end)
{
    // if it is a drumset that is removed
    if(parent == mp_beatsModel->drmFolderIndex()){
        for(int row = start; row <= end; row++){
            removeDrumset(mp_beatsModel->index(row, AbstractTreeItem::NAME, parent).data().toString());
        }
    }

    // TODO manage current song removal
}

void PlaybackPanel::slotOnRowInserted(const QModelIndex &parent, int start, int end)
{
    // if it is a drumset that is inserted
    if(parent == mp_beatsModel->drmFolderIndex()){
        for(int row = start; row <= end; row++){
            addDrumset(mp_beatsModel->index(row, AbstractTreeItem::NAME, parent).data().toString(),
                       mp_beatsModel->index(row, AbstractTreeItem::ABSOLUTE_PATH, parent).data().toString());
        }
    }
}

void PlaybackPanel::setPlayingPart(int part, unsigned int partNumber = 0, unsigned int drumfillNumber = 0)
{
    QModelIndex partIndex;
    QModelIndex trackPtrIndex;

    if (m_LastPlayingPart != NO_SONG_LOADED) {

        switch(m_LastPlayingPart)
        {
        case INTRO:
            partIndex = mp_beatsModel->introPartIndex(m_SongIndex);
            trackPtrIndex = mp_beatsModel->trackPtrIndex(partIndex, 1);
            break;
        case PLAYING_MAIN_TRACK:
            partIndex = mp_beatsModel->partIndex(m_SongIndex, m_LastPlayingPartNumber);
            trackPtrIndex = mp_beatsModel->trackPtrIndex(partIndex, 0);
            break;
        case TRANFILL_ACTIVE:
            partIndex = mp_beatsModel->partIndex(m_SongIndex, m_LastPlayingPartNumber);
            trackPtrIndex = mp_beatsModel->trackPtrIndex(partIndex, 2);
            break;
        case DRUMFILL_ACTIVE:
            partIndex = mp_beatsModel->partIndex(m_SongIndex, m_LastPlayingPartNumber);
            trackPtrIndex = mp_beatsModel->trackPtrIndex(partIndex, 1, m_LastPlayingDrumfillNumber);
            break;
        case OUTRO:
            partIndex = mp_beatsModel->outroPartIndex(m_SongIndex);
            trackPtrIndex = mp_beatsModel->trackPtrIndex(partIndex, 1);
            break;
        }
        if (partIndex.isValid()) {
            if (!mp_beatsModel->setData(partIndex.sibling(partIndex.row(), AbstractTreeItem::PLAYING), QVariant(false))) {
            }
        }
        if (trackPtrIndex.isValid()) {
            if (!mp_beatsModel->setData(trackPtrIndex.sibling(trackPtrIndex.row(), AbstractTreeItem::PLAYING), QVariant(false))) {
            }
        }
        if(m_LastPlayingPart == STOPPED && m_SongIndex.isValid()){
            if (!mp_beatsModel->setData(m_SongIndex.sibling(m_SongIndex.row(), AbstractTreeItem::PLAYING), QVariant(true))) {
            }
        }

        partIndex = QModelIndex();
        trackPtrIndex = QModelIndex();
        switch(part)
        {
        case INTRO:
            partIndex = mp_beatsModel->introPartIndex(m_SongIndex);
            trackPtrIndex = mp_beatsModel->trackPtrIndex(partIndex, 1);
            break;
        case PLAYING_MAIN_TRACK:
            partIndex = mp_beatsModel->partIndex(m_SongIndex, partNumber);
            trackPtrIndex = mp_beatsModel->trackPtrIndex(partIndex, 0);
            break;
        case TRANFILL_ACTIVE:
            partIndex = mp_beatsModel->partIndex(m_SongIndex, partNumber);
            trackPtrIndex = mp_beatsModel->trackPtrIndex(partIndex, 2);
            break;
        case DRUMFILL_ACTIVE:
            partIndex = mp_beatsModel->partIndex(m_SongIndex, partNumber);
            trackPtrIndex = mp_beatsModel->trackPtrIndex(partIndex, 1, drumfillNumber);
            break;
        case OUTRO:
            partIndex = mp_beatsModel->outroPartIndex(m_SongIndex);
            trackPtrIndex = mp_beatsModel->trackPtrIndex(partIndex, 1);
            break;
        }
        if (partIndex.isValid()) {
            if (!mp_beatsModel->setData(partIndex.sibling(partIndex.row(), AbstractTreeItem::PLAYING), QVariant(true))) {
            }
        }
        if (trackPtrIndex.isValid()) {
            if (!mp_beatsModel->setData(trackPtrIndex.sibling(trackPtrIndex.row(), AbstractTreeItem::PLAYING), QVariant(true))) {
            }
        }
        if(part == STOPPED && m_SongIndex.isValid()){
            if (!mp_beatsModel->setData(m_SongIndex.sibling(m_SongIndex.row(), AbstractTreeItem::PLAYING), QVariant(false))) {
            }
        }
        m_LastPlayingPart = part;
        m_LastPlayingPartNumber = partNumber;
        m_LastPlayingDrumfillNumber = drumfillNumber;
    }
}

void PlaybackPanel::setTempo(int tempo)
{
    if (!mp_Player->isRunning()) {
        if(m_enabled){
            mp_LabelTempo->setText(tr("%1 BPM").arg(tempo));
            mp_SliderTempo->setValue(tempo);
        }
        mp_Player->setTempo(tempo);
    }
}

void PlaybackPanel::setSelectedDrm(const QString &drmName)
{

   if(mp_ComboBoxDrumset->currentText() != drmName){
      // verify if combo box contains drumset
      for(int i = 0; i < mp_ComboBoxDrumset->count(); i++){
         if(mp_ComboBoxDrumset->itemText(i) == drmName){
            // Should select proper drumset in player through callbacks.
            mp_ComboBoxDrumset->setCurrentIndex(i);
            break;
         }
      }
   }
}

/**
 * @brief Get the tempo of the player
 * @return tempo of the player
 */
int PlaybackPanel::getTempo(void){
    return mp_Player->getTempo();
}

/**
 * @brief PlaybackPanel::slotChangeTempo
 *        This slot is called to update the tempo on the player and the
 *        playback panel
 * @param tempo range grom 40 to 300)
 */
void PlaybackPanel::slotChangeTempo(int tempo)
{
    qDebug() << "PlaybackPanel:tempoChanged " << tempo;

    if(m_enabled){
        mp_LabelTempo->setText(tr("%1 BPM").arg(tempo));
        mp_Player->setTempo(tempo); // when disabled, tempo should not change due to slider
    }
}

/**
 * @brief PlaybackPanel::play
 *       Try to load the current song. There are five step before the song starts to play:
 *       #1 : Verify if the model is open
 *       #2 : If editing a song part, use it instead of the current song, going directly to #6
 *       #3 : Verify is the there is a selected song
 *       #4 : Verify is the selected song is valid
 *       #5 : Save any current modifications
 *       #6 : Start the playback of the song
 */
void PlaybackPanel::play(void)
{
    /* Verify the model */
    if (!mp_beatsModel) {
        QMessageBox::warning(this, tr("Playback"), tr("No project opened"));
        return;
    }

    /* Check whether we are in the editor mode */
    if (m_editorMode) {
        mp_Player->setSingleTrack();
        return mp_Player->play();
    }

    /* Verify the selection */
    if (!m_SongIndex.isValid()) {
        QMessageBox::warning(this, tr("Playback"), tr("No song selected"));
        return;
    }

    /* Verify the validity */
    QModelIndex validityIndex = m_SongIndex.sibling(m_SongIndex.row(), AbstractTreeItem::INVALID);
    auto invalid = validityIndex.data(Qt::DisplayRole).toString();
    if (validityIndex.isValid() && !invalid.isEmpty()) {
        MainWindow* w = nullptr;
        for (auto p = parent(); p && !(w = qobject_cast<MainWindow*>(p)); p = p->parent());
        QMessageBox::warning(this, tr("Playback"), tr("Song \"%1\" does not contain a Main Drum Loop.\n"
            "All song parts should contain a Main Drum Loop.\n\nPossible fixes:\n"
            "\tAdd missing song parts\n"
            "\tUse %2 > %3 to make sure it was not deleted by mistake\n"
            "\tRemove song part without a Main Drum Loop\n\tRemove the invalid song altogether")
                .arg(m_SongIndex.data(Qt::DisplayRole).toString())
                .arg(w->mp_edit->title().replace("&", "")).arg(w->mp_Undo->text().replace("&", "")));
        return;
    }

    /* Save the current song */
    QModelIndex saveIndex = m_SongIndex.sibling(m_SongIndex.row(), AbstractTreeItem::SAVE);

    if (saveIndex.isValid() && saveIndex.data(Qt::DisplayRole).toBool()) {
        if (QMessageBox::Cancel == QMessageBox::question(this, tr("Playback"), tr("There are unsaved modifications in the current song.\n\nDo you want to save it now?"), QMessageBox::Save | QMessageBox::Cancel, QMessageBox::Save)) {
            return;
        }
        // saving the song
        mp_beatsModel->setData(mp_beatsModel->index(m_SongIndex.row(), AbstractTreeItem::SAVE, m_SongIndex.parent()), QVariant(false));
    }

    /* Start the playback */
    mp_Player->play();

}

void PlaybackPanel::slotSelectTrack(const QByteArray &trackData, int trackIndex, int typeId, int partIndex)
{
   // TODO Make sure proper signals (like sigPlayerStarted) are emmited
   mp_Player->setSingleTrack(trackData, trackIndex, typeId, partIndex);
   mp_Player->play();
}

void PlaybackPanel::slotOnStartEditing(const QString& name, const QByteArray& data)
{
    name; // currently unused
    m_editorMode = true;
    mp_Player->setSingleTrack(data);
}

void PlaybackPanel::slotOnTrackEditing(const QByteArray& data)
{
    mp_Player->setSingleTrack(data);
}

void PlaybackPanel::slotOnTrackEdited(const QByteArray& data)
{
    auto play = mp_Player->isRunning();
    if (play) {
        mp_Player->stop();
        mp_Player->wait();
    }
    mp_Player->setSingleTrack(data);
    m_editorMode = false;
    mp_Player->setSong(mp_Player->song());
}

void PlaybackPanel::stop(void)
{
    mp_Player->stop();
}

void PlaybackPanel::playerStarted(void)
{
    emit signalIsPlaying(true);

    mp_ComboBoxDrumset->setEnabled(false);
    mp_ButtonPlay->setEnabled(false);
    mp_ButtonStop->setEnabled(true);

    m_LastPlayingPart = STOPPED;
}

void PlaybackPanel::playerStopped(void)
{
    emit signalIsPlaying(false);

    // deselect all parts
    setPlayingPart(STOPPED);

    // remove memorized content from combo box
    if(!m_drmToRemove.isEmpty()){
        for(int i = mp_ComboBoxDrumset->count() - 1; i >= 0; i--){
            if(m_drmToRemove.contains(mp_ComboBoxDrumset->itemText(i))){
                m_hashDrumset.remove(mp_ComboBoxDrumset->itemText(i));
                mp_ComboBoxDrumset->removeItem(i);
            }
        }
    }

    m_drmToRemove.clear();

    // apply memorized song
    bool songChanged = false;
    if(m_SongIndex != m_NextSongIndex){
        songChanged = true;
        m_SongIndex = m_NextSongIndex;
        mp_Player->setSong(mp_beatsModel->index(m_SongIndex.row(), AbstractTreeItem::ABSOLUTE_PATH, m_SongIndex.parent()).data().toString());
    }

    // determine if player needs to be disabled
    if(!m_enabled || m_hashDrumset.isEmpty() || !m_SongIndex.isValid()){
        // Complete any disable that was triggered while player was running
        disable();
    } else {
        // reactivate controls and info
        mp_ComboBoxDrumset->setEnabled(true);
        mp_ButtonPlay->setEnabled(true);
        mp_ButtonStop->setEnabled(false);

        if(songChanged){
            mp_Title->setText(tr("Playback - %1").arg(mp_beatsModel->index(m_SongIndex.row(), AbstractTreeItem::NAME, m_SongIndex.parent()).data().toString()));
            setTempo(mp_beatsModel->index(m_SongIndex.row(), AbstractTreeItem::TEMPO, m_SongIndex.parent()).data().toInt());
        }
    }

}

void PlaybackPanel::slotPlayerError(QString errorMessage)
{
   QMessageBox::critical(this, tr("Playback"), errorMessage);
}

void PlaybackPanel::addDrumset(const QString &name, const QString &path)
{
    if(m_drmToRemove.contains(name)){
        // list of drumsets to remove when player is stopped
        m_drmToRemove.remove(name);
    } else {

        m_hashDrumset[name] = path;
        mp_ComboBoxDrumset->addItem(name);
        // Not needed to set drumset in player, if it's the only one, a drumsetChanged event will be called
    }

    enable(); // will manage conditions that prevent enabling
}

void PlaybackPanel::removeDrumset(const QString &name)
{
    if (mp_Player->isRunning()) {
        // While player is playing, memorize the drumsets to remove
        m_drmToRemove.insert(name);
    } else {
        // otherwise, remove immediatley
        m_hashDrumset.remove(name);
        for(int i = 0; i < mp_ComboBoxDrumset->count(); i++){
            if(mp_ComboBoxDrumset->itemText(i) == name){
                mp_ComboBoxDrumset->removeItem(i);
                break;
            }
        }

        if(m_hashDrumset.isEmpty()){
            disable();
        }
    }
}

void PlaybackPanel::clearDrumsets(void)
{
    m_hashDrumset.clear();
    mp_ComboBoxDrumset->clear();
    disable();
}

void PlaybackPanel::onDrumsetChanged(const QString &name)
{
    if (m_hashDrumset.contains(name)) {
        auto x = m_hashDrumset[name];
        mp_Player->setDrumset(x);
        emit drumsetChanged(x);
    }
}

void PlaybackPanel::setSong(const QModelIndex &modelIndex)
{
    if (mp_beatsModel) {
        m_NextSongIndex = QPersistentModelIndex(modelIndex);
        if (!mp_Player->isRunning()) {
            m_SongIndex = m_NextSongIndex;
            mp_Player->setSong(mp_beatsModel->index(m_SongIndex.row(), AbstractTreeItem::ABSOLUTE_PATH, m_SongIndex.parent()).data().toString());
            if(m_enabled){
                mp_Title->setText(tr("Playback - %1").arg(mp_beatsModel->index(m_SongIndex.row(), AbstractTreeItem::NAME, m_SongIndex.parent()).data().toString()));
            }
        }
    }
}

void PlaybackPanel::setEffectsPath(const QString &path)
{
    mp_Player->setEffectsPath(path);
}


void PlaybackPanel::slotPedalPress(void)
{
    if (mp_Player->isRunning()) {
        mp_Player->pedalPress();
    }
}

void PlaybackPanel::slotPedalRelease(void)
{
   if (mp_ButtonPlay->isEnabled()){
      play();
   }else{
      mp_Player->pedalRelease();
   }
}

void PlaybackPanel::slotPedalDoubleTap(void)
{
   if (mp_Player->isRunning()) {
      mp_Player->pedalDoubleTap();
   }
}

void PlaybackPanel::slotPedalLongPress(void)
{
   if (mp_Player->isRunning()) {
      mp_Player->pedalLongPress();
   }
}

void PlaybackPanel::slotEffect(void)
{
    mp_Player->effect();
}

void PlaybackPanel::enable(void)
{
    if(m_enabled){
        return;
    }

    // Verify that all conditions to enable are present
    if (!mp_beatsModel) {
        return;
    }

    if(m_hashDrumset.isEmpty()){
        return;
    }

    if(!m_SongIndex.isValid()){
        return;
    }

    // Make sure all display information is present
    mp_Title->setText(tr("Playback - %1").arg(mp_beatsModel->index(m_SongIndex.row(), AbstractTreeItem::NAME, m_SongIndex.parent()).data().toString()));
    mp_LabelTempo->setText(tr("%1 BPM").arg(mp_Player->tempo()));
    mp_SliderTempo->setValue(mp_Player->tempo());

    // Enable controls
    mp_SliderTempo->setEnabled(true);
    mp_ComboBoxDrumset->setEnabled(true);

    if (!mp_Player->isRunning()) {
        mp_ButtonPlay->setEnabled(true);
        mp_ButtonStop->setEnabled(false);
    } else {
        mp_ButtonPlay->setEnabled(false);
        mp_ButtonStop->setEnabled(true);
    }

    m_enabled = true;
    emit sigPlayerEnabled(m_enabled);
}

void PlaybackPanel::disable(void)
{
    m_enabled = false;

    if (mp_Player->isRunning()){
        // if player is running, every other change will be performed when player is stopped
        return;
    }

    // Hide all player information
    mp_Title->setText(tr("Playback"));
    mp_LabelTempo->setText(tr("%1 BPM").arg(""));
    mp_SliderTempo->setValue(mp_SliderTempo->minimum());

    // Disable all control
    mp_ButtonPlay->setEnabled(false);
    mp_ButtonStop->setEnabled(false);
    mp_SliderTempo->setEnabled(false);
    mp_ComboBoxDrumset->setEnabled(false);

    emit sigPlayerEnabled(m_enabled);
}

void PlaybackPanel::playingIntro(void)
{
    qDebug() << "PlaybackPanel::playingIntro - DEBUG ";
    setPlayingPart(INTRO);
}

void PlaybackPanel::playingMainTrack(unsigned int PartIndex)
{
    qDebug() << "PlaybackPanel::playingMainTrack #" << PartIndex;
    setPlayingPart(PLAYING_MAIN_TRACK, PartIndex);
}

void PlaybackPanel::playingOutro(void)
{
    qDebug() << "PlaybackPanel::playingOutro";
    setPlayingPart(OUTRO);
}

void PlaybackPanel::playingTranfill(unsigned int PartIndex)
{
    qDebug() << "PlaybackPanel::playingTranfill #" << PartIndex;
    setPlayingPart(TRANFILL_ACTIVE, PartIndex);
}

void PlaybackPanel::playingDrumfill(unsigned int PartIndex, unsigned int DrumfillIndex)
{
    qDebug() << "PlaybackPanel::playingDrumfill #" << PartIndex << "," << DrumfillIndex;
    setPlayingPart(DRUMFILL_ACTIVE, PartIndex, DrumfillIndex);
}

void PlaybackPanel::slotSetBufferTime_ms(int time_ms){
   // NOTE: validation is performed in player
   if(!mp_Player){
      return;
   }
   mp_Player->slotSetBufferTime_ms(time_ms);
}
int PlaybackPanel::bufferTime_ms(){
   // NOTE: validation is performed in player
   if(!mp_Player){
      return MIXER_DEFAULT_BUFFERRING_TIME_MS;
   }
   return mp_Player->bufferTime_ms();
}

