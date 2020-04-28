#ifndef TRACKPTRITEM_H
#define TRACKPTRITEM_H
#include "../../filegraph/songtrack.h"
#include "../abstracttreeitem.h"
#include "../../filegraph/songtracksmodel.h"

class SetTrackOperation;

class TrackPtrItem : public AbstractTreeItem
{
   Q_OBJECT
public:
   explicit TrackPtrItem(AbstractTreeItem *parent, SongTracksModel * p_TracksModel, SongTrack * p_SongTrack = nullptr);

   virtual QVariant data(int column);
   virtual bool setData(int column, const QVariant & value);
   bool setName(const QString &name, bool init);

   bool setSongTrack(SongTrack * p_SongTrack);
   SongTrack * songTrack();
   bool exportTo(const QString &dstDirPath);

   int trackIndex();

   QByteArray getTrackData();
   bool setTrackData(const QByteArray&);

signals:
   void sigFileSet(SongTrack *p_Track, int row);

private:
   QList<QVariant> autoPilotValues();
   void setAutoPilotValues(QList<QVariant> value);


   SongTrack * mp_SongTrack;         // Reference received in constructor, or returned by track model, no need to delete
   SongTracksModel * mp_TracksModel; // Reference received in constructor, no need to delete (should be handled in graph)
   bool m_playing;
   QStringList m_parseErrors;
};

#endif // TRACKPTRITEM_H
