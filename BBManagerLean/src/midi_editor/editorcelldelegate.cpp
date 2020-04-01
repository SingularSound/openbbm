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
#include <midi_editor/editorcelldelegate.h>

EditorCellDelegate::EditorCellDelegate(EditorTableWidget* parent) :
    QStyledItemDelegate(parent)
{}

const QColor EditorCellDelegate::played = Qt::green;
const QColor EditorCellDelegate::selected = Qt::cyan;
const QColor EditorCellDelegate::hover = Qt::darkBlue;
const QColor EditorCellDelegate::nonHoverHighlight = Qt::black;
const QColor EditorCellDelegate::selectedInParent = Qt::blue;
const QColor EditorCellDelegate::unsupported = Qt::red;

QRadialGradient EditorCellDelegate::createGradientFromCellState(const QRect cellRectangle, const QVariant velData, const QModelIndex& index) const
{
    auto isCellPressed = index.data(EditorData::X::Pressed).toBool();

    QRadialGradient fill(cellRectangle.center(), (cellRectangle.width()+2*cellRectangle.height())/3.*.8);
    fill.setColorAt(isCellPressed, m_colorizer.colorFor(velData.toInt()));
    fill.setColorAt(!isCellPressed, pickCellColorForState(index));
    return fill;
}

QColor EditorCellDelegate::pickCellColorForState(const QModelIndex& index, bool unreasonablySmallRect) const
{
    auto parentWidget = static_cast<EditorTableWidget*>(parent());

    auto columnNeedsHighlight = index.data(EditorData::X::HighlightColumn).toBool();
    auto rowNeedsHighlight = index.data(EditorData::X::HighlightRow).toBool();
    auto columnAndRowBothNeedHighlight = columnNeedsHighlight && rowNeedsHighlight;
    auto isCellSelected = parentWidget->selectionModel()->isSelected(index);

    bool normalRectAndNeedsOneHighlight = (!unreasonablySmallRect) && (columnNeedsHighlight || rowNeedsHighlight);
    bool smallRectNonZeroVelocity = (unreasonablySmallRect && index.data(EditorData::X::Velocity).toInt());

    if(index.data(EditorData::X::Played).toBool()){
        return played;
    }else if((parentWidget->currentIndex() == index)){
        return selected;
    }else if(columnAndRowBothNeedHighlight){
        return hover;
    }else if(normalRectAndNeedsOneHighlight || smallRectNonZeroVelocity){
        return nonHoverHighlight;
    }else if(isCellSelected){
        return selectedInParent;
    }else{
        return QColor(index.data(Qt::BackgroundRole).toUInt());
    }
}

void EditorCellDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
    {
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing);

    auto parentWidget = static_cast<EditorTableWidget*>(parent());
    auto parentWidgetModel = parentWidget->model();

    auto v = index.data(EditorData::X::DrawNumber).toBool();
    auto notesInCell = index.data(EditorData::X::Notes).toMap();
    auto cellRectangle = option.rect;

    if (cellRectangle.width() < parentWidget->minimumColumnSize()) {
        painter->fillRect(cellRectangle, pickCellColorForState(index, true));
    } else {

        if (notesInCell.size() > 1) {
            painter->setPen(QPen(Qt::magenta, 4));
        } else if (!index.data(EditorData::X::DrawBorder).toBool()) {
            painter->setPen(Qt::NoPen);
        }

        QVariant velData = index.data(EditorData::X::Velocity);
        painter->setBrush(createGradientFromCellState(cellRectangle, velData, index));
        painter->drawRoundedRect(cellRectangle, 4, 4);

        auto cellVelocityText = index.data().toString();
        if (!cellVelocityText.isNull() || (velData.isValid() && v)) {
            if (cellVelocityText.isNull()) {
                cellVelocityText = QVariant(velData.toInt()).toString();

                bool rowInstrumentIsSupported = !parentWidgetModel->headerData(index.row(), Qt::Vertical, EditorData::V::Unsupported).toBool();

                const QColor velocityTextColor = rowInstrumentIsSupported ? nonHoverHighlight : unsupported;

                painter->setPen(velocityTextColor);
                painter->drawText(cellRectangle, cellVelocityText, QTextOption(Qt::AlignCenter));
            }
        }
    }
    painter->restore();
}
