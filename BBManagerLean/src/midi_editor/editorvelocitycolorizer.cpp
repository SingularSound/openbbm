#include "editorvelocitycolorizer.h"

EditorVelocityColorizer::EditorVelocityColorizer()
{
    data.append(QGradientStop(0/128.,   qRgb(255, 255, 255)));
    data.append(QGradientStop(1/128.,   qRgb(192, 255, 192)));
    data.append(QGradientStop(64/128.,  qRgb(0, 255, 0)));
    data.append(QGradientStop(100/128., qRgb(255, 255, 0)));
    data.append(QGradientStop(128/128., qRgb(255, 0, 0)));
}

QColor EditorVelocityColorizer::colorFor(int velocity) const
{
    auto v = velocity/128.;
    if (v <= data.front().first)
        return data.front().second;
    for (int i = 1; i < data.size(); ++i)
        if (v < data[i].first) {
            auto k = (v-data[i-1].first)/(data[i].first-data[i-1].first);
            auto a = data[i-1].second, b = data[i].second;
            return QColor
                ( a.red()+static_cast<int>(k*(b.red()-a.red()))
                , a.green()+static_cast<int>(k*(b.green()-a.green()))
                , a.blue()+static_cast<int>(k*(b.blue()-a.blue()))
                );
        }
    return data.back().second;
}
