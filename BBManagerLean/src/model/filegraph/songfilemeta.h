#ifndef SONGFILEMETA_H
#define SONGFILEMETA_H

#include "songfile.h"
#include "abstractfilepartmodel.h"

#include <QUuid>

#define DRM_NAME_OFFSET 40
#define MAX_DRM_NAME_LENGTH 127

class SongFileMeta : public AbstractFilePartModel
{
   Q_OBJECT
public:
   explicit SongFileMeta();

   virtual uint32_t readFromBuffer(uint8_t * p_Buffer, uint32_t size, QStringList *p_ParseErrors);
   void generateUuid();
   QUuid uuid();
   void setUuid(const QUuid &uuid);
   QString defaultDrmName();
   void setDefaultDrmName(QString drmName);

protected:
   virtual uint32_t maxInternalSize();
   virtual uint32_t minInternalSize();
   virtual uint8_t *internalData();

private:

   QByteArray m_Data;

};

#endif // SONGFILEMETA_H
