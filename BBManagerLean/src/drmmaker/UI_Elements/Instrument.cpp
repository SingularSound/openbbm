/*
  	This software and the content provided for use with it is Copyright © 2014-2020 Singular Sound 
 	BeatBuddy Manager is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2 as published by
    the Free Software Foundation.
    
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
    
    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/
#include "Instrument.h"
#include "../Model/drmmakermodel.h"
#include "../DrumsetPanel.h"
#include "../../workspace/workspace.h"
#include "../../workspace/contentlibrary.h"
#include "../../workspace/libcontent.h"

#include <QUndoCommand>
#include <QGroupBox>

// ********************************************************************************************** //
// ************************************* UNDO/REDO COMMANDS ************************************* //
// ********************************************************************************************** //
class DrumIndex
{
protected:
    DrmMakerModel* m_model;
    int m_midi_id, m_start, m_end;
    QString m_file;

    DrumIndex(DrmMakerModel* model, Instrument* instr, Velocity* vel = nullptr, const QString& name = nullptr)
        : m_model(model)
        , m_midi_id(instr->getMidiId())
        , m_start(vel ? vel->getStart() : 0)
        , m_end(vel ? vel->getEnd() : 127)
        , m_file(!name.isEmpty() ? name : vel && vel->getFileInfo().exists() ? vel->getFileInfo().absoluteFilePath() : nullptr)
    {}

    Instrument* instrument(int midi_id = -1) const
    {
        if (midi_id < 0) midi_id = m_midi_id;
        foreach (auto x, m_model->getInstrumentList()) {
            if (x->getMidiId() == midi_id) {
                return x;
            }
        }
        return nullptr;
    }

    Velocity* velocity(Instrument* instr) const { return velocity(nullptr, instr); }
    Velocity* velocity(int start, int end, Instrument* instr = nullptr) { return velocity(nullptr, instr, start, end); }
    Velocity* velocity(QString file, Instrument* instr = nullptr, int start = -1, int end = -1) const
    {
        Velocity* name_match = nullptr;
        if (!instr) instr = instrument();
        if (file.isEmpty()) file = m_file;
        if (start < 0) start = m_start;
        if (end < 0) end = m_end;
        auto name = QFileInfo(file).fileName();
        foreach (auto x, instr->getVelocityList()) {
            if (x->getStart() != start) {
            } else if (x->getEnd() != end) {
            } else if (x->getFileInfo().absoluteFilePath() == file) {
                return x;
            } else if (x->getFileInfo().fileName() == name) {
                name_match = x;
            }
        }
        return name_match;
    }
};

static int undo_command_enumerator = 200;

class CmdChangeVelocityFile : public QUndoCommand, DrumIndex
{
    QString m_new, m_old;

    CmdChangeVelocityFile(DrmMakerModel* model, Instrument* instr, Velocity* vel, const QString& file, const QString& old)
        : QUndoCommand(QObject::tr("Changing velocity file", "Undo Commands"))
        , DrumIndex(model, instr, vel)
        , m_new(file)
        , m_old(old)
    {}

public:
    static void queue(Instrument* instr, Velocity* vel, QString file)
    {
        auto model = instr->getModel();

        QDir bkp(model->dirUndoRedo().absolutePath()); // use root tmpdir (one level up the UndoRedo stack)
        bkp.cdUp();


        QRegularExpression re("[ \\\"/<|>:*_?]+");
        auto drumset_name = model->getDrumsetName().replace(re, "_"); 
        bkp.mkdir(drumset_name);
        bkp.cd(drumset_name);
        auto instrument_name = QString("%1-%2").arg(instr->getMidiId()).arg(instr->getName()).replace(re, "_"); 
        bkp.mkdir(instrument_name);
        bkp.cd(instrument_name);
        auto oldinfo = vel->getFileInfo();
        QString old;
        if (oldinfo.exists()) {
            old = bkp.absoluteFilePath(oldinfo.fileName());
            auto name = oldinfo.absoluteFilePath();
            if (name != old) {
                QFile(old).remove();
                QFile(name).copy(old);
            }
        }
        if (!file.isEmpty()) {
            auto name = bkp.absoluteFilePath(QFileInfo(file).fileName());
            if (name != file) {
                QFile(name).remove();
                QFile(file).copy(name);
                file.swap(name);
            }
        }
        model->undoStack()->push(new CmdChangeVelocityFile(model, instr, vel, file, old));
    }
    static void apply(Velocity* vel, const QString& file)
    {
        vel->on_Browse_select_file(file, false);
    }

    void redo() { apply(velocity(m_old), m_new); }
    void undo() { apply(velocity(m_new), m_old); }

    int id() const { static int id = undo_command_enumerator++; return id; }
    bool mergeWith(const QUndoCommand* other)
    {
        auto next = (CmdChangeVelocityFile*)other;
        if (m_model != next->m_model) {
        } else if (m_midi_id != next->m_midi_id) {
        } else if (m_start != next->m_start) {
        } else if (m_end != next->m_end) {
        } else if (m_new != next->m_old) {
        } else {
            m_new = next->m_new;
            return true;
        }
        return false;
    }
};

class CmdAddRemoveVelocity : public QUndoCommand, DrumIndex
{
    const bool m_add;

    CmdAddRemoveVelocity(bool add, DrmMakerModel* model, Instrument* instr, Velocity* vel = nullptr, const QString& name = nullptr)
        : QUndoCommand(add ? QObject::tr("Adding velocity", "Undo Commands") : QObject::tr("Removing velocity", "Undo Commands"))
        , DrumIndex(model, instr, vel, name)
        , m_add(add)
    {}

    void add()
    {
        auto instr = instrument();
        instr->on_mPbAdd(m_start, m_end, m_file);
        instr->on_Sort_clicked();
    }

    void remove()
    {
        auto instr = instrument();
        auto vel = velocity(instr);
        auto pos = instr->mVelocityList.indexOf(vel);
        instr->on_mPbRemove(pos);
    }

public:
    static void queue(bool add, Instrument* instr, Velocity* vel = nullptr)
    {
        auto model = instr->mp_Model;
        QString name;
        if (vel && vel->getFileInfo().exists()) {

            QDir bkp(model->dirUndoRedo().absolutePath()); // use root tmpdir (one level up the UndoRedo stack)
            bkp.cdUp();

            QRegularExpression re("[ \\\"/<|>:*_?]+");
            auto drumset_name = model->getDrumsetName().replace(re, "_"); 
            bkp.mkdir(drumset_name);
            bkp.cd(drumset_name);
            auto instrument_name = QString("%1-%2").arg(instr->getMidiId()).arg(instr->getName()).replace(re, "_"); 
            bkp.mkdir(instrument_name);
            bkp.cd(instrument_name);
            auto file = vel->getFileInfo();
            name = bkp.absoluteFilePath(file.fileName());
            if (name != file.absoluteFilePath()) {
                QFile(name).remove();
                QFile(file.absoluteFilePath()).copy(name);
            }
        }
        model->undoStack()->push(new CmdAddRemoveVelocity(add, model, instr, vel, name));
    }

    void redo() { (this->*(m_add ? &CmdAddRemoveVelocity::add : &CmdAddRemoveVelocity::remove))(); }
    void undo() { (this->*(m_add ? &CmdAddRemoveVelocity::remove : &CmdAddRemoveVelocity::add))(); }
};

class CmdChangeVelocityRange : public QUndoCommand, DrumIndex
{
    static bool block;
    int m_start_new, m_start_old, m_end_new, m_end_old;

    CmdChangeVelocityRange(DrmMakerModel* model, Instrument* instr, Velocity* vel, int id, int value, int old)
        : QUndoCommand(QObject::tr("Changing velocity range", "Undo Commands"))
        , DrumIndex(model, instr, vel)
        , m_start_new(id ? m_start : value)
        , m_start_old(id ? m_start : old)
        , m_end_new(id ? value : m_end)
        , m_end_old(id ? old : m_end)
    {}

public:
    static void queue(Instrument* instr, Velocity* vel, int id, int value, int old)
    {
        if (block) return;
        auto model = instr->getModel();
        model->undoStack()->push(new CmdChangeVelocityRange(model, instr, vel, id, value, old));
    }
    static void apply(Velocity* vel, int from, int to)
    {
        if (vel) try {
            block = true;
            vel->setStart(from);
            vel->setEnd(to);
            block = false;
        } catch(...) {
            block = false;
            throw;
        }
    }

    void redo() { apply(velocity(m_start_old, m_end_old), m_start_new, m_end_new); }
    void undo() { apply(velocity(m_start_new, m_end_new), m_start_old, m_end_old); }

    int id() const { static int id = undo_command_enumerator++; return id; }
    bool mergeWith(const QUndoCommand* other)
    {
        auto next = (CmdChangeVelocityRange*)other;
        if (m_model != next->m_model) {
        } else if (m_midi_id != next->m_midi_id) {
        } else {
            m_start_new = next->m_start_new;
            m_end_new = next->m_end_new;
            return true;
        }
        return false;
    }
};
bool CmdChangeVelocityRange::block = false;

class CmdChangeInstrumentDetails : public QUndoCommand, DrumIndex
{
    QString m_new_name, m_old_name;
    int m_new_midi_id, m_old_midi_id;
    int m_new_choke_group, m_old_choke_group;
    uint m_new_polyphony, m_old_polyphony;
    uint m_new_volume, m_old_volume;
    uint m_new_fill_choke_group, m_old_fill_choke_group;
    uint m_new_fill_choke_delay, m_old_fill_choke_delay;
    uint m_new_nonPercussion, m_old_nonPercussion;

    CmdChangeInstrumentDetails(DrmMakerModel* model, Instrument* instr,
            const QString& new_name, int new_midi_id, int new_choke_group, uint new_polyphony, uint new_volume, uint new_fill_choke_group, uint new_fill_choke_delay, uint new_nonPercussion,
            const QString& old_name, int old_midi_id, int old_choke_group, uint old_polyphony, uint old_volume, uint old_fill_choke_group, uint old_fill_choke_delay, uint old_nonPercussion)
        : QUndoCommand(QObject::tr("Changing instrument details", "Undo Commands"))
        , DrumIndex(model, instr)
        , m_new_name(new_name), m_old_name(old_name)
        , m_new_midi_id(new_midi_id), m_old_midi_id(old_midi_id)
        , m_new_choke_group(new_choke_group), m_old_choke_group(old_choke_group)
        , m_new_polyphony(new_polyphony), m_old_polyphony(old_polyphony)
        , m_new_volume(new_volume), m_old_volume(old_volume)
        , m_new_fill_choke_group(new_fill_choke_group), m_old_fill_choke_group(old_fill_choke_group)
        , m_new_fill_choke_delay(new_fill_choke_delay), m_old_fill_choke_delay(old_fill_choke_delay)
        , m_new_nonPercussion(new_nonPercussion), m_old_nonPercussion(old_nonPercussion)
    {}

public:
    static void queue(Instrument* instr,
        const QString& new_name, int new_midi_id, int new_choke_group, uint new_polyphony, uint new_volume, uint new_fill_choke_group, uint new_fill_choke_delay, uint new_nonPercussion,
        const QString& old_name, int old_midi_id, int old_choke_group, uint old_polyphony, uint old_volume, uint old_fill_choke_group, uint old_fill_choke_delay, uint old_nonPercussion)
    {
        if (new_name != old_name) {
        } else if (new_midi_id != old_midi_id) {
        } else if (new_choke_group != old_choke_group) {
        } else if (new_polyphony != old_polyphony) {
        } else if (new_volume != old_volume) {
        } else if (new_fill_choke_group != old_fill_choke_group) {
        } else if (new_fill_choke_delay != old_fill_choke_delay) {
        } else if (new_nonPercussion != old_nonPercussion) {
        } else {
            return; // no changes
        }
        auto model = instr->getModel();
        model->undoStack()->push(new CmdChangeInstrumentDetails(model, instr,
            new_name, new_midi_id, new_choke_group, new_polyphony, new_volume, new_fill_choke_group, new_fill_choke_delay, new_nonPercussion,
            old_name, old_midi_id, old_choke_group, old_polyphony, old_volume, old_fill_choke_group, old_fill_choke_delay, old_nonPercussion));
    }
    static void apply(Instrument* instr, const QString& name, int midi_id, int choke_group, uint polyphony, uint volume, uint fill_choke_group, uint fill_choke_delay, uint nonPercussion)
    {
        auto label = instr->mName;
        label->setName(name);
        label->setMidiId(midi_id);
        label->setChokeGroup(choke_group);
        label->setPolyphony(polyphony);
        label->setVolume(volume);
        label->setFillChokeGroup(fill_choke_group);
        label->setFillChokeDelay(fill_choke_delay);
        label->setNonPercussion(nonPercussion);
    }

    void redo() { apply(instrument(m_old_midi_id), m_new_name, m_new_midi_id, m_new_choke_group, m_new_polyphony, m_new_volume, m_new_fill_choke_group, m_new_fill_choke_delay, m_new_nonPercussion); }
    void undo() { apply(instrument(m_new_midi_id), m_old_name, m_old_midi_id, m_old_choke_group, m_old_polyphony, m_old_volume, m_old_fill_choke_group, m_old_fill_choke_delay, m_old_nonPercussion); }

    int id() const { static int id = undo_command_enumerator++; return id; }
    bool mergeWith(const QUndoCommand* other)
    {
        auto next = (CmdChangeInstrumentDetails*)other;
        if (m_model != next->m_model) {
        } else if (m_new_midi_id != next->m_old_midi_id) {
        } else {
            m_new_name = next->m_new_name;
            m_new_midi_id = next->m_new_midi_id;
            m_new_choke_group = next->m_new_choke_group;
            m_new_polyphony = next->m_new_polyphony;
            m_new_volume = next->m_new_volume;
            m_new_fill_choke_group = next->m_new_fill_choke_group;
            m_new_fill_choke_delay = next->m_new_fill_choke_delay;
            m_new_nonPercussion = next->m_new_nonPercussion;
            return true;
        }
        return false;
    }
};

// ********************************************************************************************** //
// ************************************* CONSTRUCTOR & INIT ************************************* //
// ********************************************************************************************** //

// New instrument with name and midi Id
Instrument::Instrument(DrmMakerModel* model, QString name, DrumsetPanel* parent, uint midiId, uint chokeGroup,
                       uint polyPhony, uint volume, uint fillChokeGroup, uint fillChokeDelay, uint nonPercussion)
    : QWidget(parent)
{
    // Mouse hover management
    installEventFilter(this);

    mp_Model = model;

    // Make drop possible
    this->acceptDrops();
    this->setAutoFillBackground(true); // Used to change background on "on enter/exit" events

    // Get the instrument widget's pallet
    QPalette pal(this->palette());

    // Remove background
    pal.setColor(QPalette::Background, Qt::transparent);
    this->setPalette(pal);

    // Common init
    Init(name, false, midiId, chokeGroup, polyPhony, volume,fillChokeGroup, fillChokeDelay, nonPercussion);
    connect(this, SIGNAL(deleteButtonPressed(Instrument*)), parent, SLOT(on_instrument_deleteButt_onPressed(Instrument*)));

    Velocity* vel = new Velocity(mp_Model);
    vel->on_Volume_Changed(volume);
    mVelocityList.append(vel);
    mVerticalLayout->addWidget(vel);
    mInstrumentSize = vel->size();

    connect(vel, SIGNAL(spinBoxValueChanged(Velocity*, int, int, int)), this, SLOT(sortAndCheck()));
    connect(vel, SIGNAL(textFieldValueChanged()), this, SLOT(sortAndCheck()));
    connect(vel, SIGNAL(removeRequest(Velocity*)), this, SLOT(on_mPbRemove_clicked(Velocity*)));
    connect(vel, SIGNAL(newWavSize(quint32,quint32)),this, SLOT(onVelocitySizeChange(quint32,quint32)));
    connect(vel, SIGNAL(fileSelected(Velocity*, const QString&)),this, SLOT(onVelocityFileSelected(Velocity*, const QString&)));
    connect(vel, SIGNAL(spinBoxValueChanged(Velocity*, int, int, int)), this, SLOT(onVelocitySpinBoxValueChanged(Velocity*, int, int, int)));
    connect(mName, SIGNAL(sigVolumeChanged(uint)), vel, SLOT(on_Volume_Changed(uint)));

    vel->setRemoveEnabled(false);

    // Emit new instrument size signal

    mp_Model->setInstrumentSize(0,mInstrumentSize);
}

void Instrument::Init(QString name, bool error, uint midiId, uint chokeGroup, uint polyPhony, uint volume, uint fillChokeGroup, uint fillChokeDelay, uint nonPercussion){
    // Set Object name
    setObjectName(name+"_obj");

    // Accept drag and drops
    this->setAcceptDrops(true);

    // At creation this object has no spinBox errors
    mError = error;

    // Empty instrument at the creation
    mInstrumentSize = 0;

    // Set the size policy of this widget
    this->setMaximumWidth(650);
    QSizePolicy thisSizePolicy(QSizePolicy::Maximum, QSizePolicy::MinimumExpanding);
    this->setSizePolicy(thisSizePolicy);

    // Assign the instrument name, midi ID, chokeGroup and polyPhony
    mName = new InstrumentLabel(mp_Model, name, midiId, chokeGroup, nonPercussion);
    mName->setMidiId(midiId);
    mName->setVolume(volume);
    mName->setErrorState(error);
    mName->setChokeGroup(chokeGroup);
    mName->setPolyphony(polyPhony);
    mName->setFillChokeGroup(fillChokeGroup);
    mName->setFillChokeDelay(fillChokeDelay);
    mName->setNonPercussion(nonPercussion);
    mName->refreshText();

    connect(mName, &ClickableLabel::clicked, this, &Instrument::on_nameClicked);

    // Set sizing constraints to label
    QSizePolicy labelSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
    mName->setSizePolicy(labelSizePolicy);

    // Create X button (delete instrument)
    mDeleteButton = new XButton(mp_Model);

    auto lo = new QGridLayout(this);
    auto group = new QGroupBox(this);
    lo->addWidget(group, 0, 0);

    // Create main vertical layout
    mVerticalLayout = new QVBoxLayout(group);
    mVerticalLayout->setObjectName(name+"_VerticalLayout");
    mVerticalLayout->setAlignment(Qt::AlignTop);
    mVerticalLayout->setSizeConstraint(QLayout::SetMinimumSize);
    mVerticalLayout->setSpacing(0);

    // Create name vertical layout
    mNameLayout = new QHBoxLayout(group);
    mNameLayout->setObjectName(name+"_NameLayout");
    mNameLayout->setAlignment(Qt::AlignTop);
    mNameLayout->setSizeConstraint(QLayout::SetMinimumSize);

    // Create title vertical layout
    mTitleLayout = new QHBoxLayout(group);
    mTitleLayout->setObjectName(name+"_TitleLayout");
    mTitleLayout->setAlignment(Qt::AlignTop);
    mTitleLayout->setSizeConstraint(QLayout::SetMinimumSize);

    // Create the buttons
    QIcon addIcon;
    addIcon.addFile(":/drawable/add.png", QSize(), QIcon::Normal, QIcon::Off);
    mPbAdd = new QPushButton(addIcon, tr("Add"), this);

    QIcon sortIcon;
    sortIcon.addFile(":/drawable/refresh.png", QSize(), QIcon::Normal, QIcon::Off);
    mPbSort = new QPushButton(sortIcon, tr("Sort"), this);

    // Connect button signals to appropriate slot
    connect(mPbSort, SIGNAL(clicked()), this, SLOT(on_Sort_clicked()));
    connect(mPbAdd, SIGNAL(clicked()), this, SLOT(on_mPbAdd_clicked()));
    connect(mDeleteButton,SIGNAL(Clicked()),this,SLOT(on_deleteButtonPressed()));

    // Initial value
    mEmptyFiles = true;

    // Add UI elements to gridlayout
    mNameLayout->addWidget(mName, 0, Qt::AlignLeft);
    mNameLayout->addWidget(mDeleteButton, 0, Qt::AlignRight);
    mTitleLayout->addSpacing(12);
    mTitleLayout->addWidget(new QLabel(tr("Start")), 0, Qt::AlignLeft);
    mTitleLayout->addSpacing(18);
    mTitleLayout->addWidget(new QLabel(tr("End")), 0, Qt::AlignLeft);
    mTitleLayout->addSpacing(24);
    mTitleLayout->addWidget(new QLabel(tr("Filename")), 0, Qt::AlignLeft);
    mTitleLayout->addStretch();
    mTitleLayout->addWidget(mPbSort, 0, Qt::AlignRight);
    mTitleLayout->addWidget(mPbAdd, 0, Qt::AlignRight);

    // Add Horizontal layouts to Vertical layout
    mVerticalLayout->addLayout(mNameLayout);
    mVerticalLayout->addLayout(mTitleLayout);

    setStyleSheet("QGroupBox { border: 1px solid black; border-radius: 9px }");
}

// ******************************************************************************************** //
// **************************************** DESTRUCTOR **************************************** //
// ******************************************************************************************** //

Instrument::~Instrument(){
    // Remove all velocities
    foreach (Velocity* v, mVelocityList) {
        // remove the size (useless but...)
        mInstrumentSize -= v->size();
        delete v;
    }

    delete mDeleteButton;
    delete mName;
    delete mPbAdd;
    delete mPbSort;
    delete mNameLayout;
    delete mTitleLayout;
    delete mVerticalLayout;
}

// *********************************************************************************************** //
// ****************************************** OPERATORS ****************************************** //
// *********************************************************************************************** //

bool Instrument::operator<(const Instrument& obj) const {
    return (this->getMidiId() < obj.getMidiId());
}

// ************************************************************************************************ //
// **************************************** PUBLIC METHODS **************************************** //
// ************************************************************************************************ //

QString Instrument::getName() const{
    return mName->getName();
}

bool Instrument::isValid() {
    bool oneValidFile = false;
    bool oneEmptyField = false;
    if (!mError) {
        for (int i = 0; i < mVelocityList.size(); ++i) {
            // If there is an empty field and at least one file was in another field, return an arror
            if (mVelocityList.at(i)->isEmpty()) {
                oneEmptyField = true;
                if (oneValidFile){
                    return false;
                }
                // If the field in not empty and valid, but there was at least one empty field, return an error
            } else if (mVelocityList.at(i)->isFileValid()) {
                mEmptyFiles = false;
                oneValidFile = true;
                if (oneEmptyField){
                    return false;
                }
                // Field is not empty and invalid, return an error
            } else {
                mEmptyFiles = false;
                return false;
            }
        }
        return true;
    } else {
        return false;
    }
}

QList<int> Instrument::getStarts() const{
    QList<int> list;
    foreach (Velocity* v, mVelocityList) {
        list.append(v->getStart());
    }
    return list;
}

QList<int> Instrument::getEnds() const{
    QList<int> list;
    foreach (Velocity* v, mVelocityList) {
        list.append(v->getEnd());
    }
    return list;
}

QList<QFileInfo> Instrument::getFiles() const{
    QList<QFileInfo> list;
    foreach (Velocity* v, mVelocityList) {
        list.append(v->getFileInfo());
    }
    return list;
}

void Instrument::setName(QString name){
    mName->setText(name);
}

void Instrument::setChokeGroup(uint chokeGroup){
    mName->setChokeGroup(chokeGroup);
}

void Instrument::setVelocityList(const QList<Velocity*>& velocityList){
    quint32 oldsize = mInstrumentSize;

    // Remove all velocities
    foreach (Velocity* v, mVelocityList) {
        // Remove its connections and delete it
        disconnect(v, SIGNAL(spinBoxValueChanged(Velocity*, int, int, int)), this, SLOT(sortAndCheck()));
        disconnect(v, SIGNAL(textFieldValueChanged()), this, SLOT(sortAndCheck()));
        disconnect(v, SIGNAL(removeRequest(Velocity*)), this, SLOT(on_mPbRemove_clicked(Velocity*)));
        disconnect(v, SIGNAL(fileSelected(Velocity*, const QString&)),this, SLOT(onVelocityFileSelected(Velocity*, const QString&)));
        disconnect(v, SIGNAL(spinBoxValueChanged(Velocity*, int, int, int)), this, SLOT(onVelocitySpinBoxValueChanged(Velocity*, int, int, int)));
        disconnect(mName, SIGNAL(sigVolumeChanged(uint)), v, SLOT(on_Volume_Changed(uint)));


        // Add the of the velocity to the instrument and connect size callback
        mInstrumentSize -= v->size();
        disconnect(v, SIGNAL(newWavSize(quint32,quint32)),this, SLOT(onVelocitySizeChange(quint32,quint32)));
        delete v;
    }
    mVelocityList.clear();

    // Assign the new list
    mVelocityList = velocityList;

    // Add to layout
    foreach (Velocity* v, mVelocityList) {
        mVerticalLayout->addWidget(v);
        v->on_Volume_Changed(mName->getVolume());

       mInstrumentSize += v->size();

        connect(v, SIGNAL(spinBoxValueChanged(Velocity*, int, int, int)), this, SLOT(sortAndCheck()));
        connect(v, SIGNAL(textFieldValueChanged()), this, SLOT(sortAndCheck()));
        connect(v, SIGNAL(removeRequest(Velocity*)), this, SLOT(on_mPbRemove_clicked(Velocity*)));
        connect(v, SIGNAL(newWavSize(quint32,quint32)),this, SLOT(onVelocitySizeChange(quint32,quint32)));
        connect(v, SIGNAL(fileSelected(Velocity*, const QString&)),this, SLOT(onVelocityFileSelected(Velocity*, const QString&)));
        connect(v, SIGNAL(spinBoxValueChanged(Velocity*, int, int, int)), this, SLOT(onVelocitySpinBoxValueChanged(Velocity*, int, int, int)));
        connect(mName, SIGNAL(sigVolumeChanged(uint)), v, SLOT(on_Volume_Changed(uint)));


        v->setRemoveEnabled(true);
    }

    // Disable remove button if it's the case
    if(mVelocityList.size() == 1){
        mVelocityList.first()->setRemoveEnabled(false);
    }

    // Disable add button if the list is full
    if (mVelocityList.size() == MIDIPARSER_MAX_NUMBER_VELOCITY-1) {
        mPbAdd->setDisabled(true);
    }

    // Emit instrument size change

    mp_Model->setInstrumentSize(oldsize,mInstrumentSize);
    //emit newInstrumentSize(oldsize,mInstrumentSize);

}

uint Instrument::getMidiId() const {
    return mName->getMidiId();
}

uint Instrument::getChokeGroup() {
    return mName->getChokeGroup();
}

uint Instrument::getPolyPhony() {
    return mName->getPolyphony();
}

uint Instrument::getVolume(){
    return mName->getVolume();
}

uint Instrument::getFillChokeGroup()
{
    return mName->getFillChokeGroup();
}

uint Instrument::getFillChokeDelay()
{
    return mName->getFillChokeDelay();
}

uint Instrument::getNonPercussion()
{
    return mName->getNonPercussion();
}

quint32 Instrument::getInstrumentSize(){
    return mInstrumentSize;
}

bool Instrument::getEmptyFilesFlag(){
    return mEmptyFiles;
}

// *************************************************************************************************** //
// ********************************************** SLOTS ********************************************** //
// *************************************************************************************************** //

void Instrument::on_mPbAdd_clicked()
{
    CmdAddRemoveVelocity::queue(true, this);
}

void Instrument::on_mPbAdd(int start, int end, const QString& file)
{
    // Enable remove buttons
    mVelocityList.first()->setRemoveEnabled(true);

    // Add to grid layout
    auto vel = new Velocity(mp_Model, start, end, file);
    vel->on_Volume_Changed(mName->getVolume());
    mVelocityList.append(vel);
    mVerticalLayout->addWidget(vel);


    // Connect signals
    connect(vel, SIGNAL(spinBoxValueChanged(Velocity*, int, int, int)), this, SLOT(sortAndCheck()));
    connect(vel, SIGNAL(textFieldValueChanged()), this, SLOT(sortAndCheck()));
    connect(vel, SIGNAL(removeRequest(Velocity*)), this, SLOT(on_mPbRemove_clicked(Velocity*)));
    connect(vel, SIGNAL(newWavSize(quint32,quint32)),this, SLOT(onVelocitySizeChange(quint32,quint32)));
    connect(vel, SIGNAL(fileSelected(Velocity*, const QString&)),this, SLOT(onVelocityFileSelected(Velocity*, const QString&)));
    connect(vel, SIGNAL(spinBoxValueChanged(Velocity*, int, int, int)), this, SLOT(onVelocitySpinBoxValueChanged(Velocity*, int, int, int)));
    connect(mName, SIGNAL(sigVolumeChanged(uint)), vel, SLOT(on_Volume_Changed(uint)));

    // Disable add button if we added the last one
    if (mVelocityList.size() == MIDIPARSER_MAX_NUMBER_VELOCITY-1) {
        mPbAdd->setDisabled(true);
    }

    // Update error state
    sortAndCheck();

}

/**
 * @brief Callback when the size of a velocity is changed
 * @param old_velocity : Old size value
 * @param new_velocity : New size value
 */
void Instrument::onVelocitySizeChange(quint32 old_velocity, quint32 new_velocity){
    quint32 oldSize = mInstrumentSize;
    mInstrumentSize += (new_velocity - old_velocity);

    // Emit new intrument size signal
    mp_Model->setInstrumentSize(oldSize,mInstrumentSize);

}

void Instrument::onVelocityFileSelected(Velocity* self, const QString& file)
{
    CmdChangeVelocityFile::queue(this, self, file);
}

void Instrument::onVelocitySpinBoxValueChanged(Velocity* self, int id, int value, int old)
{
    CmdChangeVelocityRange::queue(this, self, id, value, old);
}

/**
 * @brief Callback from the push button to remove a velocity
 * @param reference to the calling object (velocity)
 */
void Instrument::on_mPbRemove_clicked(Velocity* vel)
{
    CmdAddRemoveVelocity::queue(false, this, vel);
}

void Instrument::on_mPbRemove(int pos)
{
    auto oldSize = mInstrumentSize;
    auto velocity = mVelocityList[pos];
    mInstrumentSize -= velocity->size();

    mVelocityList.removeAt(pos);
    delete velocity;

    // Disable remove button if it's the case
    if(mVelocityList.size() == 1){
        mVelocityList.first()->setRemoveEnabled(false);
    }

    // Enable add button
    mPbAdd->setEnabled(true);

    // Update error state
    sortAndCheck();

    // Emit new instrument size signal
    mp_Model->setInstrumentSize(oldSize,mInstrumentSize);
}

void Instrument::sortAndCheck(){
    bool error = false;

    // Sort the list
    qSort(mVelocityList.begin(), mVelocityList.end(), [](Velocity* v1, Velocity* v2) { return *v1 < *v2; });

    // Case where the user could put 0 at the end value of first velocity range
    if(mVelocityList.first()->getStart() != 0){
        error = true;
    } else {
        for (int i = 1; i < mVelocityList.size(); ++i) {
            // End value of preceding range shoud be the start value of the current range minus 1 (-1) to avoid gaps
            if (mVelocityList.at(i-1)->getEnd() != (mVelocityList.at(i)->getStart())-1) {

                // If the preceding condition is false both values from each line (current and preceding) should be equal
                if( (mVelocityList.at(i)->getStart() == mVelocityList.at(i-1)->getStart()) &&
                        (mVelocityList.at(i)->getEnd() == mVelocityList.at(i-1)->getEnd()) ){

                    // If both lines are equal, end value should be bigger than the start value of
                    if (mVelocityList.at(i)->getStart() >= mVelocityList.at(i)->getEnd()) {
                        error = true;
                        break;
                    }
                } else {
                    error = true;
                    break;
                }
            }
        }
        // Check file paths if the instrument has files
        if(!mEmptyFiles){
            foreach(Velocity* vel, mVelocityList){
                if(!vel->isFileValid()){
                    error = true;
                }
            }
        }
    }

    // Set the flag
    mError = error;

    // Change label according to error state
    mName->setErrorState(mError);
}





void Instrument::on_deleteButtonPressed(){
    emit deleteButtonPressed(this);
}

void Instrument::on_nameClicked(QMouseEvent*){
    auto name = mName->getName();
    auto midi_id = mName->getMidiId();
    auto choke_group = mName->getChokeGroup();
    auto polyphony = mName->getPolyphony();
    auto volume = mName->getVolume();
    auto fill_choke_group = mName->getFillChokeGroup();
    auto fill_choke_delay = mName->getFillChokeDelay();
    auto nonPercussion= mName->getNonPercussion();

    InstrumentConfigDialog dialog(mp_Model, name, midi_id, choke_group, polyphony, volume, fill_choke_group, fill_choke_delay, nonPercussion, this);
    // If accepted, get the new values since the info are self contain in the dialog
    if(dialog.exec()) {
        CmdChangeInstrumentDetails::queue(this,
            dialog.getName(), dialog.getMidiId(), dialog.getChokeGroup(), dialog.getPolyPhony(), dialog.getVolume(), dialog.getFillChokeGroup(), dialog.getFillChokeDelay(),dialog.getNonPercussion(),
            name, midi_id, choke_group, polyphony, volume, fill_choke_group, fill_choke_delay, nonPercussion);

        mName->refreshText();
    }
}

void Instrument::on_Sort_clicked(){
    // Check for errors
    sortAndCheck();

    // If there is no error in spinboxes, update Ui
    if(!mError){
        // Remove all velocities from layout
        foreach (Velocity* v, mVelocityList) {
            mVerticalLayout->removeWidget(v);
            v->hide();
        }
        // Put them back in
        foreach (Velocity* v, mVelocityList) {
            mVerticalLayout->addWidget(v);
            v->show();
        }
    }
}

// *********************************************************************************************** //
// ************************************** PROTECTED METHODS ************************************** //
// *********************************************************************************************** //

void Instrument::dragEnterEvent(QDragEnterEvent *event){
    // Get the instrument widget's pallet
    QPalette pal(this->palette());

    // Set gray background
    if(mVelocityList.size() < MIDIPARSER_MAX_NUMBER_VELOCITY){
        pal.setColor(QPalette::Background, Qt::lightGray);
    } else {
        bool emptySpace = false;
        foreach (Velocity* vel, mVelocityList) {
            if(vel->isEmpty()){
                emptySpace = true;
                break;
            }
        }
        if(emptySpace){
            pal.setColor(QPalette::Background, Qt::lightGray);
        } else {
            pal.setColor(QPalette::Background, QColor(255, 0, 0, 127));
        }
    }
    this->setPalette(pal);
    event->accept();
}
void Instrument::dragLeaveEvent(QDragLeaveEvent *event){
    // Get the instrument widget's pallet
    QPalette pal(this->palette());

    // Remove background
    pal.setColor(QPalette::Background, Qt::transparent);
    this->setPalette(pal);
    event->accept();
}

void Instrument::dropEvent(QDropEvent *event){
    // Get the instrument widget's pallet
    QPalette pal(this->palette());
    // Remove background
    pal.setColor(QPalette::Background, Qt::transparent);
    this->setPalette(pal);

    // Get the files
    QList<QUrl> fileList = event->mimeData()->urls();

    // Populate empty velocities
    int fileIndex = 0;
    int velIndex = 0;
    while((fileIndex < fileList.size()) && (velIndex < mVelocityList.size())){
        Velocity* vel = mVelocityList.at(velIndex++);
        if(vel->isEmpty()){
            vel->setFilePath(fileList.at(fileIndex++).toLocalFile());
            mEmptyFiles = false;
        }
    }

    // Check the velocity count with included files
    if((fileList.size()-fileIndex + mVelocityList.size()) >= MIDIPARSER_MAX_NUMBER_VELOCITY){    // More files than the max number of velocities
        while(mVelocityList.size() < MIDIPARSER_MAX_NUMBER_VELOCITY){
            Velocity* currentVelocity = addVelocity();
            currentVelocity->setFilePath(fileList.at(fileIndex++).toLocalFile());
            mEmptyFiles = false;
        }
    } else {                                                                          // Less files than the max number of velocities
        for(int i=fileIndex; i<fileList.size(); i++) {
            Velocity* currentVelocity = addVelocity();
            currentVelocity->setFilePath(fileList.at(i).toLocalFile());
            mEmptyFiles = false;
        }
    }

    // Sort the list
    on_Sort_clicked();
    event->accept();
}

bool Instrument::eventFilter(QObject*, QEvent* event) {
    if (event->type() == QEvent::Enter) {
        QString color("white");
        setStyleSheet("QGroupBox { border: 1px solid " + color + "; border-radius: 9px }");
    } else if(event->type() == QEvent::Leave) {
        setStyleSheet("QGroupBox { border: 1px solid black; border-radius: 9px }");
    }
    return false;
}
// *********************************************************************************************** //
// *************************************** PRIVATE METHODS *************************************** //
// *********************************************************************************************** //

Velocity* Instrument::addVelocity(){
    // Enable remove buttons
    mVelocityList.first()->setRemoveEnabled(true);

    // Add to grid layout
    Velocity* vel = new Velocity(nullptr, 127);
    vel->on_Volume_Changed(mName->getVolume());
    mVelocityList.append(vel);
    mVerticalLayout->addWidget(vel);

    // Add the of the velocity to the instrument
    quint32 oldSize = mInstrumentSize;
    mInstrumentSize += vel->size();

    connect(vel, SIGNAL(spinBoxValueChanged(Velocity*, int, int, int)),      this, SLOT(sortAndCheck()));
    connect(vel, SIGNAL(textFieldValueChanged()),    this, SLOT(sortAndCheck()));
    connect(vel, SIGNAL(removeRequest(Velocity*)),   this, SLOT(on_mPbRemove_clicked(Velocity*)));
    connect(vel, SIGNAL(newWavSize(quint32,quint32)),this, SLOT(onVelocitySizeChange(quint32,quint32)));
    connect(vel, SIGNAL(fileSelected(Velocity*, const QString&)),this, SLOT(onVelocityFileSelected(Velocity*, const QString&)));
    connect(vel, SIGNAL(spinBoxValueChanged(Velocity*, int, int, int)), this, SLOT(onVelocitySpinBoxValueChanged(Velocity*, int, int, int)));
    connect(mName, SIGNAL(sigVolumeChanged(uint)), vel, SLOT(on_Volume_Changed(uint)));

    // Disable add button if we added the last one
    if (mVelocityList.size() == MIDIPARSER_MAX_NUMBER_VELOCITY-1) {
        mPbAdd->setDisabled(true);
    }

    // Emit new instrument size sigal

    mp_Model->setInstrumentSize(oldSize,mInstrumentSize);
    return vel;
}
