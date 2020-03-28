#ifndef EDITORVELOCITYCOLORIZER_H
#define EDITORVELOCITYCOLORIZER_H

#include <QGradientStops>

class EditorVelocityColorizer
{
public:
    QGradientStops data;
    EditorVelocityColorizer();
    QColor colorFor(int velocity) const;
};
#endif // EDITORVELOCITYCOLORIZER_H
