#ifndef PLAYER_SETTINGS_H
#define PLAYER_SETTINGS_H



typedef enum {
    FOOTSWITCH_TYPE,                        /* Type of the footswitch (Momentary/Latching)                          */
    FOOTSWITCH_ORDER,                       /* Order of the footswitch (left->right / right->left)                  */
    FOOTSWITCH_POLARITY,                    /* Polarity of the switchs (Normaly ON/OFF)                             */
    MAIN_UNPAUSE_MODE_TAP,                  /* What to do when unpausing song by tapping main pedal                 */
    MAIN_UNPAUSE_MODE_HOLD,                 /* What to do when unpausing song by holding main pedal                 */
    ACTIVE_PAUSE,                           /* Active pause feature (YES/NO/MIDI_ACTIVATED)                         */
    PRIMARY_FOOTSWITCH_ACTION_PLAYING,      /* Left Pedal action when playing                                       */
    PRIMARY_FOOTSWITCH_ACTION_STOPPED,      /* Left Pedal action when stopped                                       */
    SECONDARY_FOOTWTWICH_ACTION_PLAYING,    /* Right Pedal action when paying                                       */
    SECONDARY_FOOTSWITCH_ACTION_STOPPED,    /* Right Pedal action when stopped                                      */
    FOOTSWITCH_DETECTOR_KEY,                /* Action to start Footswitch detector                                  */
    EXIT_CMD,                               /* Exit CMD                                                             */
    INFO_CMD,                               /* (NOT_USED)                                                           */
    TRIPLE_TAP_STOP,                        /* Tripple tap feature that stops immediatly the player                 */
    RETURN_CMD,                             /* (NOT_USED)                                                           */
    RESTORE_FACTORY_SETTINGS_TRUE,          /* Restore to factory settins action                                    */
    RESTORE_FACTORY_SETTINGS_FALSE,         /* Cancel Restore to factory settings action (TODO new type of menu)    */
    FIRMWARE_VERSION,                       /* Current firmware version of the beatbuddy                            */
    ERASE_SETTINGS_FILES,                   /* Action Settings that will should erase settings files                */
    CUE_PERIOD,                             /* Position in the bar (in %) where transitions are called              */
    MIDI_MESSAGE_START_SONG,                /* When to send Start message / Where to start song when rx Start Msg   */
    RELEASE_TIME,                           /*  */
    NUMBER_OF_KEY
}SETTINGS_main_key_enum;

typedef enum {
    TYPE_NONE,          /* No params data */
    TYPE_STRING,        /* String Address */
    TYPE_UIN32,     /* The data is pointed by a pointer */
    NUMBER_OF_SETTINGS_INFO_DATA_TYPE
} SETTINGS_info_entry_type;

typedef enum {
    PERMANENT,
    MOMENTARY,
    NUM_OF_FOOTSWITCH_TYPE
} FOOTSWITCH_TYPE_params;

typedef enum {
    FOOTSWITCH_SWAP_FALSE,
    FOOTSWITCH_SWAP_TRUE,
    NUM_OF_FOOTSWITCH_SWAP
}FOOTSWITCH_SWAP_params;

typedef enum {
    FOOTSWITCH_NORMAL_POL,
    FOOTSWITCH_INVERTED_POL,
}FOOTSWITCH_POLARITY_params;

typedef enum {
    MAIN_UNPAUSE_TAP_TO_INTRO,
    MAIN_UNPAUSE_TAP_TO_FILL,
    NUM_OF_MAIN_UNPAUSE_TAP
}MAIN_UNPAUSE_TAP_params;

typedef enum  {
    CUE_PERIOD_0_75_PERCENT,
    CUE_PERIOD_0_66_PERCENT,
    CUE_PERIOD_0_50_PERCENT,
    CUE_PERIOD_0_33_PERCENT,
    CUE_PERIOD_0_25_PERCENT,
    NUM_CUE_PERIOD_PERCENT,
}CUE_PERIOD_FIXES_params;

typedef enum {
    MIDI_START_MESSAGE_INTRO,
    MIDI_START_MESSAGE_FIRST_PART,
    NUM_START_MESSAGE_params
}MIDI_START_MESSAGE_params;

typedef enum {
    MAIN_UNPAUSE_HOLD_TO_STOP_SONG,
    MAIN_UNPAUSE_HOLD_TO_START_TRAN,
    NUM_OF_MAIN_UNPAUSE_HOLD
}MAIN_UNPAUSE_HOLD_params;

typedef enum {
    STOPPED_NO_ACTION,
    STOPPED_ACCENT_HIT,
    STOPPED_SONG_ADVANCE,
    STOPPED_TAP_TEMPO,
    STOPPED_SONG_BACK,
    NUM_STOPPED_ACTION
}STOPPED_ACTION_params;

typedef enum {
    TRIPLE_TAP_STOP_ENABLE,
    TRIPLE_TAP_STOP_DISABLE,
    NUM_TRIPLE_TAP_STOP,
}TRIPLE_TAP_STOP_params;

typedef enum {
    PLAYING_NO_ACTION,
    PLAYING_ACCENT_HIT,
    PLAYING_TAP_TEMPO,
    PLAYING_PAUSE_UNPAUSE,
    PLAYING_SONG_BACK,
    NUM_PLAYING_ACTION
}PLAYING_ACTION_params;

typedef enum {
    ACTIVE_PAUSE_DISABLE,
    ACTIVE_PAUSE_ENABLE,
    ACTIVE_PAUSE_MIDI_ACTIVATED,
    NUM_ACTIVE_PAUSE
}ACTIVE_PAUSE_params;

typedef struct {
    SETTINGS_info_entry_type type;
    void* entry_data;
}SETTINGS_information_menu_entry;

typedef struct {
    int value;
    const char* name;
}SETTINGS_single_choice_entry;

typedef struct {
    const char* name;
    const char* section_name;
    const char* key_name;
    const char * file_path;
    char* dynamic_data;
}SETTINGS_information_string_entry;

typedef struct {
    const char* name;
    const char* section_name;
    const char* key_name;
    const char * file_path;
    unsigned int * value;
}SETTINGS_information_uint32_entry;


typedef enum {
    SINGLE_CHOICE_MENU,
    INFORMATIONS_MENU,
    SETTINGS_MENU,
    ACTION_MENU,
    NUMBER_OF_ENTRY_TYPE
}SETTINGS_menu_type;

typedef enum {
    RELEASE_TIME_0MS,
    RELEASE_TIME_10MS,
    RELEASE_TIME_20MS,
    RELEASE_TIME_30MS,
    RELEASE_TIME_40MS,
    RELEASE_TIME_50MS,
    RELEASE_TIME_60MS,
    RELEASE_TIME_70MS,
    RELEASE_TIME_80MS,
    RELEASE_TIME_90MS,
    RELEASE_TIME_100MS,
    RELEASE_TIME_250MS,
    RELEASE_TIME_500MS,
    NUM_RELEASE_TIME
}SETTINGS_release_time;

typedef struct {
    const char* title;
    SETTINGS_main_key_enum key;
}SETTINGS_action_menu;

typedef struct {
    char* title;
    int n_param;
    SETTINGS_information_menu_entry * params;
    SETTINGS_action_menu *lastItemAction;
}SETTINGS_information_menu;



typedef struct {

    const char* title;                      /* Title that will be diplayed in the menu                  */
    const char* section_name;               /* String of the section that will be in the ini file       */
    const char* key_name;                   /* String of the key that will be written in the ini file   */

    SETTINGS_main_key_enum key;
    int selected;                           /* Selection option on the menu                             */
    int default_value;                      /* Number of possible single choice in the setting entry    */

    int n_param;                            /* Number of entry in the menu                              */
    SETTINGS_single_choice_entry *params;   /* Array of single choice params entry                      */
    const char* file_path;                  /* Where to save the settings entry                         */
    SETTINGS_action_menu *lastItemAction;
}SETTINGS_single_choice_menu;




/**
 * param = Pointer to the next structure of setttings:
 *      If the sturcture is a folder : *SettingEntry
 *      If the structure is a single choice : *setting_param
 */
typedef struct {
    SETTINGS_menu_type type;
    void * param;
}SETTINGS_setting_menu_entry;

typedef struct {
    char* title;
    int n_entries;
    SETTINGS_setting_menu_entry *entries;
    SETTINGS_action_menu *lastItemAction;
} SETTINGS_settingList_menu;


typedef void (*SETTINGS_SettingChangedPtr)(SETTINGS_main_key_enum key, int value);



/*****************************************************************************
 **                     EXTERNAL MACROS (DEFAULT SETTINGS)
 *****************************************************************************/
#define DEFAULT_FOOTSWITCH_TYPE             PERMANENT
#define DEFAULT_FOOTSWITCH_SWAP             FOOTSWITCH_SWAP_FALSE
#define DEFAULT_FOOTSWITCH_POLARITY			FOOTSWITCH_NORMAL_POL
#define DEFAULT_MAIN_UNPAUSE_TAP            MAIN_UNPAUSE_TAP_TO_FILL
#define DEFAULT_MAIN_UNPAUSE_HOLD           MAIN_UNPAUSE_HOLD_TO_STOP_SONG
#define DEFAULT_STOPPED_PRIMARY_ACTION      STOPPED_ACCENT_HIT
#define DEFAULT_PLAYING_PRIMARY_ACTION      PLAYING_ACCENT_HIT
#define DEFAULT_STOPPED_SECONDARY_ACTION    STOPPED_SONG_ADVANCE
#define DEFAULT_PLAYING_SECONDARY_ACTION    PLAYING_PAUSE_UNPAUSE
#define DEFAULT_ACTIVE_PAUSE                ACTIVE_PAUSE_MIDI_ACTIVATED
#define DEFAULT_TRIPLE_TAP_STOP             TRIPLE_TAP_STOP_DISABLE
#define DEFAULT_CUE_PERIOD                  CUE_PERIOD_0_75_PERCENT
#define DEFAULT_MIDI_START_MSG              MIDI_START_MESSAGE_FIRST_PART
#define DEFAULT_RELEASE_TIME                RELEASE_TIME_20MS


#define SAVE_DEFAULT_VALUE_WHEN_MISSING     0

/*****************************************************************************
 **                     FUNCTION PROTOTYPES
 *****************************************************************************/

void SETTINGS_init(void);
void SETTINGS_task(void);
void SETTINGS_load(void);
int SETTINGS_setSingleChoice(SETTINGS_single_choice_menu *menu, int value, int save_to_file);
void SETTINGS_restoreFactorySettings(void);
void SETTINGS_setPedalOrder(FOOTSWITCH_SWAP_params value);
void SETTINGS_setPedalType(FOOTSWITCH_TYPE_params value);
void SETTINGS_setPedalPolarity(FOOTSWITCH_POLARITY_params value);
void SETTINGS_registerCallback(SETTINGS_SettingChangedPtr clbk);

void SETTINGS_RequestsaveCurrentSong();
int SETTINGS_loadCurrentSongName(char* file_name, int max_length, int *folder_index, int *song_index);


void SETTINGS_setActivePause(ACTIVE_PAUSE_params value, int save_to_file);

#endif // PLAYER_SETTINGS_H

