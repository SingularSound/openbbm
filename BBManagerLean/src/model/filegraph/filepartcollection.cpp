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
#include "filepartcollection.h"

FilePartCollection::FilePartCollection()
{
   m_InternalSize = 0;
   m_Default_Name = tr("FilePartCollection");
   m_Name = m_Default_Name;
}

uint32_t FilePartCollection::maxInternalSize()
{
   return 0;
}

uint32_t FilePartCollection::minInternalSize()
{
   return 0;
}

uint8_t *FilePartCollection::internalData()
{
   return nullptr;
}
