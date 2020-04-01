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
#include "DrumSetExtractor.h"
#include "DrumsetPanel.h"

#include <QProgressDialog>
#include <QDateTime>

#include "../model/beatsmodelfiles.h"

// ********************************************************************************************* //
// **************************************** CONSTRUCTOR **************************************** //
// ********************************************************************************************* //

int DrumSetExtractor::read(const QString& filePath)
{
    // Save the file path
    mSourceFilePath = filePath;

    // Open the file and populate the structs, ~QFile() will close it
    QFile file(mSourceFilePath);

    if(file.size() <= DRM_HEADER_BYTE_SIZE) {
        return -2;    // Not a DRM file
    }

    if (!file.open(QIODevice::ReadOnly)) {
        return -1;    // File could not be opened
    }

    // Read header
    file.read((char*)&mHeaderData,DRM_HEADER_BYTE_SIZE);

    if(mHeaderData.header[0] != 'B' ||
            mHeaderData.header[1] != 'B' ||
            mHeaderData.header[2] != 'd' ||
            mHeaderData.header[3] != 's') {
        return -2;    // Not a DRM file
    }

    // Read instruments
    file.read((char*)&mInstruments,DRM_INSTRUMENT_BYTE_SIZE*DRM_MAX_INSTRUMENT_COUNT);

    // Read Metadata information
    DrmMetadataHeader_t metadataHeader;
    file.read((char*)&metadataHeader,sizeof(DrmMetadataHeader_t));

    // Read Extension information (minimum v1.1)

    DrmExtensionHeader_t extensionHeader;
    bool extensionHeaderValid = false;

    if((mHeaderData.version == 1 && mHeaderData.revision >= 1) || mHeaderData.version > 1){
        file.read((char*)&extensionHeader,sizeof(DrmExtensionHeader_t));
        char expected[] = DRM_EXTENSION_HEADER_MAGIC_VALUE;

        if(memcmp(extensionHeader.header, expected, DRM_EXTENSION_HEADER_MAGIC_SIZE) == 0){
            extensionHeaderValid = true;
        }
    }

    // Count the amount of serialized metadata items
    uint instrumentCount = 0;
    uint velocityCount = 0;
    for(int i=0; i<DRM_MAX_INSTRUMENT_COUNT; i++){
        // If there is a number of velocities, the instrument is valid
        if(mInstruments[i].nVel != 0){
            instrumentCount++;
            velocityCount += mInstruments[i].nVel;
        }
    }

    // Go to metadata and read it
    file.seek(metadataHeader.offset);
    QDataStream stream(&file);

    // Get the Drumset name
    QString drumsetName;
    stream >> drumsetName;
    mp_Model->setDrumsetName(drumsetName);

    for(uint i=0; i<instrumentCount; i++){
        QString data;
        stream >> data;
        mInstrumentNames.append(data);
    }

    for(uint i=0; i<velocityCount; i++){
        QString data;
        stream >> data;
        mVelocityFiles.append(data);
    }

    // Blank instruments
    uint blankCount;
    stream >> blankCount;

    // Create the list of blank instruments
    for (uint i = 0; i < blankCount; ++i){
        QString name;
        uint midiId, chokeGroup, polyPhony, velocityCount, volume, fillChokeGroup, fillChokeDelay, nonPercussion;
        // Fetch data
        stream >> name;
        stream >> midiId;
        stream >> chokeGroup;
        stream >> polyPhony;

        if (mHeaderData.version == 0){

            if (mHeaderData.revision >= 1){
                stream >> volume;
            } else {
                volume = 100;
            }

            if (mHeaderData.revision >=2){
                stream >> fillChokeGroup;
                stream >> fillChokeDelay;
            } else {
                fillChokeGroup = 0;
                fillChokeDelay = 0;
            }
        } else if (mHeaderData.version == 1){
            stream >> volume;
            stream >> fillChokeGroup;
            stream >> fillChokeDelay;
        }
        stream >> velocityCount;

        // Create instrument
        auto ins = new Instrument(mp_Model, name, nullptr, midiId, chokeGroup, polyPhony, volume,fillChokeGroup,fillChokeDelay,nonPercussion);

        // Create velocities
        QList<Velocity*> velocityList;
        for(uint j=0; j<velocityCount; j++){
            uint start, end;

            // Fetch data
            stream >> start;
            stream >> end;

            // Create new velocity
            velocityList.append(new Velocity(mp_Model, start, end));
        }
        // Assign the velocity list to instrument
        ins->setVelocityList(velocityList);

        // Add the instrument to the list
        mBlankInstruments.append(ins);
    }

    // Read drumset volume
    mVolume = 0xFF;
    if((mHeaderData.version == 1 && mHeaderData.revision == 0) &&
            !stream.atEnd()){
        stream >> mVolume;
    } else if(extensionHeaderValid){
        file.seek(extensionHeader.offsets[DRM_EXTENSION_VOLUME_INDEX].offset);

        DrmExtensionVolume_t extensionVolume;
        file.read((char*)&extensionVolume,sizeof(DrmExtensionVolume_t));
        char expected[] = DRM_EXTENSION_VOLUME_MAGIC;

        if(memcmp(extensionVolume.header, expected, DRM_EXTENSION_VOLUME_MAGIC_SIZE) == 0){
            mVolume = extensionVolume.globalVolume;
        }
    }

    // If volume was not read or is invalid, set to max
    if(mVolume < DRM_MIN_DRUMSET_VOLUME || mVolume > DRM_MAX_DRUMSET_VOLUME){
        mVolume = 100;
    }

    return 0; // ok
}

// ************************************************************************************************ //
// **************************************** PUBLIC METHODS **************************************** //
// ************************************************************************************************ //

void DrumSetExtractor::buildInstrumentList(bool newPath, DrumsetPanel* p_parentWidget){
    // Clear instrument list
    mp_Model->clearInstrumentList();

    // Set drumset volume
    mp_Model->setVolume(mVolume);

    // Create indexes to follow which instrument and velocity we are at
    uint velocityIndex = 0;
    uint instrumentIndex = 0;

    QProgressDialog *p_progress = nullptr;

    if(p_parentWidget){
        p_progress = new QProgressDialog("Building Instrument List...", "Abort", 0, DRM_MAX_INSTRUMENT_COUNT, p_parentWidget);
        p_progress->setWindowModality(Qt::WindowModal);
        p_progress->setMinimumDuration(0);
        p_progress->setValue(0);
    }

    for(int i=0; i<DRM_MAX_INSTRUMENT_COUNT; i++){
        if(p_progress && p_progress->wasCanceled()){
            break;
        }
        // Loop util we find a valid instrument
        if(mInstruments[i].nVel){
            // Create the ins trument with name, midi ID and choke group

            // Old drumset don't have an instrument velocity flag.
            char volume = mInstruments[i].volume;
            if (volume == 0){
                volume = 100;
            }

            Instrument *ins = new Instrument(mp_Model,
                                             mInstrumentNames.at(instrumentIndex++),
                                             p_parentWidget,
                                             i,
                                             mInstruments[i].chokeGroup,
                                             mInstruments[i].poly,
                                             volume,
                                             mInstruments[i].fillChokeGroup,
                                             mInstruments[i].fillChokeDelay,
                                             mInstruments[i].nonPercussion);
            // Create its velocity list
            QList<Velocity*> velocityList;
            for(uint j=0; j<mInstruments[i].nVel; j++){
                // Velocity lower and upper bound
                uint lower = mInstruments[i].vel[j].vel;
                uint upper;

                // Find the upper bound
                uint index = j+1;
                bool found = false;
                while(!found){
                    // Are we at the last velocity?
                    if(index == mInstruments[i].nVel) {
                        upper = 127;
                        found = true;
                        // Is the next range different from this one?
                    } else if(mInstruments[i].vel[j].vel != mInstruments[i].vel[index].vel) {
                        upper = mInstruments[i].vel[index].vel-1;
                        found = true;
                        // Velocities are the same, go to next
                    } else {
                        index++;
                    }
                }
                // Create new velocity
                velocityList.push_back(new Velocity(mp_Model, lower, upper));

                // Set the filename (old or new path)
                if(newPath){
                    velocityList.back()->setFilePath(mVelocityNewFiles.at(velocityIndex++));
                } else {
                    velocityList.back()->setFilePath(mVelocityFiles.at(velocityIndex++));
                }
            }
            // Assign the velocity list to instrument
            ins->setVelocityList(velocityList);

            // Insert instrument to model
            mp_Model->addInstrument(ins);
        }

        if(p_progress){
            p_progress->setValue(i+1);
        }
    }

    // Add blank instruments
    foreach (auto i, mBlankInstruments) {
        // Insert instrument to model
        mp_Model->addInstrument(i);
    }

    if(p_progress){
        delete p_progress;
        p_progress = nullptr;
    }
}

QString DrumSetExtractor::generateWaves(const QString &dstDirPath, QWidget *p_parentWidget){
    // Working data
    QFileInfo fileInfo(mSourceFilePath);
    QString basePath = fileInfo.completeBaseName();
    const auto drmLastModified = fileInfo.lastModified();
    QDir workingDirectory(dstDirPath);

    if (!workingDirectory.exists()) {
        return basePath;
    }

    // Regular expression to remove special characters from instrument names
    QRegularExpression re("[ \\\"/<|>:*_?]+");

    // Create the WAVE directory (index it if needed)
    basePath += "_WAVES";
    if (!workingDirectory.exists(basePath)) {
        workingDirectory.mkdir(basePath);
    }
    workingDirectory.cd(basePath);
    basePath = workingDirectory.absolutePath();

    // Create indexes to follow which instrument and velocity we are at
    uint velocityIndex = 0;
    uint instrumentIndex = 0;

    QProgressDialog *p_progress = nullptr;

    if (p_parentWidget) {
        p_progress = new QProgressDialog(QObject::tr("Generating Wave files..."), QObject::tr("Abort"), 0, DRM_MAX_INSTRUMENT_COUNT, p_parentWidget);
        p_progress->setWindowModality(Qt::WindowModal);
        p_progress->setMinimumDuration(0);
        p_progress->setValue(0);
    }

    for (auto i=0; i<DRM_MAX_INSTRUMENT_COUNT; i++) {
        if (p_progress && p_progress->wasCanceled()) {
            break;
        }

        // Loop until we find a valid instrument
        if(mInstruments[i].nVel){
            // Get the name and clean it
            QString name = mInstrumentNames.at(instrumentIndex++);
            name = name.replace(re, "_"); // we replace inappropriate symbols with a sad smilie in hope for happy future

            // Create the instrument directory and go into it
            workingDirectory.mkdir(QString::number(i) + "-" + name);
            workingDirectory.cd(QString::number(i) + "-" + name);

            // Create its velocity directory tree
            for (auto j = 0u; j < mInstruments[i].nVel; ++j) {
                // Velocity lower and upper bound
                auto lower = mInstruments[i].vel[j].vel, upper = 127u;

                // Find the upper bound
                auto index = j+1;
                auto found = false;
                while (!found) {
                    // Are we at the last velocity?
                    if(index == mInstruments[i].nVel) {
                        found = true;
                        // Is the next range different from this one?
                    } else if(mInstruments[i].vel[j].vel != mInstruments[i].vel[index].vel) {
                        upper = mInstruments[i].vel[index].vel-1;
                        found = true;
                        // Velocities are the same, go to next
                    } else {
                        index++;
                    }
                }

                // Create the velocity directory and go into it
                workingDirectory.mkdir(QString::number(lower) + "-" + QString::number(upper));
                workingDirectory.cd(QString::number(lower) + "-" + QString::number(upper));

                // Get wave file name and path
                QFileInfo velOrigFileInfo(mVelocityFiles.at(velocityIndex++));
                QString waveFileName = velOrigFileInfo.completeBaseName();

                // Create the wave file full path
                QString waveFileFullPath = workingDirectory.absolutePath() + "/" + waveFileName + "." BMFILES_WAVE_EXTENSION;

                // Verify if the file exists and modify the name if needed
                QFileInfo waveFile(waveFileFullPath);
                if (waveFile.exists() && waveFile.lastModified() > drmLastModified) {
                    waveFileFullPath += ".orig";
                }

                // Create output file ByteArray
                QFile output(waveFileFullPath);
                output.open(QIODevice::WriteOnly);

                // Create output file header
                auto bitsPerSample = mInstruments[i].vel[j].bps;
                auto numberChannels = mInstruments[i].vel[j].nChannel;
                auto sampleRate = mInstruments[i].vel[j].fs;
                auto rawSize = (bitsPerSample / 8) * mInstruments[i].vel[j].nSample;
                output.write("RIFF");
                output.write(Utils::intToQByteArray(rawSize + 36));                                           // Total file size (header is 44 bytes long minus 8 bytes = 36)
                output.write("WAVE");
                output.write("fmt ");
                output.write(Utils::intToQByteArray(16));                                                     // Length in bytes of what has been written so far
                output.write(Utils::shortToQByteArray(1));                                                    // 1 = PCM format
                output.write(Utils::shortToQByteArray(numberChannels));                                       // Number of channels
                output.write(Utils::intToQByteArray(sampleRate));                                             // Sample rate
                output.write(Utils::intToQByteArray((uint)((sampleRate*bitsPerSample*numberChannels)/8)));    // Byte rate
                output.write(Utils::shortToQByteArray((bitsPerSample*numberChannels)/8));                     // Data block size -> 1: 8 bit mono, 2: 8 bit stereo/16 bit mono, 4: 16 bit stereo, 6: 24 bit stereo
                output.write(Utils::shortToQByteArray(bitsPerSample));                                        // Bits per sample
                output.write("data");
                output.write(Utils::intToQByteArray(rawSize));                                                // Size of audio sample

                // Read raw wave data from drumset
                QFile input(mSourceFilePath);
                input.open(QIODevice::ReadOnly);

#if !(defined(__x86_64__) || defined(_M_X64))
                input.seek((uint)mInstruments[i].vel[j].addr);
#else
                input.seek((uint)mInstruments[i].vel[j].offset);
#endif
                output.write(input.read(rawSize));
                input.close();
                output.close();

                // Check content file
                if (waveFileFullPath.endsWith(".orig")) {
                    auto realFileFullPath = waveFileFullPath.left(waveFileFullPath.size()-5);
                    QFile(waveFileFullPath).size() == QFile(realFileFullPath).size() && [&waveFileFullPath, &realFileFullPath] (){
                        QFile wave(realFileFullPath), orig(waveFileFullPath);
                        char w, o;
                        for (wave.open(QIODevice::ReadOnly), orig.open(QIODevice::ReadOnly); !wave.atEnd();)
                            if (wave.read(&w, 1) != orig.read(&o, 1) || w != o)
                                return false;
                        return true;
                    }() ? QFile::remove(waveFileFullPath) : QFile::remove(realFileFullPath), QFile::rename(waveFileFullPath, realFileFullPath);
                    waveFileFullPath = realFileFullPath;
                }

                // Add the file name to list
                mVelocityNewFiles.append(waveFileFullPath);

                // Get back to parent directory
                workingDirectory.cdUp();
            }
            // Get back to parent directory
            workingDirectory.cdUp();
        }

        if(p_progress){
            p_progress->setValue(i+1);
        }
    }

    if (p_progress) {
        delete p_progress;
        p_progress = nullptr;
    }

    return basePath;
}
