#ifndef SONGTRACK_H
#define SONGTRACK_H

#include <QString>
#include <QList>
#include <stdint.h>

class SongTrackIndexItem;
class SongTrackMetaItem;
class SongTrackDataItem;
class SongPartModel;

class SongTrack
{
public:
   explicit SongTrack(int index);
   explicit SongTrack(int index, SongTrackIndexItem *trackIndexes, SongTrackMetaItem  *trackMeta, SongTrackDataItem  *trackData);
   ~SongTrack();

   const QString & name() const;
   QString fileName() const;
   QString fullFilePath() const;
   void setFullPath(const QString&);

   bool parseByteArray(const QByteArray&);
   bool parseMidiFile(const QString &fileName, int trackType, QStringList *p_ParseErrors);
   bool importTrackFile(const QString &fileName, QStringList *p_ParseErrors);
   bool extractToTrackFile(const QString &fileName, uint32_t timeSigNum, uint32_t timeSigDen, uint32_t tickPerBar, uint32_t bpm, QStringList *p_ParseErrors);
   void setDeleteSubParts(bool doDelete);
   void addUser(SongPartModel* user);
   void removeUser(SongPartModel* user);
   void removeAllUser(SongPartModel* user);
   inline int userCount(){return m_Users.count();}


   void ajustOffsets(SongTrack * previousTrack);

   SongTrackIndexItem *trackIndexes();
   SongTrackMetaItem  *trackMeta();
   SongTrackDataItem  *trackData();
   inline void setIndex(int index){m_Index = index;}
   inline int index(){return m_Index;}


   void print();

private:
   bool m_DeleteSubParts;
   int m_Index;

   SongTrackIndexItem *mp_Indexes;
   SongTrackMetaItem  *mp_Meta;
   SongTrackDataItem  *mp_Data;
   QString m_Name;

   QList<SongPartModel*> m_Users;
   void removeUser();

};

#endif // SONGTRACK_H
