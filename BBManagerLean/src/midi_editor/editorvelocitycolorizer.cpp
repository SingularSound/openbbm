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
