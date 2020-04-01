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
#include <midi_editor/editorhorizontalheader.h>

void EditorHorizontalHeader::enterEvent(QEvent *event)
{
    auto p = static_cast<EditorTableWidget*>(parent());
    p->setSelectionMode(QAbstractItemView::SelectionMode::MultiSelection);
    p->leaveEvent(event);
}

void EditorHorizontalHeader::leaveEvent(QEvent *)
{
    auto p = static_cast<EditorTableWidget*>(parent());
    p->setSelectionMode(QAbstractItemView::SelectionMode::SingleSelection);
    if (m_hl+1) {
        p->model()->setHeaderData(m_hl, Qt::Horizontal, QVariant(), EditorData::H::Highlight);
        m_hl = -1;
    }
}

void EditorHorizontalHeader::mouseMoveEvent(QMouseEvent *e)
{
    int ix = logicalIndexAt(e->pos());
    if (ix != m_hl) {
        auto p = static_cast<EditorTableWidget*>(parent());
        auto m = p->model();
        if (m_hl+1) {
            m->setHeaderData(m_hl, Qt::Horizontal, QVariant(), EditorData::H::Highlight);
        }
        if (ix >= 0 && ix < count()) {
            m->setHeaderData(m_hl = ix, Qt::Horizontal, true, EditorData::H::Highlight);
        } else {
            m_hl = -1;
        }
    }
    return QHeaderView::mouseMoveEvent(e);
}

void EditorHorizontalHeader::paintSection(QPainter *painter, const QRect &rc, int ix) const
{
    auto p = static_cast<EditorTableWidget*>(parent());
    auto m = p->model();
    auto h = m->headerData(ix, Qt::Horizontal, EditorData::H::Highlight).toBool();
    auto l = m->headerData(ix, Qt::Horizontal, EditorData::H::Played).toBool();
    auto t = m->headerData(ix, Qt::Horizontal, EditorData::H::Tick).toInt();
    auto q = m->headerData(ix, Qt::Horizontal, EditorData::H::Quantized);
    auto x = m->headerData(ix, Qt::Horizontal).toString(); if (x.isNull()) x = QVariant(t).toString();
    auto c = p->currentIndex().column() == ix;
    auto s = p->selectionModel()->isColumnSelected(ix, QModelIndex());
    auto w = rc.width();
    auto z = w < p->minimumColumnSize();
    auto a = !!(painter->renderHints() & QPainter::Antialiasing); // save for later
    auto pen = painter->pen(); // save for later
    auto brush = painter->brush(); // save for later
    QBrush bg(Qt::NoBrush);
    auto bgr = m->headerData(ix, Qt::Horizontal, Qt::BackgroundRole);
    if (bgr.canConvert<QBrush>()) bg = qvariant_cast<QBrush>(bgr);
    painter->fillRect(rc.adjusted(0, 0, 1, 1), !z ? bg : l ? Qt::green : h ? Qt::black : s ? Qt::blue : Qt::red);
    if (z) return;
    bgr = m->headerData(ix, Qt::Horizontal, Qt::ForegroundRole);
    if (bgr.canConvert<QBrush>()) {
        painter->fillRect(QRect(rc.left(), rc.bottom()-rc.height()/4, rc.right()+1, rc.bottom()+1), qvariant_cast<QBrush>(bgr));
    }
    if (h || l) {
        painter->setPen(h ? Qt::cyan : Qt::green);
        painter->setBrush(Qt::NoBrush);
        painter->setRenderHint(QPainter::Antialiasing);
        painter->drawRoundedRect(rc.adjusted(1, 1, -1, -1), 4, 4);
        painter->setRenderHint(QPainter::Antialiasing, false);
    }
    auto fg = QPen(l ? Qt::green : c ? Qt::cyan : s ? Qt::blue : Qt::white);
    painter->setPen(fg);
    painter->setBrush(Qt::NoBrush);
    auto rcx = rc;
    if (q.isValid()) {
        auto qq = q.toPoint();
        x.clear();
        auto draw_note = w > 10;
        auto draw_tuplet = 0;
        QPoint center(rc.left()+rc.width()/2, rc.top()+3*rc.height()/4), top(center.x(), rc.top()+rc.height()/4), slash(top), ofs(0, 5); slash += QPoint(7, 11);
        (draw_note ? top.rx() : center.ry()) += 5;
        if (int tuplet = char(qq.y())) {
            if (tuplet > 0) {
                if (int pos = char(qq.y() >> 8)) {
                    auto mid = tuplet/2+1;
                    rcx = rc.adjusted(0, 0, 0, -3*rc.height()/4);
                    if (draw_note) rcx.adjust(5, 0, 5, 0);
                    if (pos == mid) {
                        x = QVariant(tuplet).toString();
                        draw_tuplet = -1;
                    } else {
                        draw_tuplet = 2*abs(pos-mid) == tuplet-1 ? 1+(pos>mid) : -2;
                    }
                } else {
                    x = QVariant(tuplet).toString();
                    rcx = rc.adjusted(0, 0, -rc.width()/2, -rc.height()/2);
                }
            } else {
                painter->drawPoint(center.x(), top.ry()+3+5*draw_note);
            }
        }
        if (draw_tuplet) {
            auto p = rcx.center();
            if (draw_tuplet > 0) {
                painter->drawLine(top.x(), p.y(), top.x(), (p.y()+top.y())/2);
                painter->drawLine(top.x(), p.y(), draw_tuplet-1 ? rc.left() : rc.right(), p.y());
            } else if (draw_tuplet < -1) {
                painter->drawLine(rc.left(), p.y(), rc.right(), p.y());
            } else {
                painter->drawLine(rc.left(), p.y(), p.x()-5, p.y());
                painter->drawLine(rc.right(), p.y(), p.x()+5, p.y());
            }
        }
        int power = qq.x();
        if (power --> 0) {
            painter->drawLine(top.x(), center.y()-1, top.x(), top.y());
        }
        if (draw_note) {
            fg.setWidth(2);
            painter->setPen(fg);
            painter->setRenderHint(QPainter::Antialiasing);
            if (power --> 0) {
                painter->setBrush(fg.color());
            } else {
                painter->drawEllipse(center, 4, 3);
                painter->drawEllipse(center, 3, 3);
            }
            painter->drawEllipse(center, 5, 3);
            painter->setBrush(Qt::NoBrush);
            painter->setRenderHint(QPainter::Antialiasing, false);
        } else {
            --power;
        }
        int from = char(qq.y() >> 16);
        int to = char(qq.y() >> 24);
        if (from || to) {
            fg.setWidth(3);
            painter->setPen(fg);
            auto dir = from > to;
            while (from > 0 && power > 0 && to > 0) {
                painter->drawLine(rc.left()+1, top.y(), rc.right(), top.y());
                top += ofs;
                --from; --power; --to;
            }
            while (from > 0 && power > 0) {
                painter->drawLine(rc.left()+1, top.y(), top.x()-1, top.y());
                top += ofs;
                --from; --power;
            }
            while (power > 0 && to > 0) {
                painter->drawLine(top.x()+1, top.y(), rc.right(), top.y());
                top += ofs;
                --power; --to;
            }
            while (power --> 0) {
                if (dir)
                    painter->drawLine(rc.left()+1, top.y(), top.x()-1, top.y());
                else
                    painter->drawLine(top.x()+1, top.y(), rc.right(), top.y());
            }
            painter->setPen(fg.color());
        } else {
            while (power --> 0) {
                painter->setPen(fg.color());
                if (draw_note) {
                    painter->drawLine(top.x(), top.y(), slash.x(), slash.y());
                } else {
                    painter->drawLine(rc.left()+1, top.y(), rc.right(), top.y());
                }
                top += ofs;
                slash += ofs;
            }
        }
    }
    if (!x.isNull()) {
        QTextOption o;
        o.setAlignment(Qt::AlignCenter);
        painter->drawText(rcx, x, o);
    }
    painter->setRenderHint(QPainter::Antialiasing, a); // restore
    painter->setBrush(brush); // restore
    painter->setPen(pen); // restore
}

EditorHorizontalHeader::EditorHorizontalHeader(QTableWidget *parent)
    : QHeaderView(Qt::Horizontal, parent)
    , m_hl(-1)
{
    setSectionsClickable(true);
    setHighlightSections(true);
    connect(this, &QHeaderView::sectionCountChanged, [this](int oldCount, int newCount) {

        (void)oldCount;
        auto parent = static_cast<EditorTableWidget*>(parentWidget());
        for (int i = 0; i < newCount; ++i)
            if (!parent->horizontalHeaderItem(i))
                parent->setHorizontalHeaderItem(i, new QTableWidgetItem);
    });
    connect(this, &QHeaderView::sectionEntered, [parent](int ix) { parent->model()->setHeaderData(ix, Qt::Horizontal, true, EditorData::H::Highlight); });
}

void EditorHorizontalHeader::decorate(int column)
{
    auto m = model();
    QVariant ttr;
    if (!m->property(EditorData::Quantized).toBool()) {
        // set tooltip only for not quantized patterns
        auto t = m->headerData(column, Qt::Horizontal, EditorData::H::Tick).toInt();
        auto l = m->headerData(column, Qt::Horizontal, EditorData::H::Length).toInt();
        auto b = m->property(EditorData::BarLength).toInt();
        QString tt = tr("Offset = %1\n").arg(t);
        tt += tr("Length = %1\n").arg(l);
        auto qq = quantize_opt(double(l)/b);
        for (auto it = qq.begin(); it != qq.end(); ++it) {
            auto n = it->first;
            auto q = it->second;
            auto e = int(q.second*b+.5);
            tt += tr("\n%1/%2%3%4").arg(q.first.first).arg(q.first.second)
                    .arg(n == 3 ? " triplet" : n == 5 ? " quintuplet" : "")
                    .arg(e ? tr("  (error = %1)").arg(e) : "");
        }
        ttr = tt;
    }
    m->setHeaderData(column, Qt::Horizontal, ttr, Qt::ToolTipRole);
    colorize(column);
}

void EditorHorizontalHeader::colorize(int column)
{
    auto m = model();
    auto b = m->property(EditorData::ButtonStyle);
    auto s = m->property(EditorData::ShowVelocity);
    for (int col = column+1 ? column : m->columnCount()-1; column+1 ? col==column : col+1; --col) {
        auto tick = m->headerData(col, Qt::Horizontal, EditorData::H::Tick).toInt();
        m->setHeaderData(col, Qt::Horizontal, QBrush(m_colorizer.colorForHeader(tick, m)), Qt::BackgroundRole);
        auto color = m_colorizer.colorForCell(tick, m);
        m->setHeaderData(col, Qt::Horizontal, QBrush(color), Qt::ForegroundRole);
        for (int row = m->rowCount()-1; row+1; --row) {
            m->setData(m->index(row, col), color, Qt::BackgroundRole);
            m->setData(m->index(row, col), b, EditorData::X::DrawBorder);
            m->setData(m->index(row, col), s, EditorData::X::DrawNumber);
        }
    }
}

void EditorHorizontalHeader::colorize(const QList<QRgb> &colors)
{
    m_colorizer.colors = colors.empty() ? EditorColumnsColorizer().colors : colors;
    colorize();
}
