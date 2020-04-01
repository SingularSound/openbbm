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
#include "DrumSetMaker.h"



// ********************************************************************************************* //
// **************************************** CONSTRUCTOR **************************************** //
// ********************************************************************************************* //

DrumSetMaker::DrumSetMaker(DrmMakerModel *model)
{
   mp_Model = model;

   // Initialize the instrument table and WAVE buffers
   for(int i=0; i<DRM_MAX_INSTRUMENT_COUNT; i++){
      mInstrumentBuffers[i] = nullptr;
      mWaveBuffer[i] = nullptr;
   }

   // Create a blank instrument
   for(uint i=0; i<(DRM_INSTRUMENT_BYTE_SIZE/4); i++){
      mBlankInstrumentBuffer.append(Utils::intToQByteArray((uint)0));
   }

   // Set the valid build flag
   mValidBuild = false;
}

// ******************************************************************************************** //
// **************************************** DESTRUCTOR **************************************** //
// ******************************************************************************************** //

DrumSetMaker::~DrumSetMaker() {
   for(int i=0; i<DRM_MAX_INSTRUMENT_COUNT; i++){
      if(mInstrumentBuffers[i] != nullptr){
         delete mInstrumentBuffers[i];
      }
      if(mWaveBuffer[i] != nullptr){
         delete mWaveBuffer[i];
      }
   }
}

// ************************************************************************************************ //
// **************************************** PUBLIC METHODS **************************************** //
// ************************************************************************************************ //

bool DrumSetMaker::buildDRM() {
   makeWavBuffer();
   // After making the wav buffer, if the offset list is blank, it means that no waves are in the drum set maker fields: return an error
   if(mOffsetList.size() == 0){
      // To make sure, we clean up
      for(int i=0; i<DRM_MAX_INSTRUMENT_COUNT; i++) {
         if(mWaveBuffer[i] != nullptr){
            delete mWaveBuffer[i];
            mWaveBuffer[i] = nullptr;
         }
      }
      // Reset CRC (getting the final value resets CRC)
      mCRC.getCRC(true);

      // Set the valid build flag
      mValidBuild = false;
   } else {
      makeInstrumentBuffers();
      makeMetadataBuffers();
      makeExtensionBuffers();
      makeHeaderBuffer();

      // Set the valid build flag
      mValidBuild = true;
   }
   return mValidBuild;
}

bool DrumSetMaker::containsValidBuild() {
   return mValidBuild;
}

void DrumSetMaker::clearBuild() {

   // Reset instrument table and WAVE buffers
   for(int i=0; i<DRM_MAX_INSTRUMENT_COUNT; i++) {
      if(mInstrumentBuffers[i] != nullptr ){
         delete mInstrumentBuffers[i];
         mInstrumentBuffers[i] = nullptr;
      }
      if(mWaveBuffer[i] != nullptr){
         delete mWaveBuffer[i];
         mWaveBuffer[i] = nullptr;
      }
   }

   // Clear all buffers
   mHeaderBuffer.clear();
   mMetaDataInfoBuffer.clear();
   mMetaDataBuffer.clear();
   mExtensionHeaderBuffer.clear();
   mExtensionVolumeBuffer.clear();
   mOffsetList.clear();
   mPaddingList.clear();
   mWaveInfoList.clear();

   // Reset valid build flag
   mValidBuild = false;

   // Reset CRC (getting the final value resets CRC)
   mCRC.getCRC(true);
}

void DrumSetMaker::writeToFile(QString filename) {
   QFile file(filename);
   file.open(QIODevice::WriteOnly);
   if(mValidBuild){
      file.write(mHeaderBuffer);

      // Write blank data if midi Id is not present
      for(int i=0; i<DRM_MAX_INSTRUMENT_COUNT; i++){
         if(mInstrumentBuffers[i] != nullptr){
            file.write(*mInstrumentBuffers[i]);
         } else {
            file.write(mBlankInstrumentBuffer);
         }
      }
      file.write(mMetaDataInfoBuffer);
      file.write(mExtensionHeaderBuffer);

      // Write WAVES and padding
      for(int i=0; i<mPaddingList.size(); i++){
         int paddingSize = mPaddingList[i];
         if(paddingSize){
            QByteArray padding;
            padding.fill(0x00, paddingSize);
            file.write(padding);
         }
         file.write(*mWaveBuffer[i]);
      }
      file.write(mMetaDataBuffer);
      file.write(mExtensionVolumeBuffer);
   }
   file.close();
}

bool DrumSetMaker::isEmpty(){
   return mOffsetList.size() == 0;
}


// ************************************************************************************************* //
// **************************************** PRIVATE METHODS **************************************** //
// ************************************************************************************************* //

bool DrumSetMaker::checkErrorsAndSort() {
   bool retval = false;
   foreach (Instrument *instrument, mp_Model->getInstrumentList()) {
      // Check for valid for incomplete instruments
      if (!(instrument->isValid())){
         retval = true;
      }
   }

   // Sort & check the velocity ranges and filenames
   foreach (Instrument *instrument, mp_Model->getInstrumentList()) {
      instrument->on_Sort_clicked();
   }

   // Sort the instruments if no errors occured
   if(!retval){
      mp_Model->sortInstruments();
   }
   return retval;
}

void DrumSetMaker::makeWavBuffer() {
   /* Flag used to know which instrument waves are being buffered.
     * This is used to allign WAVE chunks of each instrument at 512 byte offsets */
   bool firstInstrument = true;
   uint allignOffset;

   foreach (Instrument *i, mp_Model->getInstrumentList()) {
      // If the instrument has empty file fields, skip it.
      if(i->getEmptyFilesFlag()){
         continue;
      }

      QBuffer buffer;
      buffer.open(QIODevice::Append);

      // Create a local file for WAVE file offsets
      QList<int> offsets;
      if(firstInstrument) {
         allignOffset = DRM_WAV_START_OFFSET;
         mPaddingList.append(DRM_INSTRUMENT_TO_WAVE_PADDING);

         // Remove the first instrument flag
         firstInstrument = false;
      } else {
         // Get the offset of the last instrument WAVE chunk and calculate offset for this WAVE chunk
         int size = (mOffsetList.last().last() - mOffsetList.last().first()) + mOffsetList.last().first();

         // Calculate padding and offset
         if ((size % DRM_WAV_BYTE_ALLIGNMENT) == 0) {
            allignOffset = size;
            mPaddingList.append(0);
         } else {
            allignOffset = ((int)((size/DRM_WAV_BYTE_ALLIGNMENT)+1))*DRM_WAV_BYTE_ALLIGNMENT;
            mPaddingList.append(allignOffset-size);
         }
      }
      offsets.append(allignOffset);

      foreach (QFileInfo fileinfo, i->getFiles()) {
         char data;
         bool ok;
         int size = 0;
         QFile input(fileinfo.absoluteFilePath());
         input.open(QIODevice::ReadOnly);

         // Get the file info and store it to the list
         Vel_t waveInfo;

         // Go to fetch number of channels
         input.seek(22);
         waveInfo.nChannel = input.read(1).toHex().toUInt(&ok, 16) + (input.read(1).toHex().toUInt(&ok, 16) << 8);

         // Go to fetch sampling frequency
         waveInfo.fs = input.read(1).toHex().toUInt(&ok, 16) + (input.read(1).toHex().toUInt(&ok, 16) << 8) + (input.read(1).toHex().toUInt(&ok, 16) << 16) + (input.read(1).toHex().toUInt(&ok, 16) << 24);

         // Go to fetch bit depth
         input.seek(32);
         uint bytesPerSample = input.read(1).toHex().toUInt(&ok, 16) + (input.read(1).toHex().toUInt(&ok, 16) << 8);
         switch(bytesPerSample){
            case 1:     // 8bit Mono
               waveInfo.bps = 8;
               break;
            case 2:     // 8bit Stereo or 16bit Mono
               if(waveInfo.nChannel == 1){
                  waveInfo.bps = 16;
               } else {
                  waveInfo.bps = 8;
               }
               break;
            case 3:     // 24bit Mono (if it exists)
               waveInfo.bps = 24;
               break;
            case 4:     // 16bit Stereo
               waveInfo.bps = 16;
               break;
            case 6:     // 24bit Stereo
               waveInfo.bps = 24;
               break;
         }

         // Seek to "data", read data size and break out of seek loop
         while(!input.atEnd()){
            input.read(&data, 1);
            if(data == 'd'){
               input.read(&data, 1);
               if(data == 'a'){
                  input.read(&data, 1);
                  if(data == 't'){
                     input.read(&data, 1);
                     if(data == 'a'){
                        // Read size as big endian
                        size = input.read(1).toHex().toUInt(&ok, 16) + (input.read(1).toHex().toUInt(&ok, 16) << 8) + (input.read(1).toHex().toUInt(&ok, 16) << 16) + (input.read(1).toHex().toUInt(&ok, 16) << 24);
                        break;
                     }
                  }
               }
            }
         }

         // If the data was found with proper size (bigger than 0), write it to buffer
         if(size){
            // Write to buffer
            buffer.write(input.read(size));

            // Add the offset to the instrument's offset list
            offsets.append(allignOffset+buffer.size());

            // Set the total number of samples
            waveInfo.nSample = size/(waveInfo.bps/8);

            // Add the wave info to the wave info list
            mWaveInfoList.append(waveInfo);
         }
         input.close();
      }

      // Write the set of offsets for this instrument
      mOffsetList.append(offsets);

      // Create WAVE Buffer and write the complete buffer to it
      mWaveBuffer[mOffsetList.size()-1] = new QByteArray();
      *mWaveBuffer[mOffsetList.size()-1] = buffer.data();
      buffer.close();
   }
   // Calculate CRC of waves and padding
   for(int i=0; i<mPaddingList.size(); i++){
      int paddingSize = mPaddingList[i];
      if(paddingSize){
         QByteArray padding;
         padding.fill(0x00, paddingSize);
         mCRC.update((uint8_t*)padding.data(), padding.size());
      }
      mCRC.update((uint8_t*)mWaveBuffer[i]->data(), mWaveBuffer[i]->size());
   }
}


void DrumSetMaker::makeInstrumentBuffers(){
   uint instrumentCount = 0;
   uint velocityCount = 0;
   foreach (Instrument *inst, mp_Model->getInstrumentList()) {
      // If the instrument has empty file fields, skip it.
      if(inst->getEmptyFilesFlag()){
         continue;
      }

      // Offset list for this instrument
      QList<int> offsetList = mOffsetList[instrumentCount++];

      // Working buffer
      QBuffer buffer;
      buffer.open(QIODevice::Append);

      // Fetch midiId and create QByteArray acording to index
      int midiId = inst->getMidiId();
      mInstrumentBuffers[midiId] = new QByteArray();

      // Choke group
      ushort chokeGroup = inst->getChokeGroup();
      buffer.write(Utils::shortToQByteArray(chokeGroup));

      // Polyphony
      ushort polyPhony = inst->getPolyPhony();
      buffer.write(Utils::shortToQByteArray(polyPhony));

      // Number of wav files associated to instrument
      QList<int> starts = inst->getStarts();
      uint numberOfWaves = starts.size();
      buffer.write(Utils::intToQByteArray(numberOfWaves));

      // Instrument WAV section size
      buffer.write(Utils::intToQByteArray((uint)(offsetList.last()-offsetList.first())));

      // Reserved data #0
      // First byte is Instrument Volume
      qDebug() << inst->getName() << inst->getMidiId()<< inst->getVolume() << inst->getNonPercussion();
      buffer.write(QByteArray(1, (char)inst->getVolume()));
      buffer.write(QByteArray(1, (char)inst->getFillChokeGroup()));
      buffer.write(QByteArray(1, (char)inst->getFillChokeDelay()));
      buffer.write(QByteArray(1, (char)inst->getNonPercussion()));

      // Reserved data #1
      buffer.write(Utils::intToQByteArray((uint)0));

      // Vel_t array
      for (uint i=0; i<MIDIPARSER_MAX_NUMBER_VELOCITY; i++){
         if(i<numberOfWaves){
            Vel_t waveInfo = mWaveInfoList.at(velocityCount++);
            buffer.write(Utils::shortToQByteArray(waveInfo.bps));              // Bit depth
            buffer.write(Utils::shortToQByteArray(waveInfo.nChannel));         // Number of channels
            buffer.write(Utils::intToQByteArray(waveInfo.fs));                 // Sampling frequency
            buffer.write(Utils::intToQByteArray((uint)starts[i]));             // Lower bound velocity
            buffer.write(Utils::intToQByteArray(waveInfo.nSample));
            buffer.write(Utils::intToQByteArray((uint)0));                     // Reserved
            buffer.write(Utils::intToQByteArray((uint)0));                     // Reserved
            buffer.write(Utils::intToQByteArray((uint)offsetList[i]));         // Address offset according to Wave buffer
         } else {
            // Pack everything else with zeros
            buffer.write(Utils::shortToQByteArray((ushort)0));                 // Bit depth
            buffer.write(Utils::shortToQByteArray((ushort)0));                 // Number of channels
            buffer.write(Utils::intToQByteArray((uint)0));                     // Sampling frequency
            buffer.write(Utils::intToQByteArray((uint)0));                     // Lower bound velocity
            buffer.write(Utils::intToQByteArray((uint)0));                     // Number of samples
            buffer.write(Utils::intToQByteArray((uint)0));                     // Reserved
            buffer.write(Utils::intToQByteArray((uint)0));                     // Reserved
            buffer.write(Utils::intToQByteArray((uint)0));                     // Address offset according to Wave buffer
         }
      }

      // Dump the buffer data in according midi# instrument
      *mInstrumentBuffers[midiId] = buffer.data();
      buffer.close();
   }
   // Calculate CRC
   for(int i=0; i<DRM_MAX_INSTRUMENT_COUNT; i++){
      if(mInstrumentBuffers[i] == nullptr) {
         mCRC.update((uint8_t*)mBlankInstrumentBuffer.data(), mBlankInstrumentBuffer.size());
      } else {
         mCRC.update((uint8_t*)mInstrumentBuffers[i]->data(), mInstrumentBuffers[i]->size());
      }
   }
}


void DrumSetMaker::makeMetadataBuffers(){
   DrmMetadataHeader_t metaDataHeader = { 0,0 };


   // Calculate WAVE chunk size with padding
   uint waveSize = 0;
   for(int i=0; i<mPaddingList.size(); i++){
      // Do not count the first padding since it is known: ( WAVE start address - (Header + Instrument table) )
      if(i!=0){
         waveSize += mPaddingList[i];
      }
      waveSize += mWaveBuffer[i]->size();
   }

   metaDataHeader.offset = ( DRM_WAV_START_OFFSET + waveSize);

   // Create datastream to serialize all the metadata
   QBuffer buffer;
   buffer.open(QIODevice::Append);
   QDataStream stream(&buffer);

   // Serialize the drumset name
   stream << mp_Model->getDrumsetName();

   // Serialize all Instrument names & count empty instruments
   uint emptyInstrumentCount = 0;
   foreach (Instrument *instrument, mp_Model->getInstrumentList()) {
      // Get only the instruments that have populated fields
      if(!instrument->getEmptyFilesFlag()){
         stream << instrument->getName();
      } else {
         emptyInstrumentCount++;
      }
   }

   // Serialize all file names
   foreach (Instrument *instrument, mp_Model->getInstrumentList()) {
      // Get only the instruments that have populated fields
      if(!instrument->getEmptyFilesFlag()){
         QList<QFileInfo> files = instrument->getFiles();
         foreach(QFileInfo fi, files){
            stream << fi.absoluteFilePath();
         }
      }
   }

   // Serialize empty instrument count
   stream << emptyInstrumentCount;

   // Serialize empty instruments (name, midi id, chokegroup, polyphony)
   foreach (Instrument *instrument, mp_Model->getInstrumentList()) {
      // Get only the blank instruments
      if(instrument->getEmptyFilesFlag()){
         uint velocityCount = instrument->getStarts().size();
         stream << instrument->getName();
         stream << instrument->getMidiId();
         stream << instrument->getChokeGroup();
         stream << instrument->getPolyPhony();
         qDebug() << instrument->getVolume();
         stream << instrument->getVolume();
         stream << instrument->getFillChokeGroup();
         stream << instrument->getFillChokeDelay();
         stream << velocityCount;
         for(uint i=0; i<velocityCount; i++){
            stream << instrument->getStarts().at(i);
            stream << instrument->getEnds().at(i);
         }
      }
   }

   // Dump the buffer data
   mMetaDataBuffer = buffer.data();
   buffer.close();

   // Write size to info buffer
   metaDataHeader.size = mMetaDataBuffer.size();

   char *p_metaDataHeader = (char *) &metaDataHeader;
   for(int i = 0; i < sizeof(DrmMetadataHeader_t); i++){
       mMetaDataInfoBuffer.append(p_metaDataHeader[i]);
   }

   // Calculate CRC
   mCRC.update((uint8_t*)mMetaDataInfoBuffer.data(), mMetaDataInfoBuffer.size());
   mCRC.update((uint8_t*)mMetaDataBuffer.data(), mMetaDataBuffer.size());
}

void DrumSetMaker::makeExtensionBuffers(){

    DrmExtensionHeader_t extensionHeader = {
        DRM_EXTENSION_HEADER_MAGIC_VALUE, // Magic
        {
            {0,0},                        // Volume offset,size
            {0,0},                        // Reserved
            {0,0},                        // Reserved
            {0,0},                        // Reserved
        }
    };

    DrmExtensionVolume_t extensionVolume = {
        DRM_EXTENSION_VOLUME_MAGIC,  // Magic
        {0,0,0},                     // Padding
        0,                           // Volume
    };

    // Update header for volume data
    DrmMetadataHeader_t *p_metaHeader = (DrmMetadataHeader_t *)mMetaDataInfoBuffer.data();
    extensionHeader.offsets[DRM_EXTENSION_VOLUME_INDEX].offset = p_metaHeader->offset + p_metaHeader->size;
    extensionHeader.offsets[DRM_EXTENSION_VOLUME_INDEX].size   = sizeof(DrmExtensionVolume_t);


    // Update volume
    extensionVolume.globalVolume = mp_Model->volume();

    // Write to buffers
    char *p_extensionHeader = (char *) &extensionHeader;
    char *p_extensionVolume = (char *) &extensionVolume;

    for(int i = 0; i < sizeof(DrmExtensionHeader_t); i++){
        mExtensionHeaderBuffer.append(p_extensionHeader[i]);
    }

    for(int i = 0; i < sizeof(DrmExtensionVolume_t); i++){
        mExtensionVolumeBuffer.append(p_extensionVolume[i]);
    }

    // Compute CRC
    mCRC.update((uint8_t*)mExtensionHeaderBuffer.data(), mExtensionHeaderBuffer.size());
    mCRC.update((uint8_t*)mExtensionVolumeBuffer.data(), mExtensionVolumeBuffer.size());
}

void DrumSetMaker::makeHeaderBuffer(){
   // Initialize the header
   // TODO: SET proper values when integrated to application
   mHeaderBuffer.append("BBds");                                       // Header
   mHeaderBuffer.append((char)DRM_VERSION);                            // Version
   mHeaderBuffer.append((char)DRM_REVISION);                           // Revision
   mHeaderBuffer.append(Utils::shortToQByteArray((ushort)DRM_BUILD));  // Build
   // Calculate CRC for the calculable data of header
   mCRC.update((uint8_t*)mHeaderBuffer.data(), mHeaderBuffer.size());
   // Write the CRC value
   mHeaderBuffer.append(Utils::intToQByteArray((uint)mCRC.peakCRC(true)));
}
