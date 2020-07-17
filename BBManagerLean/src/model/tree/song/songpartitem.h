#ifndef SONGPARTITEM_H
#define SONGPARTITEM_H

#include "filepartitem.h"
#include "../../filegraph/songpartmodel.h"

class SongPartItem : public FilePartItem
{
   Q_OBJECT
public:
   SongPartItem(SongPartModel * songPart, AbstractTreeItem *parent);

   // Data interaction
   virtual QVariant data(int column);
   virtual bool setData(int column, const QVariant & value);

   void setEffectFileName(const QString &fileName, bool init = false);
   void setPartFileName(const QString &fileName, bool init = false);
   QString effectFileName();
   QString PartFileName();
   void clearEffectUsage();

   uint32_t timeSigNum();
   uint32_t timeSigDen();
   uint32_t ticksPerBar();
   uint32_t loopCount();

private slots:
   void slotValidityChanged();

private:
   bool m_playing;
};

#endif // SONGPARTITEM_H
