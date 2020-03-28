#ifndef SONGTRACKDATAITEM_H
#define SONGTRACKDATAITEM_H

#include "songfile.h"
#include "abstractfilepartmodel.h"

class SongTrackDataItem : public AbstractFilePartModel
{
   Q_OBJECT
public:
   explicit SongTrackDataItem();

   bool parseMidi(unsigned char* file, uint32_t size, int trackType, QStringList *p_ParseErrors);
   bool parseByteArray(const QByteArray&);

   uint32_t readFromBuffer(uint8_t * p_Buffer, uint32_t size, QStringList *p_ParseErrors);

   void print();

   QByteArray toByteArray();

protected:
   virtual uint32_t maxInternalSize();
   virtual uint32_t minInternalSize();
   virtual uint8_t *internalData();

private:

   MIDIPARSER_MidiTrack m_Data;
   QByteArray m_Cache; 
};

#endif // SONGTRACKDATAITEM_H
