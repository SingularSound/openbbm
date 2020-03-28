#ifndef PLAYER_H
#define PLAYER_H

#include <QThread>
#include <QAudioOutput>
#include <QIODevice>
#include <QTime>
#include <QString>
#include <QQueue>
#include <QReadWriteLock>

#include "button.h"
#include "../model/filegraph/song.h"
#include "songPlayer.h"
#include "mixer.h"

class Player : public QThread
{
    Q_OBJECT
public:

   enum partEnum{
      stopped,
      intro,
      outro,
      mainLoop,
      drumFill,
      transFill,
      pause,
      partEnum_size
   };

    explicit Player(QObject *parent = nullptr);
    ~Player();

    QString song(void);

    inline int tempo(){return m_tempo;}

    inline bool started(){return m_prevStarted;}
    inline int sigNum(){return m_prevSigNum;}
    inline int beatInBar(){return m_prevBeatInBar;}
    inline partEnum part(){return m_prevPart;}
    inline int bufferTime_ms(){return m_bufferTime_ms;}

    void updateTempo();
    
private:
    void initAudio(void);
    void initMixer(void);
    void loadDrumset(const QString &filepath);
    bool loadSong(const QString &filepath);
    bool loadEffect(int part, const QString &filepath);
    void clearEffect(int part);
    int processTime(int samplesToProcess);
    void processAudio(int samplesToProcess);
    void processEvent(void);
    void updateStatus(bool forceEmit);
    void run(void);

    QAudioDeviceInfo m_device;
    QAudioOutput *m_audioOutput;
    QIODevice *m_ioDevice;
    QAudioFormat m_format;

    QByteArray m_drumset;
    QByteArray m_song;
    QByteArray m_effects[MAX_SONG_PARTS];

    unsigned int m_soundCardLimit;
    char m_buffer[MIXER_BUFFER_LENGTH_BYTES_STEREO];
    int m_bufferTime_ms;
    int m_bufferSize_bytes; // NOTE: m_bufferSize_bytes is only computed at start of thread.
    MIDIPARSER_MidiTrack mp_singleTrack;
    int m_singleTrackOffset;
    bool m_singleTrack;
    int m_singleTrackTrackIndex;
    int m_singleTrackTypeId;
    int m_singleTrackPartIndex;


    QString m_drumsetPath;
    QString m_songPath;
    QString m_effectsPath;
    int m_tempo;
    bool m_AutoPilot;

    int m_prepareStop;
    int m_trailingSounds;
    int m_stop;

    double m_processedSamples_real;

    QReadWriteLock m_lock;
    QQueue<BUTTON_EVENT> m_queue;

    SongPlayer_PlayerStatus m_lastPlayerStatus;

    // for status
    bool m_prevStarted;
    int m_prevSigNum;
    int m_prevBeatInBar;
    int m_prevTick;
    partEnum m_prevPart;


signals:
    void sigPlayerStarted(void);
    void sigPlayerStopped(void);
    void sigPlayerPosition(int tick);
    void sigPlayerError(QString errorMessage);

    void sigPlayingIntro(void);
    void sigPlayingMainTrack(unsigned int PartIndex);
    void sigPlayingOutro(void);
    void sigPlayingTranfill(unsigned int PartIndex);
    void sigPlayingDrumfill(unsigned int PartIndex, unsigned int DrumfillIndex);

    void sigStartedChanged(bool);
    void sigSigNumChanged(int);
    void sigBeatInBarChanged(int);
    void sigPartChanged(int);
    void sigTempoChangedBySong(int);

public slots:
    void play(void);
    void stop(void);

    void setDrumset(const QString &path);
    void setSong(const QString &path);
    void setSingleTrack(const QByteArray &trackData = QByteArray(), int trackIndex = -1, int typeId = -1, int partIndex = -1);
    void setEffectsPath(const QString &path);
    void setTempo(int bpm);
    int getTempo(void);

    void setAutoPilot(bool ap);
    bool getAutoPilot(void);
    void pedalPress(void);
    void pedalRelease(void);
    void pedalLongPress(void);
    void pedalDoubleTap(void);

    void effect(void);
    void slotSetBufferTime_ms(int time_ms);
};

#endif // PLAYER_H

