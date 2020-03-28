#ifndef EDITORCELLDELEGATE_H
#define EDITORCELLDELEGATE_H

#include <QtWidgets>
#include <QStyledItemDelegate>

#include <midi_editor/editor.h>
#include <midi_editor/editordata.h>
#include <midi_editor/editorvelocitycolorizer.h>

class EditorCellDelegate
    : public QStyledItemDelegate
{
    const EditorVelocityColorizer m_colorizer;

public:
    EditorCellDelegate(EditorTableWidget* parent = nullptr);
    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const;

private:
    QRadialGradient createGradientFromCellState(QRect cellRectangle, QVariant velData, const QModelIndex& index) const;
    QColor pickCellColorForState(const QModelIndex& index, bool unreasonablySmallRect = false) const;
    static const QColor played;
    static const QColor selected;
    static const QColor hover;
    static const QColor nonHoverHighlight;
    static const QColor selectedInParent;
    static const QColor unsupported;

};

#endif // EDITORCELLDELEGATE_H
