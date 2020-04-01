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
#include "DrumsetLabel.h"

DrumsetLabel::DrumsetLabel(const QString& text, QWidget * parent)
    : BigTextClickableLabelWithErrorIndicator(text,parent)
{
    this->setContentsMargins(10,0,0,0);
    this->setMinimumWidth(125);
}
