/**
 *  \file   songPlayer.c
 *
 *  \brief  Song Player of the beat buddy. This code get a file from the soung manager and ticks from tempo.c
 *          and play note.
 */
#ifndef am335x
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <QDebug>


#include "../model/filegraph/songfile.h"
#include "button.h"
#include "songPlayer.h"
#include "soundManager.h"
#include "settings.h"
#else

#include "songPlayer.h"
#include "button.h"
#include "soundManager.h"
#include "stdio.h"
#include "string.h"
#include "stdlib.h"
#include "tempo.h"
#include "uartStdio.h"
#include "button.h"
#include "soc_AM335x.h"
#include "gpio_v2.h"
#include "interrupt.h"
#include "hw_types.h"
#include "song.h"
#include "gui.h"
#include "uartMidi.h"
#include "settings.h"

#endif

/*****************************************************************************
 **                      	INTERNAL TYPEDEF
 *****************************************************************************/
 
typedef enum {
    REQUEST_DONE,
    START_REQUEST,
    EXTERNAL_START_REQUEST,
    STOP_REQUEST,
    KILL_REQUEST,
    DRUMFILL_REQUEST,
    TRANFILL_REQUEST, // Set NextPartNumber. 1 @ nPart means a specific next part
    TRANFILL_QUIT_REQUEST,
    TRANFILL_CANCEL_REQUEST,
    OUTRO_CANCEL_REQUEST,
    PAUSE_REQUEST,
    SWAP_TO_OUTRO_REQUEST,
    NUMBER_OF_REQUEST
} SongFlags_t;

typedef enum {
    ACTION_NONE,
    ACTION_SPECIAL_EFFECT,
    ACTION_PAUSE_UNPAUSE,
    ACTION_OUTRO,
	ACTION_MAIN_PEDAL
} Player_FootswitchActions;

typedef enum {
    START_INTRO,
    STOP_SONG,
    START_FILL,
    START_TRANSITION
}PLAYER_actions;



typedef enum {
    MIDI_POS_INTRO,
    MIDI_POS_FIRST_PART,
    MIDI_POS_OUTRO,             /* na */
    MIDI_POS_END_OF_OUTRO,      /* na */
    N_MIDI_POS

}MidiTriggerPosition;

#ifndef am335x
typedef SONG_SongStruct     Song_t;
typedef SONG_SongPartStruct SongPart_t;
#endif

/*****************************************************************************
 **                      	INTERNAL MACROS
 *****************************************************************************/
#define POST_EVENT_MAX_TICK         (200)


#ifdef am335x

#define COUNT_IN_PART_ID        NO_PART_ID
#define COUNT_IN_VELOCITY       (100)
#define NUM_COUNT_IN_NOTE       (4)

#define bool uint32_t

#define MAIN_LOOP_PTR(partPtr)         (partPtr->trackPtr)
#define TRANS_FILL_PTR(partPtr)		   (partPtr->tranFillPtr)
#define DRUM_FILL_PTR(partPtr, index)  (partPtr->drumFillPtr[index])
#define INTRO_TRACK_PTR(songPtr)       (songPtr->intro.trackPtr)
#define OUTRO_TRACK_PTR(songPtr)       (songPtr->outro.trackPtr)

#else 
#define MAIN_LOOP_PTR(partPtr)         (partPtr->mainLoopIndex+1        ? &Tracks[partPtr->mainLoopIndex] : 0)
#define TRANS_FILL_PTR(partPtr)        (partPtr->transFillIndex+1       ? &Tracks[partPtr->transFillIndex] : 0)
#define DRUM_FILL_PTR(partPtr, index)  (partPtr->drumFillIndex[index]+1 ? &Tracks[partPtr->drumFillIndex[index]] : 0)
#define INTRO_TRACK_PTR(songPtr)       (songPtr->intro.mainLoopIndex+1  ? &Tracks[songPtr->intro.mainLoopIndex] : 0)
#define OUTRO_TRACK_PTR(songPtr)       (songPtr->outro.mainLoopIndex+1  ? &Tracks[songPtr->outro.mainLoopIndex] : 0)

#ifndef FALSE
#define FALSE false
#endif

#ifndef TRUE
#define TRUE true
#endif


#define SONGFILE_INVALID_FILE_FLAG_MASK SONGFILE_FLAG_SONG_FILE_INVALID_MASK
#define GUI_TAP_WINDOW  123456789
#define GUI_SONG_SEL    123456789





#endif


#define NO_DELAY                (0)



/*****************************************************************************
 **                     INTERNAL FUNCTION PROTOTYPE
 *****************************************************************************/

static bool isEndOfTrack(int pos, SONG_SongPartStruct *CurrPartPtr/*, int loop, int loopCount*/);

static void TrackPlay(MIDIPARSER_MidiTrack *track, int startTick, int endTick, float ratio,
        int manualOffset, unsigned int partID);

static int CalculateStartBarSyncTick(unsigned int tickPos,
        unsigned int tickPerBar, unsigned int barTriggerLimit);

static unsigned int CalculateTranFillQuitSyncTick(unsigned int tickPos,
        unsigned int tickPerBar);


static void ExternalNoteOnHandler(uint8_t note, uint8_t velocity);
static void ExternalNoteOffHandler(uint8_t note, uint8_t velocity);

/*****************************************************************************
 **                        STATIC FUNCTION PROTOTYPE
 *****************************************************************************/

// Simple Function to move in the song
static void NextPart(void);
static void SamePart(unsigned int nextDrumfill);
static void EndPart(void);
static void OutroPart(void);
static void SwapToOutro(void);
static void StopSong(void);
static void FirstPart(void);
static void IntroPart(void);

static void PauseUnpauseHandler(void);
static void SpecialEffectManager(void);

#ifdef am335x
static void CountIn(void);
static void settingsCallback(SETTINGS_main_key_enum key, void * value);
static getNotesTrack(MIDIPARSER_MidiTrack *track, uint8_t *present_note, uint8_t *note_list, int32_t *count);
#else

static uint32_t GUI_GetCurrentWindows(void);
static void UartMidi_SendStop(void);
static void UartMidi_sendContinue(void);
static void UartMidi_sendNextPartMessage(int32_t);
static void SpecialEffectManager(void);
static void GUI_ForceRefresh(void);
static void UartMidi_SendStop(void);
static void UartMidi_SendStart(void);
static uint8_t IntDisable(void);
static void IntEnable(uint8_t intStatus);
static void TEMPO_startWithInt(void);
static void ResetSongPosition(void);
#endif

/*****************************************************************************
 **                         INTERNAL GLOBAL VARIABLES
 *****************************************************************************/

// Variable for the song and the track in the player
static SONG_SongStruct *CurrSongPtr = nullptr;
static SONG_SongPartStruct *CurrPartPtr;

static volatile int MasterTick = 0;
static int TmpMasterPartTick = 0;

static volatile unsigned int PartIndex; // Current part index of the song
static unsigned int DrumFillIndex;      // Current drumfill index of the current part

AUTOPILOT_AutoPilotDataStruct * APPtr;
int32_t BeatCounter = 0;
int32_t AutopilotAction = FALSE;
int32_t AutopilotCueFill = FALSE;
int32_t AutopilotTransitionCount;

static int PartStopSyncTick;
static int PartStopPickUpSyncTickLength;

static int DrumFillStartSyncTick;
static int DrumFillPickUpSyncTickLength;

static int TranFillStartSyncTick;
static int TranFillStopSyncTick;
static int TranFillPickUpSyncTickLength;

static uint8_t PedalPressFlag; // Important to eliminate drumfill when quitting tap windows by the long pedal press
static uint8_t WasPausedFlag;
static uint8_t MultiTapCounter;
static uint8_t WasLongPressed;

static uint8_t DelayStartCmd = 0;
static volatile SongPlayer_PlayerStatus PlayerStatus;
static volatile SongPlayer_PlayerStatus LastPlayerStatus;
static volatile SongPlayer_PlayerStatus UnPausedPlayerStatus;
static volatile SongPlayer_PlayerStatus PausedPlayerStatus;
static volatile SongFlags_t RequestFlag;

static Player_FootswitchActions PrimaryFootSwitchPlayingAction;
static Player_FootswitchActions PrimaryFootSwitchStoppedAction;
static Player_FootswitchActions SecondaryFootSwitchPlayingAction;
static Player_FootswitchActions SecondaryFootSwitchStoppedAction;
static PLAYER_actions MainUnpauseModeTapToFill = START_FILL;
static PLAYER_actions MainUnpauseModeHoldToTransition = START_TRANSITION;
static int8_t ActivePauseEnable = DEFAULT_ACTIVE_PAUSE;
static int8_t TrippleTapEnable  = DEFAULT_TRIPLE_TAP_STOP;
static int8_t StartBeatOnPress;


static uint8_t MidiInNoteOnEnable;
static uint8_t MidiInNoteOffEnable;

static uint32_t NextPartNumber;
static int8_t SendStopOnPause = 0;
static int8_t SendStopOnEnd = 0;


#ifdef am335x
static SETTINGS_intro_fill IntroFillParam;
static SETTINGS_midi_out_start_message MidiOutStartMsg = DEFAULT_MIDI_OUT_START_MSG;
static SETTINGS_midi_out_sync_message MidiOutSyncSettings;
static SETTINGS_sobriety SobrietyFeature;

/*
 * List of drumset note for the count-in
 * Cross stick
 * Closed Hi-Hat
 * Open Hi-Hat
 * Snare
 */
static const uint8_t CountInNoteList[NUM_COUNT_IN_NOTE] = {37, 42, 46, 38};
static SETTINGS_autopilot_feature AutopilotFeature = DEFAULT_AUTOPILOT_FEATURE;
#else

// In VM, player is started like with an external midi message
// Need to start with Intro
static char MidiMessageStart = MIDI_POS_INTRO;
static SONGFILE_FileStruct *CurrSongFilePtr = nullptr;
static std::vector<MIDIPARSER_MidiTrack> Tracks;
static MIDIPARSER_MidiTrack *SingleMidiTrackPtr = nullptr;


#endif


static uint32_t SobrietyDrumTranFill;
static uint32_t SobriertySpecialEffectTickDelay;

static uint32_t Test;




/*****************************************************************************
 **                      	FUNCTION DEFINITION
 *****************************************************************************/

/**
 * @brief SongPlayer_init
 *    Make the initialisation of the songPlayer
 */
void SongPlayer_init(void) {

    PlayerStatus = NO_SONG_LOADED;
    RequestFlag = REQUEST_DONE;

    PedalPressFlag = 0; // Important to eliminate drumfill when quitting tap windows by the long pedal press
    WasPausedFlag = 0;
    MultiTapCounter = 0;
    WasLongPressed = 0;
    Test = 0;

    PrimaryFootSwitchPlayingAction = ACTION_SPECIAL_EFFECT;
    PrimaryFootSwitchStoppedAction = ACTION_SPECIAL_EFFECT;
    SecondaryFootSwitchPlayingAction = ACTION_SPECIAL_EFFECT;
    SecondaryFootSwitchStoppedAction = ACTION_SPECIAL_EFFECT;

   // AutopilotFeature = DEFAULT_AUTOPILOT_FEATURE;
    SobrietyDrumTranFill = 0;
    SobriertySpecialEffectTickDelay = 0;


    CurrSongPtr = nullptr;
    APPtr = nullptr;

    NextPartNumber = 0;

#ifdef am335x    // Register a callback from the butten class
    MidiOutSyncSettings = DEFAULT_MIDI_OUT_SYNC_MSG;
    SobrietyFeature = DEFAULT_SOBRIETY;
    SETTINGS_registerCallback(settingsCallback);
    UartMidi_setNoteOnListener(ExternalNoteOnHandler);
    UartMidi_setNoteOffListener(ExternalNoteOffHandler);
#endif
}

#ifdef am335x

void SongPlayer_setActivePauseEnable(int32_t enable){
    if (enable){
        ActivePauseEnable = 1;
    } else {
        ActivePauseEnable = 0;
    }
}

static void settingsCallback(SETTINGS_main_key_enum key, void* value) {

    switch (key) {
    case MAIN_UNPAUSE_MODE_TAP:

        if (MAIN_UNPAUSE_TAP_TO_FILL == (MAIN_UNPAUSE_TAP_params) (*(int32_t*)value)) {
            MainUnpauseModeTapToFill = START_FILL;
        } else {
            MainUnpauseModeTapToFill = START_INTRO;
        }
        break;

    case MAIN_UNPAUSE_MODE_HOLD:
        if (MAIN_UNPAUSE_HOLD_TO_START_TRAN == (MAIN_UNPAUSE_HOLD_params) (*(int32_t*)value)) {
            MainUnpauseModeHoldToTransition = START_TRANSITION;
        } else {
            MainUnpauseModeHoldToTransition = STOP_SONG;
        }
        break;

    case PRIMARY_FOOTSWITCH_ACTION_PLAYING:
        if ((*(int32_t*)value) == PLAYING_ACCENT_HIT) {
            PrimaryFootSwitchPlayingAction = ACTION_SPECIAL_EFFECT;
        } else if ((*(int32_t*)value) == PLAYING_PAUSE_UNPAUSE) {
            PrimaryFootSwitchPlayingAction = ACTION_PAUSE_UNPAUSE;
        } else if ((*(int32_t*)value) == PLAYING_OUTRO_FILL) {
            PrimaryFootSwitchPlayingAction = ACTION_OUTRO;
        } else {
            PrimaryFootSwitchPlayingAction = ACTION_NONE;
        }
        break;

    case SECONDARY_FOOTWTWICH_ACTION_PLAYING:
        if ((*(int32_t*)value) == PLAYING_ACCENT_HIT) {
            SecondaryFootSwitchPlayingAction = ACTION_SPECIAL_EFFECT;
        } else if ((*(int32_t*)value) == PLAYING_PAUSE_UNPAUSE) {
            SecondaryFootSwitchPlayingAction = ACTION_PAUSE_UNPAUSE;
        } else if ((*(int32_t*)value) == PLAYING_OUTRO_FILL) {
            SecondaryFootSwitchPlayingAction = ACTION_OUTRO;
        } else {
            SecondaryFootSwitchPlayingAction = ACTION_NONE;
        }
        break;

    case PRIMARY_FOOTSWITCH_ACTION_STOPPED:
        if ((*(int32_t*)value) == PLAYING_ACCENT_HIT) {
            PrimaryFootSwitchStoppedAction = ACTION_SPECIAL_EFFECT;
        } else {
            PrimaryFootSwitchStoppedAction = ACTION_NONE;
        }
        break;

    case SECONDARY_FOOTSWITCH_ACTION_STOPPED:
        if ((*(int32_t*)value) == PLAYING_ACCENT_HIT) {
            SecondaryFootSwitchStoppedAction = ACTION_SPECIAL_EFFECT;
        } else {
            SecondaryFootSwitchStoppedAction = ACTION_NONE;
        }
        break;

    case TRIPLE_TAP_STOP:
        if ((*(int32_t*)value) == TRIPLE_TAP_STOP_ENABLE) {
            TrippleTapEnable = 1;
        } else {
            TrippleTapEnable = 0;
        }
        break;
    case MIDI_OUT_START_MESSAGE:
        MidiOutStartMsg = (SETTINGS_midi_out_start_message) (*(int32_t*)value);
        break;

    case INTRO_FILL_CONTROL:
        IntroFillParam = (SETTINGS_intro_fill) (*(int32_t*)value);
        break;

    case START_BEAT:
        if ((*(int32_t*)value) == START_BEAT_ON_PRESS) {
            StartBeatOnPress = 1;
        } else {
            StartBeatOnPress = 0;
        }
        break;

    case MIDI_OUT_STOP_MESSAGE:
        if ((*(int32_t*)value) == MIDI_OUT_STOP_MESSAGE_PAUSE_ONLY) {
            SendStopOnPause = TRUE;
            SendStopOnEnd = FALSE;
        } else if ((*(int32_t*)value) == MIDI_OUT_STOP_MESSAGE_PAUSE_AND_END) {
            SendStopOnPause = TRUE;
            SendStopOnEnd = TRUE;
        } else if ((*(int32_t*)value) == MIDI_OUT_STOP_MESSAGE_END_ONLY) {
            SendStopOnPause = FALSE;
            SendStopOnEnd = TRUE;
        } else { // If disabled
            SendStopOnPause = FALSE;
            SendStopOnEnd = FALSE;
        }
        break;

    case MIDI_IN_NOTES_ON_MESSAGE:
        if (MIDI_IN_NOTES_ON_MESSAGE_ENABLE == (SETTINGS_midi_in_notes_on_message) (*(int32_t*)value)){
            MidiInNoteOnEnable = TRUE;
        } else {
            MidiInNoteOnEnable = FALSE;
        }
        break;

    case MIDI_IN_NOTES_OFF_MESSAGE:
        if (MIDI_IN_NOTES_OFF_MESSAGE_ENABLE == (SETTINGS_midi_in_notes_off_message) (*(int32_t*)value)){
            MidiInNoteOffEnable = TRUE;
        } else {
            MidiInNoteOffEnable = FALSE;
        }
        break;

    case MIDI_OUT_SYNC_MESSAGE:
        MidiOutSyncSettings = (SETTINGS_midi_out_sync_message) (*(int32_t*)value);
        break;

    case SOBRIETY_FEATURE:
        SobrietyFeature = (SETTINGS_sobriety) (*(int32_t*)value);
        SobrietyDrumTranFill = 0;
        Test = 0;
        break;
    case AUTO_PILOT_FEATURE:
        AutopilotFeature = (SETTINGS_autopilot_feature) (*(int32_t*)value);
        break;
    }
}
#endif

int32_t SongPlayer_getBeatInbar(int32_t *startBeat) {
    int32_t return_value = 0;
    int32_t offset;
    uint8_t status = IntDisable();
    *startBeat = 0;

#ifdef am335x
    if (CurrSongPtr != NULL ) {

        switch (PlayerStatus) {

        case NO_SONG_LOADED:
            break;
        case STOPPED:
            if ((IntroFillParam == INTRO_FILL_ENABLE) && INTRO_TRACK_PTR(CurrSongPtr)){
                return_value = ((MasterTick
                        / (INTRO_TRACK_PTR(CurrSongPtr)->barLength
                                / INTRO_TRACK_PTR(CurrSongPtr)->timeSigNum))
                        % INTRO_TRACK_PTR(CurrSongPtr)->timeSigNum);
                // Calculate the offset for not complete intro ( 2 beat in 4/4) ( 6 beat in 4/4)
                offset = INTRO_TRACK_PTR(CurrSongPtr)->barLength -
                        (INTRO_TRACK_PTR(CurrSongPtr)->nTick % INTRO_TRACK_PTR(CurrSongPtr)->barLength);
                *startBeat = (offset /
                        (INTRO_TRACK_PTR(CurrSongPtr)->barLength / INTRO_TRACK_PTR(CurrSongPtr)->timeSigNum)) %
                                INTRO_TRACK_PTR(CurrSongPtr)->timeSigNum;
            } else {
                return_value = ((MasterTick
                        / (CurrSongPtr->part[0].trackPtr->barLength
                                / CurrSongPtr->part[0].trackPtr->timeSigNum))
                        % CurrSongPtr->part[0].trackPtr->timeSigNum);
            }
            break;
#else
        if (CurrPartPtr != nullptr ) {
            switch (PlayerStatus) {

            case NO_SONG_LOADED:
            case STOPPED:
                return_value = -2;
                break;
#endif


            case INTRO:
                if (CurrPartPtr != nullptr ) {
                    // Calculate the offset for not complete intro ( 2 beat in 4/4) ( 6 beat in 4/4)
                    offset = MAIN_LOOP_PTR(CurrPartPtr)->barLength
                            - (MAIN_LOOP_PTR(CurrPartPtr)->nTick
                                    % MAIN_LOOP_PTR(CurrPartPtr)->barLength);
                    return_value = (((MasterTick + offset)
                            / (MAIN_LOOP_PTR(CurrPartPtr)->barLength
                                    / MAIN_LOOP_PTR(CurrPartPtr)->timeSigNum))
                            % MAIN_LOOP_PTR(CurrPartPtr)->timeSigNum);
                } else {
                    return_value = 0;
                }

                break;
            case PAUSED:
                if (CurrPartPtr != nullptr ) {
                    if (!ActivePauseEnable) {
                        return_value = -2;
                    } else {
                        return_value = ((MasterTick
                                / (MAIN_LOOP_PTR(CurrPartPtr)->barLength
                                        / MAIN_LOOP_PTR(CurrPartPtr)->timeSigNum))
                                % MAIN_LOOP_PTR(CurrPartPtr)->timeSigNum);
                    }
                }
                break;

            case PLAYING_MAIN_TRACK_TO_END:
            case PLAYING_MAIN_TRACK:
#ifdef am335x
            case COUNT_IN_TO_MAIN:
#endif
            case DRUMFILL_WAITING_TRIG:
            case TRANFILL_WAITING_TRIG:
            case NO_FILL_TRAN:
            case NO_FILL_TRAN_QUITTING:
            case NO_FILL_TRAN_CANCEL:
            case OUTRO:
            case OUTRO_CANCELED:
            case OUTRO_WAITING_TRIG:
            if (CurrPartPtr != nullptr ) {
                return_value = ((MasterTick
                        / (MAIN_LOOP_PTR(CurrPartPtr)->barLength
                                / MAIN_LOOP_PTR(CurrPartPtr)->timeSigNum))
                        % MAIN_LOOP_PTR(CurrPartPtr)->timeSigNum);
            }
                break;
            case TRANFILL_ACTIVE:
            case TRANFILL_QUITING:
            case TRANFILL_CANCEL:
                if (CurrPartPtr != nullptr ) {
                    return_value = (MasterTick
                            / (TRANS_FILL_PTR(CurrPartPtr)->barLength
                                    / TRANS_FILL_PTR(CurrPartPtr)->timeSigNum)
                                    % TRANS_FILL_PTR(CurrPartPtr)->timeSigNum);
                }
                break;
            case DRUMFILL_ACTIVE:
                if (CurrPartPtr != nullptr ) {
                return_value =
                        (MasterTick
                                / (DRUM_FILL_PTR(CurrPartPtr, DrumFillIndex)->barLength
                                        / DRUM_FILL_PTR(CurrPartPtr, DrumFillIndex)->timeSigNum)
                                        % DRUM_FILL_PTR(CurrPartPtr, DrumFillIndex)->timeSigNum);
                }
                break;
            default:
                return_value = -2;
            break;
        }
#ifndef am335x
    } else if (PlayerStatus == SINGLE_TRACK_PLAYER && SingleMidiTrackPtr != nullptr ){
        return_value =
                (MasterTick
                        / (SingleMidiTrackPtr->barLength
                                / SingleMidiTrackPtr->timeSigNum)
                                % SingleMidiTrackPtr->timeSigNum);
#endif
    } else {
        return_value = -2;
    }

    IntEnable(status);
    return return_value;
}

int SongPlayer_getMasterTick(void) {
    if (CurrSongPtr && CurrPartPtr && PlayerStatus == INTRO) {
        // Calculate the offset for not complete intro ( 2 beat in 4/4) ( 6 beat in 4/4)
        return MasterTick + MAIN_LOOP_PTR(CurrPartPtr)->barLength
                            - (MAIN_LOOP_PTR(CurrPartPtr)->nTick
                                    % MAIN_LOOP_PTR(CurrPartPtr)->barLength);
    }
    return MasterTick;
}
#ifdef am335x
void SongPlayer_deInit(void) {
    // Stop the player par placing an empty song in it
    SongPlayer_setSong(NULL, NULL);
}

void SongPlayer_reInit(void) {
    // No action to reinit. ( need to load a new song from the gui
}
#endif

/**
 *
 */
int SongPlayer_getTimeSignature(TimeSignature * timeSig) {

    unsigned char status = IntDisable();
    if (CurrSongPtr != nullptr) {
        if (CurrPartPtr != nullptr) {
            if (MAIN_LOOP_PTR(CurrPartPtr)) {
                timeSig->num = MAIN_LOOP_PTR(CurrPartPtr)->timeSigNum;
                timeSig->den = MAIN_LOOP_PTR(CurrPartPtr)->timeSigDen;
                IntEnable(status);
                return 1;
            }
        } else if (INTRO_TRACK_PTR(CurrSongPtr)) {
            timeSig->num = INTRO_TRACK_PTR(CurrSongPtr)->timeSigNum;
            timeSig->den = INTRO_TRACK_PTR(CurrSongPtr)->timeSigDen;
            IntEnable(status);
            return 1;
        } else {
            timeSig->num = MAIN_LOOP_PTR( ((SONG_SongPartStruct *)&(CurrSongPtr->part[0])) )->timeSigNum;
            timeSig->den = MAIN_LOOP_PTR( ((SONG_SongPartStruct *)&(CurrSongPtr->part[0])) )->timeSigDen;
            IntEnable(status);
            return 1;
        }
    }
#ifndef am335x
    else if (PlayerStatus == SINGLE_TRACK_PLAYER && SingleMidiTrackPtr != NULL ){
        timeSig->num = SingleMidiTrackPtr->timeSigNum;
        timeSig->den = SingleMidiTrackPtr->timeSigDen;
        IntEnable(status);
        return 1;
    }
#endif
    IntEnable(status);
    return 0;
}

int SongPlayer_getTempo() {
    unsigned char status = IntDisable();
    if (CurrSongPtr != nullptr) {
        if (CurrPartPtr != nullptr) {
            if (MAIN_LOOP_PTR(CurrPartPtr)) {
                IntEnable(status);
                if (MAIN_LOOP_PTR(CurrPartPtr)->bpm>0) {
                    return MAIN_LOOP_PTR(CurrPartPtr)->bpm;
                }
                return CurrSongPtr->bpm;
            }
        }
    }
    IntEnable(status);
    return 0;
}

void SongPlayer_forceStop(void) {
    StopSong();

}

#ifndef am335x
char* SongPlayer_getSoundEffectName(uint32_t part) {

    if (CurrSongPtr == nullptr) return nullptr;
    if (part >= CurrSongPtr->nPart) return nullptr;

    return (char*)CurrSongPtr->part[part].effectName;
}
#endif

void SongPlayer_externalPause(void) {
    PauseUnpauseHandler();
}

void SongPlayer_externalStop(void) {
    StopSong();
}

void SongPlayer_externalStart(void) {

    uint8_t status = IntDisable();
    if (PlayerStatus == STOPPED) {
        if (CurrSongPtr != nullptr) {
            RequestFlag = EXTERNAL_START_REQUEST;
            TEMPO_startWithInt();
        }
    }
    IntEnable(status);
}

void SongPlayer_externalDrumfill(void) {
    uint8_t status = IntDisable();
    // Trigger a drumfil only if the Main track is playing and there is no pending request
    if (PlayerStatus == PLAYING_MAIN_TRACK && RequestFlag == REQUEST_DONE) {
        RequestFlag = DRUMFILL_REQUEST;
    }
    IntEnable(status);
}

void SongPlayer_externalOutro(void) {
    uint8_t status = IntDisable();

    if (RequestFlag == REQUEST_DONE) {
        if (PlayerStatus == INTRO || (
                WasPausedFlag && PlayerStatus == PLAYING_MAIN_TRACK)) {
            RequestFlag = SWAP_TO_OUTRO_REQUEST;
        } else if ((PlayerStatus == PLAYING_MAIN_TRACK) ||
                (PlayerStatus == NO_FILL_TRAN) ||
                (PlayerStatus == NO_FILL_TRAN_QUITTING) ||
                (PlayerStatus == TRANFILL_WAITING_TRIG) ||
                (PlayerStatus == TRANFILL_ACTIVE) ||
                (PlayerStatus == TRANFILL_QUITING) ||
                (PlayerStatus == DRUMFILL_WAITING_TRIG) ||
                (PlayerStatus == DRUMFILL_ACTIVE)) {
            RequestFlag = STOP_REQUEST;
        }
    }




    IntEnable(status);
}

/**
 * @param part_index
 *     0 quit transition
 *     1 part 1           --> part 1 (next part)
 *     2 part 2           --> part 2 (next next part)
 *     3 part 3           --> part 3 (next next next part)
 *     > number of parts  --> next part
 *
 */
void SongPlayer_externalTransition(uint32_t part_number) {
    uint8_t status = IntDisable();

    if (RequestFlag == REQUEST_DONE) {
        if (part_number == 0 &&
                ((PlayerStatus == NO_FILL_TRAN) || (PlayerStatus == TRANFILL_WAITING_TRIG) || (PlayerStatus == TRANFILL_ACTIVE))) {
            RequestFlag = TRANFILL_QUIT_REQUEST;
        } else if (part_number > 0) {
            if ((PlayerStatus == PAUSED) ||
                    (PlayerStatus == DRUMFILL_WAITING_TRIG) ||
                    (PlayerStatus == DRUMFILL_ACTIVE) ||
                    (PlayerStatus == PLAYING_MAIN_TRACK)) {
                NextPartNumber = part_number;
                RequestFlag = TRANFILL_REQUEST;
                // Start song on external transition request if the song if stopped
            } else if (PlayerStatus == STOPPED) {
                NextPartNumber = part_number;
                SongPlayer_externalStart();
            }
        }
    }
    IntEnable(status);
}

#ifndef am335x
static void ResetSongPosition(void) {
    unsigned char status = IntDisable();
    DrumFillIndex = 0;
    PartIndex = 0;
    MasterTick = 0;
    CurrPartPtr = nullptr;
    if (CurrSongPtr != nullptr) {
        PlayerStatus = STOPPED;
    } else {
        PlayerStatus = NO_SONG_LOADED;
    }
    IntEnable(status);
}
#endif

void SongPlayer_getPlayerStatus(SongPlayer_PlayerStatus *playerStatus,
        unsigned int *partIndex, unsigned int *drumfillIndex) {
    *playerStatus = PlayerStatus;
    *partIndex = PartIndex;
    *drumfillIndex = DrumFillIndex;
}
#ifdef am335x
int32_t SongPlayer_getNextNoteValue(uint8_t * nextNote, uint8_t *presentNotes) {
    uint8_t note;
    int32_t count = 0;
    int32_t idx;

    uint8_t status = IntDisable();


    // If no song is loaded
    if (PlayerStatus == NO_SONG_LOADED || CurrSongPtr == (void*)0) {
        IntEnable(status);
        return 0;
    }

    // Copy all the note from the Count-In buffer
    for(idx = 0; idx < NUM_COUNT_IN_NOTE; idx++){
        note = CountInNoteList[idx];
        presentNotes[note] = 1;
        nextNote[idx] = note;
    }
    count = NUM_COUNT_IN_NOTE;


    if (INTRO_TRACK_PTR(CurrSongPtr)){
        getNotesTrack(INTRO_TRACK_PTR(CurrSongPtr),presentNotes,nextNote,&count);
    }
    getNotesTrack(CurrSongPtr->part[PartIndex].trackPtr,presentNotes,nextNote,&count);
    IntEnable(status);
    return count;
}

int32_t SongPlayer_isCurrentPartAutoPilot(void) {
    uint8_t status = IntDisable();
    int32_t retValue;

    retValue = AutopilotFeature != AUTOPILOT_FEATURE_DISABLE &&
            CurrSongPtr != NULL && APPtr != NULL;


    IntEnable(status);

    return retValue;
}

static getNotesTrack(MIDIPARSER_MidiTrack *track, uint8_t *present_note, uint8_t *note_list, int32_t *count) {
    uint8_t tmpVal;
    int32_t i;

    for (i = 0; i < track->nEvent; i++){
        tmpVal = track->event[i].note;
        if (!(present_note[tmpVal]))
        {
            present_note[tmpVal] = 1;
            note_list[*count] = tmpVal;
            (*count) += 1;
        }
    }
}

#else


static void adjust_length(int ix){
    if (ix == -1) return;
    auto& t = Tracks[ix];
    if ((t.nTick  / (4 * 480 / t.timeSigDen)) % t.timeSigNum){
        int nBeatMissing = t.timeSigNum - ((t.nTick  / (4 * 480 / t.timeSigDen)) % t.timeSigNum);
        t.nTick += nBeatMissing * (4 * 480 / t.timeSigDen);
    }
}

// Preparse the pick up note legth to be a multiple of the player tick to simplify shits.
static void adjust_trig_length(int ix) {
    if (ix == -1) return;
    auto& t = Tracks[ix];
    t.trigPos = (int)(0.70f * (float)t.barLength);
}



/**
 * @brief SongPlayer_loadSong
 * @param file
 * @param length
 * @return
 */
int SongPlayer_loadSong(char* file, uint32_t length)
{
    unsigned int i;
    unsigned int j;
    SONG_SongStruct *SongPtr;

    (void)length;

    CurrSongFilePtr = (SONGFILE_FileStruct*) file;

    // Verify the file type
    if (strncmp(CurrSongFilePtr->header.fileType,"BBSF",4)) {
        return -1;
    }

    // Verify the version, revision & build number

    // if invalid flag is set (e.g. No main part, etc...)
    if (CurrSongFilePtr->header.flags & SONGFILE_INVALID_FILE_FLAG_MASK) return -1;


    SongPtr = &CurrSongFilePtr->song;

    if (CurrSongPtr != SongPtr)
    { // cache all tracks
        auto sz = 0; // find track count
        if (auto s = SongPtr->outro.mainLoopIndex+1) if (sz < s) sz = s;
        if (auto s = SongPtr->intro.mainLoopIndex+1) if (sz < s) sz = s;
        for (int i = SongPtr->nPart-1; i >= 0; --i) {
            auto p = SongPtr->part[i];
            if (auto s = p.transFillIndex+1) if (sz < s) sz = s;
            for (int j = p.nDrumFill-1; j >= 0; --j)
                if (auto s = p.drumFillIndex[j]+1) if (sz < s) sz = s;
            if (auto s = p.mainLoopIndex+1) if (sz < s) sz = s;
        }
        Tracks.resize(sz--);
        for (auto p = file + CurrSongFilePtr->offsets.tracksDataOffset; sz >= 0; --sz)
            Tracks[sz].read(p + CurrSongFilePtr->trackIndexes[sz].dataOffset);
    }

    /* Intro */
    adjust_trig_length(SongPtr->intro.mainLoopIndex);


    for (i = 0; i < SongPtr->nPart; i++) {
        /* Main Loop */
        if (SongPtr->part[i].mainLoopIndex < 0)
            return 0;
        auto ix = SongPtr->part[i].mainLoopIndex;
        adjust_trig_length(ix);
        adjust_length(ix);

        /* Drumfills */
        for (j = 0; j < SongPtr->part[i].nDrumFill; j++) {
            if (SongPtr->part[i].drumFillIndex[j] < 0)
                return 0;
            adjust_trig_length(SongPtr->part[i].drumFillIndex[j]);
        }
        /* Transition Fill */
        adjust_trig_length(SongPtr->part[i].transFillIndex);
    }

    /* Outro */
    adjust_trig_length(SongPtr->outro.mainLoopIndex);


    /* Retreive the autopilot strucutre */
    if (CurrSongFilePtr->offsets.autoPilotDataOffset != 0) {
        APPtr = (AUTOPILOT_AutoPilotDataStruct *)(file + CurrSongFilePtr->offsets.autoPilotDataOffset);

        /* Validate Autopilot Flags */
        if (!APPtr->internalData.autoPilotFlags & AUTOPILOT_ON_FLAG ||
                !APPtr->internalData.autoPilotFlags & AUTOPILOT_VALID_FLAG) {
            APPtr = nullptr;
        }
    }

    CurrSongPtr = SongPtr;

    ResetSongPosition();
    return 1;
}

#endif

#ifdef am335x

void SongPlayer_setSong(SONG_SongStruct *songPtr, AUTOPILOT_AutoPilotDataStruct *autoPilotPtr) {
    uint8_t status;
    status = IntDisable();

    StopSong();

    CurrSongPtr = songPtr;


    // AUTOPILOT VALIDATION
    if (autoPilotPtr != NULL &&
            autoPilotPtr->internalData.autoPilotFlags & AUTOPILOT_ON_FLAG &&
            autoPilotPtr->internalData.autoPilotFlags & AUTOPILOT_VALID_FLAG) {
    	APPtr = autoPilotPtr;
    } else {
        APPtr = NULL;
    }

    if (CurrSongPtr == NULL ) {

        PlayerStatus = NO_SONG_LOADED;
        IntEnable(status);
        return;
    } else {
        PlayerStatus = STOPPED;
    }
    IntEnable(status);

    SpecialEffectManager();
    GUI_ForceRefresh();
}

#endif


#ifndef am335x

void SongPlayer_SetSingleTrack(MIDIPARSER_MidiTrack *track) {

    SingleMidiTrackPtr = track;
    MasterTick = 0;
    PlayerStatus = SINGLE_TRACK_PLAYER;
}

void SongPlayer_ProcessSingleTrack(float ratio, int32_t nTick, int32_t offset) {

    TmpMasterPartTick = MasterTick + nTick;
    if (PlayerStatus == NO_SONG_LOADED) {
        return;
    }

    if (PlayerStatus == SINGLE_TRACK_PLAYER) {

        TrackPlay(SingleMidiTrackPtr,MasterTick - offset ,TmpMasterPartTick - offset ,ratio,0,MAIN_PART_ID);

        // If its the end of the track
        if (TmpMasterPartTick >= SingleMidiTrackPtr->nTick + offset) {
            TrackPlay(SingleMidiTrackPtr,SingleMidiTrackPtr->nTick,SingleMidiTrackPtr->nTick+ 200,ratio,nTick,MAIN_PART_ID);
            PlayerStatus = STOPPED;
        }

    }

    MasterTick = TmpMasterPartTick;
}

int32_t SongPlayer_calculateSingleTrackOffset(uint32_t nTicks, uint32_t tickPerBar) {
    if (tickPerBar < 1) {
        return -1;
	}
    if ((nTicks % tickPerBar) <= 0) {
        return 0;
    } else {
        return tickPerBar - (nTicks % tickPerBar);
    }
}
#endif

/**
 * @brief SongPlayer_processSong
 * @param ratio
 * @param nTick
 */
void SongPlayer_processSong(float ratio, int32_t nTick) {

#ifdef am335x
    if (0 == (MasterTick % 480))
        GUI_SetTempoBeat();
#endif

    // When no songs are loaded, no need to go through the song processing
    if (PlayerStatus == NO_SONG_LOADED) {
        MasterTick += nTick;
        return;
    }

    // Clear previous flag
    AutopilotCueFill = FALSE;

#ifdef am335x
    if (AutopilotFeature != AUTOPILOT_FEATURE_DISABLE && APPtr != NULL && CurrPartPtr != NULL) {
#else
    if (APPtr != nullptr && CurrPartPtr != nullptr) {
#endif
    	// BEAT COUNTER
		if ((MasterTick % (CurrPartPtr->tickPerBar / CurrPartPtr->timeSigNum)) == 0) {
			BeatCounter++;
		}

		if (PlayerStatus == PLAYING_MAIN_TRACK) {
			int32_t tmpBeatCounter = APPtr->part[PartIndex].mainLoop.playFor > 0 ? BeatCounter % APPtr->part[PartIndex].mainLoop.playFor : BeatCounter;
			if (APPtr->part[PartIndex].drumFill[DrumFillIndex].playAt > 0 && APPtr->part[PartIndex].drumFill[DrumFillIndex].playAt == tmpBeatCounter) {
				RequestFlag = DRUMFILL_REQUEST;
				AutopilotCueFill = TRUE;
			} else if (APPtr->part[PartIndex].mainLoop.playAt > 0 && APPtr->part[PartIndex].mainLoop.playAt == BeatCounter) {
				if (PartIndex < CurrSongPtr->nPart - 1) {
					RequestFlag = TRANFILL_REQUEST;
					AutopilotCueFill = TRUE;
					AutopilotAction = TRUE;
					AutopilotTransitionCount = BeatCounter;
				} else {
					RequestFlag = STOP_REQUEST;
					AutopilotCueFill = TRUE;
				}
			}
		} else if ((AutopilotAction == TRUE) &&
				(PlayerStatus == NO_FILL_TRAN || PlayerStatus == TRANFILL_ACTIVE && BeatCounter >= (AutopilotTransitionCount + APPtr->part[PartIndex].transitionFill.playFor))) {
				RequestFlag = TRANFILL_QUIT_REQUEST;
				AutopilotCueFill = TRUE;
				AutopilotAction = FALSE;
		}
    }


    if (PlayerStatus == PAUSED) {

        switch (RequestFlag) {
        case START_REQUEST:
            if (MainUnpauseModeTapToFill == START_FILL) {
                if (PausedPlayerStatus == PLAYING_MAIN_TRACK
                        || PausedPlayerStatus == DRUMFILL_ACTIVE
                        || PausedPlayerStatus == DRUMFILL_WAITING_TRIG
                        || PausedPlayerStatus == TRANFILL_WAITING_TRIG
                        || PausedPlayerStatus == TRANFILL_ACTIVE
                        || PausedPlayerStatus == TRANFILL_CANCEL
                        || PausedPlayerStatus == TRANFILL_QUITING
                        || PausedPlayerStatus == NO_FILL_TRAN
                        || PausedPlayerStatus == NO_FILL_TRAN_CANCEL
                        || PausedPlayerStatus == NO_FILL_TRAN_QUITTING) {
                    // If there is are drumfills in the current part
                    if (CurrPartPtr->nDrumFill != 0) {

                        // If the Active Pause mode is enable
                        if (ActivePauseEnable) {
                            // the track will continue where it is supposed to be
                            MasterTick %= MAIN_LOOP_PTR(CurrPartPtr)->nTick;
                        } else {
                            // The track will restart a the begining
                            MasterTick = 0;
                        }
                        RequestFlag = DRUMFILL_REQUEST;
                    } else {
                        SamePart(FALSE);
                    }
                } else {
                    SamePart(FALSE);
                }
#ifdef am335x
                // Start command should also be sent when unpaused, if Stop command was sent when paused.
                if (SendStopOnPause && (MidiOutStartMsg != MIDI_OUT_START_MESSAGE_DISABLED)) {
                    UartMidi_SendStart();
                }
#endif
            } else {
                IntroPart();
#ifdef am335x
                if (MidiOutStartMsg != MIDI_OUT_START_MESSAGE_DISABLED) {
                    // Always resend start on intro
                    if (PlayerStatus == INTRO &&  MidiOutStartMsg == MIDI_OUT_START_MESSAGE_INTRO){
                        UartMidi_SendStart();
                        DelayStartCmd = 0;
                    } else if (PlayerStatus == PLAYING_MAIN_TRACK && MidiOutStartMsg != MIDI_OUT_START_MESSAGE_DISABLED){
                        UartMidi_SendStart();
                        DelayStartCmd = 0;
                    } else {
                        DelayStartCmd = 1;
                    }
                }
#endif
            }
            break;

        case PAUSE_REQUEST:
            PlayerStatus = UnPausedPlayerStatus;

            // If the Active Pause mode is enable
            if (ActivePauseEnable) {
                // the track will continue where it is supposed to be
                MasterTick %= MAIN_LOOP_PTR(CurrPartPtr)->nTick;
            } else {
                // The track will restart a the begining
                MasterTick = 0;
            }

            SpecialEffectManager(); // Change the special effect on UNPAUSED
            // Send continue message when UNPAUSED
            WasPausedFlag = 1;
#ifdef am335x
            if (!ActivePauseEnable && MidiOutStartMsg != MIDI_OUT_START_MESSAGE_DISABLED){
                UartMidi_SendStart();
            }
#endif

            RequestFlag = REQUEST_DONE;
            break;

        default:
            break;
        }
    }

#ifdef am335x
    // Sobriery Special Handler
    if (SobrietyFeature != SOBRIETY_SOBER && RequestFlag == REQUEST_DONE) {
        if (PlayerStatus == PLAYING_MAIN_TRACK) {

            int barNumber = 0;
            if (CurrPartPtr->trackPtr->nTick != CurrPartPtr->trackPtr->barLength) {
                barNumber = MasterTick / CurrPartPtr->tickPerBar;
            } else if (MasterTick == 0) {
                if (Test == 1) {
                    barNumber = 1;
                    Test = 0;
                } else {
                    Test = 1;
                }
            }


            if (barNumber == 1) {
                if (SobrietyFeature == SOBRIETY_DRUNK || (SobrietyFeature == SOBRIETY_WASTED && (SobrietyDrumTranFill == 0 && CurrPartPtr->nDrumFill > 0))) {
                    RequestFlag = DRUMFILL_REQUEST;
                    SobrietyDrumTranFill = 1;
                } else if (SobrietyFeature == SOBRIETY_WASTED && (SobrietyDrumTranFill == 1 || CurrPartPtr->nDrumFill == 0)) {
                    SobrietyDrumTranFill = 2;
                    RequestFlag = TRANFILL_REQUEST;
                    NextPartNumber = 0; // Means no specefic next part
                }
            }
        } else if ((PlayerStatus == TRANFILL_ACTIVE || PlayerStatus == NO_FILL_TRAN) && SobrietyDrumTranFill == 2) {
            RequestFlag = TRANFILL_QUIT_REQUEST;
            SobrietyDrumTranFill = 0;
        }
    }

    // Generate a random special effect in wasted mode when playing
    if (SobrietyFeature == SOBRIETY_WASTED) {
        if (SobriertySpecialEffectTickDelay >= nTick) {
            SobriertySpecialEffectTickDelay -= nTick;
        } else if ((PlayerStatus != PAUSED) && (PlayerStatus != STOPPED)) {
            SoundManager_playSpecialEffect(100, FALSE);
            SobriertySpecialEffectTickDelay = nTick * (48 + rand() % 96);
        }
    }


    // Start request handler
    if (((RequestFlag == START_REQUEST || RequestFlag == EXTERNAL_START_REQUEST)) &&
            PlayerStatus == STOPPED) {
        if (IntroFillParam == INTRO_COUNT_IN) {
            CountIn();
        } else {
            IntroPart();
            if (MidiOutStartMsg == MIDI_OUT_START_MESSAGE_INTRO){
                UartMidi_SendStart();
                DelayStartCmd = 0;
            } else if (MidiOutStartMsg == MIDI_OUT_START_MESSAGE_FIRST_PART){
                if (PlayerStatus == PLAYING_MAIN_TRACK){
                    UartMidi_SendStart();
                    DelayStartCmd = 0;
                } else {
                    DelayStartCmd = 1;
                }
            }
        }
    }

#else
    if ((RequestFlag == START_REQUEST) && PlayerStatus == STOPPED) {
            IntroPart();
    }
#endif


    if (RequestFlag == PAUSE_REQUEST) {

        int32_t testMasterTick = MasterTick;
        int32_t testTmpTick = TmpMasterPartTick;
        PausedPlayerStatus = PlayerStatus;

        switch (PlayerStatus) {
        case PLAYING_MAIN_TRACK:
            SamePart(FALSE);
            UnPausedPlayerStatus = PLAYING_MAIN_TRACK;
            PlayerStatus = PAUSED;
            if (!ActivePauseEnable && SendStopOnPause) UartMidi_SendStop();
            break;

        case DRUMFILL_ACTIVE:
        case DRUMFILL_WAITING_TRIG:
            SamePart(TRUE);
            UnPausedPlayerStatus = PLAYING_MAIN_TRACK;
            PlayerStatus = PAUSED;
            if (!ActivePauseEnable && SendStopOnPause) UartMidi_SendStop();
            break;

        case TRANFILL_CANCEL:
        case NO_FILL_TRAN_CANCEL:
            SamePart(FALSE);
            UnPausedPlayerStatus = PLAYING_MAIN_TRACK;
            PlayerStatus = PAUSED;
            if (!ActivePauseEnable && SendStopOnPause) UartMidi_SendStop();
            break;

        case TRANFILL_WAITING_TRIG:
        case TRANFILL_ACTIVE:
        case TRANFILL_QUITING:
        case NO_FILL_TRAN:
        case NO_FILL_TRAN_QUITTING:

            NextPart();

            UnPausedPlayerStatus = PLAYING_MAIN_TRACK;
            PlayerStatus = PAUSED;
            if (!ActivePauseEnable && SendStopOnPause) UartMidi_SendStop();
            break;

        case PLAYING_MAIN_TRACK_TO_END:
            StopSong();
            SpecialEffectManager();
            break;

        case OUTRO_WAITING_TRIG:
            // Stop and restart in the outro
            OutroPart();
            UnPausedPlayerStatus = OUTRO;
            PlayerStatus = PAUSED;
            // We need to reset the flag since it wont be usefull on unpaused
            PartStopSyncTick = 0;
            if (!ActivePauseEnable && SendStopOnPause) UartMidi_SendStop();
            break;

        case INTRO:
            FirstPart();
            UnPausedPlayerStatus = PlayerStatus;
            PlayerStatus = PAUSED;
            if (!ActivePauseEnable && SendStopOnPause) UartMidi_SendStop();
            break;

        case OUTRO:
            DrumFillIndex = 0;
            SamePart(FALSE);
            UnPausedPlayerStatus = PlayerStatus;
            PlayerStatus = PAUSED;
            if (!ActivePauseEnable && SendStopOnPause) UartMidi_SendStop();
            break;

        case OUTRO_CANCELED:
            SamePart(FALSE);
            UnPausedPlayerStatus = PLAYING_MAIN_TRACK;
            PlayerStatus = PAUSED;
            if (!ActivePauseEnable && SendStopOnPause) UartMidi_SendStop();
            break;

        case STOPPED:
        case NO_SONG_LOADED:

            break;
        default:
            break;
        }

        if (ActivePauseEnable) {
            if (PlayerStatus == PAUSED) {
                MasterTick = testMasterTick;
                TmpMasterPartTick = testTmpTick;
            }
        }
        // Make sure the gui follows the
        GUI_ForceRefresh();

    } else {

        switch (RequestFlag) {

        case DRUMFILL_REQUEST:
            // If there is drum fills in the current part
            if (CurrPartPtr->nDrumFill > 0) {

                DrumFillStartSyncTick = CalculateStartBarSyncTick(MasterTick,
                        MAIN_LOOP_PTR(CurrPartPtr)->barLength,
                        AutopilotCueFill ? MAIN_LOOP_PTR(CurrPartPtr)->barLength : MAIN_LOOP_PTR(CurrPartPtr)->trigPos);


                // Adjust offset if drumfill is smaller than one bar
                if (DRUM_FILL_PTR(CurrPartPtr, DrumFillIndex)->nTick < MAIN_LOOP_PTR(CurrPartPtr)->barLength) {

                    DrumFillStartSyncTick += MAIN_LOOP_PTR(CurrPartPtr)->barLength -
                            (DRUM_FILL_PTR(CurrPartPtr, DrumFillIndex)->nTick % MAIN_LOOP_PTR(CurrPartPtr)->barLength);
                }

                if (DRUM_FILL_PTR(CurrPartPtr, DrumFillIndex)->pickupNotesLength){
                    if (DRUM_FILL_PTR(CurrPartPtr, DrumFillIndex)->pickupNotesLength % nTick){
                        DrumFillPickUpSyncTickLength = (( 1 + DRUM_FILL_PTR(CurrPartPtr, DrumFillIndex)->pickupNotesLength/ nTick) * nTick);
                    } else {
                        DrumFillPickUpSyncTickLength = (( 0 + DRUM_FILL_PTR(CurrPartPtr, DrumFillIndex)->pickupNotesLength/ nTick) * nTick);
                    }
                } else {
                    DrumFillPickUpSyncTickLength = 0;
                }
                DrumFillStartSyncTick -= DrumFillPickUpSyncTickLength;

                PlayerStatus = DRUMFILL_WAITING_TRIG;
            }
            break;

        case TRANFILL_REQUEST:


            // If there is drum fills in the current part
              if (TRANS_FILL_PTR(CurrPartPtr)) {
                if (TRANS_FILL_PTR(CurrPartPtr)->pickupNotesLength){
                    if (TRANS_FILL_PTR(CurrPartPtr)->pickupNotesLength % nTick){
                        TranFillPickUpSyncTickLength = (( 1 + TRANS_FILL_PTR(CurrPartPtr)->pickupNotesLength/ nTick) * nTick);
                    } else {
                        TranFillPickUpSyncTickLength = (( 0 + TRANS_FILL_PTR(CurrPartPtr)->pickupNotesLength/ nTick) * nTick);
                    }
                } else {
                    TranFillPickUpSyncTickLength = 0;
                }
                if (PlayerStatus == PAUSED){
                    if (!ActivePauseEnable){
                        // Use nTick instead of 0 to avoid negative number for master ticks
                        MasterTick = TRANS_FILL_PTR(CurrPartPtr)->nTick - TranFillPickUpSyncTickLength;
                        TranFillStartSyncTick = MasterTick;
#ifdef am335x
                        if (MidiOutStartMsg != MIDI_OUT_START_MESSAGE_DISABLED) {
                            UartMidi_SendStart();
                        }
#endif
                    } else {
                        TranFillStartSyncTick = CalculateStartBarSyncTick(MasterTick,
                                MAIN_LOOP_PTR(CurrPartPtr)->barLength,
                                AutopilotCueFill ? MAIN_LOOP_PTR(CurrPartPtr)->barLength : MAIN_LOOP_PTR(CurrPartPtr)->trigPos);
                        TranFillStartSyncTick -= TranFillPickUpSyncTickLength;

                    }
                } else {
                    TranFillStartSyncTick = CalculateStartBarSyncTick(MasterTick,
                            MAIN_LOOP_PTR(CurrPartPtr)->barLength,
                            AutopilotCueFill ? MAIN_LOOP_PTR(CurrPartPtr)->barLength : MAIN_LOOP_PTR(CurrPartPtr)->trigPos);
                    TranFillStartSyncTick -= TranFillPickUpSyncTickLength;

                }

                // Adjust offset of tran fil if it is smaller than one bar
                if (TRANS_FILL_PTR(CurrPartPtr)->nTick < TRANS_FILL_PTR(CurrPartPtr)->barLength) {

                    TranFillStartSyncTick += MAIN_LOOP_PTR(CurrPartPtr)->barLength -
                            (TRANS_FILL_PTR(CurrPartPtr)->nTick % TRANS_FILL_PTR(CurrPartPtr)->barLength);
                }

                PlayerStatus = TRANFILL_WAITING_TRIG;
            } else {

                // If there is only one part no transition fill since we stay on the same part all the time
                if (CurrSongPtr->nPart > 1){
                    // Cancel the transition fill request
                    PlayerStatus = NO_FILL_TRAN;
                }
            }
            break;

        case TRANFILL_QUIT_REQUEST:
            if ((PlayerStatus == TRANFILL_WAITING_TRIG) || (PlayerStatus == TRANFILL_ACTIVE)) {
                TranFillStopSyncTick = CalculateTranFillQuitSyncTick(MasterTick,
                        MAIN_LOOP_PTR(CurrPartPtr)->barLength);
                PlayerStatus = TRANFILL_QUITING;
            } else {
                PartStopSyncTick = CalculateTranFillQuitSyncTick(MasterTick,
                        MAIN_LOOP_PTR(CurrPartPtr)->barLength);
                PlayerStatus = NO_FILL_TRAN_QUITTING;
            }
            break;

        case TRANFILL_CANCEL_REQUEST:
            if (PlayerStatus == TRANFILL_QUITING){
                PlayerStatus = TRANFILL_CANCEL;
            } else if (PlayerStatus == NO_FILL_TRAN_QUITTING){
                PlayerStatus = NO_FILL_TRAN_CANCEL;
            }
            break;

        case STOP_REQUEST:
            if (OUTRO_TRACK_PTR(CurrSongPtr) != NULL ) {
                PartStopSyncTick = CalculateStartBarSyncTick(MasterTick,
                        MAIN_LOOP_PTR(CurrPartPtr)->barLength,
                        AutopilotCueFill ? MAIN_LOOP_PTR(CurrPartPtr)->barLength : MAIN_LOOP_PTR(CurrPartPtr)->trigPos);

                // Adjust offset of tran fil if it is smaller than one bar
                if (OUTRO_TRACK_PTR(CurrSongPtr)->nTick < MAIN_LOOP_PTR(CurrPartPtr)->barLength) {
                    PartStopSyncTick += MAIN_LOOP_PTR(CurrPartPtr)->barLength - (OUTRO_TRACK_PTR(CurrSongPtr)->nTick % MAIN_LOOP_PTR(CurrPartPtr)->barLength);
                }


                // Adjust delay for pick-up notes
                if (OUTRO_TRACK_PTR(CurrSongPtr)->pickupNotesLength){
                    if (OUTRO_TRACK_PTR(CurrSongPtr)->pickupNotesLength% nTick){
                        PartStopPickUpSyncTickLength = (( 1 + OUTRO_TRACK_PTR(CurrSongPtr)->pickupNotesLength/ nTick) * nTick);
                    } else {
                        PartStopPickUpSyncTickLength = (( 0 + OUTRO_TRACK_PTR(CurrSongPtr)->pickupNotesLength/ nTick) * nTick);
                    }
                } else {
                    PartStopPickUpSyncTickLength = 0;
                }
                PartStopSyncTick -= PartStopPickUpSyncTickLength;

                PlayerStatus = OUTRO_WAITING_TRIG;
            } else {
                PartStopSyncTick = CalculateTranFillQuitSyncTick(MasterTick,
                        MAIN_LOOP_PTR(CurrPartPtr)->barLength);
                PlayerStatus = PLAYING_MAIN_TRACK_TO_END;
            }
            break;

        case SWAP_TO_OUTRO_REQUEST:
            SwapToOutro();
            break;

        case OUTRO_CANCEL_REQUEST:

            if (PlayerStatus == PLAYING_MAIN_TRACK_TO_END) {
                PlayerStatus = PLAYING_MAIN_TRACK;
                break;
            }
            // Switch the player to OUTRO cancelled state if there is a cancel request during the outro
            if (PlayerStatus == OUTRO) {
                PlayerStatus = OUTRO_CANCELED;
                break;
            }
            // Switch back the player to the main track if the outro is cancelled before started
            if (PlayerStatus == OUTRO_WAITING_TRIG) {
                PlayerStatus = PLAYING_MAIN_TRACK;
                break;
            }
            break;

        default:
            break;
        }
    }
    RequestFlag = REQUEST_DONE;

    TmpMasterPartTick = MasterTick + nTick;

    // Trigger Manager
    switch (PlayerStatus) {

    case DRUMFILL_WAITING_TRIG:


        // If still waiting for trigger
        if (TmpMasterPartTick <= DrumFillStartSyncTick) {
            TrackPlay(MAIN_LOOP_PTR(CurrPartPtr), MasterTick, TmpMasterPartTick, ratio, 0, MAIN_PART_ID);

            // If its the end of the track
            if (TmpMasterPartTick >= MAIN_LOOP_PTR(CurrPartPtr)->nTick) {
                DrumFillStartSyncTick -= MAIN_LOOP_PTR(CurrPartPtr)->nTick;
                TmpMasterPartTick = 0u;
            }
        } else {
            // if trigger has happen, go in pending mode
            PlayerStatus = DRUMFILL_ACTIVE;
        }

        break;

    case TRANFILL_WAITING_TRIG:

        if (TmpMasterPartTick <= TranFillStartSyncTick) {

            // Play the standard track
            TrackPlay(MAIN_LOOP_PTR(CurrPartPtr), MasterTick, TmpMasterPartTick, ratio, 0, MAIN_PART_ID);

            // If its the end of the track
            if (TmpMasterPartTick >= MAIN_LOOP_PTR(CurrPartPtr)->nTick) {
                TranFillStartSyncTick -= MAIN_LOOP_PTR(CurrPartPtr)->nTick;
                TmpMasterPartTick = 0u;
            }
        } else {
            // Change the state to pending
            PlayerStatus = TRANFILL_ACTIVE;
        }
        break;

    case OUTRO_WAITING_TRIG:

        // If still waiting for trigger
        if (TmpMasterPartTick <= PartStopSyncTick) {
            TrackPlay(MAIN_LOOP_PTR(CurrPartPtr), MasterTick, TmpMasterPartTick, ratio, 0, MAIN_PART_ID);

            // If its the end of the track
            if (TmpMasterPartTick >= MAIN_LOOP_PTR(CurrPartPtr)->nTick) {
                PartStopSyncTick -= MAIN_LOOP_PTR(CurrPartPtr)->nTick;
                TmpMasterPartTick = 0u;
            }
        } else {
            // if trigger has happen, go in pending mode
            PlayerStatus = OUTRO;
            CurrPartPtr = &CurrSongPtr->outro;
        }

        break;

    default:
        break;
    }

    switch (PlayerStatus) {
#ifdef am335x
    case COUNT_IN_TO_MAIN:

        /* Switch fo main track after one bar */
        if (TmpMasterPartTick >= CurrPartPtr->trackPtr->barLength) {
            PlayerStatus = PLAYING_MAIN_TRACK;
            MasterTick = 0;
        } else {

            /* Play the count in sound at each time in the bar */
            if (0 ==
                    (MasterTick % (CurrPartPtr->trackPtr->barLength / CurrPartPtr->trackPtr->timeSigNum)))
            {
                SoundManager_playAvailableNote(CountInNoteList, NUM_COUNT_IN_NOTE,
                        COUNT_IN_VELOCITY, NO_DELAY, ratio, COUNT_IN_PART_ID);
            }
        }
        break;
#endif
    case INTRO:
        TrackPlay(MAIN_LOOP_PTR(CurrPartPtr), MasterTick, TmpMasterPartTick, ratio, 0, INTR_FILL_ID);


        if (TmpMasterPartTick >= (int)MAIN_LOOP_PTR(CurrPartPtr)->nTick) {
            // Play the extra note at the end
            TrackPlay(MAIN_LOOP_PTR(CurrPartPtr), MAIN_LOOP_PTR(CurrPartPtr)->nTick,
                    MAIN_LOOP_PTR(CurrPartPtr)->nTick + POST_EVENT_MAX_TICK, ratio,
                    nTick, INTR_FILL_ID);

            TmpMasterPartTick = 0u;
            DrumFillIndex = 0;

            if (NextPartNumber > 0 && NextPartNumber <= CurrSongPtr->nPart) {
                PartIndex = NextPartNumber - 1;
            } else {
                PartIndex = 0;
            }
            NextPartNumber = 0;
            CurrPartPtr = &CurrSongPtr->part[PartIndex];

            PlayerStatus = PLAYING_MAIN_TRACK;
            BeatCounter = 0;
            SpecialEffectManager();
        }

        break;

    case DRUMFILL_ACTIVE:
        TrackPlay(DRUM_FILL_PTR(CurrPartPtr, DrumFillIndex),
                MasterTick - DrumFillStartSyncTick - DrumFillPickUpSyncTickLength,
                TmpMasterPartTick - DrumFillStartSyncTick - DrumFillPickUpSyncTickLength, ratio, 0,
                DRUM_FILL_ID);

        // Drumfill end detector
        if (TmpMasterPartTick - DrumFillStartSyncTick - DrumFillPickUpSyncTickLength
                >= (int)(DRUM_FILL_PTR(CurrPartPtr, DrumFillIndex)->nTick)) {

            // Play the extra notes at the end
            TrackPlay(DRUM_FILL_PTR(CurrPartPtr, DrumFillIndex),
                    DRUM_FILL_PTR(CurrPartPtr, DrumFillIndex)->nTick,
                    DRUM_FILL_PTR(CurrPartPtr, DrumFillIndex)->nTick
                    + POST_EVENT_MAX_TICK, ratio, nTick, DRUM_FILL_ID);

            SamePart(TRUE);
            SpecialEffectManager();
        }
        break;

    case TRANFILL_ACTIVE:
        TrackPlay(TRANS_FILL_PTR(CurrPartPtr),
                MasterTick - TranFillStartSyncTick - TranFillPickUpSyncTickLength,
                TmpMasterPartTick - TranFillStartSyncTick - TranFillPickUpSyncTickLength,
                ratio,
                0,
                TRAN_FILL_ID);

        if (TmpMasterPartTick
                >= TRANS_FILL_PTR(CurrPartPtr)->nTick + TranFillStartSyncTick) {
            TranFillStartSyncTick += TRANS_FILL_PTR(CurrPartPtr)->nTick;
        }

        break;

    case TRANFILL_CANCEL:
    case TRANFILL_QUITING:

        // It is possible to be in a tranfill quiting state before the tranfill active (eg 2 tranf fill in a 4/4)
        if (TmpMasterPartTick <= TranFillStartSyncTick) {
           TrackPlay(MAIN_LOOP_PTR(CurrPartPtr),
                    MasterTick,
                    TmpMasterPartTick,
                    ratio,
                    0,
                    MAIN_PART_ID);
        } else {
            TrackPlay(TRANS_FILL_PTR(CurrPartPtr),
                    MasterTick - TranFillStartSyncTick - TranFillPickUpSyncTickLength,
                    TmpMasterPartTick - TranFillStartSyncTick - TranFillPickUpSyncTickLength,
                    ratio,
                    0,
                    TRAN_FILL_ID);
        }
        if (TmpMasterPartTick >= TRANS_FILL_PTR(CurrPartPtr)->nTick + TranFillStartSyncTick + TranFillPickUpSyncTickLength) {
            TranFillStartSyncTick += TRANS_FILL_PTR(CurrPartPtr)->nTick;
        }


        // Tranfill end detector
        if (TmpMasterPartTick >= TranFillStopSyncTick) {
            // Play the extra notes at the end
            TrackPlay(TRANS_FILL_PTR(CurrPartPtr), TRANS_FILL_PTR(CurrPartPtr)->nTick,
                    TRANS_FILL_PTR(CurrPartPtr)->nTick + POST_EVENT_MAX_TICK,
                    ratio, nTick, TRAN_FILL_ID);

            if (PlayerStatus == TRANFILL_CANCEL){
                SamePart(0);
            } else {

                    NextPart();
            }
            SpecialEffectManager();
        }

        break;

    case NO_FILL_TRAN:
        TrackPlay(MAIN_LOOP_PTR(CurrPartPtr), MasterTick, TmpMasterPartTick, ratio,
                0, MAIN_PART_ID);
        // If its the end of the track
        if (TmpMasterPartTick >= MAIN_LOOP_PTR(CurrPartPtr)->nTick) {
            // Restart the main groove as usual
            TmpMasterPartTick = 0;
        }
        break;

    case PLAYING_MAIN_TRACK:

        if (DelayStartCmd){
            UartMidi_SendStart();
            DelayStartCmd = 0;
        }

        TrackPlay(MAIN_LOOP_PTR(CurrPartPtr), MasterTick, TmpMasterPartTick, ratio,
                0, MAIN_PART_ID);

        // If its the end of the track
        if (TmpMasterPartTick >= MAIN_LOOP_PTR(CurrPartPtr)->nTick /*&& (loopCount == 0 || AutopilotFeature == AUTOPILOT_FEATURE_DISABLE)*/) {
            SamePart(0);
            SpecialEffectManager();
        } else if (isEndOfTrack(TmpMasterPartTick, CurrPartPtr/*, loop, loopCount)*/)) {

                    SamePart(2); // do a drumfill, if it exists, and loop again
            SpecialEffectManager();
        }

        break;

    case NO_FILL_TRAN_CANCEL:
        TrackPlay(MAIN_LOOP_PTR(CurrPartPtr), MasterTick, TmpMasterPartTick, ratio,
                0, MAIN_PART_ID);
        if (TmpMasterPartTick >= PartStopSyncTick) {
            SamePart(FALSE);
            SpecialEffectManager();
        }


        break;
    case NO_FILL_TRAN_QUITTING:

        TrackPlay(MAIN_LOOP_PTR(CurrPartPtr), MasterTick, TmpMasterPartTick, ratio,
                0, MAIN_PART_ID);

        if (TmpMasterPartTick >= PartStopSyncTick) {
            NextPart();
            SpecialEffectManager();
        }

        break;

    case PLAYING_MAIN_TRACK_TO_END:

        TrackPlay(MAIN_LOOP_PTR(CurrPartPtr), MasterTick, TmpMasterPartTick, ratio, 0, MAIN_PART_ID);

        if (TmpMasterPartTick >= PartStopSyncTick) {

            StopSong();
            SpecialEffectManager();
        }

        break;

    case OUTRO:
        TrackPlay(MAIN_LOOP_PTR(CurrPartPtr),
                MasterTick - PartStopSyncTick - PartStopPickUpSyncTickLength,
                TmpMasterPartTick - PartStopSyncTick - PartStopPickUpSyncTickLength,
                ratio,
                0,
                OUTR_FILL_ID);

        // If its the end of the track
        if (TmpMasterPartTick >= MAIN_LOOP_PTR(CurrPartPtr)->nTick + PartStopSyncTick + PartStopPickUpSyncTickLength) {

            // Play the extra note at the end
            TrackPlay(MAIN_LOOP_PTR(CurrPartPtr), MAIN_LOOP_PTR(CurrPartPtr)->nTick,
                    MAIN_LOOP_PTR(CurrPartPtr)->nTick + POST_EVENT_MAX_TICK, ratio,
                    nTick, OUTR_FILL_ID);

            // Stop the song after the last sounds have been launched
            StopSong();
        }

        break;

    case OUTRO_CANCELED:

        TrackPlay(MAIN_LOOP_PTR(CurrPartPtr),
                MasterTick - PartStopSyncTick- PartStopPickUpSyncTickLength,
                TmpMasterPartTick - PartStopSyncTick - PartStopPickUpSyncTickLength,
                ratio,
                0,
                OUTR_FILL_ID);

        // If it's the end of the track
        if (TmpMasterPartTick >= MAIN_LOOP_PTR(CurrPartPtr)->nTick + PartStopSyncTick + PartStopPickUpSyncTickLength) {
            // Play the extra note at the end
            TrackPlay(MAIN_LOOP_PTR(CurrPartPtr), MAIN_LOOP_PTR(CurrPartPtr)->nTick,
                    MAIN_LOOP_PTR(CurrPartPtr)->nTick + POST_EVENT_MAX_TICK,
                    ratio,
                    nTick,
                    OUTR_FILL_ID);

            // Return to the smae part before the outro
            SamePart(FALSE);
            SpecialEffectManager();
        }
        break;

    case PAUSED:
        break;
    case STOPPED:
        break; //  it was return before

    default:
        break;
    }

#ifdef am335x
    if (MidiOutSyncSettings == MIDI_OUT_SYNC_MESSAGE_ALWAYS_ENABLE ||
            (MidiOutSyncSettings == MIDI_OUT_SYNC_MESSAGE_PLAYING_ONLY &&
                    (PlayerStatus != STOPPED && PlayerStatus != NO_SONG_LOADED))) {
        UartMidi_sendTick();
    }
#endif

    // Advance the master tick counter
    MasterTick = TmpMasterPartTick;
    LastPlayerStatus = PlayerStatus;
    GUI_ForceRefresh();
}

void SongPlayer_IngoreNextRelease(void){
    PedalPressFlag = 0;
}


uint32_t SongPlayer_isPlaying(void) {
    return PlayerStatus != NO_SONG_LOADED && PlayerStatus != STOPPED;
}

static void PauseUnpauseHandler(void) {
    uint8_t status;

    status = IntDisable();
    if ((PlayerStatus != STOPPED) && (PlayerStatus != NO_SONG_LOADED)) {
        RequestFlag = PAUSE_REQUEST;
    }
    IntEnable(status);
}


static void NextPart(void) {
    TmpMasterPartTick = 0;
    MasterTick = 0;
    BeatCounter = 0;
    WasPausedFlag = 0;
    SobrietyDrumTranFill = 0;
    Test = 0;
    
    if (CurrSongPtr->nPart > 0) {
        if (NextPartNumber > 0 && NextPartNumber <= CurrSongPtr->nPart) {
            PartIndex = NextPartNumber - 1;
        } else {
            PartIndex = (PartIndex + 1) % CurrSongPtr->nPart;
        }

        NextPartNumber = 0;
        CurrPartPtr = &CurrSongPtr->part[PartIndex];
        PlayerStatus = PLAYING_MAIN_TRACK;
        BeatCounter = 0;

    } else {
        StopSong();
    }

    // Suffle drumsets if enabled in song
#ifdef am335x
    if (CurrPartPtr->shuffleFlag || SobrietyFeature == SOBRIETY_WASTED) {
#else
    if (CurrPartPtr->shuffleFlag) {
#endif
        if (CurrPartPtr->nDrumFill != 0) {
            DrumFillIndex = rand() % CurrPartPtr->nDrumFill;
        }
    } else {
        DrumFillIndex = 0;
    }

    // Send next part message on Uart Port
    UartMidi_sendNextPartMessage(PartIndex + 1);
}

static void EndPart(void) {
    TmpMasterPartTick = 0;
    MasterTick = 0;
    WasPausedFlag = 0;

    uint8_t status = IntDisable();

    if (OUTRO_TRACK_PTR(CurrSongPtr)) {
        CurrPartPtr = &CurrSongPtr->outro;
        MasterTick = 0;
        PlayerStatus = OUTRO;
    } else {
        StopSong();
    }

    IntEnable(status);
}

static void OutroPart(void) {
    TmpMasterPartTick = 0;
    MasterTick = 0;
    BeatCounter = 0;
}

static void SwapToOutro(void){
    DrumFillIndex = 0;
    PartStopSyncTick = 0;

    if (OUTRO_TRACK_PTR(CurrSongPtr)) {
        CurrPartPtr = &CurrSongPtr->outro;
        MasterTick %= MAIN_LOOP_PTR(CurrPartPtr)->barLength;
        PlayerStatus = OUTRO;
    } else {
        StopSong();
    }
    SpecialEffectManager();
}

/**
 * @brief FirstPart
 */
static void FirstPart(void) {
    TmpMasterPartTick = 0;
    MasterTick = 0;
    WasPausedFlag = 0;
    BeatCounter = 0;

    if (CurrSongPtr->nPart > 0) {

        if (NextPartNumber > 0 && NextPartNumber <= CurrSongPtr->nPart) {
            PartIndex = NextPartNumber - 1;
        } else {
            PartIndex = 0;
        }

        NextPartNumber = 0;
        CurrPartPtr = &CurrSongPtr->part[PartIndex];
    } else {
        StopSong();
        return;
    }

    if (CurrPartPtr != NULL ) {
        if (CurrPartPtr->nDrumFill != 0) {
            if (CurrPartPtr->shuffleFlag) {
                DrumFillIndex = rand() % CurrPartPtr->nDrumFill;
            } else {
                DrumFillIndex = 0; // First Drumfil always
            }
        }
    }

    PlayerStatus = PLAYING_MAIN_TRACK;
    BeatCounter = 0;

}
#ifdef am335x
static void CountIn(void){

    PlayerStatus = COUNT_IN_TO_MAIN;

    if (NextPartNumber > 0 && NextPartNumber <= CurrSongPtr->nPart) {
        PartIndex = NextPartNumber - 1;
    } else {
        PartIndex = 0;
    }

    NextPartNumber = 0;
    CurrPartPtr = &CurrSongPtr->part[PartIndex];

    MasterTick = 0;

    /// Calculate the Drumfill Index
    if (CurrPartPtr->shuffleFlag) {
        if (CurrPartPtr->nDrumFill != 0) {
            DrumFillIndex = rand() % CurrPartPtr->nDrumFill;
        }
    } else {
        DrumFillIndex = 0;
    }
    SpecialEffectManager();

    // A start command will be triggered when the MAIN_PART starts
    DelayStartCmd = true;
}

#endif

/**
 * @brief IntroPart
 */
static void IntroPart(void) {
    DrumFillIndex = 0;
    MasterTick = 0;
    WasPausedFlag = 0;
    PartIndex = 0;
#ifdef am335x
    if ((INTRO_TRACK_PTR(CurrSongPtr) != NULL) && (IntroFillParam == INTRO_FILL_ENABLE)) {
#else
    if (INTRO_TRACK_PTR(CurrSongPtr) != NULL) {
#endif
        PlayerStatus = INTRO;
        CurrPartPtr = &CurrSongPtr->intro;

        // If the intro have pickup notes
        if (MAIN_LOOP_PTR(CurrPartPtr)->pickupNotesLength){

            // If the length is not a multiple of nTick
            if (MAIN_LOOP_PTR(CurrPartPtr)->pickupNotesLength % 20){
                MasterTick =  0 - (( 1 + MAIN_LOOP_PTR(CurrPartPtr)->pickupNotesLength / 20) * 20);
            } else {
                MasterTick =  0 - (( 0 + MAIN_LOOP_PTR(CurrPartPtr)->pickupNotesLength/ 20) * 20);
            }
        } else {
            MasterTick = 0;
        }

    } else {
        if (NextPartNumber > 0 && NextPartNumber <= CurrSongPtr->nPart) {
            PartIndex = NextPartNumber - 1;
        } else {
            PartIndex = 0;
        }

        NextPartNumber = 0;
        CurrPartPtr = &CurrSongPtr->part[PartIndex];


        if (CurrPartPtr->shuffleFlag) {
            if (CurrPartPtr->nDrumFill != 0) {
                DrumFillIndex = rand() % CurrPartPtr->nDrumFill;
            }
        } else {
            DrumFillIndex = 0;
        }
        SpecialEffectManager();

        PlayerStatus = PLAYING_MAIN_TRACK;
    }
    MAIN_LOOP_PTR(CurrPartPtr)->index = 0;
}

/**
 * @brief SamePart
 * @param nextDrumfill
 */
static void SamePart(unsigned int nextDrumfill) {
    TmpMasterPartTick = 0;
    MasterTick = 0;
    WasPausedFlag = 0;

    if (nextDrumfill) {
        if (CurrPartPtr != NULL ) {
            if (CurrPartPtr->nDrumFill != 0) {
                if (CurrPartPtr->shuffleFlag) {
                    DrumFillIndex = rand() % CurrPartPtr->nDrumFill;
                } else {
                    DrumFillIndex = (DrumFillIndex + 1) % CurrPartPtr->nDrumFill;
                }
            }
        }
    }

    CurrPartPtr = &CurrSongPtr->part[PartIndex];

    PlayerStatus = PLAYING_MAIN_TRACK;
}

static bool isEndOfTrack(int pos, SONG_SongPartStruct *CurrPartPtr/*, int loop, int loopCount*/) {
    // the issue here is to decide when to make decision.  Last measure, or end of loop
    // 1) if no fills or transitions, then end of loop
    // 2) no fills, but a trans, then end of loop unless it's last, then use last measure.
    // 3) if fills, but no trans, then last measure, unless it's last, then end of loop.
    // 4) if fills and transtions, then always last measure.

    int endOfTrack = MAIN_LOOP_PTR(CurrPartPtr)->nTick;

    return pos>endOfTrack;
}

/**
 * @brief TrackPlay
 * @param track
 * @param startTick
 * @param endTick
 * @param ratio
 * @param manualOffset
 *
 */
static void TrackPlay(MIDIPARSER_MidiTrack *track, int32_t startTick, int32_t endTick, float ratio,
        int32_t manualOffset, uint32_t partID) {
    float delay;

    // if the index of the song is outside the array of event, put it to the last value
    if (track->index >= track->event.size())
        track->index = track->event.size() - 1;

    // If the play position is after the last event
    if (track->event.empty() || startTick > track->event.back().tick)
        return;

    // if the play position is before the current track position
    if (startTick < track->event[track->index].tick)
        track->index = 0u;

    // Advance the track to the right position
    while (track->event[track->index].tick < startTick) {

        track->index++;

        // If no event is found exit the function
        if (track->index >= track->event.size())
            return;
    }

    // Play all the sound between start tick and end tick
    while (track->event[track->index].tick < endTick) {

        delay = ratio * (float) (manualOffset + track->event[track->index].tick - startTick);

        // Play the event
        SoundManager_playDrumsetNote(track->event[track->index].note,
                track->event[track->index].vel,
                delay,
                ratio,
                partID);
#ifdef am335x
        MIDINOTES_playNotes(
                track->event[track->index].note,
                track->event[track->index].vel,
                delay * 1000);
#endif

        track->index++;

        // if the index is outside the array of event exit
        if (track->index >= track->event.size())
            return;
    }
}

static uint32_t CalculateTranFillQuitSyncTick(uint32_t tickPos,
        uint32_t tickPerBar) {
    return ((1 + ((uint32_t) (tickPos / tickPerBar))) * tickPerBar);
}

static int32_t CalculateStartBarSyncTick(uint32_t tickPos,
        uint32_t tickPerBar, uint32_t barTriggerLimit) {

    if ((tickPos % tickPerBar) <= barTriggerLimit) {

        return ((0 + ((int32_t) (tickPos / tickPerBar))) * tickPerBar);
    } else {
        return ((1 + ((int32_t) (tickPos / tickPerBar))) * tickPerBar);
    }
}

static void StopSong(void) {

    uint8_t status = IntDisable();
    DrumFillIndex = 0;
    PartIndex = 0;
    MasterTick = 0;
    BeatCounter = 0;
    NextPartNumber = 0;
    SobrietyDrumTranFill = 0;
    Test = 0;
//    loop = 0;
    // Cancel any pending action
    RequestFlag = REQUEST_DONE;
    if (CurrSongPtr != NULL ) {
        PlayerStatus = STOPPED;
    } else {
        PlayerStatus = NO_SONG_LOADED;
    }
    CurrPartPtr = NULL;
    IntEnable(status);

    SpecialEffectManager();
    if (SendStopOnEnd) UartMidi_SendStop();
    GUI_ForceRefresh();

}


static void ExternalNoteOnHandler(uint8_t note, uint8_t velocity) {
    if (MidiInNoteOnEnable && (velocity > 0 || MidiInNoteOffEnable)) {
        SoundManager_playDrumsetNote(note,velocity,0,0,NO_PART_ID);
    }
}

static void ExternalNoteOffHandler(uint8_t note, uint8_t velocity) {

    // Play note off (remove sound) only if note on AND note off is enabled is enabled
    if (MidiInNoteOnEnable && MidiInNoteOffEnable) {
        SoundManager_playDrumsetNote(note, 0, 0, 0, NO_PART_ID);
    }
}

#ifdef am335x
uint32_t SongPlayer_getCurrentPartIndex(uint32_t *num_part) {
    uint32_t n_part;
    uint32_t index;

    uint8_t status = IntDisable();
    if (CurrSongPtr == NULL ) {
        n_part = 0;
        index = 0;
    } else {
        n_part = CurrSongPtr->nPart;
        index = PartIndex;
    }

    IntEnable(status);

    *num_part = n_part;
    return index;
}

uint32_t SongPlayer_getDrumFillIndex(uint32_t *num_fills) {
    uint32_t n_fill;
    uint32_t index;

    uint8_t status = IntDisable();
    if (CurrPartPtr == NULL ) {
        n_fill = 0;
        index = 0;
    } else {
        n_fill = CurrSongPtr->part[PartIndex].nDrumFill;
        index = DrumFillIndex;
    }

    IntEnable(status);

    *num_fills = n_fill;
    return index;
}

static void SpecialEffectManager(void) {

    if (CurrSongPtr != NULL ) {
        if (PartIndex < CurrSongPtr->nPart) {
            SoundManager_RequestSpecialEffectLoading(
                    (char*) CurrSongPtr->part[PartIndex].effectName,
                    (char*) CurrSongPtr->part[(PartIndex + 1)% CurrSongPtr->nPart].effectName,
                    FALSE);
        }
    }
}
#else

static uint32_t GUI_GetCurrentWindows(void) {return 0;}
static uint8_t IntDisable(void) {return 0;}
static void IntEnable(uint8_t intStatus) {(void)intStatus;}
static void TEMPO_startWithInt(void) {}
static void UartMidi_SendStop(void) {}
static void UartMidi_sendContinue(void) {}
static void UartMidi_SendStart(void) {}
static void UartMidi_sendNextPartMessage(int32_t) {}
static void SpecialEffectManager(void) {}
static void GUI_ForceRefresh(void) {}
#endif


static void  NO_SONG_LOADED_ButtonHandler(BUTTON_EVENT event){
    (void)event;
}

static void  STOPPED_ButtonHandler(BUTTON_EVENT event){

    switch(event){
    case BUTTON_EVENT_PEDAL_PRESS:
        if (StartBeatOnPress && PedalPressFlag) {
            RequestFlag = START_REQUEST;
            PedalPressFlag = 0;
            SpecialEffectManager();
            TEMPO_startWithInt();
        }
        break;
    case BUTTON_EVENT_PEDAL_RELEASE:
        if (!StartBeatOnPress && PedalPressFlag) {
            RequestFlag = START_REQUEST;
            SpecialEffectManager();
            TEMPO_startWithInt();
        }
        break;

    default:
        break;
    }
}

static void  PAUSED_ButtonHandler(BUTTON_EVENT event){

    switch(event){

    case BUTTON_EVENT_PEDAL_PRESS:
        if (StartBeatOnPress) {
            RequestFlag = START_REQUEST;
            SpecialEffectManager();
            TEMPO_startWithInt();
            PedalPressFlag = 0;
        }
        break;

    case BUTTON_EVENT_PEDAL_RELEASE:
        if (!StartBeatOnPress) {
            RequestFlag = START_REQUEST;
            SpecialEffectManager();
            TEMPO_startWithInt();
        }
        break;

    case BUTTON_EVENT_PEDAL_LONG_PRESS:

        if (MainUnpauseModeHoldToTransition == START_TRANSITION) {
            NextPartNumber = 0; // Means no specefic next part
            RequestFlag = TRANFILL_REQUEST;
        } else {
            PedalPressFlag = 0; // FOrce a 0 to make sur the next release doesn't start the song again
            StopSong();
        }
        break;

    case BUTTON_EVENT_PEDAL_MULTI_TAP:
        StopSong();
        break;
    default:
        break;
    }
}


static void  INTRO_ButtonHandler(BUTTON_EVENT event){

    switch(event){
    case BUTTON_EVENT_PEDAL_MULTI_TAP:
        RequestFlag = SWAP_TO_OUTRO_REQUEST;
        if (!TrippleTapEnable) {
            MultiTapCounter = 0;
        }
        break;
    default:
        break;
    }
}

static void  PLAYING_MAIN_TRACK_ButtonHandler(BUTTON_EVENT event){

    switch(event){
    case BUTTON_EVENT_PEDAL_RELEASE:
        RequestFlag = DRUMFILL_REQUEST;
        break;
    case BUTTON_EVENT_PEDAL_LONG_PRESS:
        NextPartNumber = 0; // Means no specefic next part
        RequestFlag = TRANFILL_REQUEST;
        break;
    case BUTTON_EVENT_PEDAL_MULTI_TAP:
        if (WasPausedFlag) {
            RequestFlag = SWAP_TO_OUTRO_REQUEST;
        } else {
            RequestFlag = STOP_REQUEST;
            // will allow futur multi tap
            PedalPressFlag = 0;
            if (!TrippleTapEnable) {
                MultiTapCounter = 0;
            }
        }
        break;
    default:
        break;
    }
}

static void  PLAYING_MAIN_TRACK_TO_END_ButtonHandler(BUTTON_EVENT event){

    switch (event){
    case BUTTON_EVENT_PEDAL_RELEASE:
        RequestFlag = OUTRO_CANCEL_REQUEST;
        break;
    }
}

static void  NO_FILL_TRAN_ButtonHandler(BUTTON_EVENT event){

    switch(event){
    case BUTTON_EVENT_PEDAL_RELEASE:
        RequestFlag = TRANFILL_QUIT_REQUEST;
        break;
    case BUTTON_EVENT_PEDAL_MULTI_TAP:
        RequestFlag = STOP_REQUEST;
        PedalPressFlag = 0;
        if (!TrippleTapEnable) {
            MultiTapCounter = 0;
        }
        break;
    default:
        break;
    }
}

static void  NO_FILL_TRAN_QUITTING_ButtonHandler(BUTTON_EVENT event){

    switch (event) {

    case BUTTON_EVENT_PEDAL_RELEASE:
        RequestFlag = TRANFILL_CANCEL_REQUEST;
        break;
    case BUTTON_EVENT_PEDAL_MULTI_TAP:
        RequestFlag = STOP_REQUEST;
        PedalPressFlag = 0;
        if (!TrippleTapEnable) {
            MultiTapCounter = 0;
        }
        break;
    }

}
static void  NO_FILL_TRAN_CANCEL_ButtonHandler(BUTTON_EVENT event){
    (void) event;
}


static void  TRANFILL_WAITING_TRIG_ButtonHandler(BUTTON_EVENT event){

    switch (event){
    case BUTTON_EVENT_PEDAL_RELEASE:
        RequestFlag = TRANFILL_QUIT_REQUEST;
        break;
    case BUTTON_EVENT_PEDAL_MULTI_TAP:
        RequestFlag = STOP_REQUEST;
        PedalPressFlag = 0;
        if (!TrippleTapEnable) {
            MultiTapCounter = 0;
        }
        break;
    default:
        break;
    }
}

static void  TRANFILL_ACTIVE_ButtonHandler(BUTTON_EVENT event){

    switch (event) {
    case BUTTON_EVENT_PEDAL_RELEASE:
        RequestFlag = TRANFILL_QUIT_REQUEST;
        break;
    case BUTTON_EVENT_PEDAL_MULTI_TAP:
        RequestFlag = STOP_REQUEST;
        PedalPressFlag = 0;
        if (!TrippleTapEnable) {
            MultiTapCounter = 0;
        }
        break;
    default:
        break;
    }

}
static void  TRANFILL_QUITING_ButtonHandler(BUTTON_EVENT event){

    switch (event) {
    case BUTTON_EVENT_PEDAL_MULTI_TAP:
    case BUTTON_EVENT_PEDAL_RELEASE:
        RequestFlag = TRANFILL_CANCEL_REQUEST;
        WasLongPressed = true;
        break;
    }
}

static void  TRANFILL_CANCEL_ButtonHandler(BUTTON_EVENT event){
    (void) event;
}

static void  DRUMFILL_WAITING_TRIG_ButtonHandler(BUTTON_EVENT event){
    switch (event) {
    case BUTTON_EVENT_PEDAL_LONG_PRESS:
        NextPartNumber = 0; // Means no specefic next part
        RequestFlag = TRANFILL_REQUEST;
        break;
    case BUTTON_EVENT_PEDAL_MULTI_TAP:
        RequestFlag = STOP_REQUEST;
        PedalPressFlag = 0;
        if (!TrippleTapEnable) {
            MultiTapCounter = 0;
        }
        break;
    }
}
static void  DRUMFILL_ACTIVE_ButtonHandler(BUTTON_EVENT event){
    switch (event) {
    case BUTTON_EVENT_PEDAL_LONG_PRESS:
        RequestFlag = TRANFILL_REQUEST;
        break;
    case BUTTON_EVENT_PEDAL_MULTI_TAP:
        RequestFlag = STOP_REQUEST;
        PedalPressFlag = 0;
        if (!TrippleTapEnable) {
            MultiTapCounter = 0;
        }
        break;
    default:
        break;
    }
}

static void  OUTRO_ButtonHandler(BUTTON_EVENT event){
    switch (event){

    case BUTTON_EVENT_PEDAL_PRESS:
    case BUTTON_EVENT_PEDAL_MULTI_TAP:
        RequestFlag = OUTRO_CANCEL_REQUEST;
        PedalPressFlag = false;
        WasLongPressed = true;
        break;
    default:
        break;
    }
}
static void  OUTRO_WAITING_TRIG_ButtonHandler(BUTTON_EVENT event){
    switch (event){
    case BUTTON_EVENT_PEDAL_PRESS:
    case BUTTON_EVENT_PEDAL_MULTI_TAP:
        // ingnore next release
        PedalPressFlag = false;
        WasLongPressed = true;
        RequestFlag = OUTRO_CANCEL_REQUEST;
        if (!TrippleTapEnable) {
            MultiTapCounter = 0;
        }
        break;
    default:
        break;
    }
}

static void  OUTRO_CANCELED_ButtonHandler(BUTTON_EVENT event){
    (void)event;
}

static unsigned long long LastMultiTapTime = 0;

#ifdef am335x
void SONGPLAYER_ButtonCallback(BUTTON_EVENT event, unsigned long long time)
#else
void SongPlayer_ButtonCallback(BUTTON_EVENT event, unsigned long long time)
#endif
{
    uint8_t status = IntDisable();

    switch (event) {
    case BUTTON_EVENT_PEDAL_PRESS:
        MultiTapCounter = 0;
        WasPausedFlag = FALSE;
        PedalPressFlag = TRUE;
        WasLongPressed = FALSE;
        break;

    case BUTTON_EVENT_PEDAL_RELEASE:
        if (!PedalPressFlag) goto end_handler;
        break;

    case BUTTON_EVENT_PEDAL_LONG_PRESS:
        WasLongPressed = true;
        if (!PedalPressFlag)  goto end_handler;
        break;

    case BUTTON_EVENT_PEDAL_MULTI_TAP:

        if (MultiTapCounter){
            if ((time - LastMultiTapTime) < 350) {

                // if tripple tap is enable
                if (TrippleTapEnable){
                    StopSong();
                }
            }
        } else {
            if (WasLongPressed){
                event = BUTTON_EVENT_PEDAL_PRESS;
                MultiTapCounter = 0;
                WasPausedFlag = false;
                PedalPressFlag = true;
                WasLongPressed = false;
            }
        }

        LastMultiTapTime = time;
        MultiTapCounter++;
        break;

    case BUTTON_EVENT_FOOT_SECONDARY_PRESS:

        if (PlayerStatus != NO_SONG_LOADED) {
            if (PlayerStatus != STOPPED) {
                if (SecondaryFootSwitchPlayingAction == ACTION_PAUSE_UNPAUSE) {
                    PedalPressFlag = 0;
                    PauseUnpauseHandler();
                } else if (SecondaryFootSwitchPlayingAction == ACTION_SPECIAL_EFFECT) {
                    SoundManager_playSpecialEffect(100, FALSE);
                } else if (SecondaryFootSwitchPlayingAction == ACTION_OUTRO) {
                    event = BUTTON_EVENT_PEDAL_MULTI_TAP;
                }
            } else {
                if (SecondaryFootSwitchStoppedAction == ACTION_SPECIAL_EFFECT) {
                    SoundManager_playSpecialEffect(100, FALSE);
                }
            }
        }
        break;

    case BUTTON_EVENT_FOOT_PRIMARY_PRESS:

        if (PlayerStatus != NO_SONG_LOADED) {
            if (PlayerStatus != STOPPED) {
                if (PrimaryFootSwitchPlayingAction == ACTION_PAUSE_UNPAUSE) {
                    PedalPressFlag = 0;
                    PauseUnpauseHandler();
                } else if (PrimaryFootSwitchPlayingAction == ACTION_SPECIAL_EFFECT) {
                    SoundManager_playSpecialEffect(100, FALSE);
                } else if (PrimaryFootSwitchPlayingAction == ACTION_OUTRO) {
                    event = BUTTON_EVENT_PEDAL_MULTI_TAP;
                }
            } else {
                if (PrimaryFootSwitchStoppedAction == ACTION_SPECIAL_EFFECT) {
                    SoundManager_playSpecialEffect(100, FALSE);
                }
            }
        }
        break;
    default:
        break;
    }

    if (MultiTapCounter > 1) {
        goto end_handler;
    }

    switch(PlayerStatus) {
    case NO_SONG_LOADED:            { NO_SONG_LOADED_ButtonHandler(event);              break;}
    case STOPPED:                   { STOPPED_ButtonHandler(event);                     break;}
    case PAUSED:                    { PAUSED_ButtonHandler(event);                      break;}
    case INTRO:                     { INTRO_ButtonHandler(event);                       break;}
    case PLAYING_MAIN_TRACK:        { PLAYING_MAIN_TRACK_ButtonHandler(event);          break;}
    case PLAYING_MAIN_TRACK_TO_END: { PLAYING_MAIN_TRACK_TO_END_ButtonHandler(event);   break;}
    case DRUMFILL_WAITING_TRIG:     { DRUMFILL_WAITING_TRIG_ButtonHandler(event);       break;}
    case DRUMFILL_ACTIVE:           { DRUMFILL_ACTIVE_ButtonHandler(event);             break;}
    case TRANFILL_WAITING_TRIG:     { TRANFILL_WAITING_TRIG_ButtonHandler(event);       break;}
    case TRANFILL_ACTIVE:           { TRANFILL_ACTIVE_ButtonHandler(event);             break;}
    case TRANFILL_QUITING:          { TRANFILL_QUITING_ButtonHandler(event);            break;}
    case TRANFILL_CANCEL:           { TRANFILL_CANCEL_ButtonHandler(event);             break;}
    case NO_FILL_TRAN:              { NO_FILL_TRAN_ButtonHandler(event);                break;}
    case NO_FILL_TRAN_QUITTING:     { NO_FILL_TRAN_QUITTING_ButtonHandler(event);       break;}
    case NO_FILL_TRAN_CANCEL:       { NO_FILL_TRAN_CANCEL_ButtonHandler(event);         break;}
    case OUTRO:                     { OUTRO_ButtonHandler(event);                       break;}
    case OUTRO_WAITING_TRIG:        { OUTRO_WAITING_TRIG_ButtonHandler(event);          break;}
    case OUTRO_CANCELED:            { OUTRO_CANCELED_ButtonHandler(event);              break;}
#ifdef am335x
    case COUNT_IN_TO_MAIN:          { /* No action for now */                           break;}
#endif
    }

    end_handler:
    IntEnable(status);

}

