#ifndef EDITORCOLUMNSCOLORIZER_H
#define EDITORCOLUMNSCOLORIZER_H

#include <QColor>
#include <QtCore>
#include <midi_editor/editordata.h>

class EditorColumnsColorizer
{
public:
    QList<QRgb> colors;

    EditorColumnsColorizer()
    { // gray scale scheme
        colors.append(qRgb(127,127,127)); colors.append(qRgb(192,192,192));
    }

    QRgb colorForHeader(int ofs, QAbstractItemModel* m) const
    {
        auto bar = m->property(EditorData::BarLength).toInt();
        int color_index = (ofs/bar)%(colors.size());
        if(colors.size()>2){
           color_index = (ofs/bar)%(colors.size()/2)*2;
        }
        return colors[color_index];
    }

    QRgb colorForCell(int ofs, QAbstractItemModel* m) const
    {
        auto bar = m->property(EditorData::BarLength).toInt();
        auto part = m->property(EditorData::TimeSignature).toPoint().x();
        return colors[(ofs/bar)%(colors.size()/2)*2+(ofs*part/bar)%2];
    }
};


#endif // EDITORCOLUMNSCOLORIZER_H
