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
#include<QHBoxLayout>
#include<QVBoxLayout>

#include <QPainter>
#include <QStyleOption>
#include <QFileDialog>
#include <QDebug>

#include "beatfilewidget.h"
#include "partcolumnwidget.h"
#include "../model/tree/abstracttreeitem.h"
#include "../utils/wavfile.h"
#include "../workspace/settings.h"
#include "../workspace/workspace.h"
#include "../workspace/contentlibrary.h"
#include "../workspace/libcontent.h"
#include "../model/filegraph/songfile.h"
#include "../model/beatsmodelfiles.h"
#include "model/tree/project/beatsprojectmodel.h"

#include "newsongwidget.h" 


const int PartColumnWidget::PADDING = 10;

/*
 * INTERNAL CLASS DropPanel
 */

DropPanel::DropPanel(QWidget* parent, Qt::WindowFlags f)
    : QWidget(parent, f)
    , butt(nullptr)
{
}

void DropPanel::setExtension(const QString& e)
{
    acceptedExtensions = e.toUpper().split(",");
    setAcceptDrops(!acceptedExtensions.isEmpty());
}

void DropPanel::setVisualizer(QPushButton* butt)
{
    this->butt = butt;
}

void DropPanel::dragEnterEvent(QDragEnterEvent *event)
{
    foreach (QUrl dropUrl, event->mimeData()->urls()) {
        QFileInfo dropFI(dropUrl.toLocalFile());
        if (dropFI.exists() && dropFI.isFile() && acceptedExtensions.contains(dropFI.suffix().toUpper()))
        {
            event->setDropAction(Qt::LinkAction);
            event->accept();
            setStyleSheet(QString().sprintf("background-color:#%06X", (Settings::getColorDnDAppend()&0xffffff)));
            return;
        }
    }
}

void DropPanel::dragLeaveEvent(QDragLeaveEvent *)
{
    setStyleSheet("");
}

void DropPanel::dropEvent(QDropEvent *event)
{
    setStyleSheet("");
    if (event->isAccepted()) return;
    emit onDrop(event);
}

void DropPanel::enterEvent(QEvent*)
{
    foreach (QUrl pasteUrl, QApplication::clipboard()->mimeData()->urls()) {
        QFileInfo pasteFI(pasteUrl.toLocalFile());
        if (pasteFI.exists() && pasteFI.isFile() && acceptedExtensions.contains(pasteFI.suffix().toUpper()))
            return (butt ? butt : (QWidget*)this)->setStyleSheet(QString().sprintf("background-color:#%06X", (Settings::getColorDnDAppend()&0xffffff)));
    }
}

void DropPanel::leaveEvent(QEvent*)
{
    (butt ? butt : (QWidget*)this)->setStyleSheet(nullptr);
}

bool DropPanel::paste()
{
    QDropEvent fake(QPointF(), Qt::CopyAction, QApplication::clipboard()->mimeData(), nullptr, nullptr);
    emit onDrop(&fake);
    return fake.isAccepted();
}

/*
 * MAIN CLASS PartColumnWidget
 */

void PartColumnWidget::slotDrop(QDropEvent *event)
{
    bool accept = false;

    QStringList acceptedExtensions = modelIndex().sibling(modelIndex().row(), AbstractTreeItem::CHILDREN_TYPE).data().toString().toUpper().split(",");
    foreach (QUrl dropUrl, event->mimeData()->urls()){
        QFileInfo dropFI(dropUrl.toLocalFile());

        if (dropFI.exists() && dropFI.isFile() && acceptedExtensions.contains(dropFI.suffix().toUpper()))
        {
            accept |= slotAddButtonClicked(dropFI.absoluteFilePath());
        }
    }
    if (accept){
        event->accept();
        emit sigUpdateTran();
    }
}

PartColumnWidget::PartColumnWidget(BeatsProjectModel *p_Model, QWidget *parent) :
   SongFolderViewItem(p_Model, parent)
{
   m_MaxFileCount = 1;
   m_ShuffleEnabled = false;
   m_ShuffleActivated = false;
   mp_BeatFileItems = new QList<BeatFileWidget*>();

   // Create No File Panel (NFP)
   mp_NoFilePanel = new DropPanel(this);
   QVBoxLayout *p_NFPVLayout = new QVBoxLayout();
   mp_NoFilePanel->setLayout(p_NFPVLayout);

   mp_NFPLabel = new QLabel(mp_NoFilePanel);
   mp_NFPLabel->setText( tr("Label") );
   mp_NFPLabel->setAlignment( Qt::AlignCenter );
   mp_NFPLabel->setSizePolicy(
            QSizePolicy( QSizePolicy::Expanding, QSizePolicy::Expanding ) );
   p_NFPVLayout->addWidget(mp_NFPLabel);


   QHBoxLayout * p_NFPAddButtonHLayout = new QHBoxLayout();

   {
    auto butt = new MenuPushButton(mp_NoFilePanel);
    auto menu = butt->addMenu();
    menu->addAction(tr("Create new pattern"), this, SLOT(slotCreateNewFile()));
    menu->addAction(tr("Add from MIDI file"), this, SLOT(slotAddButtonClicked()));
    mp_NFPAddButton = butt;
   }
   mp_NoFilePanel->setVisualizer(mp_NFPAddButton);
   mp_NFPAddButton->setText(tr("..."));
   mp_NFPAddButton->setToolTip(tr("Add midi file"));


   mp_NFPAddButton->setObjectName(QStringLiteral("addButton"));
   mp_NFPAddButton->setSizePolicy(
   QSizePolicy( QSizePolicy::Minimum, QSizePolicy::Minimum ) );
   mp_NFPAddButton->setMinimumWidth(30);
   mp_NFPAddButton->setMaximumWidth(30);

   p_NFPAddButtonHLayout->addItem(new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum));
   p_NFPAddButtonHLayout->addWidget(mp_NFPAddButton);
   p_NFPAddButtonHLayout->addItem(new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum));

   p_NFPVLayout->addLayout(p_NFPAddButtonHLayout);

   // Create Multi File Panel (MFP)
   mp_MultiFilePanel = new DropPanel(this);
   QVBoxLayout *p_MFPVLayout = new QVBoxLayout();
   mp_MultiFilePanel->setLayout(p_MFPVLayout);
   mp_MultiFilePanel->setMinimumWidth(50); // 30 for button + 2x 10 for padding

   mp_MFPShuffleButton = new QPushButton(mp_MultiFilePanel);
   mp_MFPShuffleButton->setObjectName(QStringLiteral("shuffleButton"));
   mp_MFPShuffleButton->setSizePolicy(
            QSizePolicy( QSizePolicy::Minimum, QSizePolicy::Minimum ) );
   mp_MFPShuffleButton->setMinimumWidth(30);
   mp_MFPShuffleButton->setMaximumWidth(30);
   mp_MFPShuffleButton->setToolTip(tr("Shuffle the order of the drum fills"));
   mp_MFPShuffleButton->setCheckable(true);
   mp_MFPShuffleButton->setIcon(QIcon(":/images/images/Shuffle_Unchecked.png"));
   p_MFPVLayout->addWidget(mp_MFPShuffleButton);

   p_MFPVLayout->addItem(new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding));

   {
    auto butt = new MenuPushButton(mp_MultiFilePanel);
    auto menu = butt->addMenu();
    menu->addAction(tr("Create new pattern"), this, SLOT(slotCreateNewFile()));
    menu->addAction(tr("Add from MIDI file"), this, SLOT(slotAddButtonClicked()));
    mp_MFPAddButton = butt;
   }
   mp_MultiFilePanel->setVisualizer(mp_MFPAddButton);
   mp_MFPAddButton->setText(tr("..."));

   mp_MFPAddButton->setToolTip(tr("Add midi file"));
   mp_MFPAddButton->setObjectName("addButton");
   mp_MFPAddButton->setSizePolicy(
            QSizePolicy( QSizePolicy::Minimum, QSizePolicy::Minimum ) );
   mp_MFPAddButton->setMinimumWidth(30);
   mp_MFPAddButton->setMaximumWidth(30);
   p_MFPVLayout->addWidget(mp_MFPAddButton);

   // Hide at initialization
   mp_MultiFilePanel->hide();

   // connect slots to signals
   connect(mp_NoFilePanel, SIGNAL(onDrop(QDropEvent*)), this, SLOT(slotDrop(QDropEvent*)));
   connect(mp_MultiFilePanel, SIGNAL(onDrop(QDropEvent*)), this, SLOT(slotDrop(QDropEvent*)));

   connect(this, SIGNAL(sigIsMultiFileAddEnabled(bool)), mp_MFPAddButton    , SLOT(setEnabled(bool)));
   connect(this, SIGNAL(sigIsMultiFileAddEnabled(bool)), mp_MultiFilePanel    , SLOT(setEnabled(bool)));
   connect(this, SIGNAL(sigIsShuffleEnabled     (bool)), mp_MFPShuffleButton, SLOT(setEnabled(bool)));
   connect(this, SIGNAL(sigIsShuffleActivated   (bool)), mp_MFPShuffleButton, SLOT(setChecked(bool)));

   connect(mp_NFPAddButton,     SIGNAL(clicked()    ), this, SLOT(slotAddButtonClicked()));
   connect(mp_MFPAddButton,     SIGNAL(clicked()    ), this, SLOT(slotAddButtonClicked()));
   connect(mp_MFPShuffleButton, SIGNAL(clicked(bool)), this, SLOT(slotShuffleButtonClicked(bool)));

   connect(p_Model, &BeatsProjectModel::endEditMidi, this, &PartColumnWidget::endEditMidi);
}

void PartColumnWidget::endEditMidi(const QByteArray& data)
{
    if (m_editingTrackData.isEmpty())
        return;
    QByteArray prev;
    prev.swap(m_editingTrackData);
    if (data.isEmpty() || prev == data)
        return;
    auto m = model();
    auto ix = modelIndex();
    auto song = ix.parent().parent();
    // 1 Create new MIDI file from editor data in user lib
    QString name = tr("editor-%1-%2@%3.mid").arg(song.data().toString()).arg(m->songFileName(ix, m->rowCount(ix)))
        .arg(QDateTime::currentDateTime().toString("yyyy-MM-dd+hh-mm-ss"));
    QRegularExpression re("[ \\\"/<|>:*_?]+");
    name = name.replace(re, "_"); // we replace inappropriate symbols with a sad smilie in hopes for happy future
    name = QDir(Workspace().userLibrary()->libMidiSources()->currentPath()).filePath(name);
    MIDIPARSER_MidiTrack(data).write_file(name.toStdString());
    // 2 Apply file
    model()->createSongFile(modelIndex(), model()->rowCount(modelIndex()), name);
}

// Required to apply stylesheet
void PartColumnWidget::paintEvent(QPaintEvent * /*event*/)
{
   QStyleOption opt;
   opt.init(this);
   QPainter p(this);
   style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);
}

void PartColumnWidget::changeToolTip(QString const& tooltip){
   mp_NFPAddButton->setToolTip(tooltip);
}

/**
 * @brief PartColumnWidget::setLabel
 *    Change the default tooltip associated with the PartColumnWidget
 * @param label
 */
void PartColumnWidget::setLabel(QString const& label)
{
   // Configure widgets
   mp_NFPLabel->setText( label );
}

void PartColumnWidget::populate(QModelIndex const& modelIndex)
{

   // Delete all previous children widgets (if any)
   for(int i = 0; i < mp_ChildrenItems->size(); i++){
      
      delete mp_ChildrenItems->at(i);
   }

   mp_ChildrenItems->clear();

   // copy new model index
   setModelIndex(modelIndex);

   // Populate self's data
   QVariant labelVatiant = modelIndex.data();
   QString label = labelVatiant.toString();
   mp_NFPLabel->setText( label );

   // Read parameters from Model
   m_MaxFileCount = modelIndex.sibling(modelIndex.row(), AbstractTreeItem::MAX_CHILD_CNT).data().toInt();


   // Add all items from model
   // For now, we only read the the immediate children
   BeatFileWidget *p_BeatFileWidget;

   for(int i = 0; i < modelIndex.model()->rowCount(modelIndex); i++){
      QModelIndex childIndex = modelIndex.model()->index(i, 0, modelIndex); // NOTE: Widgets are associated to column 0
      if (childIndex.isValid()){
         p_BeatFileWidget = new BeatFileWidget(model(), this);
         p_BeatFileWidget->populate(childIndex);
         mp_ChildrenItems->append(p_BeatFileWidget);
         mp_BeatFileItems->append(p_BeatFileWidget);

         connect(p_BeatFileWidget, SIGNAL(sigSubWidgetClicked(QModelIndex)), this, SLOT(slotSubWidgetClicked(QModelIndex)));
         connect(p_BeatFileWidget, &BeatFileWidget::sigSelectTrack, this, &PartColumnWidget::slotSelectTrack);
         connect(p_BeatFileWidget, &BeatFileWidget::sigMainAPUpdated, this, &PartColumnWidget::slotMainAPUpdated);
         connect(p_BeatFileWidget, &BeatFileWidget::sigPartEmpty, this, &PartColumnWidget::setPartEmpty);
         isEmpty = false;
      }
   }
   // Signal, Slots, and Panel Display
   updatePanels();

   // Shuffle button (will only be displayed if m_MaxFileCount = -1 || m_MaxFileCount > 1
   updateShuffleData(modelIndex.sibling(modelIndex.row(), AbstractTreeItem::SHUFFLE));

   // set panels extension
   QString ext = model()->index(modelIndex.row(), AbstractTreeItem::CHILDREN_TYPE, modelIndex.parent()).data().toString();
   mp_NoFilePanel->setExtension(ext);
   mp_MultiFilePanel->setExtension(ext);

   if (0 == ext.compare(BMFILES_WAVE_EXTENSION)) { // we don't want these menus for accent hit
      static_cast<MenuPushButton*>(mp_NFPAddButton)->disableMenu();
      static_cast<MenuPushButton*>(mp_MFPAddButton)->disableMenu();
   }
}

void PartColumnWidget::updatePanels()
{


   switch(m_MaxFileCount){
      case -1 :
         // Infinite children allowed

         // Panel Display
         if (mp_ChildrenItems->size() > 0){
            if (!mp_NoFilePanel->isHidden()){
               mp_NoFilePanel->hide();
            }

            if (mp_MultiFilePanel->isHidden()){
               mp_MultiFilePanel->show();
            }
         } else {
            if (mp_NoFilePanel->isHidden()){
               mp_NoFilePanel->show();
            }

            if (!mp_MultiFilePanel->isHidden()){
               mp_MultiFilePanel->hide();
            }
         }

         emit sigIsMultiFileAddEnabled(true);

         break;


      case 0:
         // No children allowed
         if (!mp_NoFilePanel->isHidden()){
            mp_NoFilePanel->hide();
         }
         if (!mp_MultiFilePanel->isHidden()){
            mp_MultiFilePanel->hide();
         }
         emit sigIsMultiFileAddEnabled(false);

         break;
      case 1:
         // Max 1 children allowed

         if (!mp_MultiFilePanel->isHidden()){
            mp_MultiFilePanel->hide();
         }

         if (mp_ChildrenItems->size() > 0){
            if (!mp_NoFilePanel->isHidden()){
               mp_NoFilePanel->hide();
            }

         } else {
            if (mp_NoFilePanel->isHidden()){
               mp_NoFilePanel->show();
            }
         }

         emit sigIsMultiFileAddEnabled(false);
         break;
      default:
         // Finite children allowed

         // Panel Display
         if (mp_ChildrenItems->size() > 0){
            if (!mp_NoFilePanel->isHidden()){
               mp_NoFilePanel->hide();
            }

            if (mp_MultiFilePanel->isHidden()){
               mp_MultiFilePanel->show();
            }
         } else {
            if (mp_NoFilePanel->isHidden()){
               mp_NoFilePanel->show();
            }

            if (!mp_MultiFilePanel->isHidden()){
               mp_MultiFilePanel->hide();
            }
         }

         emit sigIsMultiFileAddEnabled(m_MaxFileCount > mp_ChildrenItems->size());
         break;
   }
}

void PartColumnWidget::updateMinimumSize()
{
   // Recursively update all children first
   for(int i = 0; i < mp_ChildrenItems->size(); i++) {
      mp_ChildrenItems->at(i)->updateMinimumSize();//beat widgets
   }

   if(modelIndex().row()==1){
      setMinimumWidth(300);
   } else {
      setMinimumWidth(100);
   }

   // Intro/outro or any single file column
   if(m_MaxFileCount <= 1){
      setMinimumHeight(58);
      // Multi file column with 2 rows of files
   } else if (mp_ChildrenItems->size() > 4){
      setMinimumHeight(138);
      // Multi file column with 1 row of files
   } else {
#ifdef Q_OS_OSX
      setMinimumHeight(83);
#else
      setMinimumHeight(93);
#endif
   }
}

void PartColumnWidget::updateLayout()
{

   // NOTE Geometry needs to be written by parent first
   // Relative to this
   mp_NoFilePanel->setGeometry( rect() ); // Same size as parent
   mp_MultiFilePanel->setGeometry(width()-50, 0, 50, height());

   int nbRows = 0;
   if (mp_ChildrenItems->size() > 4){
      nbRows = 2;
   } else if (mp_ChildrenItems->size() > 0){
      nbRows = 1;
   }

   if (nbRows == 0){
      return;
   }

   int nbColumns[2];
   switch(mp_ChildrenItems->size()){
      case 1:
         nbColumns[0] = 1;
         nbColumns[1] = 0;
         break;
      case 2:
         nbColumns[0] = 2;
         nbColumns[1] = 0;
         break;
      case 3:
         nbColumns[0] = 3;
         nbColumns[1] = 0;
         break;
      case 4:
         nbColumns[0] = 4;
         nbColumns[1] = 0;
         break;
      case 5:
         nbColumns[0] = 3;
         nbColumns[1] = 2;
         break;
      case 6:
         nbColumns[0] = 3;
         nbColumns[1] = 3;
         break;
      case 7:
         nbColumns[0] = 4;
         nbColumns[1] = 3;
         break;
      case 8:
         nbColumns[0] = 4;
         nbColumns[1] = 4;
         break;
      default:
         break;
   }

   int availableWidth;
   if (!mp_MultiFilePanel->isHidden()){
      availableWidth = width() - mp_MultiFilePanel->width();
   } else {
      availableWidth = width() - PADDING;
   }

   int availableHeight = height() - PADDING;
   //int availableHeight = height() - 2*PADDING;

   int childrenIndex = 0;
   int yPosMult = 0;
   
   for(int row = 0; row < nbRows; row ++){
      int nextYPosMult = yPosMult + availableHeight;
      int xPosMult = 0;
      for (int column = 0; column < nbColumns[row]; column++){
         int nextXPosMult = xPosMult + availableWidth;

         mp_ChildrenItems->at(childrenIndex)->setGeometry(
                  QRect(
                     QPoint(xPosMult/nbColumns[row] + PADDING,
                            yPosMult/nbRows         + PADDING),
                     QPoint(nextXPosMult/nbColumns[row] - 1,
                            nextYPosMult/nbRows         - 1)
                     )
                  );
         mp_ChildrenItems->at(childrenIndex)->updateLayout();

         xPosMult = nextXPosMult;
         childrenIndex++;
      }

      yPosMult = nextYPosMult;
   }
}

void PartColumnWidget::dataChanged(const QModelIndex &left, const QModelIndex &right)
{

   for (int column = left.column(); column <= right.column(); column++){
      QModelIndex currentIndex = model()->index(modelIndex().row(), column, modelIndex().parent());
      if(!currentIndex.isValid()){
         qWarning() << "PartColumnWidget::dataChanged - (!currentIndex.isValid())";
         return;
      }

      switch(column){
         case AbstractTreeItem::NAME:
            mp_NFPLabel->setText(currentIndex.data().toString());
            break;
         case AbstractTreeItem::SHUFFLE:
            updateShuffleData(currentIndex);
            break;
         default:
            break;
      }
   }
}

void PartColumnWidget::slotCreateNewFile()
{
    auto m = model();
    auto ix = modelIndex();
    MIDIPARSER_MidiTrack clean;
    clean.barLength = 1920;
    clean.nTick = 1920;
    clean.timeSigNum = clean.timeSigDen = 4;
    auto song = ix.parent().parent();
    m->moveSelection(song);
    emit m->beginEditMidi(tr("Creating %2 for %1", "Context: Editing '*Song Name* - *Part Name*'")
        .arg(song.data().toString()).arg(m->songFileName(ix, m->rowCount(ix))), m_editingTrackData = clean);
}


/**
 * @brief PartColumnWidget::slotAddButtonClicked
 * @param dropFileName
 * @return
 *
 * trackButtonClicked can be used for 3 reasons
 * (a) Drop/file in case of drag and drop
 *     in this case, file name is passed as parameter
 * (b) Browse to select new file (when clicked)
 *     in this case, user is asked to browse to locate file
 */

bool PartColumnWidget::slotAddButtonClicked(const QString& dropFileName)
{
   Workspace w; 

   emit sigSubWidgetClicked(modelIndex());

   // 1 - Determine fileName
   QString fileName;
   if(!dropFileName.isEmpty()){
      // 1.1 (a) Accept file in case of drag'n'drop
      fileName = dropFileName;
   } else if (model()->index(modelIndex().row(), AbstractTreeItem::CHILDREN_TYPE, modelIndex().parent()).data().toString().compare(BMFILES_WAVE_EXTENSION, Qt::CaseInsensitive) == 0){
      // 1.2 (b) Browse to select new file (wave)
      fileName = QFileDialog::getOpenFileName(
               this,
               tr("Open Wave"),
               w.userLibrary()->libWaveSources()->currentPath(),
               BMFILES_WAVE_DIALOG_FILTER);

      // Memorize last browse location
      if(!fileName.isEmpty()){
         QFileInfo newPath(fileName);
         w.userLibrary()->libWaveSources()->setCurrentPath(newPath.absolutePath());
      }
   } else if (QApplication::keyboardModifiers()) {
        // 1.3 (c) Create new file from editor (midi)
        slotCreateNewFile();
        return true;
   } else {
      // 1.4 (d) Browse to select new file (midi)
      fileName = QFileDialog::getOpenFileName(
               this,
               tr("Open Midi"),
               w.userLibrary()->libMidiSources()->currentPath(),
               BMFILES_MIDI_TRACK_DIALOG_FILTER);

      // Memorize last browse location
      if(!fileName.isEmpty()){
         QFileInfo newPath(fileName);
         w.userLibrary()->libMidiSources()->setCurrentPath(newPath.absolutePath());
      }
   }

   // 2 - Validate and apply selected file if valid
   if(!fileName.isEmpty()){

      // 2.1 Validate Wave File Format
      QFileInfo fi(fileName);
      if(fi.completeSuffix().compare(BMFILES_WAVE_EXTENSION,Qt::CaseInsensitive) == 0){
         // Limit the wave file size
         if(fi.size() > MAX_ACCENT_HIT_FILE_SIZE){
            QMessageBox::critical(this, tr("Adding file"), tr("Accent hit file exceeds file size limit of %1 kB (%2 kB)")
                     .arg(((double) MAX_ACCENT_HIT_FILE_SIZE) / 1024.0, 0, 'f', 1)
                     .arg(((double) fi.size()) / 1024.0, 0, 'f', 1));
            return false;
         }
         WavFile wavFile;
         if(!wavFile.open(fileName)){
            QMessageBox::critical(this, tr("Adding file"), tr("Unable to open or parse file %1\nWave file either cannot be read or has an unsupported format\nMust be 16-24 bits/sample, 44.1 kHz, mono/stereo standard WAVE").arg(fi.absoluteFilePath()));
            return false;
         }
         if(!wavFile.isFormatSupported()){
            QMessageBox::critical(this, tr("Adding file"), tr("Unsupported Wave File format for %1\nMust be 16-24 bits/sample, 44.1 kHz, mono/stereo standard wave").arg(fi.absoluteFilePath()));
            wavFile.close();
            return false;
         }
         wavFile.close();
      }

      // 2.2 Apply file
      return model()->createSongFile(modelIndex(), model()->rowCount(modelIndex()), fileName) != QModelIndex();
   }
   return true;
}
void PartColumnWidget::slotShuffleButtonClicked(bool checked)
{
   if(checked){
      mp_MFPShuffleButton->setIcon(QIcon(":/images/images/Shuffle_Checked.png"));
      mp_MFPShuffleButton->setToolTip(tr("Play drum fills in sequential order"));
   } else {
      mp_MFPShuffleButton->setIcon(QIcon(":/images/images/Shuffle_Unchecked.png"));
      mp_MFPShuffleButton->setToolTip(tr("Shuffle the order of the drum fills"));
   }

   emit sigSubWidgetClicked(modelIndex());
   model()->setData(model()->index(modelIndex().row(), AbstractTreeItem::SHUFFLE, modelIndex().parent()), QVariant(checked));
}

int PartColumnWidget::maxFileCount()
{
   return m_MaxFileCount;
}

int PartColumnWidget::headerColumnWidth(int /*columnIndex*/)
{
   return 0;
}

void PartColumnWidget::rowsInserted(int start, int end)
{
   // Add all items from model
   // For now, we only read the the immediate children
   BeatFileWidget *p_BeatFileWidget;
   QVariant labelVatiant = modelIndex().data();
   QString label = labelVatiant.toString();

   for(int i = start; i <= end; i++){
      QModelIndex childIndex = model()->index(i, 0, modelIndex()); // NOTE: Widgets are associated to column 0
      if (childIndex.isValid()){
         p_BeatFileWidget = new BeatFileWidget(model(), this);
         p_BeatFileWidget->setAsNew(true);
         p_BeatFileWidget->populate(childIndex);
         mp_ChildrenItems->append(p_BeatFileWidget);
         mp_BeatFileItems->append(p_BeatFileWidget);         
         justInserted = true;
         isEmpty = false;

         connect(p_BeatFileWidget, SIGNAL(sigSubWidgetClicked(QModelIndex)), this, SLOT(slotSubWidgetClicked(QModelIndex)));
         connect(p_BeatFileWidget, &BeatFileWidget::sigSelectTrack, this, &PartColumnWidget::slotSelectTrack);
         connect(p_BeatFileWidget, &BeatFileWidget::sigMainAPUpdated, this, &PartColumnWidget::slotMainAPUpdated);
         connect(p_BeatFileWidget, &BeatFileWidget::sigPartEmpty, this, &PartColumnWidget::setPartEmpty);
         p_BeatFileWidget->show();
      }
      setPartEmpty(label.contains("Trans"));
      emit sigRowInserted();
      mp_BeatFileItems->at(i)->setAsNew(false);//todo erase to do delete
   }
   updatePanels();

}

void PartColumnWidget::rowsRemoved(int start, int end)
{
   // remove all widgets corresponding to rows
   SongFolderViewItem *p_BeatFileWidget;

   for(int i = end; i >= start; i--){
      p_BeatFileWidget = mp_ChildrenItems->at(i);
      mp_ChildrenItems->removeAt(i);
      mp_BeatFileItems->removeAt(i);
      delete p_BeatFileWidget;
      isEmpty = (mp_ChildrenItems->count() == 0)?true:false;
      emit sigRowDelete();
   }

   updatePanels();
}


void PartColumnWidget::updateShuffleData(const QModelIndex &index)
{
   QVariant data = index.data();
   
   m_ShuffleEnabled = data.isValid();
   m_ShuffleActivated = data.isValid() && data.toBool();
   
   if(m_ShuffleEnabled){
      emit sigIsShuffleEnabled(true);
      emit sigIsShuffleActivated(m_ShuffleActivated);
      if (m_ShuffleActivated){
         mp_MFPShuffleButton->setIcon(QIcon(":/images/images/Shuffle_Checked.png"));
      } else {
         mp_MFPShuffleButton->setIcon(QIcon(":/images/images/Shuffle_Unchecked.png"));
      }
   } else {
      emit sigIsShuffleEnabled(false);
      emit sigIsShuffleActivated(false);
      mp_MFPShuffleButton->setIcon(QIcon(":/images/images/Shuffle_Unchecked.png"));
   }
}

void PartColumnWidget::slotSelectTrack(const QByteArray &trackData, int trackIndex)
{
   // Note: typeId = row... except for intro and outro

   emit sigSelectTrack(trackData, trackIndex, modelIndex().row());
}

void PartColumnWidget::slotMainAPUpdated(){
    emit sigUpdateTran();
}

void PartColumnWidget::parentAPBoxStatusChanged(int sigNum, bool hasMain)
{
    for(int i = 0; i < mp_BeatFileItems->size();i++){
        if(justInserted && i == mp_BeatFileItems->size()-1){
            //if this fill was recently inserted to this part should reset beat->m_playAt
            mp_BeatFileItems->at(i)->setAsNew(true);
            justInserted = false;
        }
        mp_BeatFileItems->at(i)->parentAPBoxStatusChanged(sigNum, hasMain);
        MIDIPARSER_TrackType trackType = (MIDIPARSER_TrackType)model()->index(modelIndex().row(), AbstractTreeItem::TRACK_TYPE, modelIndex().parent()).data().toInt();
        if(trackType != DRUM_FILL){
            updateAPText(trackType == MAIN_DRUM_LOOP && modelIndex().siblingAtRow(2).model()->rowCount(modelIndex().siblingAtRow(2)) > 0,false,trackType == OUTRO_FILL,sigNum,i);
        }
        mp_BeatFileItems->at(i)->setAsNew(false);
    }
}

bool PartColumnWidget::finitePart(){

    if(mp_BeatFileItems->size() > 0){
       return mp_BeatFileItems->at(0)->finiteMain();
    }
    return false;
}
void PartColumnWidget::updateAPText(bool hasTrans,bool hasMain,bool hasOutro, int idx,int sigNum, bool isLast){

    if(mp_BeatFileItems->size() > idx && idx >= 0){
       mp_BeatFileItems->at(idx)->updateAPText(hasTrans,hasMain,hasOutro,sigNum,isLast);
    }

}

int PartColumnWidget::getNumSignature(){
    if(mp_BeatFileItems->size() > 0 && !mp_BeatFileItems->at(0)->isNew()){
        QModelIndex child = mp_BeatFileItems->at(0)->modelIndex();//to get the main loop data
        QByteArray trackData = child.sibling(child.row(), AbstractTreeItem::RAW_DATA).data().toByteArray();
        return ((MIDIPARSER_MidiTrack)trackData).timeSigNum;
    }
    return 0;
}

bool PartColumnWidget::isPartEmpty()
{
    return isEmpty;
}
void PartColumnWidget::setPartEmpty(bool value)
{
    isEmpty = value;
}
