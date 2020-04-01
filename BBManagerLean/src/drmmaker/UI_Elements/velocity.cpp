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
#include "velocity.h"
//#include "QEvent.h"
#include "../Model/drmmakermodel.h"
#include "../../workspace/workspace.h"
#include "../../workspace/contentlibrary.h"
#include "../../workspace/libcontent.h"
#include "../../model/beatsmodelfiles.h"
#include "../Utils/myqsound.h"



// *********************************************************************************************** //
// ***************************************** CONSTRUCTOR ***************************************** //
// *********************************************************************************************** //

Velocity::Velocity(DrmMakerModel *model, int start, int end, const QString& file, QWidget *parent)
    : QWidget(parent)
{
    
    mp_Model = model;
    
    // no size when empty
    mWavsize = 0;
    
    // Remove all margins
    this->setContentsMargins(0, 0, 0, 0);
    
    // Create grid layout
    mLayout = new QHBoxLayout(this);
    mLayout->setAlignment(Qt::AlignTop);
    mLayout->setSizeConstraint(QLayout::SetMinimumSize);
    
    // Start spinbox
    mStartSpinBox = new FocusSpinBox(false, 0, 126, start);
    mStartSpinBox->setFocusPolicy( Qt::StrongFocus );
    mStartSpinBox->setMaximumHeight(25);
    connect(mStartSpinBox, SIGNAL(valueChanged(int, int, int)), this, SLOT(on_SpinBox_valueChanged(int, int, int)));
    
    // End spinbox
    mEndSpinBox = new FocusSpinBox(true, 1, 127, end);
    mEndSpinBox->setFocusPolicy( Qt::StrongFocus );
    mEndSpinBox->setMaximumHeight(25);
    connect(mEndSpinBox, SIGNAL(valueChanged(int, int, int)), this, SLOT(on_SpinBox_valueChanged(int, int, int)));
    
    
    // Filename field
    mFileName = new DropLineEdit();
    mFileName->setMaximumHeight(20);
    connect(mFileName, SIGNAL(textChanged(const QString)), this, SLOT(on_FileName_valueChanged()));
    
    // Browse button
    QIcon browseIcon;
    browseIcon.addFile(":/drawable/find.png", QSize(), QIcon::Normal, QIcon::Off);
    mBrowseButton = new QPushButton(browseIcon, tr("Browse"));
    mBrowseButton->setMaximumHeight(25);
    connect(mBrowseButton, SIGNAL(clicked()), this, SLOT(on_Browse_clicked()));
    
    // Play button
    QIcon playIcon;
    playIcon.addFile(":/drawable/play.png", QSize(), QIcon::Normal, QIcon::Off);
    mPlayButton = new QPushButton(playIcon, nullptr, this);
    mPlayButton->setEnabled(false);
    mPlayButton->setMaximumHeight(25);
    connect(mPlayButton, SIGNAL(clicked()), this, SLOT(on_Play_clicked()));
    
    // Remove button
    QIcon removeIcon;
    removeIcon.addFile(":/drawable/delete.png", QSize(), QIcon::Normal, QIcon::Off);
    mRemoveButton = new QPushButton(removeIcon, nullptr, this);
    mRemoveButton->setMaximumHeight(25);
    connect(mRemoveButton, SIGNAL(clicked()), this, SLOT(on_Remove_clicked()));
    
    // Add UI elements to gridlayout
    mLayout->addWidget(mStartSpinBox, 0, Qt::AlignLeft);
    mLayout->addWidget(mEndSpinBox, 0, Qt::AlignLeft );
    mLayout->addWidget(mFileName, 0, Qt::AlignVCenter);
    mLayout->addWidget(mPlayButton, 0, Qt::AlignLeft);
    mLayout->addWidget(mBrowseButton, 0, Qt::AlignLeft);
    mLayout->addWidget(mRemoveButton, 0, Qt::AlignRight);
    
    on_Browse_select_file(file, false);
}

// ******************************************************************************************** //
// **************************************** DESTRUCTOR **************************************** //
// ******************************************************************************************** //

Velocity::~Velocity(){
    delete mStartSpinBox;
    delete mEndSpinBox;
    delete mFileName;
    delete mBrowseButton;
    delete mPlayButton;
    delete mRemoveButton;
    delete mLayout;
}

// *********************************************************************************************** //
// ****************************************** OPERATORS ****************************************** //
// *********************************************************************************************** //

bool Velocity::operator<(const Velocity& obj) const {
    auto tx = getText(), tx2 = obj.getText();
    auto n = tx.isEmpty(), n2 = tx2.isEmpty();
    if (n != n2) {
        return n < n2;
    }
    auto s = getStart(), s2 = obj.getStart();
    if (s != s2) {
        return s < s2;
    }
    auto e = getEnd(), e2 = obj.getEnd();
    if (e != e2) {
        return e < e2;
    }
    if ((tx = tx.toUpper()) != (tx2 = tx2.toUpper())) {
        return tx < tx2;
    }
    return this < &obj;
}

// ************************************************************************************************** //
// ***************************************** PUBLIC METHODS ***************************************** //
// ************************************************************************************************** //

QString Velocity::getText() const{
    return mFileName->text();
}

bool Velocity::isFileValid() const{
    return mFileName->getValid();
}

bool Velocity::isEmpty() const{
    return mFileName->text().isEmpty();
}
QFileInfo Velocity::getFileInfo() const{
    return mFileName->getFileInfo();
}

int Velocity::getStart() const{
    return mStartSpinBox->value();
}

int Velocity::getEnd() const{
    return mEndSpinBox->value();
}

void Velocity::setStart(int value) {
    mStartSpinBox->setValueAndNotify(value);
}

void Velocity::setEnd(int value) {
    mEndSpinBox->setValueAndNotify(value);
}

void Velocity::setRemoveEnabled(bool value){
    mRemoveButton->setEnabled(value);
}

void Velocity::setFilePath(const QString filePath){
    mFileName->setFilePath(filePath);
}

/**
 * @brief Get the size of the data of the wav file of the velocity
 */
quint32 Velocity::size(void){
    return mWavsize;
}

// *************************************************************************************************** //
// ********************************************** SLOTS ********************************************** //
// *************************************************************************************************** //

void Velocity::on_SpinBox_valueChanged(int id, int from, int to){
    emit spinBoxValueChanged(this, id, from, to);
}


void Velocity::on_FileName_valueChanged(){
    quint32 subChunkSize = 0;
    // If the file is valid enable the play button
    if (mFileName->getValid()) {
        
        // Check if the file is a wave file
        QFile file(mFileName->getFileInfo().absoluteFilePath());
        file.open(QIODevice::ReadOnly);
        
        bool validWave = QString(file.read(4)) == "RIFF";
        file.seek(8);
        validWave = validWave && (QString(file.read(4)) == "WAVE");
        
        if (validWave) {
            // read the 4 next byte (Chunksize) and advance the file
            file.read(4);
            
            
            subChunkSize =  wordToInt(file.read(4));
            file.seek(file.pos() + subChunkSize);
            
            
            while((QString(file.read(4)) != "data")){
                subChunkSize = wordToInt(file.read(4));
                file.seek(file.pos() + subChunkSize);
            }
            
            
            quint32 newSize = wordToInt(file.read(4));

            emit newWavSize(mWavsize,newSize);
            mWavsize = newSize;
            mPlayButton->setEnabled(true);
            mFileName->setValid(true);
           
        } else {
            // Not a wave file, disable the playbutton and invalidate the file in the filename field
            mPlayButton->setEnabled(false);
            mFileName->setValid(false);
        }
        file.close();
    } else {
        mPlayButton->setEnabled(false);
    }
    
    
    emit textFieldValueChanged();
}

void Velocity::on_Browse_clicked(){
    QFileDialog dialog(
                this,
                tr("Select file"),
                mFileName->getFileInfo().absolutePath(),
                BMFILES_WAVE_DIALOG_FILTER);
    dialog.selectFile(mFileName->getFileInfo().absoluteFilePath());
    connect(&dialog, SIGNAL(currentChanged(QString)), this, SLOT(on_Browse_play_file(QString)));
    connect(&dialog, SIGNAL(fileSelected(QString)), this, SLOT(on_Browse_select_file(QString)));
    dialog.exec();
}

void Velocity::on_Play_clicked(){
    MyQSound::play(mFileName->getFileInfo().absoluteFilePath(), m_volume);
}

void Velocity::on_Remove_clicked(){
    emit removeRequest(this);
}


void Velocity::on_Browse_play_file(QString filename){
    // Check if the file is a wave file
    QFile file(filename);
    if(file.open(QIODevice::ReadOnly)){
        bool validWave = QString(file.read(4)) == "RIFF";
        file.seek(8);
        validWave = validWave && (QString(file.read(4)) == "WAVE");
        file.close();
        if (validWave) {
            QSound::play(filename);
        }
    }
}

void Velocity::on_Browse_select_file(QString filename, bool notify /* = true */){
    // If the file is valid assign it to proper DropLineEdit field and update the browse path
    auto old = mFileName->getFileInfo().absoluteFilePath();
    if (!QFileInfo(filename).exists()) {
        filename = QString::null;
    }
    if (old == filename) {
    } else if (notify) {
        emit fileSelected(this, filename);
    } else {
        QFileInfo fileInfo(filename);
        mFileName->setFileInfo(fileInfo);
        if (fileInfo.exists()) {
            Workspace w;
            w.userLibrary()->libWaveSources()->setCurrentPath(fileInfo.absolutePath());
        }
    }
}


void Velocity::on_Volume_Changed(uint volume)
{
   m_volume = volume;
}


/**
 * @brief MySpinBox::MySpinBox
 * @param parent
 */
FocusSpinBox::FocusSpinBox(int id, int start, int end, int value, QWidget *parent)
    : QSpinBox(parent)
    , m_id(id)
    , m_old(value)
{
    setRange(start, end);
    setValue(value);
    installEventFilter(this);
    connect(this, SIGNAL(valueChanged(int)), this, SLOT(onValueChanged(int)));
}

void FocusSpinBox::setValueAndNotify(int value)
{
    if (value == m_old) return;
    setValue(value);
    emit valueChanged(m_id, value, m_old);
    m_old = value;
}

void FocusSpinBox::onValueChanged(int value)
{
    emit valueChanged(m_id, value, m_old);
    m_old = value;
}

void FocusSpinBox::focusInEvent(QFocusEvent* event)
{
    setFocusPolicy(Qt::WheelFocus);
    
    QSpinBox::focusInEvent(event);
}

void FocusSpinBox::focusOutEvent(QFocusEvent* event)
{
    setFocusPolicy(Qt::StrongFocus);
    
    QSpinBox::focusOutEvent(event);
}

bool FocusSpinBox::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::Wheel && qobject_cast<FocusSpinBox*>(obj)) {
        
        if(qobject_cast<FocusSpinBox*>(obj)->focusPolicy() == Qt::WheelFocus)
        {
            event->accept();
            return false;
        }
        else
        {
            event->ignore();
            return true;
        }
    }
    
    // standard event processing
    return QObject::eventFilter(obj, event);
}


/**
 * @brief Velocity::wordToInt
 * @param Convert a byte array of 4 byte to an int (
 *        platformt endiannes independant
 * @return value
 */
quint32 Velocity::wordToInt(QByteArray b){
    
    return ((0x000000FFu  &  ((quint32) b.at(0)) << 0) |
            (0x0000FF00u  &  ((quint32) b.at(1)) << 8) |
            (0x00FF0000u  &  ((quint32) b.at(2)) << 16)|
            (0xFF000000u  &  ((quint32) b.at(3)) << 24));
}

