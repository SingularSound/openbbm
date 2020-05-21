#ifndef BEATSPROJECTMODEL_H
#define BEATSPROJECTMODEL_H

#include <QAbstractItemModel>
#include <QList>
#include <QDir>
#include <QProgressDialog>
#include <QUndoStack>

#include "version.h"

/**
 *   BEATSPROJECTMODEL_FILE #1 :
 *       Revision 0 : Original Version
 */

#define BEATSPROJECTMODEL_FILE_VERSION  1u
#define BEATSPROJECTMODEL_FILE_REVISION 0u
#define BEATSPROJECTMODEL_FILE_BUILD    VER_BUILDVERSION

class FolderTreeItem;
class AbstractTreeItem;
class ContentFolderTreeItem;
class EffectFolderTreeItem;
class DrmFolderTreeItem;
class ParamsFolderTreeModel;
class SongsFolderTreeItem;

class BeatsProjectModel : public QAbstractItemModel
{
   Q_OBJECT
public:
   BeatsProjectModel(const QString &projectDirPath, QWidget *parent, const QString &tmpDirPath);
   ~BeatsProjectModel();

   QModelIndex defaultSongFolderIndex() const;

   QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
   Qt::ItemFlags flags(const QModelIndex &index) const;
   QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;
   QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const;
   QModelIndex parent(const QModelIndex &index) const;
   inline QModelIndex selected() const { return m_selectedItem; }
   int rowCount(const QModelIndex &parent = QModelIndex()) const;
   int columnCount(const QModelIndex &parent = QModelIndex()) const;

   bool setData(const QModelIndex & index, const QVariant & value, int role = Qt::EditRole);

   bool insertRows(int row, int count, const QModelIndex & parent);
   bool removeRows(int row, int count, const QModelIndex & parent);
   bool moveRows(const QModelIndex & sourceParent, int sourceRow, int count, const QModelIndex & destinationParent, int destinationChild);

   // START of utility methods for the player
   inline QModelIndex partIndex(const QModelIndex &songIndex, int partNumber){
      return partIndexInternal(songIndex, partNumber + 1);
   }
   inline QModelIndex introPartIndex(const QModelIndex &songIndex){
      return partIndexInternal(songIndex, 0);
   }
   inline QModelIndex outroPartIndex(const QModelIndex &songIndex){
      return partIndexInternal(songIndex, -1);
   }

   /**
    * @brief trackPtrIndex returns a QModelIndex corresponding to a track Ptr
    * @param partIndex
    * @param trackArrayRow corresponds to the column in the display. 0 = main loop, 1 = drum fill, 2 = trans fill and 3 = special effect.
    * @param trackPtrRow drum fill index. For main loop and trans fill, must be 0.
    * @return a QModelIndex corresponding to a track Ptr
    */
   QModelIndex trackPtrIndex(const QModelIndex &partIndex, int trackArrayRow, int trackPtrRow = 0);

   // END of utility methods for the player


   EffectFolderTreeItem * effectFolder() const;
   DrmFolderTreeItem * drmFolder() const;
   SongsFolderTreeItem * songsFolder() const;
   ParamsFolderTreeModel * paramsFolder() const;
   QModelIndex effectFolderIndex() const;
   QModelIndex drmFolderIndex() const;
   QModelIndex songsFolderIndex() const;
   QModelIndex paramsFolderIndex() const;
   inline FolderTreeItem * rootItem() const {return mp_RootItem;}

   static bool isProjectFolder(const QString &path, bool requireBBP = false, bool requiresHash = false);
   static bool isProjectArchiveFile(const QString &path);
   static bool containsAnyProjectEntry(const QString &path, bool includeHash = false);
   QFileInfoList projectEntriesInfo(bool includeBBP = true) const;
   QStringList projectEntries(bool includeBBP = true) const;
   static QStringList projectEntriesForPath(const QString &path, bool includeBBP = true, bool includeHash = true);
   static QStringList existingProjectEntriesForPath(const QString &path, bool includeBBP = true, bool includeHash = true);

   void synchronizeModal(const QString &path, QWidget *p_parent);
   static bool cleanUpModal(const QStringList &cleanUpPaths, QProgressDialog &progress, QWidget *p_parent);
   static bool copyModal(const QStringList &srcPaths, const QStringList &dstPaths, QProgressDialog &progress, QWidget *p_parent);

   static QFileInfo createProjectFolderForProjectFile(const QFileInfo &projectFileFI);

   bool isProject3WayLinked(const QString &path);
   bool isProject2WayLinked();
   bool linkProject(const QString &path);
   static bool resetProjectInfo(const QString &path);
   bool resetProjectInfo();
   QModelIndex createNewSongFolder(int row);
   void deleteSongFolder(const QModelIndex& index);
   QModelIndex createNewSong(const QModelIndex& parent, int row);
   void deleteSong(const QModelIndex& index);
   QModelIndex createNewSongPart(const QModelIndex& parent, int row);
   void deleteSongPart(const QModelIndex& index);
   QModelIndex createSongFile(const QModelIndex& parent, int row, const QString& filename);
   void changeSongFile(const QModelIndex& part, const QString& filename);
   void changeSongFile(const QModelIndex& part, const QByteArray& data);
   void deleteSongFile(const QModelIndex& index);
   bool moveItem(const QModelIndex& index, int delta);
   bool moveOrCopySong(bool move, const QModelIndex& song, const QModelIndex& folder, int row);
   void importModal(QWidget* p_parentWidget, const QModelIndex& index, const QStringList& songs, const QStringList& folders);

   void manageParsingErrors(QWidget *p_parent);

   inline void setEditingDisabled(bool disabled){
      m_editingDisabled = disabled;
      // In order to notify that flags changed
      // Note : should also loop on songs but works like this for now
      QModelIndex topLeft = songsFolderIndex().child(0, 0);
      QModelIndex bottomRight = songsFolderIndex().child(rowCount(songsFolderIndex()) - 1, 0);
      emit dataChanged(topLeft,bottomRight);
   }
   inline bool isEditingDisabled(){
      return m_editingDisabled;
   }

   inline void setSelectionDisabled(bool disabled){
      m_selectionDisabled = disabled;
      // In order to notify that flags changed
      // Note : should also loop on songs but works like this for now
      QModelIndex topLeft = songsFolderIndex().child(0, 0);
      QModelIndex bottomRight = songsFolderIndex().child(rowCount(songsFolderIndex()) - 1, 0);
      emit dataChanged(topLeft,bottomRight);
   }
   inline bool isSelectionDisabled(){
      return m_selectionDisabled;
   }


   inline QFileInfo projectDirFI() const{
      return m_projectDirFI;
   }
   inline QFileInfo projectFileFI() const{
      return m_projectFileFI;
   }

   inline void setProjectDirty(){
      m_projectDirty = true;
   }

   inline bool isProjectDirty() const{
      return m_projectDirty;
   }

   inline bool isSongsFolderDirty() const{
      return m_songsFolderDirty;
   }
   inline void setSongsFolderDirty(){
      m_songsFolderDirty = true;
   }


   void saveModal();
   bool saveAsModal(const QFileInfo &newProjectFileFI, QWidget *p_parent);
   void saveProjectArchive(const QString& path, QWidget *p_parent);
   static void saveProjectFile(const QString& filePath);



    inline QDir copyClipboardDir() { return m_copyClipboardDir;  }
    inline QDir pasteClipboardDir(){ return m_pasteClipboardDir; }
    inline QDir dragClipboardDir() { return m_dragClipboardDir;  }
    inline QDir dropClipboardDir() { return m_dropClipboardDir;  }
    inline QDir dirUndoRedo()      { return m_dirUndoRedo;       }

    inline QUndoStack* undoStack() { return m_stack; }
    class Macro
    {
        mutable BeatsProjectModel* self;
        template <typename T> void operator=(const T&);
    public:
        Macro(BeatsProjectModel* self, const QString& name);
        Macro(const Macro& other);
        ~Macro();
    };
    inline Macro undoMacro(const QString& name) { return Macro(this, name); }

    QString songFileName(const QModelIndex& songpart);
    QString songFileName(const QModelIndex& parent, int child);

    QList<int> getAPSettingInQueue();
    void    addAPSettingInQueue(QList<int> settings);
    void clearAPSettingQueue();
    bool APSettingIsEmpty();
    QList<int> getAPSettings(QModelIndex index);
public slots:
   void itemDataChanged(AbstractTreeItem * item, int column);
   void itemDataChanged(AbstractTreeItem * item, int leftColumn, int rightColumn);
   void insertItem(AbstractTreeItem * item, int row);
   void removeItem(AbstractTreeItem * item, int row);
   void moveItem(AbstractTreeItem * item, int sourceRow, int destRow);
   void selectionChanged(const QModelIndex& current, const QModelIndex& previous);

   void moveSelection(const QModelIndex& to, bool undoable = false);

signals:
    void beginEditMidi(const QString& name, const QByteArray& data);
    void editMidi(const QByteArray& data);
    void endEditMidi(const QByteArray& data = QByteArray()); // apply changes to the song file, cancel on empty QByteArray
    void changeSelection(const QModelIndex& to);

private:
   QModelIndex m_selectedItem;
   QModelIndex partIndexInternal(const QModelIndex &songIndex, int partNumber);

   //Queue to handle Undo/Redo for autopilot settings with Drags/Drops
   QList<QList<int>> m_APSettings;

   FolderTreeItem * mp_RootItem;
   QPersistentModelIndex m_DefaultSongFolderIndex;
   QList<QVariant> mp_Header;

   QFileInfo m_projectDirFI;
   QFileInfo m_projectFileFI;

   bool m_editingDisabled;
   bool m_selectionDisabled;

   // Used to track any change in project since last save.
   // Not used for now but kept since lot of job to introduce.
   bool m_projectDirty;
   
   // Flag that tracks unsaved changes in songs. True if any song has changes that need to be saved.
   bool m_songsFolderDirty;

   QDir m_tempDir;
   QDir m_copyClipboardDir;
   QDir m_pasteClipboardDir;
   QDir m_dragClipboardDir;
   QDir m_dropClipboardDir;
   QDir m_dirUndoRedo;

    QUndoStack* m_stack;
};

class BlockUndoForSelectionChangesFurtherInThisScope
{
    bool block;
    template <typename T> void operator=(const T&);
public:
    BlockUndoForSelectionChangesFurtherInThisScope();
    BlockUndoForSelectionChangesFurtherInThisScope(BlockUndoForSelectionChangesFurtherInThisScope& other);
    ~BlockUndoForSelectionChangesFurtherInThisScope();
};

#endif // BEATSPROJECTMODEL_H
