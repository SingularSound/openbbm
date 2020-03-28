#ifndef UNDOABLE_COMMANDS_H
#define UNDOABLE_COMMANDS_H

#include <QUndoCommand>
#include <Qt>

#include <midi_editor/editor.h>
#include <midi_editor/editordata.h>

static int undo_command_enumerator = 0;
class CmdEditorNote : public QUndoCommand
{
    EditorTableWidget* editor;
    QVariant data;
    int row;
    int col;
    int vel;
    int old;
    int ofs;

    CmdEditorNote(EditorTableWidget* editor, int row, int col, int vel, int old, int ofs)
        : editor(editor), row(row), col(col), vel(vel), old(old), ofs(ofs)
    {
        updateText();
    }

    void updateText()
    {
        auto m = editor->model();
        auto instr = m->headerData(row, Qt::Vertical, EditorData::V::Name).toString();
        int tick = m->headerData(col, Qt::Horizontal, EditorData::H::Tick).toInt();
        QString message;
        if(vel<0) {
            message = "Deleting Note %3 (%1, %2)";
        } else if(!old) {
            message = "Adding Note %3 (%1, %2)";
        } else {
            message = "Changing Note %4 -> %3 (%1, %2)";
        }
        setText(message.arg(instr).arg(tick).arg(vel ? vel : old).arg(old));
    }

public:
    static void queue(EditorTableWidget* editor, const QModelIndex& ix, int velocity, int old = -1, int ofs = -1)
    {
        if (old < 0) {
            editor->model()->data(ix, EditorData::X::Velocity).toInt();
        }
        CmdEditorNote *cmdEditorNote = new CmdEditorNote(editor, ix.row(), ix.column(), velocity, old, ofs);

        editor->undoStack()->push(cmdEditorNote);
    }

    void redo()
    {
        auto m = editor->model();
        editor->setNote(row, col, vel, true, ofs);
        data = m->data(m->index(row, col), EditorData::X::Notes);
    }
    void undo()
    {
        auto m = editor->model();
        m->setData(m->index(row, col), data, EditorData::X::Notes);
        editor->setNote(row, col, old, true, ofs);
    }
    int id() const
    {
        static int id = undo_command_enumerator++;
        return id;
    }
    bool mergeWith(const QUndoCommand *other)
    {
        auto next = static_cast<const CmdEditorNote*>(other);
        bool res = (row == next->row && col == next->col);
        if (res) {
            vel = next->vel;
            updateText();
        }
        return res;
    }
};

class CmdEditorGrid : public QUndoCommand
{
    EditorTableWidget* editor;
    SongPart* sp;

    MIDIPARSER_MidiTrack old_data;
    MIDIPARSER_MidiTrack new_data;

    std::set<int> old_grid;
    std::map<int, int> old_fix;

    std::set<int> new_grid;
    std::map<int, int> new_fix;

    CmdEditorGrid(EditorTableWidget* editor, SongPart* sp, const MIDIPARSER_MidiTrack& new_data,
        const std::set<int>& new_grid, const std::map<int, int>& new_fix,
        const std::set<int>& old_grid, const std::map<int, int>& old_fix)
        : QUndoCommand(QObject::tr("Changing Editor Layout", "Undo Commands"))
        , editor(editor), sp(sp)
        , old_data(editor->collect())
        , new_data(new_data)
        , old_grid(old_grid)
        , old_fix(old_fix)
        , new_grid(new_grid)
        , new_fix(new_fix)
    {}

public:
    static void queue(EditorTableWidget* editor, SongPart* sp, const MIDIPARSER_MidiTrack& data, const std::set<int>& grid, const std::map<int, int>* fix = nullptr)
    {
        editor->undoStack()->push(new CmdEditorGrid(editor, sp, data, grid, fix ? *fix : sp->qq(), editor->grid(), sp->qq()));
    }

    void redo()
    {
        editor->changeLayout(sp, new_grid, &new_data, &(sp->qq() = new_fix));
    }
    void undo()
    {
        editor->changeLayout(sp, old_grid, &old_data, &(sp->qq() = old_fix));
    }
    int id() const
    {
        static int id = undo_command_enumerator++;
        return id;
    }
    bool mergeWith(const QUndoCommand *other)
    {
        auto next = static_cast<const CmdEditorGrid*>(other);
        new_grid = next->new_grid; new_fix = next->new_fix;
        return true;
    }
};

class CmdEditorGridFix : public QUndoCommand
{
    EditorTableWidget* editor;

    int from;
    int to;

    std::vector<int> old_parts;
    std::vector<int> new_parts;

    CmdEditorGridFix(EditorTableWidget* editor, int from, int to, const std::vector<int>& new_parts, const std::vector<int>& old_parts)
        : QUndoCommand(QObject::tr("Fixing Editor Layout", "Undo Commands"))
        , editor(editor), from(from), to(to)
        , old_parts(old_parts)
        , new_parts(new_parts)
    {}

public:
    static void queue(EditorTableWidget* editor, int from, int to, const std::vector<int>& size)
    {
        auto m = editor->model();
        std::vector<int> old(to-from+1);
        for (int i = from; i <= to; ++i)
            old[i-from] = m->headerData(i, Qt::Horizontal, EditorData::H::Length).toInt();
        editor->undoStack()->push(new CmdEditorGridFix(editor, from, to, size, old));
    }

    void redo() { editor->splitMergeColumns(from, to, new_parts); }
    void undo() { editor->splitMergeColumns(from, from+new_parts.size()-1, old_parts); }
    int id() const { static int id = undo_command_enumerator++; return id; }
    bool mergeWith(const QUndoCommand *other)
    {
        auto next = (CmdEditorGridFix*)other;
        if (from != next->from) {
            return false;
        }
        auto l = 0;
        for (auto p : old_parts) {
            l += p;
        }
        for (auto p : next->old_parts) {
            l -= p;
        }
        if (!l) {
            new_parts.swap(next->new_parts);
        }
        return !l;
    }
};

class CmdEditorClearNotes : public QUndoCommand
{
    EditorTableWidget* editor;
    std::vector<QVariant> data;
    int row;

    CmdEditorClearNotes(EditorTableWidget* editor, int row)
        : QUndoCommand(QObject::tr("Clearing all notes of %1", "Undo Commands")
            .arg(editor->model()->headerData(row, Qt::Vertical).toString()))
        , editor(editor), row(row)
    {
        auto m = editor->model();
        data.resize(m->columnCount());
        for (int i = data.size()-1; i+1; --i) {
            data[i] = m->data(m->index(row, i), EditorData::X::Notes);
        }
    }

public:
    static void queue(EditorTableWidget* editor, int row)
    {
        editor->undoStack()->push(new CmdEditorClearNotes(editor, row));
    }

    void redo()
    {
        auto m = editor->model();
        for (int i = m->columnCount()-1; i+1; --i) {
            QMapIterator<QString, QVariant> it(data[i].toMap());
            for (it.toBack(); it.hasPrevious(); ) {
                it.previous();
                editor->setNote(row, i, 0);
            }
        }
    }
    void undo()
    {
        auto m = editor->model();
        for (int i = m->columnCount()-1; i+1; --i) {
            for (int j = m->data(m->index(row, i), EditorData::X::Notes).toMap().size(); j; --j)
                editor->setNote(row, i, 0);
            for (QMapIterator<QString, QVariant> it(data[i].toMap()); it.hasNext(); ) {
                it.next();
                editor->setNote(row, i, it.value().toInt(), true, QVariant(it.key()).toInt());
            }
        }
    }
};

class CmdEditorCopyMoveNotes : public QUndoCommand
{
    EditorTableWidget* editor;
    bool copy;

    int from;
    int to;

    std::vector<QVariant> from_data;
    std::vector<QVariant> to_data;

    CmdEditorCopyMoveNotes(EditorTableWidget* editor, bool copy, int from, int to)
        : QUndoCommand((copy ? QObject::tr("Copying all notes %1 -> %2", "Undo Commands") : QObject::tr("Moving all notes %1 -> %2", "Undo Commands"))
            .arg(editor->model()->headerData(from, Qt::Vertical).toString())
            .arg(editor->model()->headerData(to, Qt::Vertical).toString()))
        , editor(editor), copy(copy), from(from), to(to)
    {
        auto m = editor->model();
        from_data.resize(m->columnCount());
        to_data.resize(m->columnCount());
        for (int i = from_data.size()-1; i+1; --i) {
            from_data[i] = m->data(m->index(from, i), EditorData::X::Notes);
            to_data[i] = m->data(m->index(to, i), EditorData::X::Notes);
        }
    }

public:
    static void queue(EditorTableWidget* editor, bool copy, int from, int to)
    {
        editor->undoStack()->push(new CmdEditorCopyMoveNotes(editor, copy, from, to));
    }

    void redo()
    {
        auto m = editor->model();
        for (int i = m->columnCount()-1; i+1; --i) {
            QMapIterator<QString, QVariant> it(from_data[i].toMap());
            for (it.toBack(); it.hasPrevious(); ) {
                it.previous();
                editor->setNote(to, i, it.value().toInt(), true, QVariant(it.key()).toInt());
                if (copy) {
                    continue;
                }
                editor->setNote(from, i, 0);
            }
        }
    }
    void undo()
    {
        auto m = editor->model();
        for (int i = m->columnCount()-1; i+1; --i) {
            for (int j = m->data(m->index(from, i), EditorData::X::Notes).toMap().size(); j; --j){
                editor->setNote(from, i, 0);
            }
            for (int j = m->data(m->index(to, i), EditorData::X::Notes).toMap().size(); j; --j){
                editor->setNote(to, i, 0);
            }
            for (QMapIterator<QString, QVariant> it(from_data[i].toMap()); it.hasNext(); ) {
                it.next();
                editor->setNote(from, i, it.value().toInt(), true, QVariant(it.key()).toInt());
            }
            for (QMapIterator<QString, QVariant> it(to_data[i].toMap()); it.hasNext(); ) {
                it.next();
                editor->setNote(to, i, it.value().toInt(), true, QVariant(it.key()).toInt());
            }
        }
    }
};

#endif // UNDOABLE_COMMANDS_H
