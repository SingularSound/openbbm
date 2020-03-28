#ifndef ABSTRACTFILEPARTMODEL_H
#define ABSTRACTFILEPARTMODEL_H

#include <QObject>
#include <QList>
#include <QFile>
#include <QVariant>
#include <QTextStream>
#include "../../crc32.h"

class AbstractFilePartModel : public QObject
{
   Q_OBJECT
public:
   explicit AbstractFilePartModel();
   ~AbstractFilePartModel();

   AbstractFilePartModel *childAt(int index);
   void append(AbstractFilePartModel * child);
   void removeAt(int index);
   inline int count(){return mp_SubParts->count();}

   virtual const QString &name();
   inline virtual const QString &defaultName(){return m_Default_Name;}
   virtual void setName(const QString &name);

   virtual uint32_t size();
   virtual uint32_t maxSize();
   virtual uint32_t minSize();

   virtual void writeToFile(QFile &file);
   virtual void readFromFile(QFile &file, QStringList *p_ParseErrors);
   virtual uint32_t readFromBuffer(uint8_t * p_Buffer, uint32_t size, QStringList *p_ParseErrors);

   virtual void updateCRC(Crc32 &crc);

protected:
   QList<AbstractFilePartModel *> *mp_SubParts;
   QString m_Name;
   QString m_Default_Name;
   uint32_t m_InternalSize;

   virtual uint32_t maxInternalSize() = 0;
   virtual uint32_t minInternalSize() = 0;
   virtual uint8_t *internalData() = 0;

   virtual void prepareData(){QTextStream(stdout) << "AbstractFilePartModel::prepareData = NO PREPARATION in " << metaObject()->className() << endl;}

};

#endif // ABSTRACTFILEPARTMODEL_H
