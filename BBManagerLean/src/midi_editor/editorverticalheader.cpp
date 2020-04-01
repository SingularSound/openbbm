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
#include <midi_editor/editorverticalheader.h>

void EditorVerticalHeader::enterEvent(QEvent *event) {
    static_cast<EditorTableWidget*>(parentWidget())->leaveEvent(event);
}

void EditorVerticalHeader::leaveEvent(QEvent *)
{
    if (m_hl+1) {
        static_cast<EditorTableWidget*>(parentWidget())->model()->setHeaderData(m_hl, Qt::Vertical, QVariant(), EditorData::V::Highlight);
        m_hl = -1;
    }
}

void EditorVerticalHeader::mouseMoveEvent(QMouseEvent *e)
{
    int ix = logicalIndexAt(e->pos());
    if (ix != m_hl) {
        auto p = static_cast<EditorTableWidget*>(parentWidget());
        auto m = p->model();
        if (m_hl+1) {
            m->setHeaderData(m_hl, Qt::Vertical, QVariant(), EditorData::V::Highlight);
        }
        if (ix >= 0 && ix < count()) {
            m->setHeaderData(m_hl = ix, Qt::Vertical, true, EditorData::V::Highlight);
        } else {
            m_hl = -1;
        }
    }
    return QHeaderView::mouseMoveEvent(e);
}

void EditorVerticalHeader::paintSection(QPainter *painter, const QRect &rc, int ix) const
{
    painter->save();

    auto parent = static_cast<EditorTableWidget*>(parentWidget());
    auto editorTableModel = parent->model();
    auto headerCellTextContent = editorTableModel->headerData(ix, Qt::Vertical).toString();

    if (headerCellTextContent.isNull()) {
        auto headerCellId = editorTableModel->headerData(ix, Qt::Vertical, EditorData::V::Id).toInt();
        auto instrumentName = editorTableModel->headerData(ix, Qt::Vertical, EditorData::V::Name).toString();

        headerCellTextContent = m_short ? QVariant(headerCellId).toString() : tr("%1 - %2").arg(headerCellId).arg(instrumentName);
    }
    QBrush bg(Qt::gray);
    auto bgr = editorTableModel->headerData(ix, Qt::Vertical, Qt::BackgroundRole);
    if (bgr.canConvert<QBrush>()) {
        bg = qvariant_cast<QBrush>(bgr);
    }
    QBrush fg(Qt::darkGray);
    bgr = editorTableModel->headerData(ix, Qt::Vertical, Qt::BackgroundColorRole);
    if (bgr.canConvert<QBrush>()) {
        fg = qvariant_cast<QBrush>(bgr);
    }

    painter->setRenderHint(QPainter::Antialiasing);
    painter->fillRect(rc, editorTableModel->headerData(ix, Qt::Vertical, EditorData::V::Count).toInt() ? fg : bg);

    auto needsHighlight = editorTableModel->headerData(ix, Qt::Vertical, EditorData::V::Highlight).toBool();
    if (needsHighlight) {
        painter->setPen(Qt::cyan);
        painter->setBrush(Qt::NoBrush);
        painter->drawRoundedRect(rc, 4, 4);
    }
    if (!headerCellTextContent.isNull()) {
        auto isCurrentHeaderCellPartOfMultiSelectInParent = false;
        foreach (auto sel, parent->selectedIndexes()) {
            if (sel.row() == ix) {
                isCurrentHeaderCellPartOfMultiSelectInParent = true;
                break;
            }
        }

        auto isUnsupported = editorTableModel->headerData(ix, Qt::Vertical, EditorData::V::Unsupported).toBool();
        auto isCurrentHeaderCellUniquelySelectedInParent = parent->currentIndex().row() == ix;

        painter->setPen(isUnsupported ? Qt::red : isCurrentHeaderCellUniquelySelectedInParent ? Qt::cyan : isCurrentHeaderCellPartOfMultiSelectInParent ? Qt::blue : Qt::black);
        painter->drawText(rc.adjusted(4, 0, -4, 0), headerCellTextContent, QTextOption((parent->isLeftToRight() ? Qt::AlignLeft : Qt::AlignRight) | Qt::AlignVCenter));
    }

    painter->restore();
}

EditorVerticalHeader::EditorVerticalHeader(EditorTableWidget *parent)
    : QHeaderView(Qt::Vertical, parent)
    , m_short(false)
    , m_hl(-1)
{
    setSectionsClickable(true);
    setHighlightSections(true);
    setSectionResizeMode(QHeaderView::Fixed);
    connect(this, &QHeaderView::sectionCountChanged, [this](int oldCount, int newCount) {

        (void)oldCount;
        auto parent = static_cast<EditorTableWidget*>(parentWidget());
        for (int i = 0; i < newCount; ++i){
            if (!parent->verticalHeaderItem(i)){
                parent->setVerticalHeaderItem(i, new QTableWidgetItem);
            }
        }
    });
}

void EditorVerticalHeader::toggleInstrumentNamesVisible()
{
    auto parent = static_cast<EditorTableWidget*>(parentWidget());
    m_short = !m_short;
    for (int i = parent->rowCount()-1; i+1; --i){
        decorate(i);
    }
}

void EditorVerticalHeader::decorate(int row)
{
    auto m = model();
    auto note = m->headerData(row, Qt::Vertical, EditorData::V::Id);
    auto name = m->headerData(row, Qt::Vertical, EditorData::V::Unsupported).toBool() ? "*Not supported*" : m->headerData(row, Qt::Vertical, EditorData::V::Name).toString();
    auto cnt = m->headerData(row, Qt::Vertical, EditorData::V::Count).toInt();
    m->setHeaderData(row, Qt::Vertical, m_short ? note.toString() : tr("%1 - %2").arg(note.toString()).arg(name));

    auto tt = tr("%1\n***\nMIDI note ID : %2").arg(name).arg(note.toString());
    if (cnt) {
        tt += tr("\nNotes total: %1").arg(cnt);
        auto n = 0;
        for (int i = m->columnCount()-1; i+1; --i){
            if (m->data(m->index(row, i), EditorData::X::Velocity).toInt()){
                ++n;
            }
        }
        if (n != cnt){
            tt += tr("\nNotes visible: %1").arg(n);
        }
    }
    m->setHeaderData(row, Qt::Vertical, tt, Qt::ToolTipRole);
}
