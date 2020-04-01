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
#include "midiParser.h"
#include "math.h"
#include <stdio.h>
#include <vector>
#include <iterator>




/*******************************************************************************
 *                          Internal Macro
 *******************************************************************************/
#define TICK_PER_QUARTER_NOTE   (480)

#define STANDART_FHEADER_SIZE   (14u)
#define STANDART_THEADER_SIZE   (8u)


#define MIDI_EVENT_MASK         (0xF0u)
#define NOTE_ON                 (0x90u)
#define NOTE_OFF                (0x80u)
#define KEY_AFTERTOUCH          (0xa0u)
#define CONTROL_CHANGE          (0xb0u)
#define PROGRAM_PATCH           (0xc0u)
#define CHANNEL_AFTERTOUCH      (0xd0u)
#define PITCH_WHEEL_CHANGE      (0xe0u)
#define META_EVENT              (0xf0u)

#define TRACK_SEQUENCE_NUMBER   (0x00u)
#define TEXT_EVENT              (0x01u)
#define COPYRIGHT               (0x02u)
#define TRACK_NAME              (0x03u)
#define INSTRUMENT_NAME         (0x04u)
#define LYRICS                  (0x05u)
#define MARKER                  (0x06u)
#define CUE_POINT               (0x07u)
#define END_OF_TRACK            (0x2fu)
#define SET_TEMPO               (0x51u)
#define TIME_SIGNATURE          (0x58u)
#define KEY_SIGNATURE           (0x59u)
#define SEQUENCER               (0x7fu)



typedef struct{
    unsigned short format;
    unsigned short numberOfTrack;
    unsigned short deltaTickToQuarter;
} File_Header;



/*****************************************************************************
 **                     FUNCTION PROTOTYPES
 *****************************************************************************/
static uint32_t readTrackLength(int *p_errors);
static uint32_t readVariableLength(void);
static uint32_t midiPostAnalyse(MIDIPARSER_MidiTrack *track, int *p_errors);
static void offsetTrack(MIDIPARSER_MidiTrack *track, int32_t offset);

MIDIPARSER_MidiTrack& MIDIPARSER_MidiTrack::read(const char* ptr, unsigned size)
{
    if (size <= 0); else if (size < header_size()+sizeof(unsigned) || size < header_size()+sizeof(unsigned)+sizeof(MIDIPARSER_MidiEvent)**(unsigned*)(ptr+header_size())) {
        memset(this, 0, header_size());
        event.clear();
        return *this;
    }
    memcpy(this, ptr, header_size()); ptr += header_size();
    event.resize(*(unsigned*)ptr); ptr += sizeof(unsigned);
    if (auto szd = event.size()*sizeof(MIDIPARSER_MidiEvent)) memcpy(&event[0], ptr, szd);
    return *this;
}
MIDIPARSER_MidiTrack::operator QByteArray() const
{
    const auto szd = event.size()*sizeof(MIDIPARSER_MidiEvent);
    QByteArray ret(header_size()+sizeof(unsigned)+szd, Qt::Uninitialized);
    auto ptr = ret.data();
    memcpy(ptr, this, header_size()); ptr += header_size();
    *(unsigned*)ptr = event.size(); ptr += sizeof(unsigned);
    if (szd) memcpy(ptr, &event[0], szd);
    return ret;
}



#include <set>
#include <fstream>
#include <sstream>

#ifdef __MINGW32__
#include <vector>
#include <memory>
#include <set>

using namespace std;
#endif

template <typename T> T reverse(const T& t)
{
    auto ret = t;
    auto p = (char*)&ret;
    for (auto s = sizeof(T)-1, i = s/2; i+1; --i)
        std::swap(p[i], p[s-i]);
    return ret;
}

template <typename T> struct strip { typedef T type; };
template <typename T> struct strip<const T> { typedef typename strip<T>::type type; };
template <typename T> struct strip<T*> { typedef typename strip<T>::type type; };
template <typename T> struct strip<T&> { typedef typename strip<T>::type type; };

#ifndef __MINGW32__

template <typename T> struct make_unsigned { typedef T type; };
template <> struct make_unsigned<char> { typedef unsigned char type; };
template <> struct make_unsigned<short> { typedef unsigned short type; };
template <> struct make_unsigned<int> { typedef unsigned type; };
template <> struct make_unsigned<long> { typedef unsigned long type; };
template <> struct make_unsigned<long long> { typedef unsigned long long type; };

#endif

template <typename T> struct varlength
{
    T _;
    varlength(T _) : _(_) {}
    typedef typename strip<T>::type type;
    typedef typename make_unsigned<type>::type flat;
    operator type() const
    {
        type ret = 0;
        for (auto copy = flat(_), go = flat(0); copy; copy >>= 7, go = 0x80)
            ret = (ret << 8) | go | (copy & 0x7F);
        return ret;
    }
    T operator=(const type& t)
    {
        auto ret = 0;
        for (auto copy = flat(_); copy; copy >>= 8)
            ret = (ret << 7) | (copy & 0x7F);
        return this->t = ret;
    }
};

template <typename T> varlength<const T&> var_q(const T& t) { return varlength<const T&>(t); }
template <typename T> varlength<T&> var_q(T& t) { return varlength<T&>(t); }

template <typename Stream, typename T> struct stream_op {
    static void write(Stream& stream, const T& t) {
        auto c = reverse(t);
#ifdef __MINGW32__
        auto x = sizeof(T);
        stream->write((char*)&c, x);
#else
        stream.write((char*)&c, sizeof(T));
#endif
    }
};
template <typename Stream> struct stream_op<Stream, std::string> {
    static void write(Stream& stream, const std::string& string) {
        // do not reverse string data
#ifdef __MINGW32__
        stream->write(string.c_str(), string.size());
#else
        stream.write(string.c_str(), string.size());
#endif
    }
};
template <typename Stream, typename T> struct stream_op<Stream, varlength<T>> {
    static void write(Stream& stream, const varlength<T>& _) {
        // handle varlength data differently
        typename varlength<T>::type c = _;
        auto p = (char*)&c;
        auto i = sizeof(T);
        for (auto go = true; go && i --> 0; go = *p++ < 0){
#ifdef __MINGW32__
            stream->write(p, 1);
#else
            stream.write(p, 1);
#endif
        }
    }
};
template <typename Stream>
struct reverse_stream
{
    Stream stream;
    reverse_stream(Stream& stream) { this->stream.swap(stream); }

    template <typename T> reverse_stream& operator<<(const T& t) { stream_op<Stream, T>::write(stream, t); return *this; }
};
template <typename Stream> reverse_stream<Stream> make_reverse(Stream stream) { return reverse_stream<Stream>(stream); }

// TODO : support pickup notes similar to ConvertBbtToMIDI::convert

#ifdef __MINGW32__
template <typename T>
class MinGW_sucks_ass
{
    std::unique_ptr<T> _;
public:
    template <typename... Args>
    explicit MinGW_sucks_ass(Args&&... args)
        :   _(new T( std::forward<Args>(args)... )) {}

    MinGW_sucks_ass (MinGW_sucks_ass &&other) noexcept
        : _(nullptr)
    {
        swap(other);
    }

    MinGW_sucks_ass &operator = (MinGW_sucks_ass &&other) noexcept
    {
        swap(other);
        return *this;
    }

    void swap(MinGW_sucks_ass &other) noexcept
    {
        std::swap(_, other._);
    }

    T& operator *() { return *_; }
    T const& operator *() const { return *_; }

    T * operator ->() { return _.get(); }
    T const * operator ->() const { return _.get(); }
};

template <typename S>
inline void swap(MinGW_sucks_ass<S> &one, MinGW_sucks_ass<S> &two) noexcept
{ one.swap(two); }
#endif

void MIDIPARSER_MidiTrack::write_file(const std::string& name) const
{
    /* preparing track */
#ifdef __MINGW32__
    auto trs = make_reverse(MinGW_sucks_ass<std::ostringstream>());
#else
    auto trs = make_reverse(std::ostringstream());
#endif
    /* copyleft */
    std::string comment("MIDI Editor v.56047 (c) Singular Sound");
    const char zero = 0x00;
    trs << zero // offset
        << char(0xFF) << char(0x02) // Copyright Meta Event
        << var_q(comment.size()) // length
        << comment // copyright data
        ;
    /* time signature */
    trs << zero // offset
        << char(0xFF) << char(0x58) << char(0x04) // Time Signature Meta Event
        << char(timeSigNum) // numerator
        << char([this] { char ret = -1; for (auto den = timeSigDen; den; ++ret, den >>= 1); return ret; }()) // denumerator, log2(timeSigDen)
        << char(24) // MIDI Clocks per quarter note
        << char(8) // Number of 1/32 notes per 24 MIDI clocks
           ;

    {
        /* note on events */
        std::set<char> on;
        auto sz = event.size();
        int pos=0;

        // detect note offs.. this won't take many notes to figure out
        bool writeOffs=true; // default is no note-offs unless you FIND one.
        std::set<char> off;
        for (auto i = 0u; i < sz && event[i].tick <= nTick; ++i) {
            if (1==event[i].type || 0==event[i].vel) { // note-off or zero vel note!
                fprintf(stderr, "found noteoff or zero vel\n");
                writeOffs=false;
                break;
            }
            if (off.count(event[i].note)) { // found two notes without a noteoff in between
                fprintf(stderr, "found noteon without noteoff\n");
                writeOffs = true;
                break;
            } else {
                off.insert(event[i].note); // save note, and keep looking
            }
        }

        std::vector<MIDIPARSER_MidiEvent> offs(sz);
        if (writeOffs) {
            int j=0;
            for (auto i = 0u; i < sz && event[i].tick <= nTick; ++i) {
                int newtick = (event[i].tick+50 < nTick ? event[i].tick+50 : nTick);
                MIDIPARSER_MidiEvent offev(newtick, event[i].note, 0, 1);
                offs[j++] = offev;
            }
        }
        std::vector<MIDIPARSER_MidiEvent> newEvent(sz*2);
        {
            auto itEvent = event.begin();
            auto itEventEnd = event.end();
            auto itOff = offs.begin();
            auto itOffEnd = offs.end();
            while (itEvent != itEventEnd && itOff != itOffEnd) {
                MIDIPARSER_MidiEvent ev = *itEvent;
                MIDIPARSER_MidiEvent offev = *itOff;

                if (ev.tick<=offev.tick) {
                    newEvent.push_back(ev);
                    itEvent++;
                } else {
                    newEvent.push_back(offev);
                    itOff++;
                }
            }
            while (itEvent != itEventEnd) {
                newEvent.push_back(*itEvent++);
            }
            while (itOff != itOffEnd) {
                newEvent.push_back(*itOff++);
            }
            offs.clear();
        }

        sz=newEvent.size();
        if (sz) {
            trs << var_q(newEvent[0].tick-0) // start offset
                    << char(0x99) //  Note ON for Channel 10 (running status)
                       ;
            pos=newEvent[0].tick;
        }
        for (auto i = 0u; i < sz && newEvent[i].tick <= nTick; ++i) {

            if (i) trs << var_q(newEvent[i].tick - newEvent[i-1].tick);

            if (i) pos+=(newEvent[i].tick - newEvent[i-1].tick);
            trs << char(newEvent[i].note) << char(newEvent[i].vel);
        }
        fprintf(stderr, "sz:%d\n", sz);
    }

    trs << zero // offset
        << char(0xFF) << char(0x2F) << char(0x00); // End of Track


#ifdef __MINGW32__
    auto trk = trs.stream->str();
    /* writing the result file as a one-liner */
    make_reverse(MinGW_sucks_ass<std::ofstream>(name, std::ios::out | std::ios::binary))
#else
    auto trk = trs.stream.str();
    /* writing the result file as a one-liner */
    make_reverse(std::ofstream(name, std::ios::out | std::ios::binary))
#endif
        /* header */
        << 'MThd' // file signature
        << 6 // header length
        << short(0) // format
        << short(1) // track count
        << short(barLength*timeSigDen/timeSigNum/4) // ticks per quarter note
        /* track */
        << 'MTrk' // track signature
        << int(trk.size()) // track length
        << trk // track content
        ;
}

/*****************************************************************************
 **                    INTERNAL GLOBAL VARIABLE
 *****************************************************************************/
static volatile uint8_t* DataPtr;
static volatile uint32_t Index;

/**
 * @brief
 * @param
 * @retval
 */
uint32_t midi_ParseFile(uint8_t* dataPtr, uint32_t length,MIDIPARSER_MidiTrack *track, MIDIPARSER_TrackType_t trackType, int* p_errors){

    uint32_t trackStopIndex = 0;
    uint32_t trackSize = 0;
    float tickRatio = 0;
    uint8_t carac = 0;
    uint8_t status = 0;
    uint8_t runningStatus = 0; // For the support of the running status midi file
    uint32_t tmpDelay = 0;
    uint32_t iTrack = 0; // Number of the current track being analysed
    int32_t lastNoteOnTick = 0;
    int32_t lastNoteOffTick = 0;
    int32_t index;

#if defined(_MSC_VER)
    trackType; // warning C4100: 'trackType' : unreferenced formal parameter
#endif

    // Clear all errors before starting parser
    *p_errors = MIDIPARSER_NO_ERROR;

    // Set the global address for the file and reset the scan index
    DataPtr = dataPtr;
    Index = 0;
    track->event.clear();


    track->bpm = 0;
    track->index = 0;

    // Initialize time signature to a valid value in case there would be no time sig in file
    // use 4/4 by default
    track->timeSigNum = 0;
    track->timeSigDen = 0;


    // Look for the standard 4 caracter format MThd in little endian
    if ((DataPtr[0] != 'M') ||
        (DataPtr[1] != 'T') ||
        (DataPtr[2] != 'h') ||
        (DataPtr[3] != 'd') ) {

       *p_errors |= MIDIPARSER_INVALID_FILE_ID_ERROR;
       return 0;
    }

    // Standard header file must have a length of 6
    if (DataPtr[7] != 6){
       *p_errors |= MIDIPARSER_INVALID_HEADER_SIZE_ERROR;
       return 0 ;
    }

    // read the file format
    track->format = ((unsigned short)DataPtr[8] << 8) | (unsigned short)DataPtr[9];
    track->nTrack = ((unsigned short)DataPtr[10] << 8) | (unsigned short)DataPtr[11];
    track->tpqn = ((unsigned short)DataPtr[12] << 8) | (unsigned short)DataPtr[13];

    // Ratio to convert the tick precicsion of the midi file to the wanted precision
    tickRatio =  (float)TICK_PER_QUARTER_NOTE/(float)track->tpqn;

    // Change the tick to quarter because all file must have the same precision
    if(track->tpqn != TICK_PER_QUARTER_NOTE){
        // set warning
        *p_errors |= MIDIPARSER_CHANGED_TICK_PER_QUARTER_NOTE_WARN;
        track->tpqn = TICK_PER_QUARTER_NOTE;
    }





    // If read success advance the index to 14 ( the standard size of a midi header file)
    Index = 14;

    // While no track with note on/off detected
    while (!track->event.size() && iTrack < track->nTrack){
        //fprintf(stderr, "analyzing track %d, %d, index:%d\n", iTrack, track->event.size(), Index);

        // Read the track size and calculate the stop index
        trackSize = readTrackLength(p_errors);

        if (trackSize <= 0){
           *p_errors |= MIDIPARSER_EMPTY_TRACK_WARN;
        }
        trackStopIndex = Index + trackSize;

        tmpDelay = 0;

        // For the length of the current track
        while(Index < trackStopIndex){

            // Read the variable length value of the delay
            tmpDelay += readVariableLength();
            carac = DataPtr[Index];

            // Verify for running status message
            if (carac & 0x80){
                status = carac;
                Index++; // Advacne the index to go to the parameter
            } else {
                status = runningStatus;
                // No need to advance the index cause it's already on the parameters of the status
            }

            // Dispatch the right message
            switch (status & MIDI_EVENT_MASK) {

            case NOTE_ON:
                {
                    track->event.push_back(MIDIPARSER_MidiEvent());
                    auto& event = track->event.back();
                    event.tick = (int32_t)((float)tmpDelay * tickRatio);
                    event.note = DataPtr[Index++];
                    event.vel = DataPtr[Index++];
                    event.reserved = 0;
                    event.type = 0;


                    // todo (verify) We also save last note on tick event if we save last note off.
                    if (track->event.back().vel == 0){
                        lastNoteOffTick =(uint32_t)((float)tmpDelay * tickRatio);
                    }

                    lastNoteOnTick = (uint32_t)((float)tmpDelay * tickRatio);

                    // Set the running status for the next event if no type is send
                    runningStatus = status;
                }
                break;

            case NOTE_OFF:
            {
                // create a zero velocity note on
                track->event.push_back(MIDIPARSER_MidiEvent());
                auto& event = track->event.back();
                event.tick = (int32_t)((float)tmpDelay * tickRatio);
                event.note = DataPtr[Index++];
                event.vel = 0;
                Index++;
                event.reserved = 0;
                event.type = 1;

                lastNoteOffTick =(uint32_t)((float)tmpDelay * tickRatio);
                
                runningStatus = status;
            }
                break;

            case KEY_AFTERTOUCH:
            case CONTROL_CHANGE:
            case PITCH_WHEEL_CHANGE:
                // Do nothing and skip the 2 parameters
                Index += 2u;
                runningStatus = status;
                break;

            case PROGRAM_PATCH:
            case CHANNEL_AFTERTOUCH:
                // Program change & Channel Aftertouch have only one parameter
                Index += 1;
                runningStatus = status;
                break;

            case META_EVENT:
                carac = DataPtr[Index++];
                runningStatus = 0;
                switch (carac) {
                case TIME_SIGNATURE:

                    length = DataPtr[Index++];
                    if (length == 4){
                        track->timeSigNum = DataPtr[Index];
                        track->timeSigDen = 1 << DataPtr[Index + 1];
                        track->midiClocksPerMetronomeClick = DataPtr[Index + 2];
                        track->n32ndNotesPerMIDIQuarterNote = DataPtr[Index + 3];
                    }
                    Index += length;
                    break;
                case SET_TEMPO:
                {
                    int len = DataPtr[Index++]; // always 3
                    int tt1 = DataPtr[Index++];
                    int tt2 = DataPtr[Index++];
                    int tt3 = DataPtr[Index++];
                    float tempdiv = 65536*tt1+256*tt2+tt3;
                    float tempofloat = 60000000.0/tempdiv+.5;
                    int tempo = int(tempofloat);

                    fprintf(stderr, "found tempo at %d! ff:%d tp:%d len:%d 1:%d 2:%d 3:%d tempo:%d %f\n", tmpDelay, 256, SET_TEMPO, len, tt1, tt2, tt3,
                            tempo, tempdiv);

                    if (0 == track->bpm) {
                        fprintf(stderr, "setting tempo in track to %d\n", tempo);
                        track->bpm = tempo;
                    }
                    break;
                }

                case TRACK_SEQUENCE_NUMBER:
                case TEXT_EVENT:
                case COPYRIGHT:
                case TRACK_NAME:
                case INSTRUMENT_NAME:
                case LYRICS:
                case MARKER:
                case CUE_POINT:
                case KEY_SIGNATURE:
                case SEQUENCER:

                    length = DataPtr[Index++];
                    Index += length;
                    break;
                case END_OF_TRACK:
                    // Will stop the track parser
                    fprintf(stderr, "end of track: %d %d\n", Index, trackStopIndex);
                    Index = trackStopIndex;
                    break;

                default:
                    break;
                }

                break;
            default:
                Index++;
                fprintf(stderr, "unknown error %d\n", Index);
                // Not suppose to happen
                *p_errors |= MIDIPARSER_UNKNOWN_EVENT_CODE_WARN;
                break;
            }
        }
        // At this point we save the last note on/off tick
        track->nTick = lastNoteOnTick;
        iTrack++;
    }



    if (track->event.size() == 0){
       *p_errors |= MIDIPARSER_NO_EVENT_ERROR;
       return 0;
    }

    // Verify for notes on without notes off (GORAN GROOVE BUG)


    // if the track finish with a note on without note off
    if (lastNoteOffTick < lastNoteOnTick){
        *p_errors |= MIDIPARSER_END_NO_NOTE_OFF_WARN;

        // Go to the last event in the array
        index = track->event.size() - 1;

        if (index == 0) {
            *p_errors |= MIDIPARSER_NO_EVENT_ERROR;
            return 0;
        }

        // Remove the note ON
        while(track->event[index].tick > lastNoteOffTick){
            index--;
            if (index < 0) {
                *p_errors |= MIDIPARSER_NO_EVENT_ERROR;
                return 0;
            }
        }

        track->nTick = track->event[index].tick;
    }



    // if the post analyse is succesful, return the number of midi event parsed
    if (midiPostAnalyse(track, p_errors)){
        return track->event.size();
    } else {
        *p_errors |= MIDIPARSER_POST_ANALYSIS_ERROR;
        return 0;
    }

}



static uint32_t midiPostAnalyse(MIDIPARSER_MidiTrack *track, int *p_errors){
    int32_t offset;
    uint32_t tickPerBeat;
    uint32_t tempBarLength;

    // Index of pickup. limit to 1 quarter note always;
    int32_t index_of_pickup;

    if (track->timeSigDen < 2){
       *p_errors |= MIDIPARSER_CHANGED_TIME_SIG_DEN_WARN;
       track->timeSigDen = 4;
    }
    if (track->timeSigNum == 0){
       *p_errors |= MIDIPARSER_CHANGED_TIME_SIG_NUM_WARN;
       track->timeSigNum = track->timeSigDen;
    }

    if (track->timeSigDen && track->timeSigDen == (track->timeSigDen & (1 +~ track->timeSigDen))); else
    {
       *p_errors |= MIDIPARSER_TIME_SIG_NUM_POWER_2_ERROR;
       return 0;
    }

    // Calculate the number of midi tick within a beat
    tickPerBeat = ((4 * track->tpqn) / track->timeSigDen);

    if (!tickPerBeat){
       *p_errors |= MIDIPARSER_INVALID_TICK_PER_BEAT_ERROR;
       return 0;
    }


    // Calculate the size of one Bar
    track->barLength = (track->timeSigNum ) * tickPerBeat;

    // Test : Take the last note on event for a nTick
    track->nTick = track->event.back().tick;

  
    index_of_pickup  = (int) (0.75f * track->barLength);

    track->trigPos = (int) (0.5f * track->barLength);
    // Pick-up notes detector
    if (track->event[0].tick > index_of_pickup){
        offset = track->barLength;
        offsetTrack(track, 0 -  offset);
        tempBarLength = track->barLength;
        track->pickupNotesLength  =  0 - track->event[0].tick;
    } else {
        // The first event must happen during the first beat
        offset = tickPerBeat * (track->event[0].tick / tickPerBeat);
        offsetTrack(track, 0 -  offset);
        tempBarLength = track->barLength - (offset % track->barLength);
        track->pickupNotesLength = 0;
    }

    // Rearrange the length of the track
    if ((track->nTick % tempBarLength) != 0){
        if ((track->nTick % tempBarLength) < (tickPerBeat)){

            // Inferior of 1 beat ( like cymbal after)
            track->nTick =  (track->nTick / tempBarLength) * tempBarLength;
        } else {
            // Over one beat go to next beat
            track->nTick = ( 1 + track->nTick / tickPerBeat) * tickPerBeat;
        }
    }

    return 1;
}




/**
 *	Read the track length
 *
 *	Returns the number of byte read (0 if invalid header format )
 */
static uint32_t readTrackLength(int *p_errors){
    uint32_t tmpIndex = Index;
    fprintf(stderr, "ind:%d %d %d %d %d\n", tmpIndex, DataPtr[tmpIndex], DataPtr[tmpIndex+1] , DataPtr[tmpIndex+2] , DataPtr[tmpIndex+3]);

    // Look for the standard 4 characters format
    if ((DataPtr[tmpIndex    ] != 'M') ||
        (DataPtr[tmpIndex + 1] != 'T') ||
        (DataPtr[tmpIndex + 2] != 'r') ||
        (DataPtr[tmpIndex + 3] != 'k') ){

       *p_errors |= MIDIPARSER_INVALID_TRACK_ID_ERROR;
       return 0;
    }


    Index +=8;
    // Get the track size ( not in variable length representation)
    return ((uint32_t)DataPtr[tmpIndex + 4] << 24) |
            ((uint32_t)DataPtr[tmpIndex + 5] << 16) |
            ((uint32_t)DataPtr[tmpIndex + 6] << 8)  |
            ((uint32_t)DataPtr[tmpIndex + 7]);
}

/**
 * Read a variable length number in the file and advence the index
 *
 * Return the value read
 */
static uint32_t readVariableLength(void){
    uint32_t value = 0;   // value of the variable length variable
    uint8_t carac;      // current character

    // do the read while the MSB of the current character is "1"
    do{
        carac = DataPtr[Index++];
        value = (value << 7) + (carac & 0x7Fu);
    }while(carac & 0x80u);

    return value;
}


/**
 * Apply an offset to each midi event of the track
 */
static void offsetTrack(MIDIPARSER_MidiTrack *track, int32_t offset){
    // Applay offset to each of the event of the track
    for (auto i = 0u; i < track->event.size(); i++){
        track->event[i].tick += offset;
    }
    track->nTick += offset;
}
