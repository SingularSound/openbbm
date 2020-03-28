#ifndef PLAYBACKPANEL_H
#define PLAYBACKPANEL_H

#include <QWidget>
#include <QComboBox>
#include <QLabel>
#include <QSlider>
#include <QPushButton>
#include <QString>
#include <QHash>
#include <QModelIndex>
#include <QItemSelectionModel>

#include "player/player.h"
#include "model/tree/project/beatsprojectmodel.h"

class PlaybackPanel : public QWidget
{
   Q_OBJECT
public:
   explicit PlaybackPanel(QWidget *parent = nullptr);
    ~PlaybackPanel();
   void setModel(BeatsProjectModel *model);

   void onBeatsCurOrSelChange(QItemSelectionModel *selectionModel);

   void setTempo(int tempo);
   int getTempo(void);
   void addDrumset(const QString &name, const QString &path);
   void removeDrumset(const QString &name);
   void clearDrumsets(void);
   void setSelectedDrm(const QString &drmName);
   void setSong(const QModelIndex &modelIndex);
   void setEffectsPath(const QString &path);
   void enable(void);
   void disable(void);

   int bufferTime_ms();

   Player *mp_Player;

   QLabel *mp_Title;
   QComboBox *mp_ComboBoxDrumset;
   QLabel *mp_LabelTempo;
   QSlider *mp_SliderTempo;
   QPushButton *mp_ButtonPlay;
   QPushButton *mp_ButtonStop;

signals:
   void signalIsPlaying(bool status);
   void sigPlayerEnabled(bool enabled);

    void drumsetChanged(const QString& name);

private slots:
   void slotChangeTempo(int tempo);
   void onDrumsetChanged(const QString &name);

   void playerStarted(void);
   void playerStopped(void);
   void slotPlayerError(QString errorMessage);

   void playingIntro(void);
   void playingMainTrack(unsigned int PartIndex);
   void playingOutro(void);
   void playingTranfill(unsigned int PartIndex);
   void playingDrumfill(unsigned int PartIndex, unsigned int DrumfillIndex);

   void dataChanged(const QModelIndex &left, const QModelIndex &right);
   void slotOnRowRemoved(const QModelIndex &parent, int start, int end);
   void slotOnRowInserted(const QModelIndex &parent, int start, int end);


public slots:
   void play(void);
   void stop(void);
   void slotSelectTrack(const QByteArray &trackData, int trackIndex, int typeId, int partIndex);
   void slotOnStartEditing(const QString& name, const QByteArray& data);
   void slotOnTrackEditing(const QByteArray& data);
   void slotOnTrackEdited(const QByteArray& data);

   void slotPedalPress(void);
   void slotPedalRelease(void);
   void slotPedalDoubleTap(void);
   void slotPedalLongPress(void);
   void slotEffect(void);
   void slotSetBufferTime_ms(int time_ms);

protected:
   virtual void paintEvent(QPaintEvent * event);
   void setPlayingPart(int part, unsigned int partNumber, unsigned int drumfillNumber);

   QSet<QString> m_drmToRemove;
   QHash<QString, QString>  m_hashDrumset;
   QPersistentModelIndex m_SongIndex;
   QPersistentModelIndex m_NextSongIndex;
   BeatsProjectModel *mp_beatsModel;

   int m_LastPlayingPart;
   unsigned int m_LastPlayingPartNumber;
   unsigned int m_LastPlayingDrumfillNumber;


   bool m_enabled;
   bool m_editorMode;

};

#endif // PLAYBACKPANEL_H

