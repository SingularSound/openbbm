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
#include <midi_editor/editor.h>
#include <QUndoStack>
#include <midi_editor/editordialog.h>
#include <QTimer>
#include <midi_editor/editorverticalheader.h>
#include <midi_editor/editorhorizontalheader.h>
#include <midi_editor/editorcelldelegate.h>
#include <midi_editor/undoable_commands.h>

EditorTableWidget::EditorTableWidget(QWidget* parent)
    : QTableWidget(parent)
    , mp_undoStack(new QUndoStack(this))
    , m_editor(new EditorDialog(this))
    , m_resizer(new QTimer(this))
    , m_dont_leave(false)
    , m_resize_block(false)
{
    setMouseTracking(true);
    setEditTriggers(QAbstractItemView::NoEditTriggers);
    setItemDelegate(new EditorCellDelegate(this));
    auto h = new EditorHorizontalHeader(this);
    auto v = new EditorVerticalHeader(this);
    setHorizontalHeader(h);
    m_resizer->setSingleShot(true);
    m_resizer->setInterval(250);
    connect(m_resizer, &QTimer::timeout, this, [this] {
        auto m = model();
        auto w = (1+m->property(EditorData::Scale).toDouble())*viewport()->width()*.75/m->property(EditorData::BarLength).toInt();
        m_resize_block = true;
        for (int i = 0; i < columnCount(); ++i){
            setColumnWidth(i, w*m->headerData(i, Qt::Horizontal, EditorData::H::Length).toInt()+.5);
        }
        m_resize_block = false;
        auto scale = m_resizer->property(EditorData::Scale).toPointF();
        auto ix = m_resizer->property(EditorData::Highlighted).toModelIndex();
        auto pos = m_resizer->property(EditorData::PlayerPosition).toPoint();
        m_resizer->setProperty(EditorData::Scale, QVariant());
        m_resizer->setProperty(EditorData::Highlighted, QVariant());
        m_resizer->setProperty(EditorData::PlayerPosition, QVariant());
        if (ix.isValid() && !scale.isNull() && !pos.isNull()) {
            // move viewport for smooth resizing
            auto rc = visualRect(ix);
            auto rpos = QPoint(rc.x()+rc.width()*scale.x()+.5, rc.y()+rc.height()*scale.y()+.5);
            auto dx = rpos.x()-pos.x(), dy = rpos.y()-pos.y();
            if (dx || dy) QTimer::singleShot(0, [this, dx, dy]() {
                if (dx) if (auto scroll = horizontalScrollBar())
                    scroll->setValue(scroll->value()+dx);
                if (dy) if (auto scroll = verticalScrollBar())
                    scroll->setValue(scroll->value()+dy);
            });
        } else {
            scrollTo(ix.isValid() ? ix : currentIndex(), QAbstractItemView::PositionAtCenter);
        }
        show();
    });
    setCornerButtonEnabled(false);
    setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
    h->setContextMenuPolicy(Qt::CustomContextMenu);
    v->setContextMenuPolicy(Qt::CustomContextMenu);
    setVerticalHeader(v);
    setSelectionMode(QAbstractItemView::SelectionMode::SingleSelection);
    connect(h, &QHeaderView::sectionResized, [this](int ix, int old, int size) {
        if (old, m_resize_block) return;
        auto m = model();
        m->setProperty(EditorData::Scale, size*m->property(EditorData::BarLength).toDouble()/
            m->headerData(ix, Qt::Horizontal, EditorData::H::Length).toInt()/viewport()->width()/.75-1);
        auto hl = m->index(0, ix);
        if (hl.isValid()) {
            m_resizer->setProperty(EditorData::Scale, QPointF(1, 0));
            m_resizer->setProperty(EditorData::Highlighted, hl);
            m_resizer->setProperty(EditorData::PlayerPosition, visualRect(hl).topRight());
        }
        m_resizer->start();
    });
    connect(h, &QHeaderView::sectionPressed, [this](int ix) {
        int min = columnCount(), max = 0;
        foreach (auto s, selectedIndexes()) {
            auto col = s.column();
            if (col == ix) continue;
            if (col < min) min = col;
            if (col > max) max = col;
        }
        if (min > max || (ix != min-1 && ix != max+1)) {
            clearSelection();
            selectColumn(min = max = ix);
            setCurrentCell(0, ix);
        } else if (min > max) {
        } else {
            setCurrentCell(0, ix < min ? min : ix > max ? max : ix);
        }
    });
    connect(h, &QWidget::customContextMenuRequested, [this, h](const QPoint& pos) {
        auto ix = h->logicalIndexAt(pos);
        if (ix < 0) return;
        auto m = model();
        int min = m->columnCount(), max = -1;
        foreach (auto s, selectedIndexes()) {
            auto col = s.column();
            if (col < min) min = col;
            if (col > max) max = col;
        }
        if (ix < min || ix > max) {
            clearSelection();
            selectColumn(min = max = ix);
        }
        QMenu n;
        QAction select(tr("Select previous"), nullptr);
        if (min && columnWidth(min-1) < minimumColumnSize()) {
            n.addAction(&select);
        }
        QAction merge(tr("Merge"), nullptr);
        QAction split2(tr("Split to 2"), nullptr);
        QAction split3(tr("Split to 3"), nullptr);
        QAction split4(tr("Split to 4"), nullptr);
        QAction split5(tr("Split to 5"), nullptr);
        QAction split6(tr("Split to 6"), nullptr);
        QAction split8(tr("Split to 8"), nullptr);
        QList<QAction*> split;
        split.append(&merge);
        split.append(&split2);
        split.append(&split3);
        split.append(&split4);
        split.append(&split5);
        split.append(&split6);
        split.append(nullptr);
        split.append(&split8);
        auto l = 0;
        for (auto i = min; i <= max; ++i)
            l += m->headerData(i, Qt::Horizontal, EditorData::H::Length).toInt();
        auto length = tr("(Length = %1/%2)");
        auto q = m->property(EditorData::Quantized).toBool();
        auto b = m->property(EditorData::BarLength).toInt();
        auto qm = quantize_map(b);
        auto qi = qm.find(l), end = qm.end();
        if (!q || qi == end) {
            length = length.arg(l).arg(b);
        } else {
            auto d = qi->second.second < 0;
            length = length.arg(1+2*d).arg(((1+d)<<qi->second.first)*(qi->second.second > 0 ? qi->second.second : 1));
        }
        if (q) {
            auto b = qi != end && qi->second.second <= 0;
            merge.setEnabled(b);
            for (int i = 2; i <= split.count(); ++i)
                if (split[i-1])
                    split[i-1]->setEnabled(b && !(l%i) && qm.find(l/i) != end);
        }
        if (min < max)
            n.addAction(split[0]);
        n.addSeparator();
        for (int i = 1; i < split.count(); ++i)
            if (auto a = split[i])
                n.addAction(a);
        n.addSeparator();
        n.addAction(length)->setEnabled(false);
        if (auto a = n.exec(h->mapToGlobal(pos))){
            if (auto n = split.indexOf(a)+1) {
                CmdEditorGridFix::queue(this, min, max, std::vector<int>(n));
            } else {
                if (a == &select) {
                    for (auto w = minimumColumnSize(); min && columnWidth(min-1) < w; --min);
                    setRangeSelected(QTableWidgetSelectionRange(0, min, m->rowCount()-1, max), true);
                }
            }
        }
    });
    connect(v, &QWidget::customContextMenuRequested, [this, v](const QPoint& pos) {
        auto row = v->logicalIndexAt(pos);
        if (row < 0) return;
        auto m = model();
        auto note = m->headerData(row, Qt::Vertical, EditorData::V::Id).toInt();
        auto name = m->headerData(row, Qt::Vertical, EditorData::V::Unsupported).toBool() ? "*Not supported*" : m->headerData(row, Qt::Vertical, EditorData::V::Name).toString();
        QMenu n;
        n.addAction(tr("%1 - %2").arg(note).arg(name))->setDisabled(true);
        n.addSeparator();
        auto clr = n.addAction(tr("Clear all notes"));
        auto move = n.addMenu(tr("Move all notes to ..."));
        if (m->headerData(row, Qt::Vertical, EditorData::V::Count).toInt()) {
            for (int i = 0; i < m->rowCount(); ++i) {
                auto id = m->headerData(i, Qt::Vertical, EditorData::V::Id).toInt();
                auto name = m->headerData(i, Qt::Vertical, EditorData::V::Unsupported).toBool() ? "*Not supported*" : m->headerData(i, Qt::Vertical, EditorData::V::Name).toString();
                auto a = move->addAction(tr("%1 - %2").arg(id).arg(name));
                a->setProperty(EditorData::PlayerPosition, i);
                a->setDisabled(id == note);
            }
        } else {
            move->setTitle(tr("(No notes to move)"));
            move->setDisabled(true);
        }
        auto copy = n.addMenu(tr("Copy all notes to ..."));
        if (m->headerData(row, Qt::Vertical, EditorData::V::Count).toInt()) {
            for (int i = 0; i < m->rowCount(); ++i) {
                auto id = m->headerData(i, Qt::Vertical, EditorData::V::Id).toInt();
                auto name = m->headerData(i, Qt::Vertical, EditorData::V::Unsupported).toBool() ? "*Not supported*" : m->headerData(i, Qt::Vertical, EditorData::V::Name).toString();
                auto a = copy->addAction(tr("%1 - %2").arg(id).arg(name));
                a->setProperty(EditorData::PlayerPosition, i);
                a->setProperty(EditorData::Quantized, true);
                a->setDisabled(id == note);
            }
        } else {
            copy->setTitle(tr("(No notes to copy)"));
            copy->setDisabled(true);
        }
        n.addSeparator();
        auto hide = n.addAction(tr("Toggle instrument names"));
        if (auto a = n.exec(v->mapToGlobal(pos))) {
            if (a == hide) {
                v->toggleInstrumentNamesVisible();
            } else if (a == clr) {
                CmdEditorClearNotes::queue(this, row);
            } else if (a->property(EditorData::PlayerPosition).isValid()) {
                CmdEditorCopyMoveNotes::queue(this, a->property(EditorData::Quantized).toInt(), row, a->property(EditorData::PlayerPosition).toInt());
            }
        }
    });
    auto m = model();
    auto tt = tr("Click and hold to add a MIDI note.\n\n")+tr(
        "For editing - hold a mouse button, and\n"
        "- either move mouse up or down\n"
        "- or type the value directly.");
    connect(m, &QAbstractItemModel::rowsInserted, [m, tt](const QModelIndex& /*parent*/, int first, int last) {
        for (int row = first; row <= last; ++row)
            for (int col = m->columnCount()-1; col+1; --col)
                m->setData(m->index(row, col), tt, Qt::ToolTipRole);
    });
    connect(m, &QAbstractItemModel::columnsInserted, [m, tt](const QModelIndex& /*parent*/, int first, int last) {
        for (int col = first; col <= last; ++col)
            for (int row = m->rowCount()-1; row+1; --row)
                m->setData(m->index(row, col), tt, Qt::ToolTipRole);
    });
}


EditorTableWidget::~EditorTableWidget()
{
    delete mp_undoStack;
    auto d = itemDelegate();
    setItemDelegate(nullptr);
    delete d;
    delete m_editor;
}

std::set<int> EditorTableWidget::grid() const
{
    std::set<int> ret;
    auto m = model();
    for (int i = m->columnCount()-1; i+1; --i)
        ret.insert(m->headerData(i, Qt::Horizontal, EditorData::H::Tick).toInt());
    ret.insert(*ret.rbegin()+m->headerData(m->columnCount()-1, Qt::Horizontal, EditorData::H::Length).toInt());
    return ret;
}

int EditorTableWidget::minimumColumnSize() const
{ // TODO : maybe add to settings
    return 3;
}

void EditorTableWidget::enterEvent(QEvent*)
{
    setFocus();
}

void EditorTableWidget::leaveEvent(QEvent*)
{
    if (m_dont_leave) return;
    auto m = model();
    auto old = m->property(EditorData::Highlighted).toModelIndex();
    if (!old.isValid()) return;
    m->setProperty(EditorData::Highlighted, QVariant());
    for (int i = columnCount()-1; i+1; --i)
        m->setData(m->index(old.row(), i), QVariant(), EditorData::X::HighlightRow);
    m->setHeaderData(old.row(), Qt::Vertical, QVariant(), EditorData::V::Highlight);
    for (int i = rowCount()-1; i+1; --i)
        m->setData(m->index(i, old.column()), QVariant(), EditorData::X::HighlightColumn);
    m->setHeaderData(old.column(), Qt::Horizontal, QVariant(), EditorData::H::Highlight);
    m->setProperty(EditorData::MultiNoteSelector, QVariant());
}

void EditorTableWidget::keyPressEvent(QKeyEvent* event)
{
    // TODO : handle key events
    return QTableWidget::keyPressEvent(event);
}

void EditorTableWidget::mouseMoveEvent(QMouseEvent* event)
{
    auto m = model();
    auto old = m->property(EditorData::Highlighted).toModelIndex();
    auto cur = indexAt(event->pos());
    if (old == cur)
        return;
    m->setProperty(EditorData::Highlighted, cur);
    m->setProperty(EditorData::MultiNoteSelector, QVariant());
    if (old.row() != cur.row()) {
        for (int i = columnCount()-1; i+1; --i) {
            m->setData(m->index(old.row(), i), QVariant(), EditorData::X::HighlightRow);
            m->setData(m->index(cur.row(), i), true, EditorData::X::HighlightRow);
        }
        m->setHeaderData(old.row(), Qt::Vertical, QVariant(), EditorData::V::Highlight);
        m->setHeaderData(cur.row(), Qt::Vertical, true, EditorData::V::Highlight);
    }
    if (old.column() != cur.column()) {
        for (int i = rowCount()-1; i+1; --i) {
            m->setData(m->index(i, old.column()), QVariant(), EditorData::X::HighlightColumn);
            m->setData(m->index(i, cur.column()), true, EditorData::X::HighlightColumn);
        }
        m->setHeaderData(old.column(), Qt::Horizontal, QVariant(), EditorData::H::Highlight);
        m->setHeaderData(cur.column(), Qt::Horizontal, true, EditorData::H::Highlight);
    }
}

void EditorTableWidget::mousePressEvent(QMouseEvent* event)
{
    if (event->modifiers() & Qt::ControlModifier && event->button() == Qt::MiddleButton) {
        zoom(0);
        event->accept();
        return;
    }
    if (event->modifiers() == Qt::NoModifier) {
        auto ix = indexAt(event->pos());
        if (!ix.isValid())
            return;
        auto m = model();
        auto d = m->data(ix, EditorData::X::Notes).toMap();
        m->setData(ix, true, EditorData::X::Pressed);
        setCurrentCell(ix.row(), ix.column(), QItemSelectionModel::ClearAndSelect);
        int cur = 0, max = d.count(), vel = -1, ofs = -1;
        if (max && -- max) {
            auto sel = m->property(EditorData::MultiNoteSelector);
            auto dir = event->button() == Qt::LeftButton ? 1 : event->button() == Qt::RightButton ? -1 : 0;
            auto it = d.cbegin();
            for (auto i = cur = sel.isNull() ? dir+1 ? dir ? 0 : max/2 : max : (sel.toInt()+dir+d.count())%d.count(); i; --i)
                ++it;
            ofs = QVariant(it.key()).toInt();
            vel = it.value().toInt();
            m->setProperty(EditorData::MultiNoteSelector, cur);
        }
        if (vel == -1) {
            vel = m->data(ix, EditorData::X::Velocity).toInt();
        }
        m_dont_leave = true;
        int note = m_editor->exec(vel, event, cur, max);
        m_dont_leave = false;

        if (vel - note) { 
            CmdEditorNote::queue(this, ix, note, vel, ofs);
        }

        QMouseEvent fake(QEvent::MouseMove, viewport()->mapFromGlobal(QCursor::pos()), Qt::NoButton, QApplication::mouseButtons(), QApplication::keyboardModifiers());
        mouseMoveEvent(&fake);
        model()->setData(ix, QVariant(), EditorData::X::Pressed);
        event->accept();
        return;
    }
    return QTableWidget::mousePressEvent(event);
}

void EditorTableWidget::mouseReleaseEvent(QMouseEvent* event)
{
    model()->setData(model()->property(EditorData::Highlighted).toModelIndex(), QVariant(), EditorData::X::Pressed);
    return QTableWidget::mousePressEvent(event);
}

void EditorTableWidget::wheelEvent(QWheelEvent* event)
{
    auto pos = event->pos();
    QMouseEvent fake(QEvent::MouseMove, pos, Qt::NoButton, event->buttons(), event->modifiers());
    mouseMoveEvent(&fake);
    if (event->modifiers() & Qt::ControlModifier) {
        auto diff = 0.;
        auto p = event->pixelDelta();
        if (!p.isNull())
            diff = p.x() ? p.x() : p.y();
        else if (!(p = event->angleDelta()).isNull())
            diff = p.x() ? p.x() : p.y();
        if (diff /= 120) {
            auto hl = model()->property(EditorData::Highlighted).toModelIndex();
            if (hl.isValid()) {
                auto rc = visualRect(hl);
                QPointF scale(double(pos.x()-rc.x())/rc.width(), double(pos.y()-rc.y())/rc.height());
                m_resizer->setProperty(EditorData::Scale, scale);
                m_resizer->setProperty(EditorData::Highlighted, hl);
                m_resizer->setProperty(EditorData::PlayerPosition, pos);
            }
            zoom(diff);
            event->accept();
            return;
        }
    }
    if (!horizontalScrollBar()->isVisible())
        return QTableWidget::wheelEvent(event);
    QWheelEvent inverted(event->posF(), event->globalPosF(),
        event->pixelDelta(), event->angleDelta(), event->delta(),
        event->orientation() == Qt::Horizontal ? Qt::Vertical : Qt::Horizontal,
        event->buttons(), event->modifiers(), event->phase());
    QTableWidget::wheelEvent(&inverted);
    if (inverted.isAccepted())
        event->accept();
}

void EditorTableWidget::paintEvent(QPaintEvent* event)
{
    QTableWidget::paintEvent(event);
    if (!showGrid())
        return;
    const QPoint offset = dirtyRegionOffset();
    const auto gridSize = 1;
    const auto h = horizontalHeader();
    const auto v = verticalHeader();
    const bool rtl = isRightToLeft();
    const int x = h->length()-h->offset()-rtl;
    const int y = v->length()-v->offset()-1;
    int firstVisualColumn = h->visualIndexAt(0);
    int lastVisualColumn = h->visualIndexAt(h->viewport()->width());
    if (rtl)
        qSwap(firstVisualColumn, lastVisualColumn);
    if (firstVisualColumn == -1)
        firstVisualColumn = 0;
    if (lastVisualColumn == -1)
        lastVisualColumn = h->count() - 1;
    auto viewport = QTableWidget::viewport();
    QPainter painter(viewport);
    painter.setPen(Qt::red);
    foreach (auto rc, event->region().translated(offset).rects()) {
        auto pos = -1;
        rc.setBottom(qMin(rc.bottom(), y));
        if (rtl) {
            rc.setLeft(qMax(rc.left(), viewport->width()-x));
        } else {
            rc.setRight(qMin(rc.right(), x));
        }
        int left = h->visualIndexAt(rc.left());
        int right = h->visualIndexAt(rc.right());
        if (rtl)
            qSwap(left, right);
        if (left == -1) left = 0;
        if (right == -1) right = h->count() - 1;
        else if (right < columnCount()-1) ++right;
        for (int i = left; i <= right; ++i) {
            int col = h->logicalIndex(i);
            if (h->isSectionHidden(col))
                continue;
            int colp = columnViewportPosition(col);
            colp += offset.x();
            if (!rtl)
                colp += columnWidth(col) - gridSize;
            if (pos <= 0 || abs(pos-colp) > minimumColumnSize()) { pos = colp; continue; }
            else
                painter.drawLine(colp, rc.top(), colp, rc.bottom());
            if (pos != colp) {
                painter.drawLine(pos, rc.top(), pos, rc.bottom()); pos = colp;
            }
        }
    }
}

void EditorTableWidget::zoom(double step)
{
    auto m = model();
    if (!step) {
        m->setProperty(EditorData::Scale, QVariant());
    } else {
        auto os = m->property(EditorData::Scale).toDouble(), s = os;
        if (s < 0) s = 1-1/(1+s);
        s += step;

        if (s < 0) s = 1/(1-s)-1;
        m->setProperty(EditorData::Scale, s);
    }
    m_resizer->start();
}

void EditorTableWidget::changeLayout(SongPart* sp, std::set<int> grid, const MIDIPARSER_MidiTrack* new_data, const std::map<int, int>* qq)
{
    const MIDIPARSER_MidiTrack& data = new_data ? *new_data : sp->data();
    const std::map<int, int>& fix = qq && !qq->empty() ? *qq : sp->qq();
    auto m = model();
    std::map<int, int> notes, ticks;
    for (auto i = 0u; i < data.event.size(); ++i) {
        ++notes[data.event[i].note];
        auto t = fix.find(data.event[i].tick);
        ++ticks[t == fix.end() ? data.event[i].tick : t->second];
    }
    std::map<int, int> note_row;
    auto v = (EditorVerticalHeader*)verticalHeader();
    for (int i = m->rowCount()-1; !notes.empty(); --i) {
        auto note = notes.begin()->first;
        auto n = 0;
        m->setHeaderData(i, Qt::Vertical, QVariant(), EditorData::V::Count);
        if (i < 0 || (n = m->headerData(i, Qt::Vertical, EditorData::V::Id).toInt()) > note) {
            insertRow(++i);
            m->setHeaderData(i, Qt::Vertical, note, EditorData::V::Id);
            m->setHeaderData(i, Qt::Vertical, 1, EditorData::V::Unsupported);
            m->setHeaderData(i, Qt::Vertical, QBrush(Qt::red), Qt::ForegroundRole);
            v->decorate(i);
        } else if (n != note) {
            if (m->headerData(i, Qt::Vertical, EditorData::V::Unsupported).toInt())
                removeRow(i);
            continue;
        }
        notes.erase(note);
        note_row[note] = i;
    }
    if (!grid.size())
        foreach (auto t, ticks)
            grid.insert(t.first);
    for (auto t = 0; t <= data.nTick; t += data.barLength)
        grid.insert(t); // adding bar limits
    setColumnCount(0);
    for (int i = m->rowCount(); i --> 0; )
        m->setHeaderData(i, Qt::Vertical, QVariant(), EditorData::V::Count);
    setColumnCount(grid.size());
    auto k = 0, prev = 0;
    auto h = (EditorHorizontalHeader*)horizontalHeader();
    for (auto it = grid.begin(); it != grid.end(); ++it, ++k) {
        m->setHeaderData(k, Qt::Horizontal, QString("%1").arg(*it));
        m->setHeaderData(k, Qt::Horizontal, *it, EditorData::H::Tick);
        if (it != grid.begin()) {
            m->setHeaderData(k-1, Qt::Horizontal, *it - prev, EditorData::H::Length);
            h->decorate(k-1);
        }
        prev = *it;
    }
    m->setHeaderData(k-1, Qt::Horizontal, data.nTick - prev, EditorData::H::Length);
    h->decorate(k-1);
    std::map<int, int> tick_col;
    k = 0;
    auto g1 = grid.begin(), g2 = ++grid.begin();
    foreach (auto tick, ticks) {
        while (g2 != grid.end() && tick.first >= *g2) { g1 = g2++; ++k; }
        auto x = k + !(g2 == grid.end() || 20*(tick.first -* g1) <= 19*(*g2 -* g1));
        tick_col[tick.first] = x;
    }
    int afternote = 0;
    for (auto i = 0u; i < data.event.size(); ++i) {
        auto t = data.event[i].tick;
        auto tt = fix.find(t);
        auto tick = tt == fix.end() ? tick_col[t] : tick_col[tt->second];
        if (t == data.nTick && !afternote) afternote = 1;
        if (t > data.nTick) { if (afternote) afternote = -1; tick = data.nTick; }
        setNote(note_row[data.event[i].note], tick, data.event[i].vel, false);
    }
    if (!afternote)
        removeColumn(grid.size()-1);
    m->setProperty(EditorData::AfterNote, afternote);
    for (int i = m->rowCount()-1; i+1; --i)
        v->decorate(i);
    resizeRowsToContents();
    resizeColumnsToContents();
    auto nTick = m->property(EditorData::TickCount).toInt();
    if (data.nTick != nTick) {
        m->setProperty(EditorData::TickCount, data.nTick);
        emit sp->nTickChanged(data.nTick);
    }
    auto q = m->property(EditorData::Quantized).toBool();
    h->setMinimumHeight(make_quantized(m)*60);
    if (q != m->property(EditorData::Quantized).toBool())
        emit sp->quantizeChanged(!q);
    emit changed();
}

void EditorTableWidget::splitMergeColumns(int from, int to, std::vector<int>& parts)
{
    auto s = 0;
    if (!parts.size())
        parts.push_back(0);
    else for (auto p : parts)
        s += p;
    auto n = parts.size();
    auto m = model();
    auto t = m->headerData(from, Qt::Horizontal, EditorData::H::Tick).toInt();
    auto l = m->headerData(from, Qt::Horizontal, EditorData::H::Length).toInt();
    auto w = columnWidth(from);
    for (int i = to; i > from; --i) {
        l += m->headerData(i, Qt::Horizontal, EditorData::H::Length).toInt();
        w += columnWidth(i);
    }
    if (!s)
        if (auto v = l/n)
            for (auto& p : parts)
                s += (p = v);
    std::set<std::pair<int, int>> fix;
    for (int i = n-1; i+1; --i)
        fix.insert(std::make_pair(parts[i], i));
    if (auto d = s - l) {
        auto b = d > 0;
        for (auto i = fix.begin(); d; ++i == fix.end() ? i = fix.begin() : i, b ? --d : ++d)
            b ? ++parts[i->second] : --parts[i->second];
    }
    std::vector<std::list<std::pair<int,int>>> data(m->rowCount());
    const auto min = false, max = false;
    for (int i = data.size()-1; i+1; --i)
        for (int col = from-min; col <= to+max; ++col)
            foreach (auto note, m->data(m->index(i, col), EditorData::X::Notes).toMap().toStdMap())
                data[i].push_back(std::make_pair(QVariant(note.first).toInt(), note.second.toInt()));
    clearSelection();
    m_resize_block = true;
    m->removeColumns(from, to-from+1);
    m->insertColumns(from, n);
    to = from+n-1;
    auto h = (EditorHorizontalHeader*)horizontalHeader();
    auto k = (1+m->property(EditorData::Scale).toDouble())*viewport()->width()*.75/m->property(EditorData::BarLength).toInt();
    std::vector<int> ws(n);
    for (int i = from; i <= to; ++i) {
        auto v = parts[i-from], z = int(v*k+.5);
        ws[i-from] = z; w -= z;
        m->setHeaderData(i, Qt::Horizontal, QVariant(t).toString());
        m->setHeaderData(i, Qt::Horizontal, t, EditorData::H::Tick);
        m->setHeaderData(i, Qt::Horizontal, v, EditorData::H::Length);
        h->decorate(i);
        t += v;
    }
    if (w) {
        auto b = w > 0;
        for (auto i = fix.begin(); w; ++i == fix.end() ? i = fix.begin() : i, b ? --w : ++w)
            b ? ++ws[i->second] : --ws[i->second];
    }
    for (int i = ws.size()-1; i+1; --i)
        setColumnWidth(i+from, ws[i]);
    m_resize_block = false;
    setCurrentCell(0, from);
    setRangeSelected(QTableWidgetSelectionRange(0, from, m->rowCount()-1, to), true);
    std::vector<int> grid(n += min+max);
    from -= min; to += max;
    for (int i = from; i <= to; ++i)
        grid[i-from] = m->headerData(i, Qt::Horizontal, EditorData::H::Tick).toInt();
    for (int i = data.size()-1; i+1; --i) {
        const auto& notes = data[i];
        if (notes.empty()) continue;
        for (auto note : notes) {
            auto k = 0, diff = abs(note.first-grid[0]);
            for (auto j = n-1; j > 0; --j) if (auto d = abs(note.first-grid[j])) if (d < diff) { k = j; diff = d; } else; else { k = j; diff = 0; break; }
            setNote(i, from+k, note.second, false, note.first);
        }
    }
    h->setMinimumHeight(make_quantized(m)*60);
    emit changed();
}

void EditorTableWidget::removeColumn(int col)
{
    auto m = model();
    for (int i = 0; i < rowCount(); ++i)
        if (note(i, col)>-1)
            m->setHeaderData(i, Qt::Vertical, m->headerData(i, Qt::Vertical, EditorData::V::Count).toInt()-1, EditorData::V::Count);
    QTableWidget::removeColumn(col);
}

void EditorTableWidget::resizeColumnsToContents()
{
    hide();
    m_resizer->start();
}

int EditorTableWidget::note(int row, int col) const
{
    QVariant v = model()->index(row, col).data(EditorData::X::Velocity);
    if (v.isValid())
        return v.toInt();
    return -1;
}

void EditorTableWidget::setNote(int row, int col, int vel, bool count, int tick)
{
    auto m = model();
    auto i = m->index(row, col);
    auto d = m->data(i);
    auto x = m->data(i, EditorData::X::Velocity);
    auto s = m->data(i, EditorData::X::Notes).toMap();
    auto c = s.size();
    if (tick < 0) tick = c ? s.lastKey().toInt() : m->headerData(col, Qt::Horizontal, EditorData::H::Tick).toInt();
    
    if (vel<0)
        s.remove(QVariant(tick).toString());
    else
        s[QVariant(tick).toString()] = vel;
    auto z = s.size();
#ifdef QT_DEBUG // this allows to verify quantization algorithm never merges two adjacent notes of any single instrument
    if (z > 1) {
        auto inst = m->headerData(row, Qt::Vertical, EditorData::V::Id).toInt();
        tick += inst-inst; // set breakpoint here to see whether any notes were merged
    }
#endif // QT_DEBUG
    m->setData(i, s , EditorData::X::Notes);
    auto n = vel<0 ? QVariant() : QVariant(vel);

    m->setData(i, n, EditorData::X::Velocity);
    auto tt = tr(
        "For editing - hold a mouse button, and\n"
        "- either move mouse up or down\n"
        "- or type the value directly.");
    if (!n.isValid())
        tt = tr("Click and hold to add a MIDI note.\n\n")+tt;
    else
        tt = tr("Instrument: %1\nVelocity: %2\n\n").arg(m->headerData(row, Qt::Vertical, EditorData::V::Name).toString()).arg(vel)+tt;
    if (s.size() > 1)
        tt += tr("\n\nWarning:\nThis cell contains %1 notes\nConsider splitting this column!").arg(s.size());
    m->setData(i, tt, Qt::ToolTipRole);
    if (auto t = !!z-!!c)
        m->setHeaderData(row, Qt::Vertical, m->headerData(row, Qt::Vertical, EditorData::V::Count).toInt()+t, EditorData::V::Count);
    if (count && x != n) {
        ((EditorVerticalHeader*)verticalHeader())->decorate(row);
        emit changed();
    }
}

void EditorTableWidget::dropUnusedUnsupportedRows()
{
    auto m = model();
    for (int i = rowCount()-1; i+1; --i)
        if (m->headerData(i, Qt::Vertical, EditorData::V::Unsupported).toBool() && !m->headerData(i, Qt::Vertical, EditorData::V::Count).toInt())
            removeRow(i);
}

MIDIPARSER_MidiTrack EditorTableWidget::collect() const
{
    MIDIPARSER_MidiTrack ret;
    auto m = model();
    auto ts = m->property(EditorData::TimeSignature).toPoint();
    ret.timeSigNum = ts.x();
    ret.timeSigDen = ts.y();
    ret.barLength = m->property(EditorData::BarLength).toInt();
    ret.nTick = m->property(EditorData::TickCount).toInt();
    ret.bpm = m->property(EditorData::Tempo).toInt();
    auto rows = rowCount()-1;
    for (int i = 0; i < columnCount(); ++i) {
        const auto tick = int(m->headerData(i, Qt::Horizontal, EditorData::H::Tick).toInt());
        for (int j = rows; j+1; --j) {
            auto id = m->headerData(j, Qt::Vertical, EditorData::V::Id).toInt();
            auto n = note(j, i);
            if (n>-1) {
                ret.event.push_back(MIDIPARSER_MidiEvent(tick, id, n));
            }
        }
    }
    qDebug() << "collected" << ret.event.size() << "ts:" << ret.timeSigNum << ret.timeSigDen << "bpm:" << ret.bpm << "ntick:" << ret.nTick;
    return ret;
}
