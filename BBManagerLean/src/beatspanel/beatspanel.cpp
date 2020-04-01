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
#include <QVBoxLayout>
#include <QLabel>
#include <QPainter>
#include <QDebug>
#include <QStyleOption>

#include "beatspanel.h"

#include "songfolderview.h"
#include "../stylesheethelper.h"
#include "../model/tree/abstracttreeitem.h"


BeatsPanel::BeatsPanel(QWidget *parent) :
   QWidget(parent)
{

   QVBoxLayout * p_VBoxLayout = new QVBoxLayout();
   setLayout(p_VBoxLayout);
   p_VBoxLayout->setContentsMargins(0,0,0,0);
   mp_Title = new QLabel(this);
   mp_Title->setText(tr("Beats"));
   mp_Title->setObjectName(QStringLiteral("titleBar"));
   p_VBoxLayout->addWidget(mp_Title, 0);

   QWidget *p_MainContainer = new QWidget(this);
   p_VBoxLayout->addWidget(p_MainContainer, 1);
   p_MainContainer->setLayout(new QGridLayout());

   mp_SongFolderView = new SongFolderView(p_MainContainer);
   p_MainContainer->layout()->addWidget(mp_SongFolderView);
   connect(mp_SongFolderView, SIGNAL(rootIndexChanged(QModelIndex)), this, SLOT(slotOnRootIndexChanged(QModelIndex)));
   connect(mp_SongFolderView, &SongFolderView::sigSelectTrack, this, &BeatsPanel::slotSelectTrack);
   connect(mp_SongFolderView, SIGNAL(sigSetTitle(QString)), this, SLOT(setTitle(QString)));
}

// Required to apply stylesheet
void BeatsPanel::paintEvent(QPaintEvent *)
{
   QStyleOption opt;
   opt.init(this);
   QPainter p(this);
   style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);
}




void BeatsPanel::slotSelectTrack(const QByteArray &trackData, int trackIndex, int typeId, int partIndex)
{
   emit sigSelectTrack(trackData, trackIndex, typeId, partIndex);
}

void BeatsPanel::slotOnRootIndexChanged(const QModelIndex &index)
{
   if(!index.isValid() || !mp_SongFolderView->model()){
      setTitle(tr("No Folder Selected"));
   } else {
     setTitle(tr("Beats - %1").arg(index.data().toString()));
   }
}

void BeatsPanel::slotSetPlayerEnabled(bool enabled)
{
   mp_SongFolderView->slotSetPlayerEnabled(enabled);
}

QModelIndex BeatsPanel::rootIndex()
{
   return mp_SongFolderView->rootIndex();
}
void BeatsPanel::setRootIndex(const QModelIndex &root)
{
   mp_SongFolderView->setRootIndex(root);
}

void BeatsPanel::setModel(QAbstractItemModel * model)
{
   mp_SongFolderView->setModel(model);
}

void BeatsPanel::setSelectionModel(QItemSelectionModel *selectionModel)
{
   mp_SongFolderView->setSelectionModel(selectionModel);
}

QItemSelectionModel * BeatsPanel::selectionModel()
{
   return mp_SongFolderView->selectionModel();
}

void BeatsPanel::setTitle(const QString &text)
{
    mp_Title->setText(text);
}

/**
 * @brief BeatsPanel::selectionChanged
   Change selection of the beatpanel
 * @param index
 */
void BeatsPanel::changeSelection(const QModelIndex& index)
{
    auto ix = mp_SongFolderView->model()->index(index.row(), AbstractTreeItem::NAME, index.parent());
    selectionModel()->select(ix, QItemSelectionModel::SelectCurrent | QItemSelectionModel::Clear);
    selectionModel()->setCurrentIndex(ix, QItemSelectionModel::SelectCurrent);
}

/**
 * @brief BeatsPanel::getTitle
 * Get the current folder's name write in the beatpanel
 * @return
 */
QString BeatsPanel::getTitle() const
{
   return mp_Title->text();
}


void BeatsPanel::setSongsEnabled(bool enabled)
{
   mp_SongFolderView->setSongsEnabled(enabled);
}
