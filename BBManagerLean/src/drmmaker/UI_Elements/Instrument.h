#ifndef INSTRUMENT_H
#define INSTRUMENT_H

#include <QWidget>
#include <QHBoxLayout>
#include <QLayoutItem>
#include <QLayout>
#include <QPushButton>
#include <QVBoxLayout>
#include <QSizePolicy>
#include <QList>
#include <QDebug>
#include <QPalette>

#include "Labels/InstrumentLabel.h"
#include "Xbutton.h"
#include "Dialogs/instrumentconfigdialog.h"
#include "velocity.h"
#include "../Utils/Common.h"

class DrmMakerModelkerModel;

class Instrument : public QWidget
{
    Q_OBJECT

public:
    explicit Instrument(DrmMakerModel *model,
                        QString name,
                        class DrumsetPanel* parent,
                        uint midiId = 0,
                        uint chokeGroup = 0,
                        uint polyPhony = 0,
                        uint volume = 100,
                        uint fillChokeGroup = 0,
                        uint fillChokeDelay = 0,
                        uint nonPercussion = 0);
    ~Instrument();

    // Operators
    bool operator<(const Instrument& obj) const;

    // Getters
    QString getName() const;
    bool isValid();
    QList<int> getStarts() const;
    QList<int> getEnds() const;
    QList<QFileInfo> getFiles() const;
    const QList<Velocity*>& getVelocityList() const { return mVelocityList; }
    DrmMakerModel* getModel() { return mp_Model; }
    uint getMidiId() const;
    uint getChokeGroup();
    uint getPolyPhony();
    uint getVolume();
    uint getFillChokeGroup();
    uint getFillChokeDelay();
    uint getNonPercussion();
    quint32 getInstrumentSize();
    bool getEmptyFilesFlag();

    // Setters
    void setName(QString name);
    void setChokeGroup(uint chokeGroup);
    void setVelocityList(const QList<Velocity*>& velocityList);
protected:
    void dragEnterEvent(QDragEnterEvent *event);
    void dragLeaveEvent(QDragLeaveEvent *event);
    void dropEvent(QDropEvent *event);
    bool eventFilter(QObject*, QEvent* event);

signals:
    void clicked(int id);
    void newInstrumentSize(quint32 old_size, quint32 new_size);
    void deleteButtonPressed(Instrument* self);

private:
    void on_mPbAdd(int start, int end, const QString& file);
    void on_mPbRemove(int pos);

private slots:
    void on_mPbAdd_clicked();
    void on_mPbRemove_clicked(Velocity* vel);
    void sortAndCheck();
    void on_deleteButtonPressed();
    void on_nameClicked(QMouseEvent*);
    void onVelocitySizeChange(quint32 old_velocity, quint32 new_velocity);
    void onVelocityFileSelected(Velocity* self, const QString& file);
    void onVelocitySpinBoxValueChanged(Velocity* self, int id, int value, int old);

public slots:
    void on_Sort_clicked();

private:

    void Init(QString name, bool error, uint midiId, uint chokeGroup, uint polyPhony, uint volume, uint fillChokeGroup, uint fillChokeDelay, uint nonPercussion);
    Velocity* addVelocity();
    quint32 mInstrumentSize;

/* Variables */

    // Layout for all the stuff
    QHBoxLayout* mNameLayout;
    QHBoxLayout* mTitleLayout;
    QVBoxLayout* mVerticalLayout;

    // Instrument name
    InstrumentLabel *mName;

    // Remove Button
    XButton *mDeleteButton;

    // UI items lists
    QList<Velocity *> mVelocityList;

    // Push buttons
    QPushButton *mPbAdd;
    QPushButton *mPbSort;

    // Error flag
    bool mError;
    bool mEmptyFiles;

    DrmMakerModel *mp_Model;

    friend class CmdAddRemoveVelocity;
    friend class CmdChangeInstrumentDetails;
};

#endif // INSTRUMENT_H
