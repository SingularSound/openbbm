#ifndef PARTMODEL_H
#define PARTMODEL_H

#include <QObject>
#include <QString>

#include "songfile.h"
#include "abstractfilepartmodel.h"

class SongTrack;
class SongTracksModel;

class SongPartModel : public AbstractFilePartModel
{
   Q_OBJECT
public:
   explicit SongPartModel(SongTracksModel *p_SongTracksModel, bool isIntro, bool isOutro);

   virtual uint32_t readFromBuffer(uint8_t * p_Buffer, uint32_t size, QStringList *p_ParseErrors);

   void setRepeatFlag(bool repeat);
   void setShuffleFlag(bool shuffle);
   void setBpmDelta(int32_t delta);
   void setTimeSig(uint32_t num, uint32_t den);
   void setTicksPerBar(uint32_t ticks);
   void setLoopCount(uint32_t count);
   void setPartName(QString partName);
   void setMainLoop(const QString &midiFilePath);
   void setTransFill(const QString &midiFilePath);
   void appendDrumFill(const QString &midiFilePath);
   void setEffectFileName(const QString &fileName);

   bool repeatFlag();
   bool shuffleFlag();
   int32_t bpmDelta();
   uint32_t timeSigNum();
   uint32_t timeSigDen();
   uint32_t ticksPerBar();
   uint32_t loopCount();

   uint32_t mainLoopIndex();
   SongTrack *mainLoop();
   uint32_t transFillIndex();
   SongTrack *mainTrackPtr();
   SongTrack *transFill();
   QString effectFileName();

   const uint32_t *drumFillIndexes();
   uint32_t drumFillIndex(uint32_t index);
   SongTrack *drumFill(uint32_t index);
   const QList<SongTrack *> &drumFillList();
   uint32_t nbDrumFills();

   bool isIntro();
   bool isOutro();

   inline SongTracksModel *songTracksModel(){
      return mp_SongTracksModel;
   }

public slots:
   void setMainLoop(SongTrack * p_Track, int position);
   void deleteMainLoop(int position);
   void setTransFill(SongTrack * p_Track, int position);
   void deleteTransFill(int position);
   void setDrumFill(SongTrack * p_Track, int position);
   void appendDrumFill(SongTrack * p_Track, int position);
   void replaceDrumFill(SongTrack * p_NewTrack, int position);
   void deleteDrumFill(int position);
   void deleteEffect(int position);

protected:
   virtual uint32_t maxInternalSize();
   virtual uint32_t minInternalSize();
   virtual uint8_t *internalData();
   virtual void prepareData();

private:
   bool m_Intro;
   bool m_Outro;
   SONG_SongPartStruct m_SongPart;
   SongTracksModel *mp_SongTracksModel; // Reference received in constructor, no need to delete

   SongTrack * mp_MainLoop;             // Delete should be handled by tracks model
   SongTrack * mp_TransFill;            // Delete should be handled by tracks model
   QList<SongTrack *> m_DrumFills;      // Delete should be handled by tracks model

};

#endif // PARTMODEL_H
