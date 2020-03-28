#include <QDataStream>
#include <QTextStream>
#include <QByteArray>
#include <QStringList>
#include <QDebug>
#include "songfilemeta.h"

SongFileMeta::SongFileMeta() :
   AbstractFilePartModel(),
   m_Data(SONGFILE_META_SIZE, 'Z')
{
   m_Default_Name = tr("SongTrackMetaItem");
   m_Name = m_Default_Name;
   m_InternalSize = SONGFILE_META_SIZE;

   generateUuid();

   setDefaultDrmName(nullptr);
}


uint32_t SongFileMeta::readFromBuffer(uint8_t * p_Buffer, uint32_t size, QStringList *p_ParseErrors)
{
   // Memorize generated uuid
   QUuid uuid = this->uuid();

   // Read content
   size = AbstractFilePartModel::readFromBuffer(p_Buffer, size, p_ParseErrors);

   if((int)size < 0){
      return size;
   }

   // if uuid is invalid
   if(this->uuid().isNull()){
      setUuid(uuid);
   } else {
      // The uuid that was read is valid
   }

   return size;
}

void SongFileMeta::generateUuid()
{
   QUuid generatedUuid = QUuid::createUuid();

   setUuid(generatedUuid);
}

void SongFileMeta::setUuid(const QUuid &uuid)
{
   int i = 0;
   for(; i < uuid.toByteArray().count(); i++){
      m_Data.data()[i] = uuid.toByteArray().data()[i];
   }
   // Pad up to DRM name
   for(; i < DRM_NAME_OFFSET; i++){
      m_Data.data()[i] = 0;
   }

   //delete p_Stream;
}

QUuid SongFileMeta::uuid()
{
   // create ByteArray with only content of
   QByteArray uuidByteArray = m_Data.left(m_Data.indexOf('}')+1);
   return QUuid(uuidByteArray);

}

QString SongFileMeta::defaultDrmName()
{
   QByteArray drmNameByteArray = m_Data.mid(DRM_NAME_OFFSET, MAX_DRM_NAME_LENGTH);

   return QString::fromLocal8Bit(drmNameByteArray);
}

void SongFileMeta::setDefaultDrmName(QString drmName)
{

   if(drmName.count() > MAX_DRM_NAME_LENGTH){
      drmName.truncate(MAX_DRM_NAME_LENGTH);
   }

   QByteArray drmNameByteArray = drmName.toLocal8Bit();

   int i = 0;
   for(; i < MAX_DRM_NAME_LENGTH && i < drmNameByteArray.count() ; i++){
      m_Data.data()[i + DRM_NAME_OFFSET] = drmNameByteArray.data()[i];
   }
   // Pad with 0
   for(; i < MAX_DRM_NAME_LENGTH + 1; i++){
      m_Data.data()[i + DRM_NAME_OFFSET] = 0;
   }

}

uint32_t SongFileMeta::maxInternalSize()
{
   return SONGFILE_META_SIZE;
}

uint32_t SongFileMeta::minInternalSize()
{
   return 0;
}

uint8_t *SongFileMeta::internalData()
{
   return (uint8_t *)m_Data.data();
}
