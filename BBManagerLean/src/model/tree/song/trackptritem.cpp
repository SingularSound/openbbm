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
#include <QDebug>
#include <QElapsedTimer>
#include <QVariant>

#include "trackptritem.h"
#include "songpartitem.h"
#include "songfileitem.h"
#include "../../filegraph/songfilemodel.h"
#include "../../filegraph/songpartmodel.h"
#include "../../filegraph/songmodel.h"
#include "../../filegraph/songtrack.h"
#include "../../filegraph/songtrackdataitem.h"
#include "../../filegraph/autopilotdatamodel.h"
#include "../../filegraph/autopilotdatafillmodel.h"
#include "../../beatsmodelfiles.h"
#include "../project/beatsprojectmodel.h"
#include "trackarrayitem.h"

QElapsedTimer lastCall;
QList<int> droppedDrumFill;//drumfillindex,lastplayat, drumfillsongpart

TrackPtrItem::TrackPtrItem(AbstractTreeItem *parent, SongTracksModel * p_TracksModel, SongTrack * p_SongTrack):
   AbstractTreeItem(parent->model(), parent),
   mp_SongTrack(p_SongTrack),
   mp_TracksModel(p_TracksModel),
   m_playing(false)
{
}

QVariant TrackPtrItem::data(int column)
{
    switch (column) {
    case NAME:
        return mp_SongTrack ? mp_SongTrack->name() : tr("Undefined File");
    case ABSOLUTE_PATH:
        return mp_SongTrack ? mp_SongTrack->fullFilePath() : QString::null;
    case MAX_CHILD_CNT:
        return 0; // Cannot have child
    case TEMPO:
        return QVariant();  // Does not apply
    case SHUFFLE:
        return QVariant();  // Does not apply
    case TRACK_TYPE:
        return parent()->data(TRACK_TYPE);
    case PLAYING:
        return m_playing;
    case RAW_DATA:
        return getTrackData();
    case PLAY_AT_FOR:
        return autoPilotValues();
    case ERROR_MSG:
        return m_parseErrors;
    case EXPORT_DIR:
        // used to read the file name used to export the track
        return mp_SongTrack ? QString("%1.%2").arg(mp_SongTrack->name(), BMFILES_SONG_TRACK_EXTENSION) : QVariant();
    default:
        return AbstractTreeItem::data(column);
    }
}

bool TrackPtrItem::setData(int column, const QVariant & value)
{
    switch (column) {
    case NAME:
        return setName(value.toString(), false);
    case ABSOLUTE_PATH:
        if (mp_SongTrack) {
            mp_SongTrack->setFullPath(value.toString());
            return true;
        }
        return false;
    case PLAYING:
        m_playing = value.toBool();
        return true;
    case RAW_DATA:
        return setTrackData(value.toByteArray());
    case PLAY_AT_FOR:
        setAutoPilotValues(value.toList());
        return true;
    case ERROR_MSG:
        m_parseErrors = value.toStringList();
        return true;
    case EXPORT_DIR:
        // used trigger track export and choose export location
        return exportTo(value.toString());
    default:
        return AbstractTreeItem::setData(column, value);
    }
}


bool TrackPtrItem::setName(const QString &name, bool init)
{
   int trackType = -1;
   if(parent()->data(TRACK_TYPE).isValid()){
      trackType = parent()->data(TRACK_TYPE).toInt();
   }

   QStringList parseErrors;

   // Creates the track if it doesn't exist
   SongTrack *p_TrackIndex = mp_TracksModel->createTrack(name, trackType, &parseErrors);

   if(!parseErrors.empty()){
      // Manage Error
      setData(ERROR_MSG, QVariant(parseErrors));
      return false;
   }
   if(setSongTrack(p_TrackIndex)){

      emit sigFileSet(p_TrackIndex, parent()->rowOfChild(this));

      if(!init){
          auto ppp = parent()->parent()->parent();
          ppp->setData(SAVE, QVariant(true)); // unsaved changes, handles set dirty
          model()->itemDataChanged(ppp, SAVE);
      }
   }
   return true;

}

bool TrackPtrItem::exportTo(const QString &dstDirPath)
{
    QDir dstDir(dstDirPath);
    QStringList parseErrors;

    if(!dstDir.exists()){
        qWarning() << "TrackPtrItem::exportSongTrack - ERROR 1 - !dstDir.exists() - " << dstDirPath;
        parseErrors.append(tr("TrackPtrItem::exportSongTrack - ERROR 1 - !dstDir.exists() - %1").arg(dstDirPath));
        setData(ERROR_MSG, QVariant(parseErrors));
        return false;
    }

    QFileInfo dstFI(dstDir.absoluteFilePath(QString("%1.%2").arg(mp_SongTrack->name(), BMFILES_SONG_TRACK_EXTENSION)));

    // retrieve required information
    SongPartItem *p_spi = static_cast<SongPartItem *>(parent()->parent());
    SongFileItem *p_sfi = static_cast<SongFileItem *>(p_spi->parent());

    uint32_t timeSigNum = p_spi->timeSigDen();
    uint32_t timeSigDen = p_spi->timeSigNum();
    uint32_t tickPerBar = p_spi->ticksPerBar();
    uint32_t bpm = p_sfi->data(TEMPO).toInt();


    if(!mp_SongTrack->extractToTrackFile(dstFI.absoluteFilePath(),timeSigNum, timeSigDen, tickPerBar, bpm, &parseErrors)){
        qWarning() << "TrackPtrItem::exportSongTrack - ERROR 2 - Error while exporting";
        setData(ERROR_MSG, QVariant(parseErrors));
        return false;
    }

    return true;

}

bool TrackPtrItem::setSongTrack(SongTrack * p_SongTrack)
{
   if(p_SongTrack != nullptr && mp_SongTrack != p_SongTrack){
      mp_SongTrack = p_SongTrack;

      parent()->parent()->parent()->setData(SAVE, QVariant(true)); // unsaved changes, handles set dirty
      model()->itemDataChanged(parent()->parent()->parent(), SAVE);
      return true;
   }
   return false;
}
SongTrack * TrackPtrItem::songTrack()
{
   return mp_SongTrack;
}

int TrackPtrItem::trackIndex()
{
   return mp_SongTrack->index();
}

QByteArray TrackPtrItem::getTrackData()
{
   return mp_SongTrack->trackData()->toByteArray();
}

bool TrackPtrItem::setTrackData(const QByteArray& data)
{
    if (mp_SongTrack->parseByteArray(data)) {
        parent()->parent()->parent()->setData(SAVE, true); // unsaved changes, handles set dirty
        model()->itemDataChanged(parent()->parent()->parent(), SAVE);
        return true;
    }
    return false;
}

void TrackPtrItem::setAutoPilotValues(QList<QVariant> value)
{
    TrackArrayItem  * tai = static_cast<TrackArrayItem *>(parent());
    SongPartItem    * spi = static_cast<SongPartItem*>(tai->parent());
    SongPartModel   * spm = static_cast<SongPartModel*>(spi->filePart());
    SongFileItem    * sfi = static_cast<SongFileItem*>(spi->parent());
    SongFileModel   * sfm  = static_cast<SongFileModel*>(sfi->filePart());

    AutoPilotDataModel *apdm = static_cast<AutoPilotDataModel*>(sfm->getAutoPilotDataModel());

    if(value.size() < 2)
    {
        qWarning() << "Warning - QList of size smaller than 2. Size is " << value.size();
        return;
    }

    if(spm->isIntro())
    {
        static_cast<AutoPilotDataPartModel*>(apdm->getIntroModel())->getMainLoop()->setPlayAt(value.at(0).toInt());
        static_cast<AutoPilotDataPartModel*>(apdm->getIntroModel())->getMainLoop()->setPlayFor(value.at(1).toInt());
    } else if( spm->isOutro()) {
        static_cast<AutoPilotDataPartModel*>(apdm->getOutroModel())->getMainLoop()->setPlayAt(value.at(0).toInt());
        static_cast<AutoPilotDataPartModel*>(apdm->getOutroModel())->getMainLoop()->setPlayFor(value.at(1).toInt());
    } else {
        MIDIPARSER_TrackType trackType = (MIDIPARSER_TrackType)parent()->data(TRACK_TYPE).toInt();
        int partRow = tai->parent()->row(); //thats the stuff for 1-32
        if(partRow<=0)
        {
            ///Should Not Happen.
            qWarning() << "trackptritem: Warning - Invalid Row int setAutoPilotValues(). Row is: " << partRow;
            return;
        }

        if(trackType == MAIN_DRUM_LOOP ){
            //Which of the 32?
            AutoPilotDataPartModel * partModel =  apdm->getPartModel(partRow-1);
            partModel->getMainLoop()->setPlayAt(value.at(0).toInt());
            partModel->getMainLoop()->setPlayFor(value.at(1).toInt());

        }else if( trackType == TRANS_FILL ){
            //Which of the 32?
            AutoPilotDataPartModel * partModel =  apdm->getPartModel(partRow-1);
            if((!lastCall.isValid() || lastCall.elapsed() > 50))
            {
                droppedDrumFill.clear();
                droppedDrumFill.push_back(0);
                droppedDrumFill.push_back(partModel->getTransitionFill()->getPlayFor());
                droppedDrumFill.push_back(partRow-1);
                partModel->getTransitionFill()->setPlayAt(value.at(0).toInt());
                partModel->getTransitionFill()->setPlayFor(value.at(1).toInt());
            }else{//if drag and drop swaps playFors
                AutoPilotDataPartModel * lastpartModel =  apdm->getPartModel(droppedDrumFill[2]);
                int crntPlayFor = partModel->getTransitionFill()->getPlayFor();
                partModel->getTransitionFill()->setPlayFor(droppedDrumFill[1]);
                lastpartModel->getTransitionFill()->setPlayFor(crntPlayFor);
            }


        }else if( trackType == DRUM_FILL){
            int drumFillIndex = row();   //thats the stuff for 0-7
            //Which of the 32?
            //Which of the 8?
            AutoPilotDataPartModel * partModel =  apdm->getPartModel(partRow-1);
            if((!lastCall.isValid() || lastCall.elapsed() > 50))
            {
                droppedDrumFill.clear();
                droppedDrumFill.push_back(drumFillIndex);
                droppedDrumFill.push_back(partModel->getDrumFill(drumFillIndex)->getPlayAt());
                droppedDrumFill.push_back(partRow-1);

                partModel->getDrumFill(drumFillIndex)->setPlayAt(value.at(0).toInt());
                partModel->getDrumFill(drumFillIndex)->setPlayFor(value.at(1).toInt());
            }else{//if drag and drop swaps playAts
                qDebug() << "The elapsed time was" <<lastCall.elapsed();
                AutoPilotDataPartModel * lastpartModel =  apdm->getPartModel(droppedDrumFill[2]);
                int crrntPlayAt = partModel->getDrumFill(drumFillIndex)->getPlayAt();
                partModel->getDrumFill(drumFillIndex)->setPlayAt(droppedDrumFill[1]);//Change the playAt to previous drumfill playat value
                lastpartModel->getDrumFill(droppedDrumFill[0])->setPlayAt(crrntPlayAt);//change previous playAt to currentplayAt
            }
        }
    }
    auto ppp = parent()->parent()->parent();
    ppp->setData(SAVE, QVariant(true)); // unsaved changes, handles set dirty
    model()->itemDataChanged(ppp, SAVE);
    lastCall.start();
}

QList<QVariant> TrackPtrItem::autoPilotValues()
{
    TrackArrayItem      * tai = static_cast<TrackArrayItem *>(parent());
    SongPartItem        * spi = static_cast<SongPartItem*>(tai->parent());
    SongPartModel       * spm = static_cast<SongPartModel*>(spi->filePart());
    SongFileItem        * sfi = static_cast<SongFileItem*>(spi->parent());
    SongFileModel       * sfm  = static_cast<SongFileModel*>(sfi->filePart());
    AutoPilotDataModel  *apdm = static_cast<AutoPilotDataModel*>(sfm->getAutoPilotDataModel());

    if(spm->isIntro())
    {
        return QList<QVariant>() << static_cast<AutoPilotDataPartModel*>(apdm->getIntroModel())->getMainLoop()->getPlayAt()
                                 << static_cast<AutoPilotDataPartModel*>(apdm->getIntroModel())->getMainLoop()->getPlayFor();
    } else if( spm->isOutro()) {
        return QList<QVariant>() << static_cast<AutoPilotDataPartModel*>(apdm->getOutroModel())->getMainLoop()->getPlayAt()
                                 << static_cast<AutoPilotDataPartModel*>(apdm->getOutroModel())->getMainLoop()->getPlayFor();
    } else {
        MIDIPARSER_TrackType trackType = (MIDIPARSER_TrackType)parent()->data(TRACK_TYPE).toInt();
        int partRow = tai->parent()->row();

        if(trackType == MAIN_DRUM_LOOP ){

            AutoPilotDataPartModel * partModel =  apdm->getPartModel(partRow-1);
            return QList<QVariant>() << partModel->getMainLoop()->getPlayAt()
                                    << partModel->getMainLoop()->getPlayFor();

        }else if( trackType == TRANS_FILL ){

            AutoPilotDataPartModel * partModel =  apdm->getPartModel(partRow-1);
            return QList<QVariant>() << partModel->getTransitionFill()->getPlayAt()
                                    << partModel->getTransitionFill()->getPlayFor();

        }else if( trackType == DRUM_FILL ){
            int drumFillIndex = row();

            AutoPilotDataPartModel * partModel =  apdm->getPartModel(partRow-1);
            return QList<QVariant>() << partModel->getDrumFill(drumFillIndex)->getPlayAt()
                                    << partModel->getDrumFill(drumFillIndex)->getPlayFor();
        }
    }
    return QList<QVariant>() << 0 << 0;
}
