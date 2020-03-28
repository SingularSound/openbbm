#ifndef DRUMSETEXTRACTOR_H
#define DRUMSETEXTRACTOR_H

#include "Utils/Common.h"
#include "Model/drmmakermodel.h"
#include "UI_Elements/Instrument.h"

#include <cstdlib>
#include <cstdio>
#include <cstddef>

#include <QFile>
#include <QDataStream>
#include <QFileInfo>
#include <QDir>


class DrumSetExtractor
{
public:
    DrumSetExtractor(DrmMakerModel *model)
        : mp_Model(model)
    {}

    void buildInstrumentList(bool newPath, class DrumsetPanel* p_parentWidget = nullptr);
    QString generateWaves(const QString &dstDirPath, QWidget *p_parentWidget = nullptr);
    int read(const QString& fileName);

private:
    DrmMakerModel* mp_Model;
    QString mSourceFilePath;
    unsigned char mVolume;
    DrmHeader_t mHeaderData;
    Instrument_t mInstruments[DRM_MAX_INSTRUMENT_COUNT];
    QList<QString> mInstrumentNames;
    QList<QString> mVelocityFiles;
    QList<QString> mVelocityNewFiles;
    QList<Instrument*> mBlankInstruments;
};

#endif // DRUMSETEXTRACTOR_H
