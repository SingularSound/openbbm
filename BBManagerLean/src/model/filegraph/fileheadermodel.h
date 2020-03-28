#ifndef FILEHEADERMODEL_H
#define FILEHEADERMODEL_H

#include "songfile.h"
#include "abstractfilepartmodel.h"

class FileHeaderModel : public AbstractFilePartModel
{
   Q_OBJECT
public:
   explicit FileHeaderModel();

   bool isHeaderValid();
   void setFileValid(bool valid);
   bool isFileValid();
   static bool isFileValidStatic(QFile &file);
   void setCRC(uint32_t crc);
   uint32_t crc() const;
   void computeCRC(uint8_t *internalData, int length);
   uint32_t actualVersion();
   void setActualVersion(uint32_t version);
   bool isVersionValid();

protected:
   virtual uint32_t maxInternalSize();
   virtual uint32_t minInternalSize();
   virtual uint8_t *internalData();

private:
   SONGFILE_HeaderStruct m_FileHeader;
};

#endif // FILEHEADERMODEL_H
