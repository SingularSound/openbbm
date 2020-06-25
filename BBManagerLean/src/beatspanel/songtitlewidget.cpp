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
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QEvent>
#include <QList>
#include <QTimer>

#include "songtitlewidget.h"
#include "../model/filegraph/songmodel.h"
#include "../model/tree/project/beatsprojectmodel.h"
#include "../model/tree/abstracttreeitem.h"

#include "../model/beatsmodelfiles.h"

#include "../workspace/settings.h"

SongTitleWidget::SongTitleWidget(QAbstractItemModel *p_Model, QWidget *parent) :
   QFrame(parent)
{
    setMinimumWidth(100);
   mp_beatsModel = static_cast<BeatsProjectModel *>(p_Model);
   m_memorizedIndex = -1;
   m_ModelDirectedChange = false;

   QVBoxLayout *p_FrameLayout = new QVBoxLayout();
   p_FrameLayout->setContentsMargins(0,0,0,0);
   setLayout(p_FrameLayout);

   p_FrameLayout->addSpacerItem(new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding));
   p_FrameLayout->addSpacerItem(new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding));

   QHBoxLayout *p_savedLayout = new QHBoxLayout();
   p_FrameLayout->addLayout(p_savedLayout);

   mp_Saved = new QLabel(this);
   mp_Saved->setText(tr("Saved"));
   mp_Saved->setObjectName(QStringLiteral("savedLabel"));
   mp_Saved->setStyleSheet("QLabel { color : green; }");
   mp_Saved->hide();

   p_savedLayout->addSpacerItem(new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum));
   p_savedLayout->addWidget(mp_Saved);
   p_savedLayout->addSpacerItem(new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum));

   if (Settings::midiIdEnabled()) {
       QHBoxLayout *p_NumberLayout = new QHBoxLayout();
       p_FrameLayout->addLayout(p_NumberLayout);

       mp_Number = new QLineEdit(this);
       mp_Number->setText(nullptr);
       mp_Number->setObjectName(QStringLiteral("numberEdit"));
       mp_Number->setMinimumHeight(20);
       mp_Number->setMaximumWidth(65);
       mp_Number->setToolTip(tr("Set the MIDI id of the song (0-128)"));
       mp_Number->setAlignment(Qt::AlignHCenter);
       mp_Number->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
       mp_Number->setValidator(new QIntValidator(0, 128, this));

       p_NumberLayout->addSpacerItem(new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum));
       p_NumberLayout->addWidget(new QLabel("Song ID:"));
       p_NumberLayout->addWidget(mp_Number);
       p_NumberLayout->addSpacerItem(new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum));

       connect(mp_Number    , SIGNAL(cursorPositionChanged(int,int)), this,         SLOT(slotSubWidgetClicked (   )));
       connect(mp_Number    , SIGNAL(editingFinished()), this,         SLOT(slotSubWidgetClicked (   )));
       connect(mp_Number    , SIGNAL(selectionChanged()), this,         SLOT(slotSubWidgetClicked (   )));
       connect(mp_Number    , SIGNAL(textChanged(QString)), this,         SLOT(slotSubWidgetClicked (   )));
       connect(mp_Number    , SIGNAL(textEdited(QString)), this,         SLOT(slotSubWidgetClicked (   )));
   }

   QHBoxLayout *p_TitleLayout = new QHBoxLayout();
   p_TitleLayout->setContentsMargins(10,5,10,5);
   p_FrameLayout->addLayout(p_TitleLayout);

   mp_Title = new QLineEdit(this);
   mp_Title->setText(nullptr);
   mp_Title->setObjectName(QStringLiteral("titleEdit"));
   mp_Title->setMinimumHeight(20);
   mp_Title->setToolTip(tr("Set the name of the song"));
   mp_Title->setAlignment(Qt::AlignHCenter);
   mp_Title->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);

   p_TitleLayout->addWidget(mp_Title);
   
   p_FrameLayout->addSpacerItem(new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding));

   QHBoxLayout *p_TempoLabelLayout = new QHBoxLayout();
   p_FrameLayout->addLayout(p_TempoLabelLayout);
   mp_TempoLabel = new QLabel(this);
   mp_TempoLabel->setText(tr("Default Tempo:"));
   mp_TempoLabel->setObjectName(QStringLiteral("tempoLabel"));

   p_TempoLabelLayout->addSpacerItem(new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum));
   p_TempoLabelLayout->addWidget(mp_TempoLabel);
   p_TempoLabelLayout->addSpacerItem(new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum));

   QHBoxLayout *p_TempoSpinLayout = new QHBoxLayout();
   p_FrameLayout->addLayout(p_TempoSpinLayout);
   mp_TempoSpin = new MySpinBox(this);
   mp_TempoSpin->setValue(120);
   mp_TempoSpin->setMaximum(MAX_BPM);
   mp_TempoSpin->setMinimum(MIN_BPM);
   mp_TempoSpin->setObjectName(QStringLiteral("tempoSpin"));
   mp_TempoSpin->setFocusPolicy( Qt::StrongFocus );
   mp_TempoSpin->setToolTip(tr("Change the default tempo of this song"));

   p_TempoSpinLayout->addSpacerItem(new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum));
   p_TempoSpinLayout->addWidget(mp_TempoSpin);
   p_TempoSpinLayout->addSpacerItem(new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum));

   p_FrameLayout->addSpacerItem(new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding));

   QHBoxLayout *p_DrmLabelLayout = new QHBoxLayout();
   p_FrameLayout->addLayout(p_DrmLabelLayout);
   mp_DrmLabel = new QLabel(this);
   mp_DrmLabel->setText(tr("Default Drum Set:"));
   mp_DrmLabel->setObjectName(QStringLiteral("tempoLabel"));

   p_DrmLabelLayout->addSpacerItem(new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum));
   p_DrmLabelLayout->addWidget(mp_DrmLabel);
   p_DrmLabelLayout->addSpacerItem(new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum));

   QHBoxLayout *p_DrmComboLayout = new QHBoxLayout();
   p_FrameLayout->addLayout(p_DrmComboLayout);
   mp_DrmCombo = new MyComboBox(this);
   mp_DrmCombo->addItem(tr("None"), QVariant(",None"));
   mp_DrmCombo->setCurrentIndex(0);
   mp_DrmCombo->setObjectName(QStringLiteral("drmCombo"));
   mp_DrmCombo->setFocusPolicy( Qt::StrongFocus );
   mp_DrmCombo->setToolTip(tr("Change the default Drum Set of this song"));

   p_DrmComboLayout->addSpacerItem(new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum));
   p_DrmComboLayout->addWidget(mp_DrmCombo);
   p_DrmComboLayout->addSpacerItem(new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum));

   p_FrameLayout->addSpacerItem(new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding));
   p_FrameLayout->addSpacerItem(new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding));

   QHBoxLayout *p_APLabelLayout = new QHBoxLayout();
   p_FrameLayout->addLayout(p_APLabelLayout);
   mp_APLabel = new QLabel(this);
   mp_APLabel->setText(tr("AutoPilot:"));
   mp_APLabel->setObjectName(QStringLiteral("APLabel"));

   p_APLabelLayout->addSpacerItem(new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum));
   p_APLabelLayout->addWidget(mp_APLabel);
   p_APLabelLayout->addSpacerItem(new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum));

   QHBoxLayout *p_APComboLayout = new QHBoxLayout();
   p_FrameLayout->addLayout(p_APComboLayout);
   mp_APBox = new QCheckBox(this);
   mp_APBox->setChecked(false);
   mp_APBox->setObjectName(QStringLiteral("APBox"));
   mp_APBox->setFocusPolicy( Qt::StrongFocus );
   mp_APBox->setToolTip(tr("Turn On or Off the AutoPilot for this song"));

   p_APComboLayout->addSpacerItem(new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum));
   p_APComboLayout->addWidget(mp_APBox);
   p_APComboLayout->addSpacerItem(new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum));

   p_FrameLayout->addSpacerItem(new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding));
   p_FrameLayout->addSpacerItem(new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding));

   // connect slots to signals
   // the order in which connections are being made is CRUCIAL!
   // connect `slotSubWidgetClicked()' first to get selection changed BEFORE value changes
   connect(mp_TempoSpin, SIGNAL(editingFinished      (   )), this,         SLOT(slotSubWidgetClicked(   )));
   connect(mp_TempoSpin, SIGNAL(valueChanged         (int)), this,         SLOT(slotSubWidgetClicked(   )));
   connect(mp_Title    , SIGNAL(cursorPositionChanged(int,int)), this,         SLOT(slotSubWidgetClicked (   )));
   connect(mp_Title    , SIGNAL(editingFinished()), this,         SLOT(slotSubWidgetClicked (   )));
   connect(mp_Title    , SIGNAL(selectionChanged()), this,         SLOT(slotSubWidgetClicked (   )));
   connect(mp_Title    , SIGNAL(textChanged(QString)), this,         SLOT(slotSubWidgetClicked (   )));
   connect(mp_Title    , SIGNAL(textEdited(QString)), this,         SLOT(slotSubWidgetClicked (   )));
   connect(mp_DrmCombo , SIGNAL(highlighted(int)), this,         SLOT(slotSubWidgetClicked (   )));
   connect(mp_APBox    , SIGNAL(clicked(bool)), this,         SLOT(slotSubWidgetClicked (   )));
   connect(mp_APBox    , SIGNAL(clicked(bool)), this,         SLOT(slotAPBoxClicked ( bool )));
   // connect value changed signals AFTER selection changed signals
   connect(this        , SIGNAL(sigTempoChangeByModel(int)), mp_TempoSpin, SLOT(setValue (int)));
   connect(this        , SIGNAL(sigAPEnableChangeByModel(bool)), mp_APBox, SLOT(setChecked(bool)));
   connect(mp_TempoSpin, SIGNAL(valueChanged         (int)), this,         SLOT(slotTempoChangeByUI (int)));
   connect(mp_Title    , SIGNAL(editingFinished      (   )), this,         SLOT(slotTitleChangeByUI (   )));
   if (Settings::midiIdEnabled()) {
        connect(mp_Number    , SIGNAL(editingFinished      (  )), this,         SLOT(slotNumberChangeByUI ()));
   }
}

void SongTitleWidget::slotTempoChangeByModel(int tempo)
{
   // Catch when not change in order to avoid crazy loops (if ui object doesn't catch it itself)
   if(tempo == mp_TempoSpin->value()){
      return;
   }
   emit sigTempoChangeByModel(tempo);
}

void SongTitleWidget::slotAutoPilotChangeByModel(bool state)
{
    if(state == mp_APBox->checkState()){
       return;
    }
    emit sigAPEnableChangeByModel(state);
}

void SongTitleWidget::slotTempoChangeByUI(int tempo)
{
   emit sigTempoChangeByUI(tempo);
}

void SongTitleWidget::slotNumberChangeByUI()
{
   emit sigNumberChangeByUI(mp_Number->text());
}

void SongTitleWidget::setTitle(const QString &title)
{
   mp_Title->setText(title);
}

QString SongTitleWidget::title() const
{
   return mp_Title->text();
}

void SongTitleWidget::setNumber(int number)
{
   m_Number = number;

   if (Settings::midiIdEnabled())
        mp_Number->setText(QString("%1").arg(m_Number));

}

void SongTitleWidget::setUnsavedChanges(bool unsaved)
{
   m_UnsavedChanges = unsaved;
   if(m_UnsavedChanges){
      mp_Saved->setText(QString("Unsaved"));
      mp_Saved->setStyleSheet("QLabel { color : red; }");
      mp_Saved->show();
   } else {
      mp_Saved->setText(QString("Saved"));
      mp_Saved->setStyleSheet("QLabel { color : green; }");
      QTimer::singleShot(3000, this, SLOT(slotBlankSaved()));

      // Remove any memorized drumset that is not selected anymore
      // If there is memorized index and memorisez index != index, remove it
      if(m_memorizedIndex > 0 && mp_DrmCombo->currentIndex() != m_memorizedIndex){
         mp_DrmCombo->removeItem(m_memorizedIndex);
         m_memorizedIndex = -1;
      }
   }
}

void SongTitleWidget::slotBlankSaved() {
    mp_Saved->hide();
}

void SongTitleWidget::slotAPBoxClicked(bool checked)
{
    emit sigAPEnableChangeByUI(mp_APBox->checkState());
    emit sigSubWidgetClicked();
}

void SongTitleWidget::slotTitleChangeByUI()
{
   emit sigTitleChangeByUI(mp_Title->text());
}

void SongTitleWidget::slotSubWidgetClicked()
{
   emit sigSubWidgetClicked();
}

void SongTitleWidget::slotRowsAboutToBeRemoved(const QModelIndex & parent, int start, int end)
{
   // Manages DRM being removed from project
   QModelIndex drmFolderIndex = mp_beatsModel->drmFolderIndex();
   if(parent.internalPointer() != drmFolderIndex.internalPointer()){
      return;
   }

   // build a list of ids to remove
   QList<QString> removedDrmFileNameList;
   for(int row = start; row <= end; row++){
      QModelIndex drmFileNameIndex = parent.child(row, AbstractTreeItem::FILE_NAME);
      removedDrmFileNameList.append(drmFileNameIndex.data().toString().toUpper());
   }

   // remove required items (first item is never removed
   for(int i = mp_DrmCombo->count() - 1; i >= 1 ; i--){
      // CSV: "FILE_NAME, NAME", file name always upper case
      QString defaultDrmCsv = mp_DrmCombo->itemData(i).toString();
      QStringList defaultDrm = defaultDrmCsv.split(",");

      if(defaultDrm.count() != 2){
         qWarning() << "SongTitleWidget::slotRowsAboutToBeRemoved - ERROR 1 - defaultDrm.count() != 2";
         continue;
      }
      QString drmFileName = defaultDrm.at(0).toUpper(); // Should be Upper Case, don't take any chances

      if(removedDrmFileNameList.contains(drmFileName)){
         // Special case if removed drm is curently selected
         if(i == mp_DrmCombo->currentIndex()){
            m_memorizedIndex = i;
            m_memorizedFileName = drmFileName;
            m_memorizedDrmName = defaultDrm.at(1);
            mp_DrmCombo->setItemText(i, QString("* %1").arg(m_memorizedDrmName));
         } else {
            mp_DrmCombo->removeItem(i);
            if(i < m_memorizedIndex){
               m_memorizedIndex--;
            }
         }

      }
   }
}

void SongTitleWidget::slotRowsInserted(const QModelIndex & parent, int start, int end)
{
   QModelIndex drmFolderIndex = mp_beatsModel->drmFolderIndex();
   if(parent.internalPointer() != drmFolderIndex.internalPointer()){
      return;
   }

   // insert all new Drms
   for(int row = start; row <= end; row++){
      insertNewDrm(row);
   }
}

void SongTitleWidget::slotDataChanged(const QModelIndex & topLeft, const QModelIndex & bottomRight, const QVector<int> & /*roles*/)
{
   // Manages Drumset being modified in project (Name or File name modified)
   QModelIndex drmFolderIndex = mp_beatsModel->drmFolderIndex();
   if(topLeft.parent().internalPointer() != drmFolderIndex.internalPointer()){
      return;
   }

   // If columns include NAME and FILE_NAME, keep only selected and re-populate table
   if (topLeft.column() <= AbstractTreeItem::NAME && bottomRight.column() >= AbstractTreeItem::FILE_NAME){
      // Remove all except selected and None
      for(int i = mp_DrmCombo->count() - 1; i >= 0; i--){
         if(i != mp_DrmCombo->currentIndex()){
            mp_DrmCombo->removeItem(i);
         }
      }
      // If selected is not None, memorize it
      if(mp_DrmCombo->currentIndex() == 1){
         // CSV: "FILE_NAME, NAME", file name always upper case
         QString defaultDrmCsv = mp_DrmCombo->itemData(1).toString();
         QStringList defaultDrm = defaultDrmCsv.split(",");
         if(defaultDrm.count() == 2){

            m_memorizedIndex = 1;
            m_memorizedFileName = defaultDrm.at(0); // Upper case
            m_memorizedDrmName = defaultDrm.at(1);
            mp_DrmCombo->setItemText(m_memorizedIndex, QString("* %1").arg(m_memorizedDrmName));

         }else {
            qWarning() << "SongTitleWidget::slotDataChanged - ERROR 1 - defaultDrm.count() != 2";
         }
      }

      // Re-Populate combo box
      for(int row = 0; row < mp_beatsModel->rowCount(drmFolderIndex); row ++){
         insertNewDrm(row);
      }


   // If columns include NAME identify with FILE_NAME
   } else if (topLeft.column() <= AbstractTreeItem::NAME && bottomRight.column() >= AbstractTreeItem::NAME){
      // Process All modified Rows
      for(int row = topLeft.row(); row <= bottomRight.row(); row++){

         QModelIndex drmFileNameIndex = drmFolderIndex.child(row, AbstractTreeItem::FILE_NAME);
         QString refDrmFileName = drmFileNameIndex.data().toString().toUpper(); // From Drm model needs to make sure converted to Upper

         // Find a row with corresponding id
         for(int i = 1; i < mp_DrmCombo->count(); i++){
            // CSV: "FILE_NAME, NAME", file name always upper case
            QString defaultDrmCsv = mp_DrmCombo->itemData(i).toString();
            QStringList defaultDrm = defaultDrmCsv.split(",");
            if(defaultDrm.count() != 2){
               qWarning() << "SongTitleWidget::slotDataChanged - ERROR 2 - defaultDrm.count() != 2";
               continue;
            }

            QString drmFileName = defaultDrm.at(0); // Upper case
            if(drmFileName == refDrmFileName){
               QModelIndex drmNameIndex = drmFolderIndex.child(row, AbstractTreeItem::NAME);
               QString drmName = drmNameIndex.data().toString();
               defaultDrmCsv = QString("%1,%2").arg(drmFileName, drmName); // drmFileName storred as upper case
               mp_DrmCombo->setItemData(i, QVariant(defaultDrmCsv));
               break;
            }
         }
      }

   // If columns include FILE_NAME identify with NAME
   } else if (topLeft.column() <= AbstractTreeItem::FILE_NAME && bottomRight.column() >= AbstractTreeItem::FILE_NAME){

      // Process All modified Rows
      for(int row = topLeft.row(); row <= bottomRight.row(); row++){

         QModelIndex drmNameIndex = drmFolderIndex.child(row, AbstractTreeItem::NAME);
         QString refDrmName = drmNameIndex.data().toString();

         // Find a row with corresponding id
         for(int i = 1; i < mp_DrmCombo->count(); i++){
            // CSV: "FILE_NAME, NAME", file name always upper case
            QString defaultDrmCsv = mp_DrmCombo->itemData(i).toString();
            QStringList defaultDrm = defaultDrmCsv.split(",");
            if(defaultDrm.count() != 2){
               continue;
            }
            QString drmName = defaultDrm.at(1);

            if(drmName == refDrmName){
               QModelIndex drmFileNameIndex = drmFolderIndex.child(row, AbstractTreeItem::FILE_NAME);
               defaultDrmCsv = QString("%1,%2").arg(drmFileNameIndex.data().toString().toUpper(), drmName); // drmFileName storred as upper case
               mp_DrmCombo->setItemData(i, QVariant(defaultDrmCsv));
               break;
            }
         }
      }
   }
   // Otherwise, change dont affect the combo box
}

void SongTitleWidget::insertNewDrm(int row)
{
   QModelIndex drmFolderIndex = mp_beatsModel->drmFolderIndex();

   QModelIndex drmFileNameIndex = drmFolderIndex.child(row, AbstractTreeItem::FILE_NAME);
   QModelIndex drmNameIndex = drmFolderIndex.child(row, AbstractTreeItem::NAME);

   QString drmFileName = drmFileNameIndex.data().toString().toUpper();  // From Drm model needs to make sure converted to Upper
   QString drmName = drmNameIndex.data().toString();

   // Verify it there is a memorized index that corresponds to inserted Drm
   if(m_memorizedIndex > 0 && drmFileName == m_memorizedFileName && drmName == m_memorizedDrmName){
      mp_DrmCombo->setItemText(m_memorizedIndex, drmName);
      m_memorizedIndex = -1;
   } else {
      //create data
      QVariant drmData(QString("%1,%2").arg(drmFileName, drmName)); // Always storred as upper case File Name
      mp_DrmCombo->insertItem(mp_DrmCombo->count(), drmName, drmData);
   }
}
void SongTitleWidget::populateDrmCombo(QModelIndex defaultDrmIndex)
{
   // Start by creating memorized entry if not None with Song Data
   // CSV: "FILE_NAME, NAME", file name always upper case
   QString defaultDrmCsv = defaultDrmIndex.data().toString();
   QStringList defaultDrm = defaultDrmCsv.split(",");
   QString drmFileName;
   if(defaultDrm.count() == 2){
      drmFileName = defaultDrm.at(0); // Upper Case
      // Do not insert 'None' or empty as drumset
      if(!drmFileName.isEmpty() && drmFileName.compare("00000000." BMFILES_DRUMSET_EXTENSION, Qt::CaseInsensitive)){
         m_memorizedIndex = 1;
         m_memorizedFileName = drmFileName;
         m_memorizedDrmName = defaultDrm.at(1);
         mp_DrmCombo->insertItem(1, QString("* %1").arg(m_memorizedDrmName), QString("%1,%2").arg(drmFileName, m_memorizedDrmName)); // drmFileName storred in upper case
      }
   } else {
      drmFileName = "";
      qWarning() << "SongTitleWidget::populateDrmCombo - ERROR 2 - defaultDrm.count() == 2";
   }

   // Then insert all drm from model
   QModelIndex drmFolderIndex = mp_beatsModel->drmFolderIndex();

   for(int row = 0; row < mp_beatsModel->rowCount(drmFolderIndex); row ++){
      insertNewDrm(row);
   }

   // Select current default Drm
   m_ModelDirectedChange = true;
   int index = indexOfDrmFileName(drmFileName);
   if(index >= 0){
      // select index
      mp_DrmCombo->setCurrentIndex(index);
   } else {
      qWarning() << "SongTitleWidget::populateDrmCombo - ERROR 2 - index < 0";
   }

   m_ModelDirectedChange = false;
   
   // Only connect after populated
   connect(mp_beatsModel, SIGNAL(rowsInserted(QModelIndex,int,int))                , this, SLOT(slotRowsInserted(QModelIndex,int,int)));
   connect(mp_beatsModel, SIGNAL(rowsAboutToBeRemoved(QModelIndex,int,int))        , this, SLOT(slotRowsAboutToBeRemoved(QModelIndex,int,int)));
   connect(mp_beatsModel, SIGNAL(dataChanged(QModelIndex,QModelIndex,QVector<int>)), this, SLOT(slotDataChanged(QModelIndex,QModelIndex,QVector<int>)));
   connect(mp_DrmCombo,   SIGNAL(currentIndexChanged(int))                         , this, SLOT(slotDefaultDrmChangeByUI(int)));
}

void SongTitleWidget::slotDefaultDrmChangeByModel(const QModelIndex &newDrmIndex)
{
   // New Drm info
   // CSV: "FILE_NAME, NAME", file name always upper case
   QString newDefaultDrmCsv = newDrmIndex.data().toString();
   QStringList newDefaultDrm = newDefaultDrmCsv.split(",");
   if(newDefaultDrm.count() != 2){
      qWarning() << "SongTitleWidget::slotDefaultDrmChangeByMode - ERROR 1 - defaultDrm.count() != 2 " << newDefaultDrm.count();
      return;
   }
   QString drmFileName = newDefaultDrm.at(0); // Upper Case

   // Retrieve current info
   // CSV: "FILE_NAME, NAME", file name always upper case
   QString defaultDrmCsv = mp_DrmCombo->currentData().toString();
   QStringList defaultDrm = defaultDrmCsv.split(",");
   if(defaultDrm.count() != 2){
      qWarning() << "SongTitleWidget::slotDefaultDrmChangeByMode - ERROR 3 - defaultDrm.count() != 2 " << defaultDrm.count();
      return;
   }

   QString currentDrmFileName = defaultDrm.at(0).toUpper(); // Upper Case

   // If there is a change find the index of the drm with this drmFileName
   if(drmFileName != currentDrmFileName){
      int index = indexOfDrmFileName(drmFileName);

      if(index < 0){
         // if index not found insert drm to list
         QString newDrmName = newDefaultDrm.at(1);
         QVariant drmData(QString("%1,%2").arg(drmFileName, newDrmName));

         index = mp_DrmCombo->count();
         mp_DrmCombo->insertItem(index, QString("* %1").arg(newDrmName), QVariant(drmData));

         // Select new item
         m_ModelDirectedChange = true;
         mp_DrmCombo->setCurrentIndex(index);

         // if there is a memorized index, remove it from combo
         if(m_memorizedIndex > 0){
            mp_DrmCombo->removeItem(m_memorizedIndex);
         }

         // Memorize the new inserted index
         m_memorizedIndex = mp_DrmCombo->count() - 1;
         m_memorizedFileName = drmFileName; // Upper Case
         m_memorizedDrmName = newDrmName;
      } else {
         // simply select index
         m_ModelDirectedChange = true;
         mp_DrmCombo->setCurrentIndex(index);
         // if there is a memorized index, remove it from combo
         if(m_memorizedIndex > 0){
            mp_DrmCombo->removeItem(m_memorizedIndex);
         }
      }
   }

   m_ModelDirectedChange = false;

}

void SongTitleWidget::slotDefaultDrmChangeByUI(int index)
{
   if(m_ModelDirectedChange){
      return;
   }

   // Retrieve new drm info
   // CSV: "FILE_NAME, NAME", file name always upper case
   QString defaultDrmCsv = mp_DrmCombo->itemData(index).toString();
   QStringList defaultDrm = defaultDrmCsv.split(",");
   if(defaultDrm.count() != 2){
      qWarning() << "SongTitleWidget::slotDefaultDrmChangeByUI - ERROR 1 - defaultDrm.count() != 2 " << defaultDrm.count();
      return;
   }
   QString drmFileName = defaultDrm.at(0); // Upper Case

   emit sigDefaultDrmChangeByUI(defaultDrm.at(1), drmFileName);
}

/**
 * refDrmFileName needs to be upper case
 */
int SongTitleWidget::indexOfDrmFileName(const QString &refDrmFileName)
{
   for(int i = 0; i < mp_DrmCombo->count(); i++){
      // CSV: "FILE_NAME, NAME", file name always upper case
      QString defaultDrmCsv = mp_DrmCombo->itemData(i).toString();
      QStringList defaultDrm = defaultDrmCsv.split(",");
      QString drmFileName = defaultDrm.at(0); // Upper Case

      if(refDrmFileName == drmFileName){
         return i;
      }
   }
   return -1;
}

/*
 * PRIVATE CLASS DEFINITION
 */

/*
 * MySpinBox
 */

MySpinBox::MySpinBox(QWidget *parent) :
   QSpinBox(parent)
{
   installEventFilter(this);
}

void MySpinBox::focusInEvent(QFocusEvent* event)
{
   setFocusPolicy(Qt::WheelFocus);

   QSpinBox::focusInEvent(event);
}

void MySpinBox::focusOutEvent(QFocusEvent* event)
{
   setFocusPolicy(Qt::StrongFocus);

   QSpinBox::focusOutEvent(event);
}

bool MySpinBox::eventFilter(QObject *obj, QEvent *event)
{
   if (event->type() == QEvent::Wheel && qobject_cast<MySpinBox*>(obj)) {

      if(qobject_cast<MySpinBox*>(obj)->focusPolicy() == Qt::WheelFocus)
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

/*
 * MyComboBox
 */

MyComboBox::MyComboBox(QWidget *parent) :
   QComboBox(parent)
{
   installEventFilter(this);
}

void MyComboBox::focusInEvent(QFocusEvent* event)
{

   setFocusPolicy(Qt::WheelFocus);

   QComboBox::focusInEvent(event);
}

void MyComboBox::focusOutEvent(QFocusEvent* event)
{
   setFocusPolicy(Qt::StrongFocus);

   QComboBox::focusOutEvent(event);
}

bool MyComboBox::eventFilter(QObject *obj, QEvent *event)
{
   if (event->type() == QEvent::Wheel && qobject_cast<MyComboBox*>(obj)) {
      if(qobject_cast<MyComboBox*>(obj)->focusPolicy() == Qt::WheelFocus)
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
