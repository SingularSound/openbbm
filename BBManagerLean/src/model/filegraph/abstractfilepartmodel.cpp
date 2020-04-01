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
#include <QDebug>

#include "abstractfilepartmodel.h"

AbstractFilePartModel::AbstractFilePartModel():
   QObject()
{
   m_Default_Name = tr("NO_NAME");
   m_Name = m_Default_Name;
   m_InternalSize = 0;

   mp_SubParts = new QList<AbstractFilePartModel *>;
}

AbstractFilePartModel::~AbstractFilePartModel()
{
   for(int i = mp_SubParts->size()-1; i >= 0; i--){
      delete mp_SubParts->at(i);
   }
   delete mp_SubParts;
}

AbstractFilePartModel *AbstractFilePartModel::childAt(int index)
{
   return mp_SubParts->at(index);
}

void AbstractFilePartModel::append(AbstractFilePartModel * child)
{
   mp_SubParts->append(child);
}

void AbstractFilePartModel::removeAt(int index)
{
   AbstractFilePartModel *p_part = mp_SubParts->takeAt(index);
   delete p_part;
}


void AbstractFilePartModel::writeToFile(QFile &file)
{
   prepareData();
   //write self
   if (m_InternalSize > 0) {
      file.write((char *)internalData(), m_InternalSize);
   }

   for(int i = 0; i < mp_SubParts->size(); i++){
      mp_SubParts->at(i)->writeToFile(file);
   }
}

void AbstractFilePartModel::readFromFile(QFile &file, QStringList *p_ParseErrors)
{
   uint8_t *p_Buffer = file.map(0, file.size());
   if(!p_Buffer){
      qWarning() << "AbstractFilePartModel::readFromFile - ERROR - unable to map p_Buffer";
      p_ParseErrors->append(tr("Unable to read file"));
      return;
   }
   int size = readFromBuffer(p_Buffer, file.size(), p_ParseErrors);
   if(file.size() != size){
      qWarning() << "AbstractFilePartModel::readFromFile - ERROR - File was not entirely read";
      p_ParseErrors->append(tr("File contains more data than expected (%1 vs %2)").arg(file.size()).arg(size));
   }
   file.unmap(p_Buffer);
}

uint32_t AbstractFilePartModel::readFromBuffer(uint8_t * p_Buffer, uint32_t size, QStringList *p_ParseErrors)
{
   uint32_t processedSize;
   uint32_t remainingSize = size;

   if(size < minSize()){
      qWarning() << "AbstractFilePartModel::readFromFile - ERROR - (size < minSize()), size = " << size << ", minSize() = " << minSize();
      p_ParseErrors->append(tr("File size is less than allowed.\n\nFile size: %1 byte(s)\nMinimum: %2 bytes").arg(size).arg(minSize()));
      return -1; 
   }

   uint32_t maxInternalSize = this->maxInternalSize();
   if (size > maxInternalSize){
      size = maxInternalSize;
   }

   processedSize = size;
   remainingSize -= size;

   // get internal data
   uint8_t *dst = internalData();

   // Copy

   // Process the bytes one by one until word aligned
   // in this case, if not using size_t, it does not compile on MAC
   for(; (((size_t)p_Buffer)%(sizeof (uint32_t)) != 0) && size > 0; size--){
      *dst++ = *p_Buffer++;
   }

   // While word aligned, process word per word
   while (size >= 4)
   {
      *(uint32_t *)dst = *(uint32_t *)p_Buffer;
      size -= 4;
      p_Buffer += 4;
      dst += 4;
   }

   // Process remaining bytes that are not word aligned
   while (size--){
      *dst++ = *p_Buffer++;
   }

   m_InternalSize = processedSize;

   // pad size in order to alway be word aligned
   // pad with zeros
   int paddingSize = (4-m_InternalSize%4)%4;

   for(int i = 0; i < paddingSize; i++){
      *dst++ = 0x00;
   }
   m_InternalSize += paddingSize;

   return processedSize;
}

void AbstractFilePartModel::updateCRC(Crc32 &crc)
{
   if(m_InternalSize > 0){
      
      crc.update(internalData(), m_InternalSize);
   }

   for(int i = 0; i < mp_SubParts->size(); i++){
      mp_SubParts->at(i)->updateCRC(crc);
   }
}

uint32_t AbstractFilePartModel::size()
{
   uint32_t size = m_InternalSize;
   for(int i = 0; i < mp_SubParts->size(); i++){
      size += mp_SubParts->at(i)->size();
   }

   return size;
}

uint32_t AbstractFilePartModel::maxSize()
{
   uint32_t size = maxInternalSize();
   for(int i = 0; i < mp_SubParts->size(); i++){
      size += mp_SubParts->at(i)->maxSize();
   }

   return size;
}

uint32_t AbstractFilePartModel::minSize()
{

   uint32_t size = minInternalSize();
   for(int i = 0; i < mp_SubParts->size(); i++){
      size += mp_SubParts->at(i)->minSize();
   }

   return size;
}


const QString &AbstractFilePartModel::name(){
   return m_Name;
}

void AbstractFilePartModel::setName(const QString &name)
{
   m_Name = name;
}

