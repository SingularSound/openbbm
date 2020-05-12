#ifndef _SONG_PLAYER_H_
#define _SONG_PLAYER_H_

#include "../model/filegraph/song.h"
#include "button.h"
#include "pragmapack.h"


/* Defined in .h on qt */
/* Defines the ID constant for fill choke group */
#define NO_PART_ID              (0)
#define MAIN_PART_ID            (1)
#define DRUM_FILL_ID            (2)
#define TRAN_FILL_ID            (3)
#define INTR_FILL_ID            (4)
#define OUTR_FILL_ID            (5)

PACK typedef struct {
  unsigned char num;
  unsigned char den;
} PACKED TimeSignature;


typedef enum{
    NO_SONG_LOADED                      =   0,
    STOPPED                             =   1,
    PAUSED                              =   2,

    INTRO                               =   3,
    PLAYING_MAIN_TRACK                  =   4,
    PLAYING_MAIN_TRACK_TO_END           =   5,

    NO_FILL_TRAN                        =   6,
    NO_FILL_TRAN_QUITTING               =   7,
    NO_FILL_TRAN_CANCEL                 =   8,

    OUTRO                               =   9,
    OUTRO_WAITING_TRIG                  =   10,
    OUTRO_CANCELED                      =   11,

    TRANFILL_WAITING_TRIG               =   12,
    TRANFILL_ACTIVE                     =   13,
    TRANFILL_QUITING                    =   14,
    TRANFILL_CANCEL                     =   15,

    DRUMFILL_WAITING_TRIG               =   16,
    DRUMFILL_ACTIVE                     =   17,
	
	SINGLE_TRACK_PLAYER                 =   18,


    N_PLAYER_STATUS                     =   19,
}SongPlayer_PlayerStatus;



/*****************************************************************************
**                     FUNCTION PROTOYPE
*****************************************************************************/
void SongPlayer_init(void); // <--
void SongPlayer_deInit(void);
void SongPlayer_reInit(void);
int SongPlayer_loadSong(char* file, uint32_t length);
void SongPlayer_forceStop(void);   // <--
void SongPlayer_processSong(float ratio, int nEvent); // <--
void SongPlayer_ProcessSingleTrack(float ratio, int nTick, int offset);
int SongPlayer_calculateSingleTrackOffset(unsigned int nTicks, unsigned int tickPerBar);
void SongPlayer_ButtonCallback(BUTTON_EVENT event,unsigned long long time);
void SongPlayer_SetSingleTrack(MIDIPARSER_MidiTrack *track);
void SongPlayer_externalStart(void);
void SongPlayer_externalStop(void);
int SongPlayer_getBeatInbar(int32_t *startBeat);
int SongPlayer_getMasterTick(void);

int SongPlayer_getTimeSignature(TimeSignature * timeSignature);
int SongPlayer_getbarLength();
int SongPlayer_getTempo();

unsigned int SongPlayer_getNextNoteValue(unsigned char * nextNote, unsigned int length);

void SongPlayer_getPlayerStatus (
    SongPlayer_PlayerStatus *playerStatus,
    unsigned int *partIndex,
    unsigned int *drumfillIndex
);




#endif
