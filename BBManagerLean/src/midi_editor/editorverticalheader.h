#ifndef EDITORVERTICALHEADER_H
#define EDITORVERTICALHEADER_H

#include <QtWidgets>

#include <midi_editor/editordata.h>
#include <midi_editor/editor.h>

class EditorVerticalHeader : public QHeaderView
{
    bool m_short;
    int m_hl;

protected:
    void enterEvent(QEvent* event);
    void leaveEvent(QEvent*);

    void mouseMoveEvent(QMouseEvent* e);

    void paintSection(QPainter* painter, const QRect& rc, int ix) const;

public:
    explicit EditorVerticalHeader(EditorTableWidget* parent);
    void toggleInstrumentNamesVisible();
    void decorate(int row);

#ifdef Q_OS_OSX
    /*
     * There's a bug in Qt for MacOS where text size is underestimated.
     * The size is underestimated by about 10%, so here we are, increasing it in about 10%
     * so that the text won't be multilined, then vertically clipped.
     */
    QSize sectionSizeFromContents(int logicalIndex) const {
        QSize computedSectionSize = QHeaderView::sectionSizeFromContents(logicalIndex);
        return computedSectionSize.scaled(static_cast<int>(computedSectionSize.width()*1.1),computedSectionSize.height(),Qt::AspectRatioMode::IgnoreAspectRatio);
     }
#endif

};

#endif // EDITORVERTICALHEADER_H
