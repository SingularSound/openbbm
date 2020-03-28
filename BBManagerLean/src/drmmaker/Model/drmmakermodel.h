#ifndef MODEL_H
#define MODEL_H

#include <QList>
#include <QStandardPaths>
#include <QDir>
#include <cstddef>

#include "../UI_Elements/Instrument.h"
#include "../Utils/Common.h"

#include "../../model/beatsmodelfiles.h"

class DrmMakerModel : public QObject
{
    Q_OBJECT
public:
    // Constructor
    DrmMakerModel(const QString& tmpDirPath);

    // Drumset
    static QString getDrumsetNameStatic(const QString &path);
    inline QString getDrumsetName() { return mDrumsetName; }
    void setDrumsetName(QString name) { emit drmNameChanged(mDrumsetName = name); }
    unsigned char volume() const { return mVolume; } // -20*log(100/volume) db, 0 = -inf, 1 = -40db, 100 = +0db, 159 = +4db
    void setVolume(unsigned char volume) { mVolume = volume; }
    static QByteArray getCRCStatic(const QString &path);
    static bool copyDrmNewName(const QString &srcPath, const QString &dstPath, const QString & newName);
    static QByteArray renameDrm(const QString &path, const QString & newName);
    bool isNewSizeAllowed(quint32 size);

    inline static QStringList getFileFilters(){
        static QStringList drmFilters(BMFILES_DRUMSET_FILTER);
        return drmFilters;
    }

    // Instruments
    quint32 getDrumsetSize() { return mDrumsetSize; }
    const QList<Instrument*>& getInstrumentList() { return mInstrumentList; }
    Instrument* getInstrument(int index) { return mInstrumentList.at(index); }
    void addInstrument(Instrument *instrument);
    void removeInstrument(Instrument *instrument) { removeInstrument(mInstrumentList.indexOf(instrument)); }
    void removeInstrument(int index);
    void setInstrumentSize(quint32 old_size, quint32 new_size) { emit drmSizeChanged(mDrumsetSize += (new_size - old_size)); }
    void clearInstrumentList();
    int getInstrumentCount() { return mInstrumentList.size(); }
    void sortInstruments();

    // Midi number management
    int getNextAvailableId(int start, bool take = true);
    bool isIdAvailable(int id);
    void switchMidiIds(int oldId, int newId);
    QList<int>* getAvailableMidiIds();

    // file path
    QString drmPath() const { return mDrmPath; }
    void setDrmPath(const QString &path) { mDrmPath = path; }
    inline bool isDrmOpened() { return mDrmOpened; }
    inline void setDrmOpened(bool opened) { mDrmOpened = opened; }

    class QUndoStack* undoStack() { return m_stack; }
    inline QDir dirUndoRedo()   { return m_dirUndoRedo; }

signals:
    void drmNameChanged(const QString& name);
    void drmSizeChanged(quint32 size);

    void instrumentAdded(int index);
    void instrumentRemoved(int index);
    void instrumentSorted();

private:
    // Data
    QString mDrumsetName;
    QList<Instrument*> mInstrumentList;
    bool mInstrumentId[DRM_MAX_INSTRUMENT_COUNT];

    // drm path
    QString mDrmPath;
    bool mDrmOpened;

    quint32 mDrumsetSize;
    unsigned char mVolume;

    QUndoStack* m_stack;
    QDir m_dirUndoRedo;
};

#endif // MODEL_H
