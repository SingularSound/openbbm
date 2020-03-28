//#ifndef CONVERTTRACKTOMIDI_H
//#define CONVERTTRACKTOMIDI_H
//#include <stdint.h>
//#include <stdlib.h>
//#include <string.h>
//#include <QFile>
//#include <QByteArray>
//#include <QString>
//
//typedef enum {
//    MIDI_CHANNEL_1  = 0,
//    MIDI_CHANNEL_2  = 1,
//    MIDI_CHANNEL_3  = 2,
//    MIDI_CHANNEL_4  = 3,
//    MIDI_CHANNEL_5  = 4,
//    MIDI_CHANNEL_6  = 5,
//    MIDI_CHANNEL_7  = 6,
//    MIDI_CHANNEL_8  = 7,
//    MIDI_CHANNEL_9  = 8,
//    MIDI_CHANNEL_10 = 9,
//    MIDI_CHANNEL_11 = 10,
//    MIDI_CHANNEL_12 = 11,
//    MIDI_CHANNEL_13 = 12,
//    MIDI_CHANNEL_14 = 13,
//    MIDI_CHANNEL_15 = 14,
//    MIDI_CHANNEL_16 = 15
//} MIDI_WRITER_MIDI_CHANNEL;
//
//typedef enum {
//    FORMAT_SINGLE_TRACK          = 0,
//    FORMAT_MULTIPLE_TRACK_SYNC   = 1,
//    FORMAT_MULTIPLE_TRACK_ASYNC  = 2,
//
//} MIDI_WRITER_TRACK_FORMAT;
//
//
//class MidiFileWriter
//{
//public:
//    MidiFileWriter(QString savedMidiPath);
//
//    bool openMidiFile();
//
//    void writeHeader(MIDI_WRITER_TRACK_FORMAT format, uint16_t nTrack, uint16_t delta_time);
//    uint32_t writeStartOfTrack();
//    void writeEndOfTrack(uint32_t start_pos);
//    void writeSetTempo(uint32_t tempo);
//    void writeDelta(uint32_t nTick);
//    void writeNoteOn(MIDI_WRITER_MIDI_CHANNEL channel, uint8_t note, uint8_t velocity);
//    void writeNoteOff(MIDI_WRITER_MIDI_CHANNEL channel, uint8_t note, uint8_t velocity);
//    void writeControl(MIDI_WRITER_MIDI_CHANNEL channel, uint8_t control, uint8_t value);
//    void writeTimeSignature(uint8_t num, uint8_t den, uint8_t nmidi, uint8_t n32note);
//
//    void saveCloseMidiFile();
//
//private:
//    QFile *m_midiFile;
//    QByteArray m_array;
//
//    void writeVariableLength(uint32_t value);
//    void writeFixedLength(uint32_t value, uint8_t nBytes);
//    void replaceFixedLength(uint32_t value, uint8_t nBytes, uint32_t position);
//
//
//};
//
//#endif // CONVERTTRACKTOMIDI_H
