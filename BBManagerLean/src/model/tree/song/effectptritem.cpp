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
#include <QUuid>

#include "effectptritem.h"

#include "songpartitem.h"
#include "../project/beatsprojectmodel.h"

EffectPtrItem::EffectPtrItem(AbstractTreeItem *parent, EffectFolderTreeItem * p_EffectFolderTreeItem, EffectFileItem * p_EffectFileItem):
   AbstractTreeItem(parent->model(), parent),
   mp_EffectFolderTreeItem(p_EffectFolderTreeItem),
   mp_EffectFileItem(p_EffectFileItem)
{
}

QVariant EffectPtrItem::data(int column)
{
   switch (column) {
      case EXPORT_DIR:
          // used to read the file name used to export the effect
          if(mp_EffectFileItem != nullptr){
             return QString("%1.%2").arg(mp_EffectFileItem->data(NAME).toString(), BMFILES_WAVE_EXTENSION);
          }
         return QVariant();
      case NAME:
         if(mp_EffectFileItem != nullptr){
            return mp_EffectFileItem->data(column);
         }
         return QVariant("Undefined File");
      case FILE_NAME:
      case ABSOLUTE_PATH:
      case HASH:
         if(mp_EffectFileItem != nullptr){
            return mp_EffectFileItem->data(column);
         }
         return QVariant();
      default:
          return AbstractTreeItem::data(column);
   }
}

bool EffectPtrItem::setData(int column, const QVariant & value)
{
    switch (column) {
    case NAME:
        setName(value.toString(), false);
        return true;
    case EXPORT_DIR:
        // used trigger accent hit export and choose export location
        return exportTo(value.toString());
    case ABSOLUTE_PATH:
        return true;
    default:
        return AbstractTreeItem::setData(column, value);
    }
}

void EffectPtrItem::setName(const QString &name, bool init)
{
   QString efxFileName;

   QUuid songUuid = parent()->parent()->parent()->data(UUID).toUuid();

   if(mp_EffectFileItem){
      efxFileName = mp_EffectFileItem->data(FILE_NAME).toString();
      mp_EffectFolderTreeItem->removeUse(efxFileName, songUuid);
   }

   mp_EffectFileItem = mp_EffectFolderTreeItem->addUse(name, songUuid);

   SongPartItem *p_grandParent = static_cast<SongPartItem *>(parent()->parent());
   efxFileName = mp_EffectFileItem->data(FILE_NAME).toString();
   p_grandParent->setEffectFileName(efxFileName);

   if(!init){
      parent()->parent()->parent()->setData(SAVE, QVariant(true)); // unsaved changes (handles dirty)
      model()->itemDataChanged(parent()->parent()->parent(), SAVE);
   }
}

bool EffectPtrItem::exportTo(const QString &dstDirPath)
{
    QDir dstDir(dstDirPath);
    QStringList parseErrors;

    if(!dstDir.exists()){
        qWarning() << "TrackPtrItem::exportSongTrack - ERROR 1 - !dstDir.exists() - " << dstDirPath;
        parseErrors.append(tr("EffectPtrItem::exportTo - ERROR 1 - !dstDir.exists() - %1").arg(dstDirPath));
        setData(ERROR_MSG, QVariant(parseErrors));
        return false;
    }

    if (!mp_EffectFileItem) {
        qWarning() << "EffectPtrItem::exportTo - ERROR 3 - Error getting name of item.  It has no name.";
        parseErrors.append(tr("EffectPtrItem::exportTo - ERROR 3 - Error getting name of item.  It has no name."));
        setData(ERROR_MSG, QVariant(parseErrors));
        return false;
    }
    QFileInfo dstFI(dstDir.absoluteFilePath(QString("%1.%2").arg(mp_EffectFileItem->data(NAME).toString(), BMFILES_WAVE_EXTENSION)));
    QFileInfo srcFI(mp_EffectFileItem->data(ABSOLUTE_PATH).toString());


    // Copy to destination
    if(!QFile::copy(srcFI.absoluteFilePath(), dstFI.absoluteFilePath())){
        qWarning() << "EffectPtrItem::exportTo - ERROR 2 - Error while copying " << srcFI.absoluteFilePath() << " to " << dstFI.absoluteFilePath();
        parseErrors.append(tr("EffectPtrItem::exportTo - ERROR 2 - Error while copying %1 to %2").arg(srcFI.absoluteFilePath(), dstFI.absoluteFilePath()));
        setData(ERROR_MSG, QVariant(parseErrors));
        return false;
    }

    return true;

}

/**
 * @brief EffectPtrItem::clearEffectUsage
 *
 * Called when part or track is being removed
 * sub calls handle model()->setArchiveDirty()
 */
void EffectPtrItem::clearEffectUsage()
{
   if(mp_EffectFileItem){
      QUuid songUuid = parent()->parent()->parent()->data(UUID).toUuid();
      QString efxFileName = mp_EffectFileItem->data(FILE_NAME).toString();
      mp_EffectFolderTreeItem->removeUse(efxFileName, songUuid);
   }
}
