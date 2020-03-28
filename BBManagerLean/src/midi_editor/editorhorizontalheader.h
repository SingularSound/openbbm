#ifndef EDITORHORIZONTALHEADER_H
#define EDITORHORIZONTALHEADER_H

#include <QHeaderView>
#include <QObject>
#include <QtWidgets>

#include <midi_editor/editor.h>
#include <midi_editor/editorcolumnscolorizer.h>
#include <midi_editor/util.h>

class EditorHorizontalHeader : public QHeaderView
{
    EditorColumnsColorizer m_colorizer;
    int m_hl;

protected:
    void enterEvent(QEvent* event);

    void leaveEvent(QEvent*);

    void mouseMoveEvent(QMouseEvent* e);

    void paintSection(QPainter* painter, const QRect& rc, int ix) const;

public:
    explicit EditorHorizontalHeader(QTableWidget* parent);

    void decorate(int column);

    void colorize(int column = -1);

    void colorize(const QList<QRgb>& colors);
};
#endif // EDITORHORIZONTALHEADER_H
