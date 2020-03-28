#ifndef DRUMSETMAKER_H
#define DRUMSETMAKER_H

#include "UI_Elements/Instrument.h"
#include "Model/drmmakermodel.h"
#include "Utils/Common.h"
#include "utils/utils.h"
#include "crc32.h"

#include <QXmlStreamWriter>
#include <QString>
#include <QByteArray>
#include <QFile>
#include <QBuffer>



class DrumSetMaker
{
public:
    DrumSetMaker(DrmMakerModel *model);
    ~DrumSetMaker();

    bool buildDRM();
    bool containsValidBuild();
    void clearBuild();
    void writeToFile(QString filename);
    bool checkErrorsAndSort();
    bool isEmpty();

private:

    // Methods
    void makeWavBuffer();
    void makeInstrumentBuffers();
    void makeMetadataBuffers();
    void makeExtensionBuffers();
    void makeHeaderBuffer();

    // Data needed for DRM file
    QByteArray mHeaderBuffer;
    QByteArray* mInstrumentBuffers[DRM_MAX_INSTRUMENT_COUNT];
    QByteArray mBlankInstrumentBuffer;
    QByteArray mMetaDataInfoBuffer;
    QByteArray mMetaDataBuffer;
    QByteArray mExtensionHeaderBuffer;
    QByteArray mExtensionVolumeBuffer;
    QByteArray* mWaveBuffer[DRM_MAX_INSTRUMENT_COUNT];
    QList< QList<int> > mOffsetList;
    QList<int> mPaddingList;
    QList<Vel_t> mWaveInfoList;
    Crc32 mCRC;
    bool mValidBuild;

    // Model reference
    DrmMakerModel *mp_Model;

};

#endif // DRUMSETMAKER_H
