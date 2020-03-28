#ifndef SONGTRACKSMODEL_H
#define SONGTRACKSMODEL_H

#include <QList>

#include "songfile.h"
#include "abstractfilepartmodel.h"

class SongTrack;
class FilePartCollection;
class SongPartModel;
class FileOffsetTableModel;

class TrackIndexCollection;
class TrackMetaCollection;
class TrackDataCollection;

class SongTracksModel : public AbstractFilePartModel
{
   Q_OBJECT
public:
   explicit SongTracksModel();
   ~SongTracksModel();

   SongTrack *createTrack(const QString &fileName, int trackType, QStringList *p_ParseErrors);

   void removePart(SongPartModel * p_Part);
   void refreshTrackUsage(SongTrack * p_Track);

   SongTrack *trackAtIndex(int i);

   TrackIndexCollection *trackIndexes();
   TrackMetaCollection *trackMeta();
   TrackDataCollection *trackData();
   uint32_t readFromBuffer(uint8_t * p_FullFileBuffer, FileOffsetTableModel *p_OffsetTable, QStringList *p_ParseErrors);
   void adjustOffsets();

protected:
   virtual uint32_t maxInternalSize();
   virtual uint32_t minInternalSize();
   virtual uint8_t *internalData();


private:
   QList<SongTrack *> * mp_SongTracks;

};

#endif // SONGTRACKSMODEL_H
