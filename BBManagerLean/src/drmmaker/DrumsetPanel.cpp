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
#include <QWidget>
#include <QPainter>
#include <QDebug>
#include <QScrollArea>
#include <QListWidgetItem>
#include <QGridLayout>
#include "DrumsetPanel.h"
#include "../workspace/workspace.h"
#include "../workspace/contentlibrary.h"
#include "../workspace/libcontent.h"
#include <QSizePolicy>
#include "../model/beatsmodelfiles.h"
#include "../utils/dirlistallsubfilesmodal.h"
#include "../utils/dircleanupmodal.h"
#include "../model/index.h"
#include "workspace/workspace.h"



#define MAXIMUM_DRUMSET_SIZE 100 * 1024 * 1024


// Default instrument constants
const QString   defaultInstrumentArray[]    = {
    QObject::tr("Bass Drum", "Default Instrument"),
    QObject::tr("Snare", "Default Instrument"),
    QObject::tr("Low Floor Tom", "Default Instrument"),
    QObject::tr("High Floor Tom", "Default Instrument"),
    QObject::tr("Low Tom", "Default Instrument"),
    QObject::tr("Stick", "Default Instrument"),
    QObject::tr("Closed Hi Hat", "Default Instrument"),
    QObject::tr("Open Hi-Hat", "Default Instrument"),
    QObject::tr("Pedal Hi-Hat", "Default Instrument"),
    QObject::tr("Crash Cymbal", "Default Instrument"),
    QObject::tr("Splash Cymbal", "Default Instrument"),
    QObject::tr("Ride Cymbal", "Default Instrument"),
    QObject::tr("Ride Bell", "Default Instrument"),
    nullptr
};
const uint      defaultInstrumentMidiIds[]  = {36, 38, 41, 43, 45, 37, 42, 46, 44, 49, 55, 51, 53};

// ********************************************************************************************* //
// ************************************* UNDO/REDO COMMANDS ************************************ //
// ********************************************************************************************* //
static int undo_command_enumerator = 100;
class CmdOpenDrumset : public QUndoCommand
{
    DrumsetPanel* m_panel;
    QString m_name, m_path, m_oldpath;

    CmdOpenDrumset(DrumsetPanel* panel, const QString& name, const QString& path, const QString& oldpath)
        : QUndoCommand()
        , m_panel(panel)
        , m_name(name)
        , m_path(path)
        , m_oldpath(oldpath)
    {
        updateText();
    }
    void updateText()
    {
        setText(QObject::tr("Opening Drumset \"%1\"", "Undo Commands")
            .arg(m_name)
            );
    }

public:
    static void queue(DrumsetPanel* panel, const QString& name, const QString& filename, const QString& oldpath)
    {
        panel->mp_drmMakerModel->undoStack()->push(new CmdOpenDrumset(panel, name, filename, oldpath));
    }
    static void apply(DrumsetPanel* panel, const QString& filename)
    {
        panel->saveCurrentDrumset();
        panel->openDrmInternal(filename);
    }

    void redo()
    {
        apply(m_panel, m_path);
    }
    void undo()
    {
        apply(m_panel, m_oldpath);
    }
    int id() const { static int id = undo_command_enumerator++; return id; }
    bool mergeWith(const QUndoCommand* other)
    {
        auto x = (CmdOpenDrumset*)other;
        m_name = x->m_name;
        m_path = x->m_path;
        updateText();
        return true;
    }
};

class CmdAddRemoveInstrument : public QUndoCommand
{
    DrumsetPanel* m_panel;
    const bool m_add;
    struct VelocityInfo {
        int start, end;
        QString file;
        VelocityInfo(Velocity* v)
            : start(v->getStart())
            , end(v->getEnd())
            , file(v->getFileInfo().exists() ? v->getFileInfo().absoluteFilePath() : nullptr)
        {}
    };
    QString m_name;
    uint m_midiId, m_chokeGroup, m_polyPhony, m_volume, m_fillChokeGroup, m_fillChokeDelay, m_nonPercussion;
    QList<VelocityInfo> m_velocity;

    CmdAddRemoveInstrument(bool add, DrumsetPanel* panel, Instrument* instr, QList<VelocityInfo>& velocity)
        : QUndoCommand(add ? QObject::tr("Adding instrument", "Undo Commands") : QObject::tr("Removing instrument", "Undo Commands"))
        , m_panel(panel)
        , m_add(add)
        , m_name(instr ? instr->getName() : QObject::tr("Instrument", "Undo Commands"))
        , m_midiId(instr ? instr->getMidiId() : panel->mp_drmMakerModel->getNextAvailableId(0, false))
        , m_chokeGroup(instr ? instr->getChokeGroup() : 0)
        , m_polyPhony(instr ? instr->getPolyPhony() : 0)
        , m_volume(instr ? instr->getVolume() : 100)
        , m_fillChokeGroup(instr ? instr->getFillChokeGroup() : 0)
        , m_fillChokeDelay(instr ? instr->getFillChokeDelay() : 0)
        , m_nonPercussion(instr ? instr->getNonPercussion() : 0)
    {
        m_velocity.swap(velocity);
    }

    void add()
    {
        auto model = m_panel->mp_drmMakerModel;
        auto instr = new Instrument(model, m_name, m_panel, m_midiId, m_chokeGroup, m_polyPhony, m_volume, m_fillChokeGroup, m_fillChokeDelay, m_nonPercussion);
        QList<Velocity*> velocity;
        foreach (auto v, m_velocity) {
            velocity.append(new Velocity(model, v.start, v.end, v.file));
        }
        if (!velocity.empty()) {
            instr->setVelocityList(velocity);
        }
        model->addInstrument(instr);
        model->sortInstruments();
    }

    void remove()
    {
        for (auto i = 0; i < m_panel->mp_drmMakerModel->getInstrumentCount(); ++i) {
            if (m_panel->mp_drmMakerModel->getInstrument(i)->getMidiId() == m_midiId) {
                m_panel->mp_drmMakerModel->removeInstrument(i);
                return;
            }
        }
        qFatal("CmdAddRemoveInstrument::remove() : instrument not found");
    }

public:
    static void queue(bool add, DrumsetPanel* panel, Instrument* instr = nullptr)
    {
        auto model = panel->mp_drmMakerModel;
        QList<VelocityInfo> velocity;
        if (instr) foreach(auto vel, instr->getVelocityList()) {
            velocity.append(vel);
        }
        model->undoStack()->push(new CmdAddRemoveInstrument(add, panel, instr, velocity));
    }

    void redo() { (this->*(m_add ? &CmdAddRemoveInstrument::add : &CmdAddRemoveInstrument::remove))(); }
    void undo() { (this->*(m_add ? &CmdAddRemoveInstrument::remove : &CmdAddRemoveInstrument::add))(); }
};

// ********************************************************************************************* //
// **************************************** CONSTRUCTOR **************************************** //
// ********************************************************************************************* //

DrumsetPanel::DrumsetPanel(QWidget *parent)
    : QWidget(parent)
    , mDrumSetMaker(nullptr)
{
    // Create a new drumset model
    mp_drmMakerModel = new DrmMakerModel(Workspace().userLibrary()->libWaveSources()->currentPath());
    // Observe instrument array
    connect(mp_drmMakerModel, SIGNAL(instrumentAdded(int)), this, SLOT(instrumentAdded(int)));
    connect(mp_drmMakerModel, SIGNAL(instrumentRemoved(int)), this, SLOT(instrumentRemoved(int)));
    connect(mp_drmMakerModel, SIGNAL(instrumentSorted()), this, SLOT(instrumentSorted()));
    // Observe drumset name
    connect(mp_drmMakerModel, SIGNAL(drmNameChanged(const QString&)), this, SLOT(drumsetNameChanged(const QString&)));
    connect(mp_drmMakerModel, SIGNAL(drmSizeChanged(quint32)), this, SLOT(drumsetSizeChanged(quint32)));

    createLayout();
}


void DrumsetPanel::createLayout()
{
    setObjectName(QStringLiteral("DrumsetPanel"));


    QLabel * p_Title = new QLabel(this);
    p_Title->setText(tr("Drumset Maker"));
    p_Title->setObjectName(QStringLiteral("titleBar"));



    mProgressBar = new DrumsetSizeProgressBar(this);
    QSizePolicy sizePolicy1(QSizePolicy::Minimum, QSizePolicy::Fixed);
    sizePolicy1.setHorizontalStretch(0);
    sizePolicy1.setVerticalStretch(0);
    mProgressBar->setSizePolicy(sizePolicy1);


    mDrumsetName = new DrumsetLabel(tr("Drumset name"), this);
    mDrumsetName->setObjectName(QStringLiteral("drumsetName"));
    mDrumsetName->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    connect(mDrumsetName, &ClickableLabel::clicked, this, &DrumsetPanel::changeDrumsetName);


    QScrollArea *p_ScrollArea = new QScrollArea(this);
    p_ScrollArea->setWidgetResizable(true);

    QWidget *p_ScrollAreaWidgetContents = new QWidget(p_ScrollArea);
    p_ScrollAreaWidgetContents->setObjectName(QStringLiteral("DrumsetScrollArea"));
    p_ScrollArea->setWidget(p_ScrollAreaWidgetContents);

    mp_InstrumentsGridLayout = new QGridLayout(p_ScrollAreaWidgetContents);
    mp_InstrumentsGridLayout->setColumnStretch(0,1);
    mp_InstrumentsGridLayout->setColumnStretch(1,1);


    // Create layout and place all widget
    QGridLayout * p_GridLayout = new QGridLayout();
    setLayout(p_GridLayout);
    p_GridLayout->setContentsMargins(0,0,0,0);

    p_GridLayout->addWidget(p_Title, 0,0,1,2);
    p_GridLayout->addWidget(mDrumsetName,1,0,1,1);
    p_GridLayout->addWidget(mProgressBar,1,1,1,1);
    p_GridLayout->addWidget(p_ScrollArea,2,0,1,2);
}

// Required to apply stylesheet
void DrumsetPanel::paintEvent(QPaintEvent *)
{
    QStyleOption opt;
    opt.init(this);
    QPainter p(this);
    style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);
}



// ******************************************************************************************** //
// **************************************** DESTRUCTOR **************************************** //
// ******************************************************************************************** //

DrumsetPanel::~DrumsetPanel() {
    if(mDrumSetMaker != nullptr) {
        delete mDrumSetMaker;
    }

    if (mp_drmMakerModel) {
        delete mp_drmMakerModel;
    }
}




// *************************************************************************************************** //
// ***************************************** PRIVATE METHODS ***************************************** //
// *************************************************************************************************** //

void DrumsetPanel::createNewDrumset(){
    // Create a new instrument list
    for(uint i=0;; i++){
        auto& x = defaultInstrumentArray[i];
        if (x.isEmpty())
            break;
        mp_drmMakerModel->addInstrument(new Instrument(mp_drmMakerModel, x, this, defaultInstrumentMidiIds[i]));
    }
    mp_drmMakerModel->setDrumsetName(tr("New Drumset"));

    mp_drmMakerModel->setDrmPath(nullptr);
    mp_drmMakerModel->setDrmOpened(true);
    emit sigOpenedDrm(nullptr); // Notify that Drm opened, even if no path set yet
}


/**
 * @brief DrumsetPanel::promptSaveUnsavedChanges
 * @return continue
 */
bool DrumsetPanel::promptSaveUnsavedChanges()
{
    switch (QMessageBox::question(this, tr("Save Drumset"), tr("There are unsaved modification in the current drumset. Do you want to save you changes?"), QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel, QMessageBox::Save)) {
    case QMessageBox::Save :
        // Save and if success continue else cancel
        return saveCurrentDrumset();
    case QMessageBox::Discard:
        // Don't save and continue
        return true;
    default:
        // Cancel and stop
        return false;
    }
}


/**
 * @brief DrumsetPanel::saveCurrentDrumset
 * @return succes
 *
 * @note This function handles the save the saving of a drumset and the display of error messages during the process
 */
bool DrumsetPanel::saveCurrentDrumset(){
    if(!mp_drmMakerModel->isDrmOpened()){
        return false;
    }

    if (mp_drmMakerModel->getDrumsetSize() > MAXIMUM_DRUMSET_SIZE){
        QMessageBox::critical(this, tr("Save Drumset"), tr("The drumset is too big to be saved. Please, remove some velocities"));
        return false;
    }

    // Save As if file name does not exist yet
    if(mp_drmMakerModel->drmPath().isEmpty()){
        return saveCurrentDrumsetAs();
    }

    // Create instance if none were created or clear the contents if there is an instance
    if(mDrumSetMaker == nullptr) {
        mDrumSetMaker = new DrumSetMaker(mp_drmMakerModel);
    } else {
        mDrumSetMaker->clearBuild();
    }

    if (!mDrumSetMaker->checkErrorsAndSort()) {
        if(!mp_drmMakerModel->drmPath().isEmpty()){

            // Build the DRM
            mDrumSetMaker->buildDRM();

            // DRM needs to be built to know if empty
            if(mDrumSetMaker->isEmpty()){
                QMessageBox::critical(this, tr("Save Drumset"), tr("Drumset is empty.\nThere must be at least one valid instrument (including a wave file)"));
                return false;
            }

            // Write to file
            mDrumSetMaker->writeToFile(mp_drmMakerModel->drmPath());
            return true;
        }
        return false;
    } else {
        QMessageBox::critical(this, tr("Drumset is empty"), tr("The current Drum Set contains errors.\n\nPlease fix instrument velocity ranges and\nmake sure WAVE files are valid before continuing"));
        return false;
    }
}
/**
 * @brief DrumsetPanel::saveCurrentDrumsetAs
 * @return succes
 *
 * @note This function handles the save the saving of a drumset and the display of error messages during the process
 */
bool DrumsetPanel::saveCurrentDrumsetAs(){
    if(!mp_drmMakerModel->isDrmOpened()){
        QMessageBox::critical(this, tr("Save Drumset As"), tr("There is no file opened."));
        return false;
    }

    // Create instance if none were created or clear the contents if there is an instance
    if(mDrumSetMaker == nullptr) {
        mDrumSetMaker = new DrumSetMaker(mp_drmMakerModel);
    } else {
        mDrumSetMaker->clearBuild();
    }

    if (!mDrumSetMaker->checkErrorsAndSort()) {

        // Build the DRM
        mDrumSetMaker->buildDRM();

        // DRM needs to be built to know if empty
        if(mDrumSetMaker->isEmpty()){
            QMessageBox::critical(this, tr("Save Drumset As"), tr("Drumset is empty.\nThere must be at least one valid instrument (including a wave file)"));
            return false;
        }
        Workspace w;

        // Remove special characters from name
        QString defaultFileName = mp_drmMakerModel->getDrumsetName().replace(QRegularExpression("[\\/:*?\"<>| ]"), "_");
        QDir defaultDir(w.userLibrary()->libDrumSets()->currentPath());
        QFileInfo defaultFI(defaultDir.absoluteFilePath(defaultFileName));

        QString filePath = QFileDialog::getSaveFileName(
                    this,
                    tr("Save Drumset As"),
                    defaultFI.absoluteFilePath(),
                    BMFILES_DRUMSET_DIALOG_FILTER);

        if(!filePath.isEmpty()){
            // Update the browse path
            QFileInfo fileInfo(filePath);
            w.userLibrary()->libDrumSets()->setCurrentPath(fileInfo.absolutePath());

            // Write to file
            mDrumSetMaker->writeToFile(filePath);

            if(fileInfo.absoluteFilePath().toUpper() != mp_drmMakerModel->drmPath().toUpper()){
                emit sigClosedDrm(mp_drmMakerModel->drmPath());
                mp_drmMakerModel->setDrmPath(filePath);
                emit sigOpenedDrm(mp_drmMakerModel->drmPath());
            }

            return true;
        }
        return false;
    } else {
        QMessageBox::critical(this, tr("Save Drumset As"), tr("The current Drum Set contains errors.\n\nPlease fix instrument velocity ranges and\nmake sure WAVE files are valid before continuing"));
        return false;
    }
}

/**
 * @brief DrumsetPanel::closeDrmInternal
 */
void DrumsetPanel::closeDrmInternal()
{
    if(!mp_drmMakerModel->isDrmOpened()){
        
        return;
    }

    // Remove all instruments
    mp_drmMakerModel->clearInstrumentList();

    // Send closed DRM;
    QFileInfo drmFI(mp_drmMakerModel->drmPath());
    if(drmFI.exists()){
        emit sigClosedDrm(drmFI.absoluteFilePath());
    }

    mp_drmMakerModel->setDrmPath(nullptr);
    mp_drmMakerModel->setDrmOpened(false);
    mp_drmMakerModel->setDrumsetName(tr("Drumset Name"));
}

/**
 * @brief DrumsetPanel::openDrmInternal
 * @param path
 * @return  success
 */
bool DrumsetPanel::openDrmInternal(const QString &path)
{
    if (mp_drmMakerModel->isDrmOpened()) {
        closeDrmInternal();
    }

    if (path.isEmpty()) {
        return false;
    }

    qDebug() << "DrumsetPanel::openDrmInternal " << path;

    // Retrieve data from DRM
    DrumSetExtractor dse(mp_drmMakerModel);
    if (auto error = dse.read(path)){
        qWarning() << "DrumsetPanel::openDrmInternal - ERROR 1 - unable to extract drumset, Error code = " << error;
        return false;
    }

    dse.generateWaves(Workspace().userLibrary()->libWaveSources()->dir().absolutePath(), this);
    dse.buildInstrumentList(true, this);

    // If succeed emit signal
    mp_drmMakerModel->setDrmPath(path);
    mp_drmMakerModel->setDrmOpened(true);
    emit sigOpenedDrm(path);

    return true;
}


// *************************************************************************************************** //
// ********************************************** SLOTS ********************************************** //
// *************************************************************************************************** //

/**
 * @brief DrumsetPanel::slotCurrentItemChanged
 * @param current
 * @param previous
 */
void DrumsetPanel::slotCurrentItemChanged(QListWidgetItem *current, QListWidgetItem *previous)
{
    if (previous && current) {
        qDebug() << "Changed drumset from " << previous->text() << " to " << current->text();
    } else if (!previous) {
        qDebug() << "Select drumset " << current->text();
    } else if (!current) {
        qDebug() << "Deselect drumset " << previous->text();
    }
}

/**
 * @brief DrumsetPanel::on_actionNew_triggered
 */
void DrumsetPanel::on_actionNew_triggered() {
    // If the user cancels the open action when asked to save to changes
    if(mp_drmMakerModel->isDrmOpened()){
        if (!promptSaveUnsavedChanges()){
            return;
        }
        closeDrmInternal();
    }

    // Initial Instrument setup
    createNewDrumset();

}



/**
 * @brief DrumsetPanel::on_actionOpen_triggered
 */
void DrumsetPanel::on_actionOpen_triggered() {
    // If the user cancels the open action when asked to save to changes
    if(mp_drmMakerModel->isDrmOpened()){
        if (!promptSaveUnsavedChanges()){
            return;
        }
    }

    // Ask for path
    Workspace w;
    QString filePath = QFileDialog::getOpenFileName(
                this,
                tr("Open Drum Set"),
                w.userLibrary()->libDrumSets()->currentPath(),
                BMFILES_DRUMSET_DIALOG_FILTER);

    if(!filePath.isEmpty()){
        // Update the browse path
        QFileInfo fileInfo(filePath);
        w.userLibrary()->libDrumSets()->setCurrentPath(fileInfo.absolutePath());
        if(mp_drmMakerModel->isDrmOpened()){
            closeDrmInternal(); // close DRM at last moment
        }

        openDrmInternal(fileInfo.absoluteFilePath());
    }
}

/**
 * @brief DrumsetPanel::slotOpenDrm
 * @param filePath
 */
void DrumsetPanel::slotOpenDrm(const QString &name, const QString &filePath)
{
    // If the user cancels the open action when asked to save to changes
    if(mp_drmMakerModel->isDrmOpened()){
        if (!promptSaveUnsavedChanges()){
            return;
        }
    }
    CmdOpenDrumset::queue(this, name, filePath, mp_drmMakerModel->drmPath());
}

void DrumsetPanel::slotOpenedDrmNameChanged(const QString & /*originalName*/, const QString &newName)
{
    mp_drmMakerModel->setDrumsetName(newName);
}

/**
 * @brief DrumsetPanel::on_actionSave_triggered
 */
void DrumsetPanel::on_actionSave_triggered() {
    // Proceed to save the current drumset beeing edited
    saveCurrentDrumset();
}


void DrumsetPanel::on_actionSaveAs_triggered(){
    // Proceed to save the current drumset beeing edited under a new file name
    saveCurrentDrumsetAs();
}


/**
 * @brief DrumsetPanel::on_actionExit_triggered
 */
void DrumsetPanel::on_actionClose_triggered() {
    // If the user cancels the open action when asked to save to changes
    if(mp_drmMakerModel->isDrmOpened()){
        if (!promptSaveUnsavedChanges()){
            return;
        }
    }

    // Actually close DRM
    closeDrmInternal();
}

/**
 * @brief DrumsetPanel::on_actionSort_Instruments_triggered
 */
void DrumsetPanel::on_actionSort_Instruments_triggered() {
    mp_drmMakerModel->sortInstruments();
}

/**
 * @brief DrumsetPanel::on_actionAdd_instrument_triggered
 */
void DrumsetPanel::on_actionAdd_instrument_triggered() {
    CmdAddRemoveInstrument::queue(true, this);
}

void DrumsetPanel::on_instrument_deleteButt_onPressed(Instrument* instr) {
    CmdAddRemoveInstrument::queue(false, this, instr);
}

/**
 * @brief DrumsetPanel::changeDrumsetName
 */
void DrumsetPanel::changeDrumsetName(QMouseEvent*){
    DrumsetNameDialog dialog(mp_drmMakerModel->getDrumsetName(), mp_drmMakerModel->volume(), this);
    if(dialog.exec()) {
        mp_drmMakerModel->setDrumsetName(dialog.name());
        mp_drmMakerModel->setVolume(dialog.volume());
    }
}

// *************************************************************************************************** //
// ******************************** INSTRUMENT OBSERVER IMPLEMENTTION ******************************** //
// *************************************************************************************************** //

void DrumsetPanel::instrumentAdded(int index) {
    // Ternary operator used to determine which column and row to set new instrument
    mp_InstrumentsGridLayout->addWidget(mp_drmMakerModel->getInstrument(index), ((index%2)==0 ? index/2 : (index-1)/2), (index%2), 1, 1);
    mp_drmMakerModel->getInstrument(index)->show();

}

void DrumsetPanel::instrumentRemoved(int index) {
    // Remove all instruments from layout after index to end
    for (int i = index+1; i < (mp_drmMakerModel->getInstrumentCount()+1); ++i) {
        // Ternary operator used to determine which column and row to get the instrument
        QLayoutItem* item = mp_InstrumentsGridLayout->itemAtPosition(((i%2)==0 ? i/2 : (i-1)/2), (i%2));
        item->widget()->hide();
        mp_InstrumentsGridLayout->removeItem(item);
        delete item;
    }

    // Re-add all instruments after index
    for (int i = index; i < mp_drmMakerModel->getInstrumentCount(); ++i) {
        // Ternary operator used to determine which column and row to set new instrument
        mp_InstrumentsGridLayout->addWidget(mp_drmMakerModel->getInstrument(i), ((i%2)==0 ? i/2 : (i-1)/2), (i%2), 1, 1);
        mp_drmMakerModel->getInstrument(i)->show();
    }
}

void DrumsetPanel::instrumentSorted(){

    // Remove all instruments from layout after index to end
    for (int i = 0; i < (mp_drmMakerModel->getInstrumentCount()); ++i) {
        // Ternary operator used to determine which column and row to get the instrument
        QLayoutItem* item = mp_InstrumentsGridLayout->itemAtPosition(((i%2)==0 ? i/2 : (i-1)/2), (i%2));
        item->widget()->hide();
        mp_InstrumentsGridLayout->removeItem(item);
        delete item;
    }

    // Re-add all instruments after index
    for (int i = 0; i < mp_drmMakerModel->getInstrumentCount(); ++i) {
        // Ternary operator used to determine which column and row to set new instrument
        mp_InstrumentsGridLayout->addWidget(mp_drmMakerModel->getInstrument(i), ((i%2)==0 ? i/2 : (i-1)/2), (i%2), 1, 1);
        mp_drmMakerModel->getInstrument(i)->show();
    }
}


// ************************************************************************************************** //
// ********************************* DRUMSET OBSERVER IMPLEMENTTION ********************************* //
// ************************************************************************************************** //

void DrumsetPanel::drumsetNameChanged(const QString& name) {
    mDrumsetName->setName(name);
}



void DrumsetPanel::on_pushButton_clicked()
{

}

void DrumsetPanel::drumsetSizeChanged(quint32 size){

    mProgressBar->setDrumsetSize(size);
}

