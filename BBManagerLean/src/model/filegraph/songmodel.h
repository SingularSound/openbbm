#ifndef SONGMODEL_H
#define SONGMODEL_H

#include <QString>

#include "songfile.h"
#include "abstractfilepartmodel.h"


class SongTrack;
class SongPartModel;
class SongTracksModel;

class SongModel : public AbstractFilePartModel
{
   Q_OBJECT
public:
   explicit SongModel(SongTracksModel *p_SongTracksModel);
   ~SongModel();
   virtual uint32_t readFromBuffer(uint8_t * p_Buffer, uint32_t size, QStringList *p_ParseErrors);

   void setLoopSong(uint32_t loopSong);
   void setBpm(uint32_t bpm);

   void insertNewPart(int i);
   void appendNewPart();
   void deletePart(int i);
   void moveParts(int firstIndex, int lastIndex, int delta);

   uint32_t loopSong();
   uint32_t bpm();
   uint32_t nPart();
   QString defaultDrmFileName();
   void setDefaultDrmFileName(const QString &defaultDrmFileName);

   SongPartModel * intro();
   SongPartModel * outro();
   SongPartModel * part(int i);
   int partCount();


   void replaceEffectFile(const QString &originalName, const QString &newName);


protected:
   virtual uint32_t maxInternalSize();
   virtual uint32_t minInternalSize();
   virtual uint8_t *internalData();
   virtual void prepareData();

private:
   SONG_SongStruct m_Song;
   SongTracksModel *mp_SongTracksModel;  // Reference received in constructor, no need to delete

};

#endif // SONGMODEL_H
