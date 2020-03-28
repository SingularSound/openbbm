/**
 *
 *
 *
 *
 *
 *
 */
#ifndef _MIDI_PARSER_H_
#define _MIDI_PARSER_H_

#include <stdint.h>
#include "../../pragmapack.h"

#include <vector>
#include <QByteArray>


/*****************************************************************************
 **                     EXTERNAL TYPDEF
 *****************************************************************************/

typedef enum MIDIPARSER_TrackType {
   INTRO_FILL,
   OUTRO_FILL,
   MAIN_DRUM_LOOP,
   DRUM_FILL,
   TRANS_FILL,
   ENUM_LENGTH
} MIDIPARSER_TrackType_t;

PACK typedef struct MIDIPARSER_MidiEvent {
    int32_t tick;       // Position in tick for the event ( can be negative for negative pick-up notes)
    uint8_t type;       // Maybe (like Note ON, Note OFF, AfterEffect...) // FOr the moment, note ON are 0
    uint8_t note;       // Notes from 0 to 127
    uint8_t vel;        // Velocity from 0 to 127
    uint8_t reserved;   // Reserved for future use and padding for a complete 32 bit

    MIDIPARSER_MidiEvent(int tick = 0, int note = 0, int vel = 0, int type = 0) : tick(tick), type(type), note(note), vel(vel) {}
}PACKED MIDIPARSER_MidiEvent;


PACK typedef struct MIDIPARSER_MidiTrack {
    uint32_t format; // Midi type format (0-1)
    uint32_t nTrack;

    int32_t nTick;
    int32_t pickupNotesLength;
    int32_t unused_0; // additionalNotesLength;
    uint8_t  timeSigNum;
    uint8_t  timeSigDen;
    uint8_t  n32ndNotesPerMIDIQuarterNote;
    uint8_t  midiClocksPerMetronomeClick;
    uint32_t tpqn; // tick to quarter
    int32_t barLength;
    int32_t trigPos;
    uint32_t bpm;
    uint32_t index;
    uint32_t reserved_1;
    uint32_t reserved_2;
    std::vector<MIDIPARSER_MidiEvent> event;

    static unsigned header_size() { static const unsigned szh = sizeof(MIDIPARSER_MidiTrack) - sizeof(std::vector<MIDIPARSER_MidiEvent>); return szh; }
    unsigned size() const { return header_size() + sizeof(unsigned) + event.size()*sizeof(MIDIPARSER_MidiEvent); }

    MIDIPARSER_MidiTrack() { memset(this, 0, header_size()); }
    MIDIPARSER_MidiTrack(const char* ptr, unsigned size = 0) { read(ptr, size); }
    MIDIPARSER_MidiTrack(const QByteArray& arr) { read(arr.data(), arr.size()); }
    MIDIPARSER_MidiTrack(MIDIPARSER_MidiTrack&& mt) { memcpy(this, &mt, header_size()); event.swap(mt.event); }
    MIDIPARSER_MidiTrack(const MIDIPARSER_MidiTrack& mt, bool copy_events = true) { memcpy(this, &mt, header_size()); if (copy_events) event = mt.event; }
    MIDIPARSER_MidiTrack& read(const char* ptr, unsigned size = 0);
    MIDIPARSER_MidiTrack& operator=(const QByteArray& arr) { read(arr); return *this; }
    MIDIPARSER_MidiTrack& operator=(const MIDIPARSER_MidiTrack& mt) { memcpy(this, &mt, header_size()); event = mt.event; return *this; }
    operator QByteArray() const;

    void write_file(const std::string& name) const;

} PACKED MIDIPARSER_MidiTrack;

typedef enum ErrorTypes {
    MIDIPARSER_NO_ERROR                           = 0x00000000,

    MIDIPARSER_CHANGED_TIME_SIG_DEN_WARN          = 0x00000001,
    MIDIPARSER_CHANGED_TIME_SIG_NUM_WARN          = 0x00000002,
    MIDIPARSER_CHANGED_TICK_PER_QUARTER_NOTE_WARN = 0x00000004,
    MIDIPARSER_UNKNOWN_EVENT_CODE_WARN            = 0x00000008,
    MIDIPARSER_END_NO_NOTE_OFF_WARN               = 0x00000010,
    MIDIPARSER_EMPTY_TRACK_WARN                   = 0x00000020,

    MIDIPARSER_INVALID_FILE_ID_ERROR              = 0x00010000,
    MIDIPARSER_INVALID_HEADER_SIZE_ERROR          = 0x00020000,
    MIDIPARSER_NO_EVENT_ERROR                     = 0x00040000,
    MIDIPARSER_TOO_MANY_EVENTS_ERROR              = 0x00080000,
    MIDIPARSER_TIME_SIG_NUM_POWER_2_ERROR         = 0x00100000,
    MIDIPARSER_INVALID_TICK_PER_BEAT_ERROR        = 0x00200000,
    MIDIPARSER_POST_ANALYSIS_ERROR                = 0x00400000,
    MIDIPARSER_INVALID_TRACK_ID_ERROR             = 0x00800000,
} MIDIPARSER_ErrorTypes_t;

/*****************************************************************************
 **                     FUNCTION PROTOTYPES
 *****************************************************************************/
uint32_t midi_ParseFile(unsigned char* file, uint32_t length, MIDIPARSER_MidiTrack *track, MIDIPARSER_TrackType_t trackType, int* p_errors);

#endif
