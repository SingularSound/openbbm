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
#include <stdint.h>
#include "drmmakermodel.h"
#include "../../utils/utils.h"
#include "crc32.h"

#include <QUndoStack>


// ********************************************************************************************* //
// **************************************** CONSTRUCTOR **************************************** //
// ********************************************************************************************* //
DrmMakerModel::DrmMakerModel(const QString& tmpDirPath)
    : mDrumsetName(QObject::tr("New Drumset"))
    , mDrmOpened(false)
    , mDrumsetSize(0)
    , mVolume(100)
    , m_stack(nullptr)
    , m_dirUndoRedo(tmpDirPath)
{
    memset(mInstrumentId, false, DRM_MAX_INSTRUMENT_COUNT*sizeof(bool));
    m_stack = new QUndoStack(this);
    m_dirUndoRedo.mkdir("history_ds");
    m_dirUndoRedo.cd("history_ds");
}

// *********************************************************************************************** //
// *************************************** PRIVATE METHODS *************************************** //
// *********************************************************************************************** //


// ********************************************************************************************** //
// *************************************** PUBLIC METHODS *************************************** //
// ********************************************************************************************** //

QString DrmMakerModel::getDrumsetNameStatic(const QString &path)
{
    QFile file(path);
    if(file.open(QIODevice::ReadOnly)){

        file.seek(DRM_HEADER_BYTE_SIZE + (DRM_INSTRUMENT_BYTE_SIZE * DRM_MAX_INSTRUMENT_COUNT));
        // Read Metadata information
        uint metadataOffset;
        file.read((char*)&metadataOffset,sizeof(uint));

        // Go to metadata and read it
        file.seek(metadataOffset);
        QDataStream stream(&file);

        // Get the Drumset name
        QString drumsetName;
        stream >> drumsetName;

        // Close the file
        file.close();

        return drumsetName;
    }
    return nullptr;
}

QByteArray DrmMakerModel::getCRCStatic(const QString &path)
{
    QFile file(path);
    if(!file.open(QIODevice::ReadOnly)){
        qWarning() << "DrmMakerModel::getCRCStatic - ERROR 1 - unable to open " << path;
        return QByteArray();
    }

    // Read Header
    DrmHeader_t header;
    qint64 ret = file.read((char*)&header, sizeof(DrmHeader_t));
    file.close();

    if(ret != sizeof(DrmHeader_t)){
        qWarning() << "DrmMakerModel::getCRCStatic - ERROR 2 - unable to read " << path;
        return QByteArray();
    }

    return Utils::intToQByteArray(header.crc);
}

bool DrmMakerModel::copyDrmNewName(const QString &srcPath, const QString &dstPath, const QString & newName)
{
    QFile srcFile(srcPath);
    QFile dstFile(dstPath);

    if(!dstFile.open(QIODevice::WriteOnly)){
        qWarning() << "DrmMakerModel::copyDrmNewName - ERROR 1 - Unable to open dst";
        return false;
    }

    if(!srcFile.open(QIODevice::ReadOnly)){
        qWarning() << "DrmMakerModel::copyDrmNewName - ERROR 2 - Unable to open src";
        dstFile.close();
        return false;
    }

    // read the whole content in a byte array
    QByteArray data = srcFile.readAll();
    srcFile.close();

    char *p_data = data.data();

    // Read meta data information
    uint metadataOffset = *((uint *)(p_data + DRM_HEADER_BYTE_SIZE + (DRM_INSTRUMENT_BYTE_SIZE * DRM_MAX_INSTRUMENT_COUNT)));
    uint metadataSize = *((uint *)(p_data + DRM_HEADER_BYTE_SIZE + (DRM_INSTRUMENT_BYTE_SIZE * DRM_MAX_INSTRUMENT_COUNT) + sizeof(uint)));

    // Read original meta data
    QByteArray metaData = QByteArray::fromRawData(p_data + metadataOffset, data.count() - metadataOffset);

    // Read initial name from meta Data
    QDataStream oldNameDin(metaData);
    QString oldName;
    oldNameDin >> oldName;

    // Write old name to Byte Array in order to determine its serialized size
    QByteArray oldNameBA;
    QDataStream oldNameDout(&oldNameBA, QIODevice::WriteOnly);
    oldNameDout << oldName;

    // Replace old name by new name
    metaData.remove(0, oldNameBA.count());
    QByteArray newNameBA;
    QDataStream newNameDout(&newNameBA, QIODevice::WriteOnly);
    newNameDout << newName;
    metaData.insert(0, newNameBA);

    // update the meta data size
    *((uint *)(p_data + DRM_HEADER_BYTE_SIZE + (DRM_INSTRUMENT_BYTE_SIZE * DRM_MAX_INSTRUMENT_COUNT) + sizeof(uint))) = metaData.count();

    // Replace meta data
    data.remove(metadataOffset, metadataSize);
    data.append(metaData);

    // Need to re-fetch data since modified structure of byte array
    p_data = data.data();

    // Update CRC
    DrmHeader_t *header = (DrmHeader_t *)p_data;

    Crc32 crc32;
    crc32.update((uint8_t*)p_data, DRM_HEADER_BYTE_SIZE - sizeof(unsigned int));
    crc32.update((uint8_t*)(p_data+DRM_HEADER_BYTE_SIZE), data.size() - DRM_HEADER_BYTE_SIZE);
    header->crc = crc32.getCRC(true);

    // Write content of byte array to output file
    dstFile.write(data);
    dstFile.flush();
    dstFile.close();

    return true;
}


QByteArray DrmMakerModel::renameDrm(const QString &drmPath, const QString & newName)
{
    QFile drmFile(drmPath);

    if(!drmFile.open(QIODevice::ReadWrite)){
        qWarning() << "DrmMakerModel::copyDrmNewName - ERROR 1 - Unable to open drm File";
        return QByteArray();
    }


    // read the whole content in a byte array
    QByteArray data = drmFile.readAll();

    char *p_data = data.data();

    // Read meta data information
    uint metadataOffset = *((uint *)(p_data + DRM_HEADER_BYTE_SIZE + (DRM_INSTRUMENT_BYTE_SIZE * DRM_MAX_INSTRUMENT_COUNT)));

    // Read original meta data
    QByteArray metaData = QByteArray::fromRawData(p_data + metadataOffset, data.count() - metadataOffset);

    // Read initial name from meta Data
    QDataStream oldNameDin(metaData);
    QString oldName;
    oldNameDin >> oldName;

    // Write old name to Byte Array in order to determine its serialized size
    QByteArray oldNameBA;
    QDataStream oldNameDout(&oldNameBA, QIODevice::WriteOnly);
    oldNameDout << oldName;

    // Replace old name by new name
    metaData.remove(0, oldNameBA.count());
    QByteArray newNameBA;
    QDataStream newNameDout(&newNameBA, QIODevice::WriteOnly);
    newNameDout << newName;
    metaData.insert(0, newNameBA);

    // update the meta data size (directly in file)
    drmFile.seek(DRM_HEADER_BYTE_SIZE + (DRM_INSTRUMENT_BYTE_SIZE * DRM_MAX_INSTRUMENT_COUNT) + sizeof(uint));
    uint metadataSize = metaData.count();
    drmFile.write((char *)&metadataSize, sizeof(uint));

    // Replace meta data
    drmFile.seek(metadataOffset);
    drmFile.write(metaData);

    // Update CRC
    Crc32 crc32;
    crc32.update((uint8_t*)p_data, DRM_HEADER_BYTE_SIZE - sizeof(unsigned int));               // Start of header
    crc32.update((uint8_t*)(p_data+DRM_HEADER_BYTE_SIZE), (DRM_INSTRUMENT_BYTE_SIZE * DRM_MAX_INSTRUMENT_COUNT) + sizeof(uint));  // up to meta data size
    crc32.update((uint8_t*)&metadataSize, sizeof(uint));  // meta data size
    crc32.update((uint8_t*)(p_data + (DRM_INSTRUMENT_BYTE_SIZE * DRM_MAX_INSTRUMENT_COUNT) + (2 * sizeof(uint))),
                 metadataOffset - ((DRM_INSTRUMENT_BYTE_SIZE * DRM_MAX_INSTRUMENT_COUNT) + (2 * sizeof(uint))));  // remaining data
    crc32.update((uint8_t*)(metaData.data()), metaData.size()); // new meta data

    // write CRC
    drmFile.seek(DRM_CRC_OFFSET);
    uint32_t crc = crc32.getCRC(true);
    drmFile.write((char*)&crc, sizeof(uint32_t));


    // update the file size (make sure any extra data is truncated)
    drmFile.resize(metadataOffset + metadataSize);

    drmFile.flush();
    drmFile.close();

    // Note: cannot use QByteArray::fromRawData() this method relies on pointer not changing and CRC is on the stack.
    return Utils::intToQByteArray(crc);
}

bool DrmMakerModel::isNewSizeAllowed(quint32 size)
{
    if ((size + mDrumsetSize) < (100 * 1024 * 1024)){
        return true;
    } else {
        return false;
    }
}

/**
 * @brief DrmMakerModel::addInstrument
 * @param instrument
 */
void DrmMakerModel::addInstrument(Instrument *instrument){
    // Add instrument to the end of the list
    mInstrumentList.append(instrument);

    // Send new instrument's index to observers
    emit instrumentAdded(mInstrumentList.size()-1);
}


/**
 * @brief Remove an insturment from the model
 * @param index
 */
void DrmMakerModel::removeInstrument(int index){
    // Delete the instrument from list (all UI references to this widget will remove it from layout)
    // Only if there is at least two elements in list
    if (mInstrumentList.size() > 1) {
        // remove from the list
        Instrument *instrument = mInstrumentList.takeAt(index);

        // Free the MidiId reference
        mInstrumentId[instrument->getMidiId()] = false;

        // get the size
        setInstrumentSize(instrument->getInstrumentSize(),0);

        delete instrument;

        // Send removed instrument's index to observers
        emit instrumentRemoved(index);
    }
}

void DrmMakerModel::clearInstrumentList(){
    // Delete all instruments in list (all UI references to this widget will remove it from layout)
    qDeleteAll(mInstrumentList);
    mInstrumentList.clear();

    // Clear all Midi Ids
    memset(mInstrumentId, false, DRM_MAX_INSTRUMENT_COUNT*sizeof(bool));

    // Notify new size at zero
    mDrumsetSize = 0;
    setInstrumentSize(0,0);
}

void DrmMakerModel::sortInstruments() {
    qSort(mInstrumentList.begin(), mInstrumentList.end(), [](Instrument* a, Instrument* b) { return *a < *b; });
    // Notify observers that all instruments are sorted
    emit instrumentSorted();
}



int DrmMakerModel::getNextAvailableId(int start, bool take /* = true */){
    // If start doesn't make sense, reset to 0
    if((start >= DRM_MAX_INSTRUMENT_COUNT) || (start < 0)){
        start = 0;
    }

    for(int i=start; i<DRM_MAX_INSTRUMENT_COUNT; i++){
        if(!mInstrumentId[i]){
            mInstrumentId[i] = take;
            return i;
        }
    }
    // Should never happen
    return -1;
}

bool DrmMakerModel::isIdAvailable(int id){
    // If start doesn't make sense, return false
    if((id >= DRM_MAX_INSTRUMENT_COUNT) || (id < 0)){
        return false;
    } else {
        return !mInstrumentId[id];
    }
}

void DrmMakerModel::switchMidiIds(int oldId, int newId) {
    mInstrumentId[oldId] = false;
    mInstrumentId[newId] = true;
}

QList<int>* DrmMakerModel::getAvailableMidiIds(){
    QList<int>* list = new QList<int>;
    for(int i=0; i<DRM_MAX_INSTRUMENT_COUNT; i++){
        if(!mInstrumentId[i]){
            list->append(i);
        }
    }
    return list;
}
