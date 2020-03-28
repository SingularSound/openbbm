#ifndef INSTRUMENTLABEL_H
#define INSTRUMENTLABEL_H

#include "ClickableLabel.h"

class DrmMakerModel;

class InstrumentLabel : public BigTextClickableLabelWithErrorIndicator
{
   Q_OBJECT
public:
    explicit InstrumentLabel(DrmMakerModel *model, const QString& text = nullptr, int midiId = 0, int chokeGroup = 0, int nonPercussion = 0,QWidget * parent = nullptr);

    void refreshText();

    uint getMidiId();
    uint getChokeGroup();
    void setMidiId(uint id);
    void setChokeGroup(uint chokeGroup);
    void setVolume(uint volume);
    uint getVolume();
    uint getPolyphony();
    void setPolyphony(uint polyPhony);
    void setName(QString name);
    void setFillChokeGroup(uint fillChokeGroup);
    uint getFillChokeGroup();
    void setFillChokeDelay(uint fillChokeDelay);
    uint getFillChokeDelay();
    void setNonPercussion(uint nonPercussion);
    uint getNonPercussion();


signals:
    void sigVolumeChanged(uint volume);

protected:

private:
    uint mMidiID;
    uint mVolume;
    uint mChokeGroup;
    uint mPolyPhony;
    uint mFillChokeGroup;
    uint mFillChokeDelay;
    uint nonPercussion;

    DrmMakerModel *mp_Model;
};

#endif // INSTRUMENTLABEL_H
