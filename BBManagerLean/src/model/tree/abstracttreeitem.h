#ifndef ABSTRACTSONGPARTMODEL_H
#define ABSTRACTSONGPARTMODEL_H

#include <QObject>
#include <QDebug>
#include <QAbstractItemModel>
#include <QByteArray>

class BeatsProjectModel;

class AbstractTreeItem: public QObject
{
   Q_OBJECT
public:

   enum Column
   {
      NAME,
      CPP_TYPE,
      FILE_NAME,
      ABSOLUTE_PATH,
      MAX_CHILD_CNT,
      ACTUAL_VERSION,
      TEMPO,
      SHUFFLE,
      SAVE,
      CHILDREN_TYPE,
      UUID,
      TRACK_TYPE,
      INVALID,
      ERROR_MSG,
      PLAYING,
      HASH,
      //LOADED,
      RAW_DATA,
      PLAY_AT_FOR,  //Created to Undo and Redo both settings at the same time with the use of a QPoint.
      AUTOPILOT_ON,
      AUTOPILOT_VALID,
      DEFAULT_DRM,
      EXPORT_DIR,  // Write: Used to export tracks/accent hit while setting location. Read - get default export file name.
      LOOP_COUNT,
      ENUM_SIZE
   };

   static QString columnName(int col) {
       switch (col)
       {
       case NAME: return tr("Title");
       case CPP_TYPE: return tr("C++ Type");
       case FILE_NAME: return tr("File Name");
       case ABSOLUTE_PATH: return tr("Absolute Path");
       case MAX_CHILD_CNT: return tr("Max Children Cnt");
       case ACTUAL_VERSION: return tr("Actual Version");
       case TEMPO: return tr("Tempo");
       case SHUFFLE: return tr("Shuffle");
       case SAVE: return tr("Save Status");
       case CHILDREN_TYPE: return tr("File type");
       case UUID: return tr("UUID");
       case TRACK_TYPE: return tr("Track Type");
       case INVALID: return tr("Invalid");
       case ERROR_MSG: return tr("Last Error Message");
       case PLAYING: return tr("Playing");
       case HASH: return tr("Hash");
       case RAW_DATA: return tr("RawData");
       case PLAY_AT_FOR:   return tr("Play At/For");
       case AUTOPILOT_ON:return tr("AutoPilot On");
       case AUTOPILOT_VALID: return tr("AutoPilot Valid");
       case DEFAULT_DRM: return tr("Default Drumset");
       case EXPORT_DIR: return tr("Export Filename");
       case LOOP_COUNT: return tr("MIDI Id");
       default: return QString::null;
       }
   }

   explicit AbstractTreeItem(BeatsProjectModel *p_model, AbstractTreeItem *parent = nullptr);
   ~AbstractTreeItem();

   // Child Interaction. Should only be called by model
   AbstractTreeItem *child(int row) const;
   int rowOfChild(AbstractTreeItem *p_Child);
   void appendChild(AbstractTreeItem *child);
   void insertChildAt(int row, AbstractTreeItem *child);
   int childCount() const;

   virtual QVariant data(int column);
   virtual bool setData(int column, const QVariant & value);
   virtual void insertNewChildAt(int row);
   virtual void removeChild(int row);
   virtual void moveChildren(int sourceFirst, int sourceLast, int delta);
   virtual void computeHash(bool recursive);
   virtual void propagateHashChange();
   virtual Qt::ItemFlags flags(int column);

   virtual QByteArray hash();
   virtual bool compareHash(const QString &path);
   virtual void prepareSync(const QString &dstPath, QList<QString> *p_cleanUp, QList<QString> *p_copySrc, QList<QString> *p_copyDst);

   inline virtual bool isFile()  { return false; }
   inline virtual bool isFolder() { return false; }

   // Other Methods required by Model
   int row() const;
   AbstractTreeItem *parent() const;
   BeatsProjectModel *model() const;

protected:
   inline QList<AbstractTreeItem*> *childItems(){return &m_childItems;}
   void removeChildInternal(int row);
   virtual void setHash(const QByteArray &hash);

private:
   QList<AbstractTreeItem*> m_childItems;
   AbstractTreeItem *mp_parentItem;
   BeatsProjectModel *mp_Model;

};

#endif // ABSTRACTSONGPARTMODEL_H
