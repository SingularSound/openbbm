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
#include "beatsprojectmodel.h"
#include "foldertreeitem.h"
#include "../song/songpartitem.h"
#include "../../../workspace/settings.h"
#include "utils/dirlistallsubfilesmodal.h"
#include "utils/dircleanupmodal.h"
#include "utils/dircopymodal.h"
#include "utils/compresszipmodal.h"
#include "../../beatsmodelfiles.h"
#include "platform/platform.h"
#include "../../index.h"
#include "minIni.h"
#include "../../filegraph/songmodel.h"

#include "contentfoldertreeitem.h"
#include "drmfoldertreeitem.h"
#include "effectfoldertreeitem.h"
#include "paramsfoldertreemodel.h"
#include "songsfoldertreeitem.h"
#include "../song/songfoldertreeitem.h"
#include "../song/songfileitem.h"
#include "quazip.h"
#include "quazipdir.h"
#include "quazipfile.h"
#include "quazipfileinfo.h"

#include <QMessageBox>
#include <QProgressDialog>
#include <QXmlStreamWriter>
#include <QFile>
#include <QIODevice>


static int undo_command_enumerator = 0;

class CmdChangeSelection : public QUndoCommand
{
    BeatsProjectModel* m_model;
    Index m_new, m_old;
    static bool block;
    friend BlockUndoForSelectionChangesFurtherInThisScope;

    CmdChangeSelection(BeatsProjectModel* model, const QModelIndex& from, const QModelIndex& to)
        : QUndoCommand()
        , m_model(model)
        , m_new(to)
        , m_old(from)
    {
        updateText();
    }
    void updateText()
    {
        setText(QObject::tr("Changing Selection to \"%1\"", "Undo Commands")
            .arg(m_new ? m_new(m_model).sibling(m_new[-1], AbstractTreeItem::NAME).data().toString() : "(_._)")
            );
    }

public:
    static void queue(BeatsProjectModel* model, const QModelIndex& from, const QModelIndex& to)
    {
        if (block) return;
        model->undoStack()->push(new CmdChangeSelection(model, from, to));
    }
    static void apply(BeatsProjectModel* model, const QModelIndex& index)
    {
        BlockUndoForSelectionChangesFurtherInThisScope _;
        emit model->changeSelection(index);
    }

    void redo()
    {
        apply(m_model, m_new(m_model));
    }
    void undo()
    {
        apply(m_model, m_old(m_model));
    }
    int id() const { static int id = undo_command_enumerator++; return id; }
    bool mergeWith(const QUndoCommand* other)
    {
        m_new = ((CmdChangeSelection*)other)->m_new;
        updateText();
        return true;
    }
};
bool CmdChangeSelection::block = false;
BlockUndoForSelectionChangesFurtherInThisScope::BlockUndoForSelectionChangesFurtherInThisScope() : block(CmdChangeSelection::block) { CmdChangeSelection::block = true; }
BlockUndoForSelectionChangesFurtherInThisScope::BlockUndoForSelectionChangesFurtherInThisScope(BlockUndoForSelectionChangesFurtherInThisScope& other) : block(other.block) { other.block = true; }
BlockUndoForSelectionChangesFurtherInThisScope::~BlockUndoForSelectionChangesFurtherInThisScope() { CmdChangeSelection::block = block; }

class CmdSetData : public QUndoCommand
{
    BeatsProjectModel* m_model;
    Index m_index;
    QVariant m_new, m_old;

    CmdSetData(BeatsProjectModel* model, const QModelIndex& index, const QVariant& value)
        : QUndoCommand()
        , m_model(model)
        , m_index(index)
        , m_new(value)
        , m_old(index.data())
    {
        updateText();
    }
    void updateText()
    {
        if (AbstractTreeItem::NAME == m_index || AbstractTreeItem::FILE_NAME == m_index) {

            qDebug() <<"check value for comma:" << m_new.toString();
            QString v = m_new.toString().replace(",", "_");
            if (0 != v.compare(m_new.toString())) {
                qWarning() << "BeatsProjectModel::setData (updateText) - comma found -- changed to _" << m_new << "to" << v;
            }
            m_new = v;
        }
        setText(QObject::tr("Changing %1 %2 <- %3", "Undo Commands")
            .arg(AbstractTreeItem::columnName(m_index()))
			.arg(m_new.toString())
            .arg(m_old.toString())
            );
    }

public:
    static bool queue(BeatsProjectModel* model, const QModelIndex& index, const QVariant& value)
    {
        model->undoStack()->push(new CmdSetData(model, index, value));
        return true;
    }
    static bool apply(BeatsProjectModel* model, const QModelIndex& index, const QVariant& value)
    {
        auto item = (AbstractTreeItem*)index.internalPointer();
        if (!item) {
            qDebug("[Undo/Redo] CmdSetData : Model Index died :(");
            return false;
        }
        if (AbstractTreeItem::NAME == index.column() || AbstractTreeItem::FILE_NAME == index.column()) {
            
            qDebug() <<"check value for comma:" + value.toString();
            QString v = value.toString().replace(",", "_");
            if (0 != v.compare(value.toString())) {
                qWarning() << "BeatsProjectModel::setData - comma found -- changed to _" << value << "to" << v;
            }
            qDebug() << "v:"+v;
            if (!item->setData(index.column(), v)){
                qWarning() << "BeatsProjectModel::setData (name or file_name) - p_Item->setData returned false for object " << item->metaObject()->className() << " with name = " << item->data(AbstractTreeItem::NAME) << endl;
                return false;
            }
        } else {
            if (!item->setData(index.column(), value)){
                qWarning() << "BeatsProjectModel::setData - p_Item->setData returned false for object " << item->metaObject()->className() << " with name = " << item->data(AbstractTreeItem::NAME) << endl;
                return false;
            }
        }
        QVector<int> roles;
        roles.append(Qt::EditRole);
        emit model->dataChanged(index, index, roles);
        return true;
    }

    void redo()
    {
        apply(m_model, m_index(m_model), m_new);
    }
    void undo()
    {
        apply(m_model, m_index(m_model), m_old);
    }
    int id() const { static int id = undo_command_enumerator++; return id; }
    bool mergeWith(const QUndoCommand* other)
    {
        auto next = (CmdSetData*)other;
        if (m_index == next->m_index && m_new == next->m_old) {
            m_new = next->m_new;
            updateText();
            return true;
        }
        return false;
    }
};

class CmdCreateItem : public QUndoCommand
{
    BeatsProjectModel* m_model;
    Index m_parent;
    int m_row, m_cnt;
    Index m_new, m_old;
    bool ok;

    CmdCreateItem(const QString& name, BeatsProjectModel* model, const QModelIndex& parent, int row, int cunt)
        : QUndoCommand()
        , m_model(model)
        , m_parent(parent)
        , m_row(row)
        , m_cnt(cunt)
        , m_new(Index(parent).child(row))
        , m_old(model->selected())
    {
        setText(QObject::tr("Adding %1%2 to \"%3\"(%4)", "Undo Commands")
            .arg(name)
            .arg(m_cnt == 1 ? "" : QString("(x%1)").arg(m_cnt))
            .arg(parent.data().toString())
            .arg(m_row)
            );
    }

public:
    static bool queue(const QString& name, BeatsProjectModel* model, const QModelIndex& parent, int row, int count)
    {
        auto cmd = new CmdCreateItem(name, model, parent, row, count);
        model->undoStack()->push(cmd);
        return cmd->ok;
    }

    void redo()
    {
        ok = m_model->insertRows(m_row, m_cnt, m_parent(m_model));
        auto ix = m_new; while (ix > 3) ix = ix.parent();
        CmdChangeSelection::apply(m_model, ix(m_model));
    }
    void undo()
    {
        if (!ok) return;
        BlockUndoForSelectionChangesFurtherInThisScope _;
        CmdChangeSelection::apply(m_model, (m_old <= m_parent || m_old[m_parent] < m_row ? m_old : m_parent.child(m_row))(m_model));
        m_model->removeRows(m_row, m_cnt, m_parent(m_model));
    }
};

class CmdMoveItem : public QUndoCommand
{
    BeatsProjectModel* m_model;
    QString m_name;
    Index m_index;
    int m_delta;
    bool ok;

    CmdMoveItem(const QString& name, BeatsProjectModel* model, const QModelIndex& index, int delta)
        : QUndoCommand()
        , m_name(name)
        , m_model(model)
        , m_index(index)
        , m_delta(delta)
    {
        updateText();
    }
    void updateText()
    {
        setText(QObject::tr("Moving %1 %2 in \"%3\"", "Undo Commands")
            .arg(m_name)
            .arg(m_delta == 1 ? QObject::tr("down", "Undo Commands") : m_delta == -1 ? QObject::tr("up", "Undo Commands") : m_delta < 0 ? QObject::tr("up(x%1)", "Undo Commands").arg(-m_delta) : QObject::tr("down(x%1)", "Undo Commands").arg(m_delta))
            .arg(m_index(m_model).parent().data().toString())
            );
    }

public:
    static bool queue(const QString& name, BeatsProjectModel* model, const QModelIndex& index, int delta)
    {
        auto cmd = new CmdMoveItem(name, model, index, delta);
        model->undoStack()->push(cmd);
        return cmd->ok;
    }
    static bool apply(BeatsProjectModel* model, const Index& index, int from, int to)
    {
        auto ix = index.parent()(model);
        auto ok = model->moveRow(ix, from, ix, to);
        if (index == 3) {
            ix = ix.child(to, 0);
        } else if (index == 2) {
            ix = ix.child(to, 0);
        }
        CmdChangeSelection::apply(model, ix);
        return ok;
    }

    void redo()
    {
        ok =
            apply(m_model, m_index, m_index[-1], m_index[-1] + m_delta);
    }
    void undo()
    {
        if (ok)
            apply(m_model, m_index, m_index[-1] + m_delta, m_index[-1]);
    }
    int id() const { static int id = undo_command_enumerator++; return id; }
    bool mergeWith(const QUndoCommand* other)
    {
        m_delta += ((CmdMoveItem*)other)->m_delta;
        updateText();
        return true;
    }
};

bool qCopyDirectoryRecursively(const QString& srcFilePath, const QString& tgtFilePath)
{
    QFileInfo srcFileInfo(srcFilePath);
    if (srcFileInfo.isDir()) {
        QDir targetDir(tgtFilePath);
        targetDir.cdUp();
        if (!targetDir.mkdir(QFileInfo(tgtFilePath).fileName()))
            return false;
        QDir sourceDir(srcFilePath);
        QStringList fileNames = sourceDir.entryList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot | QDir::Hidden | QDir::System);
        foreach (const QString& fileName, fileNames) {
            const QString newSrcFilePath = srcFilePath + QLatin1Char('/') + fileName;
            const QString newTgtFilePath = tgtFilePath + QLatin1Char('/') + fileName;
            if (!qCopyDirectoryRecursively(newSrcFilePath, newTgtFilePath))
                return false;
        }
    } else {
        if (!QFile::copy(srcFilePath, tgtFilePath))
            return false;
    }
    return true;
}

class CmdRemoveFolder : public QUndoCommand
{
    BeatsProjectModel* m_model;
    Index m_parent;
    int m_row;
    QString m_name;
    QString m_datafile;

    CmdRemoveFolder(BeatsProjectModel* model, const QModelIndex& parent, int row, const QString& datafile)
        : QUndoCommand()
        , m_model(model)
        , m_parent(parent)
        , m_row(row)
        , m_name(model->index(row, AbstractTreeItem::NAME, parent).data().toString())
        , m_datafile(datafile)
    {
        setText(QObject::tr("Deleting \"%1\" from \"%2\"", "Undo Commands")
            .arg(parent.child(m_row, 0).data().toString())
            .arg(parent.data().toString())
            );
    }

public:
    static void queue(BeatsProjectModel* model, const QModelIndex& parent, int row)
    {
        auto ix = parent.child(row, AbstractTreeItem::SAVE);
        if (ix.data() == 1) {
            // About to be removed folder is changed
            model->setData(ix, 0); // Save the folder
        }
        auto foldername = parent.child(row, AbstractTreeItem::ABSOLUTE_PATH).data().toString();
        QDir folder(foldername);
        QString backup;
        if (folder.exists()) {
            // backup song for undo history
            backup = model->dirUndoRedo().absoluteFilePath(QVariant(model->undoStack()->count()).toString());
            QDir bkp(backup);
            if (bkp.exists()) {
                bkp.removeRecursively();
            }
            model->dirUndoRedo().mkdir(bkp.dirName());
            
            qCopyDirectoryRecursively(foldername, backup = bkp.absoluteFilePath(folder.dirName()));
        }
        model->undoStack()->push(new CmdRemoveFolder(model, parent, row, backup));
    }

    void redo()
    {
        BlockUndoForSelectionChangesFurtherInThisScope _;
        m_model->removeRows(m_row, 1, m_parent(m_model));
        auto ix = m_model->selected().child(0,0);
        if (ix != QModelIndex()) {
            CmdChangeSelection::apply(m_model, ix);
        }
    }
    void undo()
    {
        auto folder = (ContentFolderTreeItem*)m_parent(m_model).internalPointer();
        folder->createFolderWithData(m_row, m_name, m_datafile, false);
        auto ix = m_parent.child(m_row).child(0)(m_model);
        if (ix == QModelIndex()) {
            ix = m_parent.child(m_row)(m_model);
        }
        CmdChangeSelection::apply(m_model, ix);
    }
};

class CmdRemoveSong : public QUndoCommand
{
    BeatsProjectModel* m_model;
    Index m_parent;
    int m_row;
    QString m_name;
    QString m_datafile;
    Index m_old;

    CmdRemoveSong(BeatsProjectModel* model, const QModelIndex& parent, int row, const QString& datafile)
        : QUndoCommand()
        , m_model(model)
        , m_parent(parent)
        , m_row(row)
        , m_name(model->index(row, AbstractTreeItem::NAME, parent).data().toString())
        , m_datafile(datafile)
        , m_old(model->selected())
    {
        setText(QObject::tr("Deleting \"%1\" from \"%2\"", "Undo Commands")
            .arg(parent.child(m_row, 0).data().toString())
            .arg(parent.data().toString())
            );
    }

public:
    static void queue(BeatsProjectModel* model, const QModelIndex& parent, int row)
    {
        auto ix = parent.child(row, AbstractTreeItem::SAVE);
        if (ix.data() == 1) {
            // About to be removed song is changed
            model->setData(ix, 0); // Save the song
        }
        auto filename = parent.child(row, AbstractTreeItem::ABSOLUTE_PATH).data().toString();
        QFileInfo file(filename);
        QString backup;
        if (file.exists()) {
            // backup song for undo history
            backup = model->dirUndoRedo().absoluteFilePath(QVariant(model->undoStack()->count()).toString());
            QDir bkp(backup);
            if (bkp.exists()) {
                bkp.removeRecursively();
            }
            model->dirUndoRedo().mkdir(bkp.dirName());
            QFile(filename).copy(backup = bkp.absoluteFilePath(file.fileName()));
        }
        model->undoStack()->push(new CmdRemoveSong(model, parent, row, backup));
    }

    void redo()
    {
        auto ix = m_parent(m_model);
        auto ixn = ix.child(m_row+1, 0);
        CmdChangeSelection::apply(m_model, ixn == QModelIndex() && !m_row ? ix : ixn);
        m_model->removeRows(m_row, 1, ix);
    }
    void undo()
    {
        auto folder = (ContentFolderTreeItem*)m_parent(m_model).internalPointer();
        folder->createFileWithData(m_row, m_name, m_datafile);
        CmdChangeSelection::apply(m_model, m_old(m_model));
    }
};

class CmdMoveOrCopySong : public QUndoCommand
{
    BeatsProjectModel* m_model;
    const bool move;
    Index m_old, m_new;
    QString m_name;
    QString m_datafile;
    bool ok;

    CmdMoveOrCopySong(bool move, BeatsProjectModel* model, const QModelIndex& song, const QModelIndex& folder, int row, const QString& datafile)
        : QUndoCommand()
        , move(move)
        , m_model(model)
        , m_old(song)
        , m_new(Index(folder).child(row))
        , m_name(song.data().toString())
        , m_datafile(datafile)
    {
        setText((move ? QObject::tr("Moving \"%1\" to \"%2\"(%3)", "Undo Commands") : QObject::tr("Copying \"%1\" to \"%2\"(%3)", "Undo Commands"))
            .arg(m_name)
            .arg(folder.data().toString())
            .arg(row)
            );
    }

public:
    static void queue(bool move, BeatsProjectModel* model, const QModelIndex& song, const QModelIndex& folder, int row)
    {
        auto ix = song.sibling(song.row(), AbstractTreeItem::SAVE);
        if (ix.data() == 1) {
            // Chosen song is changed
            model->setData(ix, 0); // Save the song
        }
        auto filename = song.sibling(song.row(), AbstractTreeItem::ABSOLUTE_PATH).data().toString();
        QFileInfo file(filename);
        QString backup;
        if (file.exists()) {
            // backup song for undo history
            backup = model->dirUndoRedo().absoluteFilePath(QVariant(model->undoStack()->count()).toString());
            QDir bkp(backup);
            if (bkp.exists()) {
                bkp.removeRecursively();
            }
            model->dirUndoRedo().mkdir(bkp.dirName());
            QFile(filename).copy(backup = bkp.absoluteFilePath(file.fileName()));
        }
        model->undoStack()->push(new CmdMoveOrCopySong(move, model, song, folder, row, backup));
    }
    static bool apply(bool move, bool apply, BeatsProjectModel* model, const Index& from, const Index& to, const QString& name, const QString& datafile)
    {
        auto parent = to.parent()(model);
        CmdChangeSelection::apply(model, parent);
        if (!apply && !move) {
            model->removeRows(from[-1], 1, from.parent()(model));
            return true;
        } else if (static_cast<ContentFolderTreeItem*>(parent.internalPointer())->createFileWithData(to[-1], name, datafile)) {
            CmdChangeSelection::apply(model, parent.child(to[-1], 0));
            if (move) {
                model->removeRows(from[-1], 1, from.parent()(model));
            }
            return true;
        }
        return false;
    }

    void redo()
    {
        ok =
            apply(move, true, m_model, m_old, m_new, m_name, m_datafile);
    }
    void undo()
    {
        if (ok)
            apply(move, false, m_model, m_new, m_old, m_name, m_datafile);
    }
};

class CmdRemoveSongPart : public QUndoCommand
{
    BeatsProjectModel* m_model;
    Index m_parent;
    int m_row;
    QStringList m_datafile; // 0 = Main Loop, 1 = Transition, 2 = Accent Hit, rest = fill

    CmdRemoveSongPart(BeatsProjectModel* model, const QModelIndex& parent, int row, const QStringList& datafile)
        : QUndoCommand()
        , m_model(model)
        , m_parent(parent)
        , m_row(row)
        , m_datafile(datafile)
    {
        setText(QObject::tr("Deleting Song Part #%1 from %2", "Undo Commands")
            .arg(m_row)
            .arg(parent.data().toString())
            );
    }

public:
    static void queue(BeatsProjectModel* model, const QModelIndex& parent, int row)
    {
        auto backup = model->dirUndoRedo().absoluteFilePath(QVariant(model->undoStack()->count()).toString());
        QDir bkp(backup);
        if (bkp.exists()) {
            bkp.removeRecursively();
        }
        model->dirUndoRedo().mkdir(bkp.dirName());
        auto ix = parent.child(row, AbstractTreeItem::SAVE);
        if (ix.data() == 1) {
            // Song with about to be removed part is changed
            model->setData(ix, 0); // Save the song
        }
        auto x = Index(parent).child(row);
        QStringList data;
        if ((ix = x.child(0).child(0)(model)) != QModelIndex()) {
            // Main Loop
            auto exportDirIndex = ix.sibling(ix.row(), AbstractTreeItem::EXPORT_DIR);
            model->setData(exportDirIndex, bkp.absolutePath()); // Export to temp dir
            data << bkp.absoluteFilePath(exportDirIndex.data().toString());
        } else {
            // No Main Loop
            data << "";
        }
        if ((ix = x.child(2).child(0)(model)) != QModelIndex()) {
            // Transition
            auto exportDirIndex = ix.sibling(ix.row(), AbstractTreeItem::EXPORT_DIR);
            model->setData(exportDirIndex, bkp.absolutePath()); // Export to temp dir
            data << bkp.absoluteFilePath(exportDirIndex.data().toString());
        } else {
            // No Transition
            data << "";
        }
        if ((ix = x.child(3).child(0)(model)) != QModelIndex()) {
            // Accent Hit
            auto exportDirIndex = ix.sibling(ix.row(), AbstractTreeItem::EXPORT_DIR);
            model->setData(exportDirIndex, bkp.absolutePath()); // Export to temp dir
            data << bkp.absoluteFilePath(exportDirIndex.data().toString());
        } else {
            // No Accent Hit
            data << "";
        }
        if ((ix = x.child(1)(model)) != QModelIndex()) {
            // Fills
            for (auto f = ix.child(0, 0); f != QModelIndex(); f = f.sibling(f.row()+1, 0)) {
                auto exportDirIndex = f.sibling(f.row(), AbstractTreeItem::EXPORT_DIR);
                model->setData(exportDirIndex, bkp.absolutePath()); // Export to temp dir
                data << bkp.absoluteFilePath(exportDirIndex.data().toString());
            }
        }
        model->undoStack()->push(new CmdRemoveSongPart(model, parent, row, data));
    }

    void redo()
    {
        CmdChangeSelection::apply(m_model, m_parent(m_model));
        m_model->removeRows(m_row, 1, m_parent(m_model));
    }
    void undo()
    {
        QString data;
        auto ix = m_parent(m_model);
        m_model->insertRows(m_row, 1, ix);
        ix = ix.child(m_row, 0);
        if (!(data = m_datafile[0]).isEmpty()) {
            // Main Loop
            auto x = ix.child(0, 0);
            m_model->insertRows(0, 1, x);
            m_model->setData(x.child(0, 0), data);
        }
        if (!(data = m_datafile[1]).isEmpty()) {
            // Transition
            auto x = ix.child(2, 0);
            m_model->insertRows(0, 1, x);
            m_model->setData(x.child(0, 0), data);
        }
        if (!(data = m_datafile[2]).isEmpty()) {
            // Accent Hit
            auto x = ix.child(3, 0);
            m_model->insertRows(0, 1, x);
            m_model->setData(x.child(0, 0), data);
        }
        if (m_datafile.size() > 3) {
            // Fills
            auto x = ix.child(1, 0);
            auto c = m_datafile.size() - 3;
            m_model->insertRows(0, c, x);
            for (int i = 0; i < c; ++i) {
                m_model->setData(x.child(i, 0), m_datafile[3+i]);
            }
        }
        CmdChangeSelection::apply(m_model, m_parent(m_model));
    }
};

static QString songFileName(const QAbstractItemModel* m, const Index& x)
{
    //      x      - TrackArrayItem
    // 3 0 0 0 1 0 - Intro (folder 0 song 1)
    // 3 0 2 0 1 0 - Intro (folder 0 song 2)
    // 3 0 0 1 0 0 - Main Part 1 (folder 0 song 1)
    // 3 0 0 1 1 0 - Fill 0 Part 1 (folder 0 song 1)
    // 3 0 0 1 1 1 - Fill 1 Part 1 (folder 0 song 1)
    // 3 0 0 2 1 0 - Outro (folder 0 song 1)
    // 3 0 1 3 1 0 - Outro (folder 0 song 1)
    // 3 0 1 1 2 0 - Trans Fill Part 1 (folder 0 song 1)
    // 3 0 1 2 2 0 - Trans Fill Part 2 (folder 0 song 1)
    // 3 0 1 1 3 0 - Accent Hit Part 1 (folder 0 song 1)
    if (!x[-3]) {
        return QObject::tr("Intro", "Undo Commands");
    } else if (!x[-2]) {
        return QObject::tr("Main Loop (Part %1)", "Undo Commands").arg(x[-3]);
    } else if (x[-2] == 1) {
        auto pp = x.parent().parent()(m);
        if (pp.sibling(pp.row()+1,0).isValid()) {
            return QObject::tr("Fill %1 (Part %2)", "Undo Commands").arg(x[-1]+1).arg(x[-3]);
        } else {
            return QObject::tr("Outro", "Undo Commands");
        }
    } else if (x[-2] == 2) {
        return QObject::tr("Transition (Part %1)", "Undo Commands").arg(x[-3]);
    } else if (x[-2] == 3) {
        return QObject::tr("Accent Hit (Part %1)", "Undo Commands").arg(x[-3]); // TODO : check "FolderTreeItem::hash - ERROR 2 - Unable to open file"
    }
    return QObject::tr("Unit", "Undo Commands");
}

QString BeatsProjectModel::songFileName(const QModelIndex& songpart) { return ::songFileName(this, songpart); }
QString BeatsProjectModel::songFileName(const QModelIndex& parent, int child) { return ::songFileName(this, Index(parent).child(child)); }

bool BeatsProjectModel::APSettingIsEmpty()
{
    return m_APSettings.isEmpty();
}

QList<int> BeatsProjectModel::getAPSettingInQueue() 
{
    if(!m_APSettings.isEmpty())
        return m_APSettings.takeFirst();
    else
        return QList<int>() << 0 << 0;
}

void BeatsProjectModel::addAPSettingInQueue(QList<int> settings)
{
    m_APSettings.push_back(settings);
}
QList<int> BeatsProjectModel::getAPSettings(QModelIndex index)
{
    auto item = (AbstractTreeItem*) index.internalPointer();
    QVariant settings = item->data(17);
    return QList<int>() << settings.toStringList()[0].toInt() << settings.toStringList()[1].toInt();
}
void BeatsProjectModel::clearAPSettingQueue()
{
    m_APSettings.clear();
}

class CmdCreateSongFile : public QUndoCommand
{
    BeatsProjectModel* m_model;
    Index m_parent;
    int m_row;
    QString m_origfile, m_datafile;
    bool ok;
    int m_NewPlayAt;
    int m_NewPlayFor;

    CmdCreateSongFile(BeatsProjectModel* model, const QModelIndex& parent, int row, const QString& datafile, const QString& origfile)
        : QUndoCommand()
        , m_model(model)
        , m_parent(parent)
        , m_row(row)
        , m_origfile(origfile)
        , m_datafile(datafile)
        , ok(false)
        , m_NewPlayAt(0)
        , m_NewPlayFor(0)
    {
        QList<int> settings = QList<int>() << 0 << 0;
        if(!model->APSettingIsEmpty()){
            settings = model->getAPSettingInQueue();
        }

        if(settings.size() >= 2){
            m_NewPlayAt = settings.at(0);
            m_NewPlayFor = settings.at(1);
        }

        setText(QObject::tr("Adding %1 to %2", "Undo Commands")
            .arg(songFileName(parent.model(), m_parent.child(m_row)))
            .arg(parent.parent().parent().data().toString())
            );
    }

public:
    static CmdCreateSongFile& queue(BeatsProjectModel* model, const QModelIndex& parent, int row, const QString& data)
    {
        auto backup = model->dirUndoRedo().absoluteFilePath(QVariant(model->undoStack()->count()).toString());
        QDir bkp(backup);
        if (bkp.exists()) {
            bkp.removeRecursively();
        }
        model->dirUndoRedo().mkdir(bkp.dirName());
        QFile(data).copy(backup = bkp.absoluteFilePath(QFileInfo(data).fileName()));
        auto ret = new CmdCreateSongFile(model, parent, row, backup, data);
        try {
          model->undoStack()->push(ret);
        } catch (...) {
            qDebug() << "There was a problem with the undo stack";
        }

        return *ret;
    }

    operator bool() const { return ok; }
    void redo()
    {
        auto ix = m_parent(m_model);
        if (!m_model->insertRow(m_row, ix)) {
            qWarning() << "CmdCreateSongFile::redo() - Unable to add row";
            return;
        }
        if (ok = m_model->setData(ix.child(m_row, AbstractTreeItem::NAME), m_datafile)) {
            if (!m_origfile.endsWith("." BMFILES_SONG_TRACK_EXTENSION, Qt::CaseInsensitive)) {
                m_model->setData(ix.child(m_row, AbstractTreeItem::ABSOLUTE_PATH), m_origfile);

            }
            m_model->setData(ix.child(m_row, AbstractTreeItem::PLAY_AT_FOR), QList<QVariant>() << m_NewPlayAt << m_NewPlayFor);
        } else {
            QStringList errorList = ix.child(m_row, AbstractTreeItem::ERROR_MSG).data().toStringList();
            if (!errorList.isEmpty()) {
                QString errorLog;
                for (int k = 0; k < errorList.count(); k++) {
                    errorLog += QString("%1 - %2\n").arg(k+1).arg(errorList.at(k));
                }
                QMessageBox::critical(nullptr, QObject::tr("Adding file"), QObject::tr("Error while parsing midi file %1\nThe error log is:\n\n%2").arg(m_origfile).arg(errorLog));
                m_model->removeRow(m_row, ix);
            }
        }
        CmdChangeSelection::apply(m_model, m_parent.parent().parent()(m_model));
    }
    void undo()
    {
        if (ok) {
            m_model->removeRows(m_row, 1, m_parent(m_model));
        }
        CmdChangeSelection::apply(m_model, m_parent.parent().parent()(m_model));
    }
};

class CmdReplaceSongFile : public QUndoCommand
{
    BeatsProjectModel* m_model;
    Index m_index;
    QString m_olddatafile, m_newdatafile, m_origfile;
    int m_OldPlayAt, m_OldPlayFor, m_NewPlayAt, m_NewPlayFor;

    CmdReplaceSongFile(BeatsProjectModel* model, const QModelIndex& index, const QString& olddatafile, const QString& newdatafile, const QString& origfile)
        : QUndoCommand()
        , m_model(model)
        , m_index(index)
        , m_olddatafile(olddatafile)
        , m_newdatafile(newdatafile)
        , m_origfile(origfile)
        , m_OldPlayAt(0)
        , m_OldPlayFor(0)
        , m_NewPlayAt(0)
        , m_NewPlayFor(0)
    {
        QList<QVariant> settings = m_model->data(index.sibling(index.row(), AbstractTreeItem::PLAY_AT_FOR)).toList();
        if(settings.size() >= 2)
        {
            m_OldPlayAt = settings.at(0).toInt();
            m_OldPlayFor = settings.at(1).toInt();
        }

        QList<int> newSettings = model->getAPSettingInQueue();
        if(newSettings.size() >= 2)
        {
            m_NewPlayAt = newSettings.at(0);
            m_NewPlayFor = newSettings.at(1);
        }

        setText(QObject::tr("Changing %1 in %2", "Undo Commands")
            .arg(songFileName(index.model(), m_index))
            .arg(index.parent().parent().parent().data().toString())
            );
    }

public:
    static void queue(BeatsProjectModel* model, const QModelIndex& index, const QString& data)
    {
        auto backup = model->dirUndoRedo().absoluteFilePath(QVariant(model->undoStack()->count()).toString());
        QDir bkp(backup);
        if (bkp.exists()) {
            bkp.removeRecursively();
        }
        model->dirUndoRedo().mkdir(bkp.dirName());
        QFile(data).copy(backup = bkp.absoluteFilePath(QFileInfo(data).fileName()));
        auto exportDirIndex = index.sibling(index.row(), AbstractTreeItem::EXPORT_DIR);
        AbstractTreeItem* item = static_cast<AbstractTreeItem*>(index.internalPointer());
        model->setData(exportDirIndex, bkp.absolutePath()); // Export to temp dir
        model->undoStack()->push(new CmdReplaceSongFile(model, index, bkp.absoluteFilePath(exportDirIndex.data().toString()), backup, data));
    }

    void redo()
    {
        auto ix = m_index(m_model);
        m_model->setData(ix, m_newdatafile);
        if (!m_origfile.endsWith("." BMFILES_SONG_TRACK_EXTENSION, Qt::CaseInsensitive)) {
            m_model->setData(ix.sibling(ix.row(), AbstractTreeItem::ABSOLUTE_PATH), m_origfile);
        }
        m_model->setData(ix.sibling(ix.row(), AbstractTreeItem::PLAY_AT_FOR),    QList<QVariant>() << m_NewPlayAt << m_NewPlayFor);
        CmdChangeSelection::apply(m_model, m_index.parent().parent().parent()(m_model));
    }
    void undo()
    {
        auto ix = m_index(m_model);
        m_model->setData(m_index(m_model), m_olddatafile);
        m_model->setData(ix.sibling(ix.row(), AbstractTreeItem::PLAY_AT_FOR),    QList<QVariant>() << m_OldPlayAt << m_OldPlayFor);
        CmdChangeSelection::apply(m_model, m_index.parent().parent().parent()(m_model));
    }
};

class CmdEditSongFile : public QUndoCommand
{
    BeatsProjectModel* m_model;
    Index m_index;
    QByteArray m_olddata, m_newdata;

    CmdEditSongFile(BeatsProjectModel* model, const QModelIndex& index, const QByteArray& olddata, const QByteArray& newdata)
        : QUndoCommand()
        , m_model(model)
        , m_index(index)
        , m_olddata(olddata)
        , m_newdata(newdata)
    {
        setText(QObject::tr("Editing %1 in %2", "Undo Commands")
            .arg(songFileName(index.model(), m_index))
            .arg(index.parent().parent().parent().data().toString())
            );
    }

public:
    static void queue(BeatsProjectModel* model, const QModelIndex& index, const QByteArray& data)
    {
        model->undoStack()->push(new CmdEditSongFile(model, index, index.data().toByteArray(), data));
    }

    void redo()
    {
        m_model->setData(m_index(m_model), m_newdata);
        CmdChangeSelection::apply(m_model, m_index.parent().parent().parent()(m_model));
    }
    void undo()
    {
        m_model->setData(m_index(m_model), m_olddata);
        CmdChangeSelection::apply(m_model, m_index.parent().parent().parent()(m_model));
    }
};

class CmdRemoveSongFile : public QUndoCommand
{
    BeatsProjectModel* m_model;
    Index m_parent;
    int m_row;
    // song file model doesn't allow reordering... Export ALL the drum fills starting from the necessary one
    QStringList m_datafile;

    CmdRemoveSongFile(BeatsProjectModel* model, const QModelIndex& parent, int row, const QStringList& datafile)
        : QUndoCommand()
        , m_model(model)
        , m_parent(parent)
        , m_row(row)
        , m_datafile(datafile)
    {
        setText(QObject::tr("Deleting %1 from %2", "Undo Commands")
            .arg(songFileName(parent.model(), m_parent.child(m_row)))
            .arg(parent.parent().parent().data().toString())
            );
    }

public:
    static void queue(BeatsProjectModel* model, const QModelIndex& parent, int row)
    {
        QStringList data;
        auto backup = model->dirUndoRedo().absoluteFilePath(QVariant(model->undoStack()->count()).toString());
        QDir bkp(backup);
        if (bkp.exists()) {
            bkp.removeRecursively();
        }
        model->dirUndoRedo().mkdir(bkp.dirName());
        for (auto ix = parent.child(row, 0); ix != QModelIndex(); ix = ix.sibling(ix.row()+1, 0)) {
            auto exportDirIndex = ix.sibling(ix.row(), AbstractTreeItem::EXPORT_DIR);
            model->setData(exportDirIndex, bkp.absolutePath()); // Export to temp dir
            data << bkp.absoluteFilePath(exportDirIndex.data().toString());
        }
        model->undoStack()->push(new CmdRemoveSongFile(model, parent, row, data));
    }

    void redo()
    {
        m_model->removeRows(m_row, 1, m_parent(m_model));
        CmdChangeSelection::apply(m_model, m_parent.parent().parent()(m_model));
    }
    void undo()
    {
        auto ix = m_parent(m_model);
        if (m_datafile.size() != 1) {
            // removing all subsequent fills
            auto pc = m_model->rowCount(ix);
            m_model->removeRows(m_row, pc-m_row, ix);
        }
        m_model->insertRows(m_row, m_datafile.size(), ix);
        for (int i = 0; i < m_datafile.size(); ++i) {
            m_model->setData(ix.child(m_row+i, AbstractTreeItem::NAME), m_datafile[i]);
        }
        CmdChangeSelection::apply(m_model, m_parent.parent().parent()(m_model));
    }
};

class CmdImportSongsAndFolders : public QUndoCommand
{
    BeatsProjectModel* m_model;
    QWidget* m_widget;
    Index m_pos;
    QStringList m_songs, m_folders;
    int m_imported_folders, m_imported_songs, m_imported_pos;

    CmdImportSongsAndFolders(BeatsProjectModel* model, QWidget* parent, const QModelIndex& pos, QStringList& songs, QStringList& folders)
        : QUndoCommand()
        , m_model(model)
        , m_widget(parent)
        , m_pos(pos)
        , m_imported_folders(0)
        , m_imported_songs(0)
    {
        auto s = songs.count(), f = folders.count();
        m_songs.swap(songs); m_folders.swap(folders);
        setText((s && f ? QObject::tr("Importing %1 song(s) and %2 folder(s)", "Undo Commands") : s ? QObject::tr("Importing %1 song(s)", "Undo Commands") : QObject::tr("Importing %2 folder(s)", "Undo Commands"))
            .arg(s)
            .arg(f)
            );
    }

public:
    static void queue(BeatsProjectModel* model, QWidget* parent, const QModelIndex& pos, QStringList songs, QStringList folders)
    {
        auto backup = model->dirUndoRedo().absoluteFilePath(QVariant(model->undoStack()->count()).toString());
        QDir bkp(backup);
        if (bkp.exists()) {
            bkp.removeRecursively();
        }
        model->dirUndoRedo().mkdir(bkp.dirName());
        for (int i = 0; i < songs.count(); ++i) {
            auto copy = bkp.absoluteFilePath(QFileInfo(songs[i]).fileName());
            QFile::copy(songs[i], copy);
            songs[i] = copy;
        }
        for (int i = 0; i < folders.count(); ++i) {
            auto copy = bkp.absoluteFilePath(QFileInfo(folders[i]).fileName());
            QFile::copy(folders[i], copy);
            folders[i] = copy;
        }
        model->undoStack()->push(new CmdImportSongsAndFolders(model, parent, pos, songs, folders));
    }

    void redo()
    {
        auto cur = (AbstractTreeItem*)m_model->rootItem();
        for (int i = 0; i < m_pos; ++i)
            cur = cur->child(m_pos[i]);
        auto song = qobject_cast<SongFileItem*>(cur);
        auto folder = qobject_cast<SongFolderTreeItem*>(song ? song->parent() : cur);
        m_imported_pos = -!!song;
        if (folder && m_songs.count()) {
            m_imported_songs = folder->childCount();
            folder->importSongsModal(m_widget, m_songs, song ? m_pos[-1] : m_imported_pos = m_imported_songs);
            m_imported_songs = folder->childCount() - m_imported_songs;
        }
        auto songs = m_model->songsFolder();
        int folderInsertionRow = song ? m_pos[-2] : folder ? m_pos[-1] : songs->childCount();
        m_imported_folders = songs->childCount();
        songs->importFoldersModal(m_widget, m_folders, folderInsertionRow);
        m_imported_folders = songs->childCount() - m_imported_folders;
        CmdChangeSelection::apply(m_model, (m_imported_folders ? song ? m_pos.parent() : m_pos : song ? m_pos : m_pos.child(m_imported_pos))(m_model));
    }
    void undo()
    {
        BlockUndoForSelectionChangesFurtherInThisScope _;
        if (m_imported_folders) {
            auto pos = m_imported_pos+1 ? m_pos : m_pos.parent();
            m_model->removeRows(pos[-1], m_imported_folders, pos.parent()(m_model));
        }
        if (m_imported_songs) {
            auto pos = m_imported_pos+1 ? m_pos.child(m_imported_pos) : m_pos;
            m_model->removeRows(pos[-1], m_imported_songs, pos.parent()(m_model));
        }
        CmdChangeSelection::apply(m_model, m_pos(m_model));
    }
};

class CmdChangeAPSettings : public QUndoCommand
{
    BeatsProjectModel* m_model;
    Index m_parent;
    int m_row;
    // song file model doesn't allow reordering... Export ALL the drum fills starting from the necessary one
    QStringList m_datafile;

    CmdChangeAPSettings(BeatsProjectModel* model, const QModelIndex& parent, int row, const QStringList& datafile)
        : QUndoCommand()
        , m_model(model)
        , m_parent(parent)
        , m_row(row)
        , m_datafile(datafile)
    {
        setText(QObject::tr("Changing AP Settings %1 from %2", "Undo Commands")
            .arg(songFileName(parent.model(), m_parent.child(m_row)))
            .arg(parent.parent().parent().data().toString())
            );
    }

public:
    static void queue(BeatsProjectModel* model, const QModelIndex& parent, int row)
    {
        QStringList data;
        auto backup = model->dirUndoRedo().absoluteFilePath(QVariant(model->undoStack()->count()).toString());
        QDir bkp(backup);
        if (bkp.exists()) {
            bkp.removeRecursively();
        }
        model->dirUndoRedo().mkdir(bkp.dirName());
        for (auto ix = parent.child(row, 0); ix != QModelIndex(); ix = ix.sibling(ix.row()+1, 0)) {
            auto exportDirIndex = ix.sibling(ix.row(), AbstractTreeItem::EXPORT_DIR);
            model->setData(exportDirIndex, bkp.absolutePath()); // Export to temp dir
            data << bkp.absoluteFilePath(exportDirIndex.data().toString());
        }
        model->undoStack()->push(new CmdChangeAPSettings(model, parent, row, data));
    }

    void redo()
    {
        m_model->removeRows(m_row, 1, m_parent(m_model));
        CmdChangeSelection::apply(m_model, m_parent.parent().parent()(m_model));
    }
    void undo()
    {
        auto ix = m_parent(m_model);
        if (m_datafile.size() != 1) {
            // removing all subsequent fills
            auto pc = m_model->rowCount(ix);
            m_model->removeRows(m_row, pc-m_row, ix);
        }
        m_model->insertRows(m_row, m_datafile.size(), ix);
        for (int i = 0; i < m_datafile.size(); ++i) {
            m_model->setData(ix.child(m_row+i, AbstractTreeItem::NAME), m_datafile[i]);
        }
        CmdChangeSelection::apply(m_model, m_parent.parent().parent()(m_model));
    }
};

BeatsProjectModel::Macro::Macro(BeatsProjectModel* self, const QString& name)
    : self(self)
{
    self->undoStack()->beginMacro(name);
}

BeatsProjectModel::Macro::Macro(const Macro& other)
    : self(other.self)
{
    other.self = nullptr;
}

BeatsProjectModel::Macro::~Macro()
{
    if (!self) {
        return;
    }
    auto stack = self->undoStack();
    stack->endMacro();
    
    if (!stack->command(stack->count()-1)->childCount())
        stack->undo();
}

/**
 * @brief BeatsProjectModel::BeatsProjectModel
 *       Creates a new master beatbuddy project model.
 *       The model is mainly construct by 4 tree model:
 *       #1 : Effect Tree
 *       #2 : Param Tree
 *       #3 : Songs Tree
 *       #4 : Drumsets Tree
 * @param projectPath The path to the project on the disk
 * @param parent
 */


BeatsProjectModel::BeatsProjectModel(const QString &projectFilePath, QWidget *parent, const QString &tmpDirPath) :
   QAbstractItemModel(parent)
{
    m_projectFileFI = QFileInfo(projectFilePath);
    m_projectDirFI =  QFileInfo(m_projectFileFI.absolutePath());

    if (!m_projectFileFI.exists()){
        saveProjectFile(m_projectFileFI.absoluteFilePath());
    }

    for (int i = 0; i < AbstractTreeItem::ENUM_SIZE; ++i) {
        mp_Header << AbstractTreeItem::columnName(i);
    }
    mp_Header << tr("Main Drum Loop", "Table Header") << tr("Drum Fill", "Table Header") << tr("Transition Fill", "Table Header") << tr("Accent Hit", "Table Header") << ""; // Five last column titles are used to display column names in View

   mp_RootItem = new FolderTreeItem(this);

   QProgressDialog *pFolderProgress = new QProgressDialog(tr("Parsing project files..."), tr("Hidden"), 0, 1, parent, nullptr);
   pFolderProgress->setCancelButton(nullptr);
   pFolderProgress->setWindowModality(Qt::WindowModal);
   pFolderProgress->setMinimumDuration(0);

   // Create base directory structure
   mp_RootItem->setName(m_projectFileFI.baseName());
   mp_RootItem->setFileName(m_projectFileFI.fileName());
   mp_RootItem->setChildrenTypes(nullptr);

   DrmFolderTreeItem *p_DrumSetsFolder = new DrmFolderTreeItem(this, mp_RootItem);
   mp_RootItem->appendChild(p_DrumSetsFolder);
   p_DrumSetsFolder->attachProgress(pFolderProgress);

   EffectFolderTreeItem *p_EffectsFolder = new EffectFolderTreeItem(this, mp_RootItem);
   mp_RootItem->appendChild(p_EffectsFolder);
   p_EffectsFolder->attachProgress(pFolderProgress);

   ParamsFolderTreeModel *p_ParamsFolder = new ParamsFolderTreeModel(this, mp_RootItem);
   mp_RootItem->appendChild(p_ParamsFolder);

   SongsFolderTreeItem *p_SongsFolder = new SongsFolderTreeItem(this, mp_RootItem);
   mp_RootItem->appendChild(p_SongsFolder);
   p_SongsFolder->attachProgress(pFolderProgress);

   // Create project skeleton if it does not exist
   m_projectDirty = false;
   m_songsFolderDirty = false;

   if(mp_RootItem->createProjectSkeleton()){
      m_projectDirty = true;
   }
   p_DrumSetsFolder->updateModelWithData(false);
   p_EffectsFolder->updateModelWithData(false);
   p_SongsFolder->updateModelWithData(false);

   p_DrumSetsFolder->detachProgress();
   p_EffectsFolder->detachProgress();
   p_SongsFolder->detachProgress();

   delete pFolderProgress;

   qDebug() << "looking for update" << p_SongsFolder->childCount();
   // There is at least one song folder
   if(p_SongsFolder->childCount() <= 0) {
      SongFolderTreeItem *p_NewSongsFolder = new SongFolderTreeItem(this, p_SongsFolder);
      p_NewSongsFolder->setName(tr("New Folder"));
      p_SongsFolder->appendChild(p_NewSongsFolder);
      p_SongsFolder->updateDataWithModel(false);
      m_projectDirty = true;
   }

   // refresh hash and save project
   if(m_projectDirty) {
      mp_RootItem->computeHash(true);
      saveModal();
      m_projectDirty = false;
   }

   // Create default song Folder index
   m_DefaultSongFolderIndex = QPersistentModelIndex(createIndex(0, 0, p_SongsFolder->child(0)));

   m_editingDisabled = false;
   m_selectionDisabled = false;

    m_tempDir = tmpDirPath;
    m_tempDir.mkpath("clipboard");
    m_copyClipboardDir = m_tempDir;
    m_copyClipboardDir.cd("clipboard");
    m_copyClipboardDir.mkpath("paste");
    m_copyClipboardDir.mkpath("drag");
    m_copyClipboardDir.mkpath("drop");
    m_copyClipboardDir.mkpath("copy");
    m_pasteClipboardDir = m_dragClipboardDir = m_dropClipboardDir = m_copyClipboardDir;
    m_copyClipboardDir.cd("copy");
    m_pasteClipboardDir.cd("paste");
    m_dragClipboardDir.cd("drag");
    m_dropClipboardDir.cd("drop");
    m_tempDir.mkpath("history");
    m_dirUndoRedo = m_tempDir;
    m_dirUndoRedo.cd("history");

    m_stack = new QUndoStack(this);
}

BeatsProjectModel::~BeatsProjectModel()
{
   delete mp_RootItem;
   // Don't clean up Temp dir systematically (in case we want to recover)

   // cleanup temp dir
   m_tempDir.removeRecursively();
}

QModelIndex BeatsProjectModel::defaultSongFolderIndex() const
{
   return index(m_DefaultSongFolderIndex.row(), m_DefaultSongFolderIndex.column(), m_DefaultSongFolderIndex.parent());
}

int BeatsProjectModel::columnCount(const QModelIndex &parent) const
{
    return parent.isValid() ? AbstractTreeItem::ENUM_SIZE : mp_Header.size();
}

QVariant BeatsProjectModel::data(const QModelIndex &index, int role) const
{
   if (!index.isValid() || role != Qt::DisplayRole && role != Qt::EditRole)
      return QVariant();
   auto item = (AbstractTreeItem*)index.internalPointer();
   return item->data(index.column());
}

Qt::ItemFlags BeatsProjectModel::flags(const QModelIndex &index) const
{
   if (!index.isValid())
      return Qt::ItemIsDropEnabled;

  AbstractTreeItem *item = static_cast<AbstractTreeItem*>(index.internalPointer());

  Qt::ItemFlags tempFlags = item->flags(index.column());

  return tempFlags | Qt::ItemIsDropEnabled | Qt::ItemIsDragEnabled;
}

QVariant BeatsProjectModel::headerData(int section, Qt::Orientation orientation,
                               int role) const
{
   if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
      return mp_Header.at(section);

   return QVariant();
}

QModelIndex BeatsProjectModel::index(int row, int column, const QModelIndex &parent)
const
{
   if (!hasIndex(row, column, parent))
      return QModelIndex();

   AbstractTreeItem *p_parentItem;

   if (!parent.isValid())
      p_parentItem = mp_RootItem;
   else
      p_parentItem = static_cast<AbstractTreeItem*>(parent.internalPointer());

   AbstractTreeItem *p_childItem = p_parentItem->child(row);
   if (p_childItem)
      return createIndex(row, column, p_childItem);
   else
      return QModelIndex();
}

QModelIndex BeatsProjectModel::parent(const QModelIndex &index) const
{
   if (!index.isValid()){
      return QModelIndex();
   }

   AbstractTreeItem *p_childItem = static_cast<AbstractTreeItem*>(index.internalPointer());
   AbstractTreeItem *p_parentItem = p_childItem->parent();

   if (p_parentItem == mp_RootItem){
      return QModelIndex();
   }

   return createIndex(p_parentItem->row(), 0, p_parentItem);
}

int BeatsProjectModel::rowCount(const QModelIndex &parent) const
{
   AbstractTreeItem *p_parentItem;
   if (parent.column() > 0)
      return 0;

   if (!parent.isValid())
      p_parentItem = mp_RootItem;
   else
      p_parentItem = static_cast<AbstractTreeItem*>(parent.internalPointer());

   return p_parentItem->childCount();
}

bool BeatsProjectModel::insertRows(int row, int count, const QModelIndex& parent)
{
    int lastRow = row + count - 1;
    AbstractTreeItem* p_parentItem = !parent.isValid() ? mp_RootItem : (AbstractTreeItem*)parent.internalPointer();
    if (row > p_parentItem->childCount()) {
        qWarning() << "BeatsProjectModel::insertRows - ERROR 1 - row > p_parentItem->childCount()";
        return false;
    }
    beginInsertRows(parent, row, lastRow);
    for (int i = row; i <= lastRow; i++) {
        p_parentItem->insertNewChildAt(i);
    }
    endInsertRows();
    return true;
}

bool BeatsProjectModel::removeRows(int row, int count, const QModelIndex& parent)
{
   int lastRow = row + count - 1;
   for(int i = lastRow; i > row; i--){
      if(!index(i, 0, parent).isValid()){
         qWarning() << "BeatsProjectModel::removeRows: cannot remove row = " << i << endl;
         return false;
      }
   }
   AbstractTreeItem* p_parentItem = !parent.isValid() ? mp_RootItem : (AbstractTreeItem*)parent.internalPointer();
   beginRemoveRows(parent, row, lastRow);
   for(int i = lastRow; i >= row; i--){
      p_parentItem->removeChild(i);
   }
   endRemoveRows();
   return true;
}

void BeatsProjectModel::insertItem(AbstractTreeItem * item, int row)
{
   AbstractTreeItem *p_parentItem = item->parent();

   QModelIndex parent;
   if(p_parentItem != mp_RootItem){
      AbstractTreeItem *p_grandParentItem = p_parentItem->parent();

      int parentRow = -1;
      for(int i = 0; i < p_grandParentItem->childCount(); i++){
         // Compare pointers
         if(p_grandParentItem->child(i) == p_parentItem){
            parentRow = i;
            break;
         }
      }

      if(parentRow < 0){
         qWarning() << "BeatsProjectModel::insertItem - ERROR - parent not part of grandparent";
         return;
      }
      parent = createIndex(parentRow, 0, p_parentItem);
   }

   beginInsertRows(parent, row, row);
   p_parentItem->insertChildAt(row, item);
   endInsertRows();
}

bool BeatsProjectModel::setData(const QModelIndex & index, const QVariant & value, int role)
{
    if (!index.isValid()) {
        qWarning() << "BeatsProjectModel::setData - Index invalid - TODO : determine what to do if it is root" << endl;
        return false;
    }
    if (role != Qt::EditRole) {
        return false;
    }
    if (index.data() == value) {
        qDebug() << "BeatsProjectModel::setData - DEBUG - Nothing to do when data is equal";
        return false;
    }

    auto queue = index.column() != AbstractTreeItem::SAVE && index.column() != AbstractTreeItem::PLAYING
        && (index.parent() == songsFolderIndex() || index.parent().parent() == songsFolderIndex()
            || index.column() == AbstractTreeItem::PLAY_AT_FOR);

    if (value.typeName()=="QString") {
        qDebug() <<"check value for comma:" + value.toString();
        QString v = value.toString().replace(",", "_");
        qDebug() << "v:"+v;
        return (queue ? CmdSetData::queue : CmdSetData::apply)(this, index, v);
    }

    return (queue ? CmdSetData::queue : CmdSetData::apply)(this, index, value);
}

void BeatsProjectModel::itemDataChanged(AbstractTreeItem * item, int column)
{
   itemDataChanged(item, column, column);
}

void BeatsProjectModel::itemDataChanged(AbstractTreeItem * item, int leftColumn, int rightColumn)
{
   AbstractTreeItem *p_Parent = item->parent();

   if (!p_Parent){
      if(leftColumn != rightColumn && leftColumn != AbstractTreeItem::HASH){
         // It is ok to set hash on root
         qWarning() << "BeatsProjectModel::itemDataChanged - ERROR - Trying to change data on root";
      }
      return;
   }

   int row = -1;
   for(int i = 0; i < p_Parent->childCount(); i++){
      // Compare pointers
      if(p_Parent->child(i) == item){
         row = i;
         break;
      }
   }

   if(row < 0){
      qWarning() << "BeatsProjectModel::itemDataChanged - ERROR - child not part of parent";
      return;
   }

   QModelIndex leftIndex = createIndex(row, leftColumn, item);
   QModelIndex rightIndex = createIndex(row, rightColumn, item);
   emit dataChanged(leftIndex, rightIndex);
}


void BeatsProjectModel::removeItem(AbstractTreeItem * item, int row)
{
   AbstractTreeItem *p_parentItem = item->parent();

   QModelIndex parent;
   if(p_parentItem != mp_RootItem){
      AbstractTreeItem *p_grandParentItem = p_parentItem->parent();

      int parentRow = -1;
      for(int i = 0; i < p_grandParentItem->childCount(); i++){
         // Compare pointers
         if(p_grandParentItem->child(i) == p_parentItem){
            parentRow = i;
            break;
         }
      }

      if(parentRow < 0){
         qWarning() << "BeatsProjectModel::removeItem - ERROR - parent not part of grandparent";
         return;
      }
      parent = createIndex(parentRow, 0, p_parentItem);
   }

   beginRemoveRows(parent, row, row);
   p_parentItem->removeChild(row);
   endRemoveRows();
}

bool BeatsProjectModel::moveRows(const QModelIndex & sourceParent, int sourceRow, int count, const QModelIndex & destinationParent, int destinationChild)
{
   if(sourceParent != destinationParent){
      qWarning() << "BeatsProjectModel::moveRows : it is not allowed to move child to different parent";
      return false;
   }

   if(sourceRow == destinationChild){
      qWarning() << "BeatsProjectModel::moveRows : sourceRow == destinationChild : no change required";
      return true;
   }

   if(sourceRow + count > rowCount(sourceParent)){
      qWarning() << "BeatsProjectModel::moveRows : sourceRow + count > rowCount(sourceParent)";
      return false;
   }

   if(destinationChild + count > rowCount(destinationParent)){
      qWarning() << "BeatsProjectModel::moveRows : destinationChild + count > rowCount(destinationParent)";
      return false;
   }
 
   AbstractTreeItem* p_parentItem;

   if (!sourceParent.isValid()){
      p_parentItem = mp_RootItem;
   } else {
      p_parentItem = static_cast<AbstractTreeItem*>(sourceParent.internalPointer());
   }

   int sourceFirst = sourceRow;
   int sourceLast = sourceRow + count - 1;

   int beginMoveRowsDest = destinationChild;

   if(destinationChild > sourceRow){
      beginMoveRowsDest += count;
   }

   if(!beginMoveRows(sourceParent, sourceFirst, sourceLast, destinationParent, beginMoveRowsDest)){
      qWarning() << "beginMoveRows returned false";
      return false;
   }

   p_parentItem->moveChildren(sourceFirst, sourceLast, destinationChild - sourceFirst);

   endMoveRows();
   return true;
}

void BeatsProjectModel::moveItem(AbstractTreeItem * item, int sourceRow, int destRow)
{
   AbstractTreeItem *p_parentItem = item->parent();

   QModelIndex parent;
   if(p_parentItem != mp_RootItem){
      AbstractTreeItem *p_grandParentItem = p_parentItem->parent();

      int parentRow = -1;
      for(int i = 0; i < p_grandParentItem->childCount(); i++){
         // Compare pointers
         if(p_grandParentItem->child(i) == p_parentItem){
            parentRow = i;
            break;
         }
      }

      if(parentRow < 0){
         qWarning() << "BeatsProjectModel::moveItem - ERROR - parent not part of grandparent";
         return;
      }
      parent = createIndex(parentRow, 0, p_parentItem);
   }

   int beginMoveRowsDest = destRow;

   if(destRow > sourceRow){
      destRow ++;
   }

   if(!beginMoveRows(parent, sourceRow, sourceRow, parent, beginMoveRowsDest)){
      qWarning() << "BeatsProjectModel::moveItem - ERROR - beginMoveRows returned false";
      return;
   }

   p_parentItem->moveChildren(sourceRow, sourceRow, destRow - sourceRow);

   endMoveRows();
}

void BeatsProjectModel::selectionChanged(const QModelIndex& current, const QModelIndex& previous)
{
    m_selectedItem = current;
    CmdChangeSelection::queue(this, previous, current);
}

EffectFolderTreeItem *BeatsProjectModel::effectFolder() const
{

   return static_cast<EffectFolderTreeItem *>(mp_RootItem->child(1));
}
DrmFolderTreeItem *BeatsProjectModel::drmFolder() const
{

   return static_cast<DrmFolderTreeItem *>(mp_RootItem->child(0));
}
SongsFolderTreeItem *BeatsProjectModel::songsFolder() const
{

   return static_cast<SongsFolderTreeItem *>(mp_RootItem->child(3));
}
ParamsFolderTreeModel *BeatsProjectModel::paramsFolder() const
{

   return static_cast<ParamsFolderTreeModel *>(mp_RootItem->child(2));
}

QModelIndex BeatsProjectModel::effectFolderIndex() const
{
   
   return index(1,0);
}
QModelIndex BeatsProjectModel::drmFolderIndex() const
{
   
   return index(0,0);
}
QModelIndex BeatsProjectModel::songsFolderIndex() const
{
   
   return index(3,0);
}
QModelIndex BeatsProjectModel::paramsFolderIndex() const
{
   
   return index(2,0);
}


QModelIndex BeatsProjectModel::partIndexInternal(const QModelIndex &songIndex, int partNumber)
{
   if(!songIndex.isValid()){
      qWarning() << "BeatsProjectModel::partIndexInternal -  ERROR 4 - if(!songIndex.isValid())";
      return songIndex;
   }

   // 5 - Final validation and create part index
   if(partNumber >= rowCount(songIndex)){
      qWarning() << "BeatsProjectModel::partIndexInternal -  ERROR 5 - if(partNumber >= rowCount(songIndex))";
      return QModelIndex();
   }

   // Replace any negative value with outro
   if(partNumber < 0){
      partNumber = rowCount(songIndex) - 1;
   }

   return index(partNumber, 0, songIndex);

}

void BeatsProjectModel::saveProjectFile(const QString& filePath)
{
    QFileInfo info(filePath);
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly))
    {
        QMessageBox::critical(nullptr, tr("Save Project"), tr("Unable to open the project file for saving"));
        return;
    }
    QXmlStreamWriter* xmlWriter = new QXmlStreamWriter();

    xmlWriter->setDevice(&file);

    // TODO add version info, etc.
    /* Writes a document start with the XML version number version. */
    xmlWriter->writeStartDocument();
    xmlWriter->writeStartElement("BBProject");
    xmlWriter->writeStartElement("Version Info");
    xmlWriter->writeAttribute("Version",QString("%1").arg(BEATSPROJECTMODEL_FILE_VERSION));
    xmlWriter->writeAttribute("Revision",QString("%1").arg(BEATSPROJECTMODEL_FILE_REVISION));
    xmlWriter->writeAttribute("Build",QString("0x%1").arg(BEATSPROJECTMODEL_FILE_BUILD, 4, 16, QLatin1Char('0')));
    xmlWriter->writeEndElement();
    xmlWriter->writeStartElement("Project Info");
    xmlWriter->writeAttribute("Name",info.baseName());
    xmlWriter->writeEndElement();
    xmlWriter->writeEndElement();
    xmlWriter->writeEndDocument();
    delete xmlWriter;
    file.close();


    Platform::CreateProjectShellIcon(info.absolutePath());

}


QModelIndex BeatsProjectModel::trackPtrIndex(const QModelIndex &partIndex, int trackArrayRow, int trackPtrRow)
{
   // 1 - verify that partIndex points to a part
   AbstractTreeItem *p_abstractTreeItem = static_cast<AbstractTreeItem *>(partIndex.internalPointer());
   SongPartItem *p_songPartItem = qobject_cast<SongPartItem *>(p_abstractTreeItem);
   if(!p_songPartItem){
      qWarning() << "BeatsProjectModel::partIndexInternal - ERROR 1 - if(!p_songPartItem)";
      return QModelIndex();
   }

   // 2 - validate trackArrayRow and obtain trackArray object
   if(trackArrayRow >= p_songPartItem->childCount()){
      qWarning() << "BeatsProjectModel::partIndexInternal - ERROR 2 - if(trackArrayRow >= p_songPartItem->childCount())";
      return QModelIndex();
   }

   AbstractTreeItem *p_trackArrayItem = p_songPartItem->child(trackArrayRow);

   // 3 - validate trackPtrRow and obtain trackPtr object
   if(trackPtrRow >= p_trackArrayItem->childCount()){
      qWarning() << "BeatsProjectModel::partIndexInternal - ERROR 3 - if(trackPtrRow >= p_trackArrayItem->childCount())";
      return QModelIndex();
   }

   AbstractTreeItem *p_trackPtrItem = p_trackArrayItem->child(trackPtrRow);

   return createIndex(trackPtrRow, 0, p_trackPtrItem);
}

/**
 * @brief BeatsProjectModel::isProjectFolder
 * @param path
 * @param requireBBP
 * @return
 *
 * Make sure all the base forders are present
 */
bool BeatsProjectModel::isProjectFolder(const QString &path, bool requireBBP, bool requiresHash)
{
   QFileInfo fi(path);

   if(!fi.exists() || !fi.isDir()){
      return false;
   }

   QDir dir(path);

   // Verify if contains a single *.bbp file
   QStringList projectFileFilters;
   projectFileFilters << BMFILES_PROJECT_FILTER;
   QStringList projectFiles = dir.entryList(projectFileFilters, QDir::Files | QDir::Hidden);
   if(requireBBP && projectFiles.count() != 1){
      qDebug() << "(requireBBP) projectFiles.count() = " << projectFiles.count();
      return false;
   }

   // Verify if contains all required static entries
   QStringList projectEntries = dir.entryList(QDir::NoDotAndDotDot | QDir::Dirs | QDir::Files | QDir::Hidden);
   QStringList requiredEntries;
   requiredEntries << "DRUMSETS" << "EFFECTS" << "PARAMS" << "SONGS";
   if (requiresHash) {
       requiredEntries << BMFILES_HASH_FILE_NAME;
   }
   foreach(const QString &requiredEntry, requiredEntries){
      if(!projectEntries.contains(requiredEntry, Qt::CaseInsensitive)){
         return false;
      }
   }

   return true;
}

bool BeatsProjectModel::isProjectArchiveFile(const QString &path)
{
   QFileInfo fi(path);

   if(!fi.exists() || !fi.isFile()){
      return false;
   }

   QuaZip zip(fi.absoluteFilePath());

   if(!zip.open(QuaZip::mdUnzip)){
      return false;
   }

   QuaZipDir zipRoot(&zip);
   QStringList zipRootEntryList = zipRoot.entryList();

   QStringList requiredEntries;
   requiredEntries << "DRUMSETS/" << "EFFECTS/" << "PARAMS/" << "SONGS/";
   foreach(const QString &requiredEntry, requiredEntries){
       if(!zipRootEntryList.contains(requiredEntry, Qt::CaseInsensitive)){
           return false;
       }
   }

   return true;
}

bool BeatsProjectModel::containsAnyProjectEntry(const QString &path, bool includeHash)
{
   QFileInfo fi(path);
   if(!fi.exists() || !fi.isDir()){
      return false;
   }

   QDir dir(path);

   // Verify if contains any *.bbp file
   QStringList projectFileFilters;
   projectFileFilters << BMFILES_PROJECT_FILTER;
   QStringList projectFiles = dir.entryList(projectFileFilters, QDir::Files | QDir::Hidden);
   if(!projectFiles.isEmpty()){
      qDebug() << "BeatsProjectModel::isProjectFolder - DEBUG - projectFiles.count() = " << projectFiles.count();
      return true;
   }

   QStringList projectEntries = dir.entryList(QDir::NoDotAndDotDot | QDir::Dirs | QDir::Files | QDir::Hidden);

   QStringList requiredEntries;
   requiredEntries << "DRUMSETS" << "EFFECTS" << "PARAMS" << "SONGS";
   if (includeHash) {
       requiredEntries << BMFILES_HASH_FILE_NAME;
   }
   foreach(const QString &requiredEntry, requiredEntries){
      if(projectEntries.contains(requiredEntry, Qt::CaseInsensitive)){
         return true;
      }
   }
   return false;
}

/**
 * @brief BeatsProjectModel::projectEntriesInfo
 * @return project entries as QFileInfoList
 *
 * List all project entries including project file
 */
QFileInfoList BeatsProjectModel::projectEntriesInfo(bool includeBBP) const
{
   // All directories included in root item
   QFileInfoList entries = rootItem()->projectDirectories();

   // Append Hash file
   QDir dir(projectDirFI().absoluteFilePath());
   entries.append(QFileInfo(dir.absoluteFilePath(BMFILES_HASH_FILE_NAME)));

   // Append project file
   if(includeBBP){
      entries.append(projectFileFI());
   }

   return entries;
}

/**
 * @brief BeatsProjectModel::projectEntries
 * @return project entries as QStringList
 *
 * List all project entries including project file
 */
QStringList BeatsProjectModel::projectEntries(bool includeBBP) const
{
   // All directories included in root item
   QStringList entries;

   foreach(const QFileInfo &fi, rootItem()->projectDirectories()){
      entries.append(fi.absoluteFilePath());
   }

   // Append Hash file
   QDir dir(projectDirFI().absoluteFilePath());
   entries.append(dir.absoluteFilePath(BMFILES_HASH_FILE_NAME));

   // Append project file
   if(includeBBP){
      entries.append(projectFileFI().absoluteFilePath());
   }

   return entries;
}

QStringList BeatsProjectModel::projectEntriesForPath(const QString &path, bool includeBBP, bool includeHash)
{
   QFileInfo fi(path);
   if(!fi.exists() || !fi.isDir()){
      return QStringList();
   }

   QStringList projectEntries;

   // list all .bbp files in the project entries
   if(includeBBP){
      QStringList projectFileFilters;
      projectFileFilters << BMFILES_PROJECT_FILTER;
      foreach(const QFileInfo &entryFI, QDir(fi.absoluteFilePath()).entryInfoList(projectFileFilters, QDir::Files | QDir::Hidden)){
         projectEntries.append(entryFI.absoluteFilePath());
      }
   }

   QDir dir(path);
   dir.setFilter(QDir::Hidden | QDir::Files | QDir::Dirs);

   QStringList requiredEntries;
   requiredEntries << "DRUMSETS" << "EFFECTS" << "PARAMS" << "SONGS";
   if (includeHash) {
       requiredEntries << BMFILES_HASH_FILE_NAME;
   }

   foreach(const QString &requiredEntry, requiredEntries){
      projectEntries.append(dir.absoluteFilePath(requiredEntry));
   }

   return projectEntries;
}

QStringList BeatsProjectModel::existingProjectEntriesForPath(const QString &path, bool includeBBP, bool includeHash)
{
   QFileInfo fi(path);
   if(!fi.exists() || !fi.isDir()){
      return QStringList();
   }

   QDir dir(path);
   QStringList projectEntries;

   // list all .bbp files in the project entries
   if(includeBBP){
      QStringList projectFileFilters;
      projectFileFilters << BMFILES_PROJECT_FILTER;
      foreach(const QFileInfo &entryFI, QDir(fi.absoluteFilePath()).entryInfoList(projectFileFilters, QDir::Files | QDir::Hidden)){
         projectEntries.append(entryFI.absoluteFilePath());
      }
   }

   // Add all the static files and structures that are present
   QStringList allEntries = dir.entryList(QDir::NoDotAndDotDot | QDir::Dirs | QDir::Files | QDir::Hidden);

   QStringList requiredEntries;
   requiredEntries << "DRUMSETS" << "EFFECTS" << "PARAMS" << "SONGS";

   if (includeHash) {
       requiredEntries << BMFILES_HASH_FILE_NAME;
   }

   foreach(const QString &requiredEntry, requiredEntries){
      if(allEntries.contains(requiredEntry, Qt::CaseInsensitive)){
         projectEntries.append(dir.absoluteFilePath(requiredEntry));
      }
   }

   return projectEntries;
}

void BeatsProjectModel::synchronizeModal(const QString &path, QWidget *p_parent)
{

    QDir dstDir(path);

    // 1 - Scan pedal in order to determine if there are any Song Modification file to synchronize to project
    QStringList modFilesFilter;
    modFilesFilter << BMFILES_SONG_MOD_FILE_FILTER;

    DirListAllSubFilesModal listModFiles(dstDir.absoluteFilePath("SONGS"), DirListAllSubFilesModal::noFolders, p_parent);
    if(!listModFiles.run(false, modFilesFilter)){
        
       qWarning() << "DrumsetPanel::cleanUp - ERROR 1 - unable to clean up temp dir";
       return;
    }

    QStringList modFiles = listModFiles.result();
    qInfo() << "Found " << modFiles.length() << " modfiles!";
    // 2 - Update project according to .mod files and resolve conflicts
    foreach(const QString & modFilePath, modFiles){
        // 2.1 find corresponding song on pedal
        QFileInfo modFI(modFilePath);
        QString   songFileName = QString("%1.%2").arg(modFI.baseName(), BMFILES_MIDI_BASED_SONG_EXTENSION);
        QFileInfo pedalSongFI(modFI.absoluteDir().absoluteFilePath(songFileName));

        if(!pedalSongFI.isFile()){
            // if song file does not exist, ignore mod file. The mod file will be deleted later.
            qWarning() << "Song folder not found :" << modFI.absoluteFilePath();
            continue;
        }

        // 2.2 find corresponding song on project
        QFileInfo pedalSongFolderFI(modFI.absolutePath());
        QString   songFolderFileName(pedalSongFolderFI.fileName());
        SongFolderTreeItem * songFolder = songsFolder()->childWithFileName(songFolderFileName);
        if(!songFolder){
            // if song folder does not exist, ignore mod file. The mod file will be deleted later.
            qWarning() << "BeatsProjectModel::synchronizeModal - WARNING 2 - Project does not contain MOD file song folder :" << songFolderFileName;
            continue;
        }

        SongFileItem *song = songFolder->childWithFileName(songFileName);
        if(!song){
            // if song does not exist, ignore mod file. The mod file will be deleted later.
            qWarning() << "BeatsProjectModel::synchronizeModal - WARNING 3 - Project does not contain MOD file song :" << songFileName;
            continue;
        }

        // 2.3 extract default drumset and default bpm info
        char buffer[MAX_DRM_NAME];
        int modLen = ini_gets("Song","Drum","", buffer, MAX_DRM_NAME, modFilePath.toLocal8Bit().data());
        int modBPM = ini_getl("Song","Tempo",-1, modFilePath.toLocal8Bit().data());
        // make sure buffer is zero terminated
        buffer[modLen] = '\0';
        QString modDefaultDrm(buffer);

        // 2.4 compare hash, drumset and bpm
        bool projectBPMChanged = false;
        bool projectDrmChanged = false;
        QByteArray pedalHash = SongFileItem::getHash(pedalSongFI.absoluteFilePath());
        QByteArray projectHash = song->hash();
        uint32_t   projectTempo = song->data(AbstractTreeItem::TEMPO).toInt();
        QString    projectDefaultDrm = song->data(AbstractTreeItem::DEFAULT_DRM).toString();

        // Keep only filename
        QStringList projectDefaultDrmCSV = projectDefaultDrm.split(",");
        if(projectDefaultDrmCSV.count() > 0){
            projectDefaultDrm = projectDefaultDrmCSV.at(0);
        }

        if(pedalHash != projectHash){

            // Project was modified, parse pedalFile in order to verify if tempo and default BPM were modified

            SongFileModel * p_pedalSong = new SongFileModel;
            QStringList parseErrors;
            // Read file content
            QFile fin(pedalSongFI.absoluteFilePath());
            if(fin.open(QIODevice::ReadOnly)){
                p_pedalSong->readFromFile(fin, &parseErrors);
                fin.close();
                if(parseErrors.empty()){
                    uint32_t pedalTempo      = p_pedalSong->getSongModel()->bpm();
                    QString  pedalDefaultDrm = p_pedalSong->getSongModel()->defaultDrmFileName();

                    if(pedalTempo != projectTempo){
                        projectBPMChanged = true;
                    }
                    if(pedalDefaultDrm.compare(projectDefaultDrm, Qt::CaseInsensitive) != 0){
                        projectDrmChanged = true;
                    }
                }
            }
        }

        // 2.5 update project song if required
        if(!projectBPMChanged && modBPM != -1){
            if(song->setData(AbstractTreeItem::TEMPO, QVariant(modBPM))){
                itemDataChanged(song, AbstractTreeItem::TEMPO);
            }
        }
        if(!projectDrmChanged && !modDefaultDrm.isEmpty()){
            // make sure pedal drm exists in updated project
            QString modDrmLongName = drmFolder()->drmLongName(modDefaultDrm);
            if(!modDrmLongName.isEmpty()){
                // DRM must be specified in the "File_name,name" CSV format
                if(song->setData(AbstractTreeItem::DEFAULT_DRM, QVariant(QString("%1,%2").arg(modDefaultDrm,modDrmLongName)))){
                    itemDataChanged(song, AbstractTreeItem::DEFAULT_DRM);
                }
            }
        }
    }

    // Save project if any modifications done
    if(isSongsFolderDirty()){
        saveModal();
    }

    // 3 - Verify if there are any updates to perform on pedal
    if(rootItem()->compareHash(path) && modFiles.isEmpty()){
        qInfo() << "Destination project is up to date!";
        QMessageBox::information(p_parent, tr("Synchronize Project"), tr("The destination project is up to date.\n\nNo need to synchronize."));
        return;
    }

    // 4 - List the affected files/folders
    QStringList cleanUp;
    QStringList copySrc;
    QStringList copyDst;


    QProgressDialog progress(tr("Synchronizing project..."), tr("Abort"), 0, 1, p_parent);
    progress.setWindowModality(Qt::WindowModal);
    progress.setMinimumDuration(0);

    // 4.1 Explore recursively all folders to determine which file needs to be replaced
    progress.setValue(0);
    // NOTE: Don't perform "rootItem()->prepareSync(...)" directly in order not to clean all content of SD card;
    for(int i = 0; i < rootItem()->childCount(); i++){
        QString childPath = dstDir.absoluteFilePath(rootItem()->child(i)->data(AbstractTreeItem::FILE_NAME).toString());
        rootItem()->child(i)->prepareSync(childPath, &cleanUp, &copySrc, &copyDst);
    }

    // 4.2 List project file in files to replace
    QStringList projectFileFilter;
    projectFileFilter << BMFILES_PROJECT_FILTER;
    // Retrieve as file info in order to retrieve absolute path
    QFileInfoList dstProjectFileEntries = dstDir.entryInfoList(projectFileFilter, QDir::Files);
    foreach(const QFileInfo &fi, dstProjectFileEntries){
        cleanUp.append(fi.absoluteFilePath());
    }
    copySrc.append(m_projectFileFI.absoluteFilePath());
    copyDst.append(dstDir.absoluteFilePath(m_projectFileFI.fileName()));

    // 4.3 List hash file last in list of files to replace(in case of abort, should help restore???)
    if(QFileInfo(dstDir.absoluteFilePath(BMFILES_HASH_FILE_NAME)).exists()){
        cleanUp.append(dstDir.absoluteFilePath(BMFILES_HASH_FILE_NAME));
    }
    copySrc.append(QDir(m_projectDirFI.absoluteFilePath()).absoluteFilePath(BMFILES_HASH_FILE_NAME));
    copyDst.append(dstDir.absoluteFilePath(BMFILES_HASH_FILE_NAME));

    if(copySrc.count() != copyDst.count()){
        progress.setValue(1);
        QMessageBox::critical(p_parent, tr("Synchronize Project"), tr("Internal error while listing files to synchronize"));
        return;
    }
    progress.setMaximum(1 + cleanUp.count() + copySrc.count() + modFiles.count());
    progress.setValue(1);

    // DEBUG list the content of the lists
    qDebug() << "BeatsProjectModel::synchronize - DEBUG - modFiles:";
    foreach(const QString & modPath, modFiles){
        qDebug() << "   " << modPath;
    }
    qDebug() << "BeatsProjectModel::synchronize - DEBUG - cleanUp:";
    foreach(const QString & cleanUpPath, cleanUp){
        qDebug() << "   " << cleanUpPath;
    }
    qDebug() << "BeatsProjectModel::synchronize - DEBUG - copySrc:";
    foreach(const QString & srcCpyPath, copySrc){
        qDebug() << "   " << srcCpyPath;
    }
    qDebug() << "BeatsProjectModel::synchronize - DEBUG - dstSrc:";
    foreach(const QString & dstCpyPath, copyDst){
        qDebug() << "   " << dstCpyPath;
    }

    // 5 - Clean up mod files, files that were ether removed or that need to be replaced
    if(progress.wasCanceled()){
        return;
    }

    // Clean up mod files
    if(!cleanUpModal(modFiles, progress, p_parent)){
        return;
    }

    // Clean up
    if(!cleanUpModal(cleanUp, progress, p_parent)){
        return;
    }

    // 6 - Copy modified/new files to destination
    if(progress.wasCanceled()){
        return;
    }

    if(!copyModal(copySrc, copyDst, progress, p_parent)){
        return;
    }

}

bool BeatsProjectModel::cleanUpModal(const QStringList &cleanUpPaths, QProgressDialog &progress, QWidget *p_parent)
{
   foreach(const QString &cleanUpPath, cleanUpPaths){
      if(progress.wasCanceled()){
         return false;
      }

      QFileInfo cleanUpFI(cleanUpPath);
      if(!cleanUpFI.exists()){
         progress.setValue(progress.value() + 1);
      } else if(cleanUpFI.isFile()){
         QFile cleanUpFile(cleanUpFI.absoluteFilePath());
         while(!cleanUpFile.remove()){

            // File was somewhat removed despite error
            if(!cleanUpFI.exists()){
               break;
            }

            progress.show(); // always make sure progress is shown before displaying another dialog.
            if (QMessageBox::Abort == QMessageBox::question(p_parent, tr("Project Cleanup"), tr("Unable to delete %1").arg(cleanUpFI.absoluteFilePath()), QMessageBox::Retry | QMessageBox::Abort, QMessageBox::Retry)) {
               progress.setValue(progress.maximum());
               return false;
            }
         }
         progress.setValue(progress.value() + 1);
      } else if (cleanUpFI.isDir()){
         QDir cleanUpDir(cleanUpFI.absoluteFilePath());
         while(!cleanUpDir.removeRecursively()){
            // File was somewhat removed despite error
            if(!cleanUpFI.exists()){
               break;
            }

            progress.show(); // always make sure progress is shown before displaying another dialog.
            if(QMessageBox::Abort == QMessageBox::question(p_parent, tr("Project Cleanup"), tr("Unable to delete %1").arg(cleanUpFI.absoluteFilePath()), QMessageBox::Retry | QMessageBox::Abort, QMessageBox::Retry)){
               progress.setValue(progress.maximum());
               return false;
            }
         }
         progress.setValue(progress.value() + 1);
      } else {
         progress.show(); // always make sure progress is shown before displaying another dialog.
         QMessageBox::warning(p_parent, tr("Project Cleanup"), tr("Skipping unknown file type %1").arg(cleanUpPath));
         progress.setValue(progress.value() + 1);
      }
   }
   if(progress.wasCanceled()){
      return false;
   }
   return true;
}

bool BeatsProjectModel::copyModal(const QStringList &srcPaths, const QStringList &dstPaths, QProgressDialog &progress, QWidget *p_parent)
{
   for(int i = 0; i < srcPaths.count(); i++){
      if(progress.wasCanceled()){
         return false;
      }
      const QString &srcPath = srcPaths.at(i);
      const QString &dstPath = dstPaths.at(i);

      QFileInfo srcFI(srcPath);
      QFileInfo dstFI(dstPath);

      if(!srcFI.exists()){
         if (!QString("bcf").compare(srcFI.suffix()))
            QMessageBox::warning(p_parent, tr("Copy files"), tr("Source file %1 does not exist").arg(srcPath));
         progress.setValue(progress.value() + 1);
      } else if(dstFI.exists()){
         QMessageBox::warning(p_parent, tr("Copy files"), tr("Destination file %1 already exists").arg(dstPath));
         progress.setValue(progress.value() + 1);
      } else if(srcFI.isFile()){
         QFile srcFile(srcFI.absoluteFilePath());
         while(!srcFile.copy(srcFI.absoluteFilePath(), dstFI.absoluteFilePath())){

            // File was somewhat copied despite of error
            if(dstFI.exists()){
               break;
            }

            progress.show(); // always make sure progress is shown before displaying another dialog.
            if (QMessageBox::Abort == QMessageBox::question(p_parent, tr("Copy files"), tr("Unable to copy %1 to %2").arg(srcFI.absoluteFilePath(), dstFI.absoluteFilePath()), QMessageBox::Retry | QMessageBox::Abort, QMessageBox::Retry)){
               progress.setValue(progress.maximum());
               return false;
            }
         }
         progress.setValue(progress.value() + 1);
      } else if (srcFI.isDir()){
         QDir dstDir(dstFI.absoluteFilePath());
         while(!dstDir.mkpath(dstFI.absoluteFilePath())){
            // Dir was somewhat created despite error
            if(dstDir.exists()){
               break;
            }

            progress.show(); // always make sure progress is shown before displaying another dialog.
            if (QMessageBox::Abort == QMessageBox::question(p_parent, tr("Copy files"), tr("Unable to create %1").arg(dstFI.absoluteFilePath()), QMessageBox::Retry | QMessageBox::Abort, QMessageBox::Retry)) {
               progress.setValue(progress.maximum());
               return false;
            }
         }
         progress.setValue(progress.value() + 1);
      } else {
         progress.show(); // always make sure progress is shown before displaying another dialog.
         QMessageBox::warning(p_parent, tr("Copy files"), tr("Skipping unknown file type"));
         progress.setValue(progress.value() + 1);
      }
   }
   if(progress.wasCanceled()){
      return false;
   }
   return true;
}

/**
 * @brief BeatsProjectModel::isProject3WayLinked
 * @param path
 * @return
 *
 * 1 - Opened project is the last local project that was linked
 * 2 - Destination project has the same uuid as local project
 * 3-4 - Destination project was last edited by this software instance (unique hash in registry key per user)
 */
bool BeatsProjectModel::isProject3WayLinked(const QString &path)
{
   QFileInfo fi(path);
   if(!fi.exists() || !fi.isDir()){
      return false;
   }

   QDir rootDir(path);

   // 1 - Compare linked project uuid with opened project
   QUuid linkedPrjUuid = Settings::getLinkedPrjUuid();
   QUuid srcPrjUuid = paramsFolder()->projectUuid();

   if(linkedPrjUuid.isNull() || srcPrjUuid.isNull() || (srcPrjUuid != linkedPrjUuid)){
       return false;
   }

   // 2 - Compare destination project uuid with linked project uuid
   QUuid dstPrjUuid = ParamsFolderTreeModel::getProjectUuid(rootDir.absoluteFilePath("PARAMS"));
   if(dstPrjUuid.isNull() || (dstPrjUuid != linkedPrjUuid)){
      return false;
   }

   // 3 - Compute expected link hash
   QCryptographicHash cr(QCryptographicHash::Sha256);

   QUuid softwareUuid = Settings::getSoftwareUuid();
   cr.addData(dstPrjUuid.toByteArray());
   cr.addData(softwareUuid.toByteArray());

   QByteArray expectedLinkHash = cr.result();

   // 4 - Compare link hash
   QByteArray dstLinkHash = ParamsFolderTreeModel::getLinkHash(rootDir.absoluteFilePath("PARAMS"));

   // the rest of this method _may_ be exchangeable for "return expectedLinkHash == dstLinkHash"
   if(expectedLinkHash.count() == 0 || dstLinkHash.count() == 0){
      return false;
   }

   if(expectedLinkHash.count() != dstLinkHash.count()){
      return false;
   }

   for(int i = 0; i < expectedLinkHash.count(); i++){
      if(expectedLinkHash.at(i) != dstLinkHash.at(i)){
         return false;
      }
   }

   // Three-way project link confirmed
   return true;
}

/**
 * @brief BeatsProjectModel::isProject2WayLinked
 * @return
 *
 * Opened project is the last local project that was linked
 */
bool BeatsProjectModel::isProject2WayLinked()
{
   // 1 - Compare linked project uuid with opened project
   QUuid linkedPrjUuid = Settings::getLinkedPrjUuid();
   QUuid srcPrjUuid = paramsFolder()->projectUuid();

   // Note: local project does not have a link hash. This is why we don't compute it here
   return ((!linkedPrjUuid.isNull()) && (!srcPrjUuid.isNull()) && (srcPrjUuid == linkedPrjUuid));
}

bool BeatsProjectModel::linkProject(const QString &path)
{
   QFileInfo fi(path);
   if(!fi.exists() || !fi.isDir()){
      return false;
   }

   QDir rootDir(path);

   // 1 - Make sure the destination project has the good uuid
   QUuid dstPrjUuid = ParamsFolderTreeModel::getProjectUuid(rootDir.absoluteFilePath("PARAMS"));
   if(dstPrjUuid.isNull()){
      qWarning() << "BeatsProjectModel::linkProject - ERROR 1 - dstPrjUuid.isNull()";
      return false;
   }

   QUuid srcPrjUuid = paramsFolder()->projectUuid();
   if(srcPrjUuid.isNull()){
      qWarning() << "BeatsProjectModel::linkProject - ERROR 2 - srcPrjUuid.isNull()";
      return false;
   }

   if(dstPrjUuid != srcPrjUuid){
      // No need to re-compute hash since Project UUID in infoMap is excluded from hash
      if(!ParamsFolderTreeModel::setProjectUuid(rootDir.absoluteFilePath("PARAMS"), srcPrjUuid)){
         qWarning() << "BeatsProjectModel::linkProject - ERROR 3 - setProjectUuid returned false";
         return false;
      }

      dstPrjUuid = ParamsFolderTreeModel::getProjectUuid(rootDir.absoluteFilePath("PARAMS"));
   }

   // 2 - Store common project uuid as linked project uuid
   Settings::setLinkedPrjUuid(srcPrjUuid);

   // 3 - Compute link hash
   QCryptographicHash cr(QCryptographicHash::Sha256);

   QUuid softwareUuid = Settings::getSoftwareUuid();
   cr.addData(srcPrjUuid.toByteArray());
   cr.addData(softwareUuid.toByteArray());

   QByteArray linkHash = cr.result();

   // 4 - Store link hash to project
   if(!ParamsFolderTreeModel::setLinkHash(rootDir.absoluteFilePath("PARAMS"), linkHash)){
      qWarning() << "BeatsProjectModel::linkProject - ERROR 5 - setLinkHash returned false";
      return false;
   }

   setProjectDirty();

   return true;
}

/**
 * @brief BeatsProjectModel::resetProjectInfo
 * @param path
 * @return
 *
 * STATIC Resets uuid and link status
 * Since info map not part of hash, no need to re-hash
 */
bool BeatsProjectModel::resetProjectInfo(const QString &path)
{
   QFileInfo fi(path);
   if(!fi.exists() || !fi.isDir()){
      qWarning() << "BeatsProjectModel::resetProjectInfo - ERROR 1 - (!fi.exists() || !fi.isDir())";
      return false;
   }

   QDir rootDir(path);

   if(!ParamsFolderTreeModel::resetProjectInfo(rootDir.absoluteFilePath("PARAMS"))){
      qWarning() << "BeatsProjectModel::resetProjectInfo - ERROR 2 - resetProjectInfo returned false";
      return false;
   }
   return true;
}

/**
 * @brief BeatsProjectModel::resetProjectInfo
 * @return
 *
 * Resets uuid and link status
 * Since info map not part of hash, no need to re-hash
 * model()->setArchiveDirty needs to be called after this call
 */
bool BeatsProjectModel::resetProjectInfo()
{
   return paramsFolder()->resetProjectInfo();
}


QModelIndex BeatsProjectModel::createNewSongFolder(int row)
{
   // Insert new row
   if (!CmdCreateItem::queue(tr("Folder", "Context: Adding *Folder*"), this, songsFolderIndex(), row, 1)) {
      qWarning() << "BeatsProjectModel::createNewSongFolder - ERROR 1 - insertRow returned false";
      return QModelIndex();
   }
   return index(row, 0, songsFolderIndex());
}


void BeatsProjectModel::deleteSongFolder(const QModelIndex& ix)
{
    CmdRemoveFolder::queue(this, ix.parent(), ix.row());
}

QModelIndex BeatsProjectModel::createNewSong(const QModelIndex& parent, int row)
{
   if (!CmdCreateItem::queue(tr("Song", "Context: Adding *Song*"), this, parent, row, 1)) {
      qWarning() << "BeatsProjectModel::createNewSong - ERROR 1 - insertRow returned false";
      return QModelIndex();
   }
   return index(row, 0, songsFolderIndex());
}

void BeatsProjectModel::deleteSong(const QModelIndex& ix)
{
    CmdRemoveSong::queue(this, ix.parent(), ix.row());
}

QModelIndex BeatsProjectModel::createNewSongPart(const QModelIndex& parent, int row)
{
   if (!CmdCreateItem::queue(tr("Part", "Context: Adding *Part*"), this, parent, row, 1)) {
      qWarning() << "BeatsProjectModel::createNewSong - ERROR 1 - insertRow returned false";
      return QModelIndex();
   }
   return index(row, 0, songsFolderIndex());
}

void BeatsProjectModel::deleteSongPart(const QModelIndex& ix)
{
    CmdRemoveSongPart::queue(this, ix.parent(), ix.row());
}

QModelIndex BeatsProjectModel::createSongFile(const QModelIndex& parent, int row, const QString& filename)
{
    return CmdCreateSongFile::queue(this, parent, row, filename) ? index(row, 0, parent) : QModelIndex();
}

void BeatsProjectModel::changeSongFile(const QModelIndex& ix, const QString& filename)
{
    CmdReplaceSongFile::queue(this, ix, filename);
}

void BeatsProjectModel::changeSongFile(const QModelIndex& ix, const QByteArray& data)
{
    CmdEditSongFile::queue(this, ix, data);
}

void BeatsProjectModel::moveSelection(const QModelIndex& to, bool undoable)
{
    undoable ? CmdChangeSelection::queue(this, selected(), to) : CmdChangeSelection::apply(this, to);
}

void BeatsProjectModel::deleteSongFile(const QModelIndex& ix)
{
    CmdRemoveSongFile::queue(this, ix.parent(), ix.row());
}

bool BeatsProjectModel::moveItem(const QModelIndex& index, int delta)
{
    auto ix = Index(index);
    QString name;
    if (ix == 2 || ix == 3) {
        name = QString("\"%1\"").arg(index.data().toString());
    } else {
        name = (ix == 4 ? tr("Part %1") : tr("Unit %1")).arg(index.row());
    }
    return CmdMoveItem::queue(name, this, index, delta);
}

bool BeatsProjectModel::moveOrCopySong(bool move, const QModelIndex& song, const QModelIndex& folder, int row)
{
    auto max_cnt = folder.sibling(folder.row(),AbstractTreeItem::MAX_CHILD_CNT).data();
    if (!max_cnt.isNull() && rowCount(folder) == max_cnt.toInt()) {
        qWarning() << "BeatsProjectModel::moveSongToAnotherFolder - ERROR 1 - max child count reached";
        return false;
    }
    CmdMoveOrCopySong::queue(move, this, song, folder, row+1 ? row : rowCount(folder));
    return true;
}

void BeatsProjectModel::importModal(QWidget* p_parentWidget, const QModelIndex& index, const QStringList& songs, const QStringList& folders)
{
    if (songs.empty() && folders.empty() || !songs.empty() && index == QModelIndex())
        return;
    CmdImportSongsAndFolders::queue(this, p_parentWidget, index, songs, folders);
}


void BeatsProjectModel::manageParsingErrors(QWidget *p_parent)
{
   songsFolder()->manageParsingErrors(p_parent);
}

void BeatsProjectModel::saveModal()
{
    // Save any unsaved song file
    songsFolder()->setData(AbstractTreeItem::SAVE, QVariant(0));

    m_songsFolderDirty = false;
    m_projectDirty = false;
    saveProjectFile(m_projectFileFI.absoluteFilePath());
}

/**
 * @brief BeatsProjectModel::saveAsModal
 * @param Path to the "AS" project folder
 * @param Name of the project file (with the extention .bbp)
 * @param p_parent
 *
 * @return true if the operation was a succes or false if it failed
 */
bool BeatsProjectModel::saveAsModal(const QFileInfo &newProjectFileFI, QWidget *p_parent)
{
   // 1 - Save any unsaved song file
   songsFolder()->setData(AbstractTreeItem::SAVE, QVariant(0));

   m_projectDirty = false;
   m_songsFolderDirty = false;

   // 2 - List all file that needs to be copied
   DirListAllSubFilesModal listAll(projectEntries(true), DirListAllSubFilesModal::rootFirst, p_parent);
   if(!listAll.run()){
      qWarning() << "BeatsProjectModel::saveAsModal - ERROR 1 - listAll failed/Aborted";
      return false;
   }

   // 3 - Copy the previous files from the original project to the "AS" project
   DirCopyModal copyAll(newProjectFileFI.absolutePath(), m_projectDirFI.absoluteFilePath(),listAll.result(),p_parent);
   copyAll.setProgressType(DirCopyModal::ProgressFileSize);
   copyAll.setTotalSize(listAll.totalFileSize());
   if (!copyAll.run()){
      qWarning() << "BeatsProjectModel::saveAsModal - ERROR 2 - copyAll failed/Aborted";
      return false;
   }

   // 4 - Rename project file and refresh content
   QFile copiedProjectFile(newProjectFileFI.dir().absoluteFilePath(m_projectFileFI.fileName()));
   if(!copiedProjectFile.exists()){
      qWarning() << "BeatsProjectModel::saveAsModal - ERROR 3 - project file was not copied " << newProjectFileFI.dir().absoluteFilePath(m_projectFileFI.fileName());
      QMessageBox::critical(p_parent, tr("Project Save As"), tr("Project file %1 was not copied").arg(newProjectFileFI.dir().absoluteFilePath(m_projectFileFI.fileName())));
      return false;
   }

   // try more than once in case os has a lock on file 
   while(copiedProjectFile.fileName() != newProjectFileFI.absoluteFilePath() && !copiedProjectFile.rename(newProjectFileFI.absoluteFilePath())){
      qWarning() << "BeatsProjectModel::saveAsModal - ERROR 4 - Unable to rename project file " << newProjectFileFI.dir().absoluteFilePath(m_projectFileFI.fileName());
      if (QMessageBox::Abort == QMessageBox::critical(p_parent, tr("Project Save As"), tr("Unable to rename project file\n%1\nto\n%2")
          .arg(QFileInfo(copiedProjectFile).absoluteFilePath())
          .arg(newProjectFileFI.absoluteFilePath()),
          QMessageBox::Retry, QMessageBox::Abort)) {
              return false;
      }
   }

   // Reset project info to remove any linking and create new UUID
   resetProjectInfo(newProjectFileFI.absolutePath()); // STATIC VERSION

   // Refresh project file content (After UUID in case we want to add this version in project)
   saveProjectFile(newProjectFileFI.absoluteFilePath());

   return true;
}

void BeatsProjectModel::saveProjectArchive(const QString& path, QWidget *p_parent)
{
    // Save any unsaved song file
    songsFolder()->setData(AbstractTreeItem::SAVE, QVariant(0));

    QFileInfo outArchiveFI(path);

    // rearchive everything
    // list all needs to be performed relative to project root
    DirListAllSubFilesModal listAll(projectEntries(true), DirListAllSubFilesModal::rootFirst, p_parent, m_projectDirFI.absoluteFilePath());
    if(!listAll.run()){
       qWarning() << "BeatsProjectModel::saveAsModal - ERROR 1 - !listAll.run()";
       return;
    }

    CompressZipModal kompressor(outArchiveFI.absoluteFilePath(), m_projectDirFI.absoluteFilePath(),listAll.result(),p_parent);
    kompressor.setProgressType(CompressZipModal::ProgressFileSize);
    kompressor.setTotalSize(listAll.totalFileSize());
    if(!kompressor.run()){
       qWarning() << "BeatsProjectModel::saveAsModal - ERROR 2 - !kompressor.run()";
       return;
    }

}

QFileInfo BeatsProjectModel::createProjectFolderForProjectFile(const QFileInfo &projectFileFI)
{
   QDir baseDir = projectFileFI.absoluteDir();

   QString projectFileName = projectFileFI.fileName();
   if(projectFileFI.suffix().compare(BMFILES_PROJECT_EXTENSION, Qt::CaseInsensitive)){
      projectFileName.append(".").append(BMFILES_PROJECT_EXTENSION);
   }
   QString baseProjectName = projectFileFI.baseName();

   QString projectDirName = tr("%1 - Project", "New project name").arg(baseProjectName);


   // Resolve Duplicate names (if required)
   int i = 1;
   while (QFileInfo(baseDir.absoluteFilePath(projectDirName)).exists()){
      projectDirName = tr("%1 (%2) - Project", "New project name").arg(baseProjectName).arg(i++);
   }

   QDir projectFolderDir(baseDir.absoluteFilePath(projectDirName));

   // create folder
   projectFolderDir.mkdir(projectFolderDir.absolutePath());

   // return the new absolute path
   return projectFolderDir.absoluteFilePath(projectFileName);
}

