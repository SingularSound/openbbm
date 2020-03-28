#ifndef TRACKARRAYITEM_H
#define TRACKARRAYITEM_H
#include "../abstracttreeitem.h"
#include "../../filegraph/songtrack.h"

class TrackArrayItem : public AbstractTreeItem
{
   Q_OBJECT
public:
   explicit TrackArrayItem(const QString & name, int maxChildCnt, const QString &childrenType, int trackType, int loopCount, AbstractTreeItem *parent);



   // Data interaction
   virtual QVariant data(int column);
   // Will actually create the data if column = 0 ?
   virtual bool setData(int column, const QVariant & value);

   void insertNewChildAt(int row);
   virtual void removeChild(int row);

   void insertTrackAt(int row, SongTrack *p_SongTrackModel, bool init = true);
   void insertTracksAt(int row, const QList<SongTrack *> &songTrackModelList, bool init = true);
   void insertEffectAt(int row, QString effectFileName, bool init = true);

   void clearEffectUsage();

public slots:
   void slotFileSet(SongTrack *p_Track, int row);
signals:
   void sigFileSet(SongTrack *p_Track, int row);
   bool sigFileRemoved(int row);
   void sigValidityChange();

private:

   int m_MaxChildCnt;
   QString m_ChildrenType;
   QString m_Name;
   int m_trackType;
   int m_loopCount;
};

#endif // TRACKARRAYITEM_H
