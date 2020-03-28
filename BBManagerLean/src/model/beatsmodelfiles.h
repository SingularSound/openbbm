#ifndef BEATSMODELFILES_H
#define BEATSMODELFILES_H

/*
 * File Extensions
 */

// General extensions
#define BMFILES_MIDI_EXTENSION              "mid"
#define BMFILES_WAVE_EXTENSION              "wav"
#define BMFILES_CSV_EXTENSION               "csv"
#define BMFILES_INI_EXTENSION               "ini"

// Portable files
#define BMFILES_PROJECT_EXTENSION           "bbp"
#define BMFILES_SINGLE_FILE_EXTENSION       "bbz"
#define BMFILES_PORTABLE_SONG_EXTENSION     "sng"
#define BMFILES_PORTABLE_FOLDER_EXTENSION   "pbf"
#define BMFILES_SONG_TRACK_EXTENSION        "bbt"

// Internal files
#define BMFILES_MIDI_BASED_SONG_EXTENSION   "bbs"
#define BMFILES_WAVE_BASED_SONG_EXTENSION   "bws"
#define BMFILES_DRUMSET_EXTENSION           "drm"
#define BMFILES_CONFIG_FILE_EXTENSION       "bcf"
#define BMFILES_SONG_MOD_EXTENSION          "mod"

/*
 * Standardly used file names
 */
#define BMFILES_HASH_FILE_NAME          "hash."    BMFILES_CONFIG_FILE_EXTENSION
#define BMFILES_EFFECT_USAGE_FILE_NAME  "usage."   BMFILES_CONFIG_FILE_EXTENSION
#define BMFILES_INFO_MAP_FILE_NAME      "info."    BMFILES_CONFIG_FILE_EXTENSION
#define BMFILES_FOOTSW_CONFIG_FILE_NAME "footsw."  BMFILES_INI_EXTENSION
#define BMFILES_PORTABLE_SONG_VERSION   "version." BMFILES_CONFIG_FILE_EXTENSION
#define BMFILES_PORTABLE_FOLDER_VERSION "version." BMFILES_CONFIG_FILE_EXTENSION
#define BMFILES_NAME_TO_FILE_MAPPING    "config."  BMFILES_CSV_EXTENSION

/*
 * Filter text types
 */
#define BMFILES_PORTABLE_SONG_FILTER    "*." BMFILES_PORTABLE_SONG_EXTENSION
#define BMFILES_PORTABLE_FOLDER_FILTER  "*." BMFILES_PORTABLE_FOLDER_EXTENSION
#define BMFILES_WAVE_FILTER             "*." BMFILES_WAVE_EXTENSION
#define BMFILES_MIDI_FILTER             "*." BMFILES_MIDI_EXTENSION
#define BMFILES_SONG_TRACK_FILTER       "*." BMFILES_SONG_TRACK_EXTENSION
#define BMFILES_DRUMSET_FILTER          "*." BMFILES_DRUMSET_EXTENSION
#define BMFILES_PROJECT_FILTER          "*." BMFILES_PROJECT_EXTENSION
#define BMFILES_SINGLE_FILE_FILTER      "*." BMFILES_SINGLE_FILE_EXTENSION
#define BMFILES_MIDI_BASED_SONG_FILTER  "*." BMFILES_MIDI_BASED_SONG_EXTENSION
#define BMFILES_INI_FILE_FILTER         "*." BMFILES_INI_EXTENSION
#define BMFILES_SONG_MOD_FILE_FILTER    "*." BMFILES_SONG_MOD_EXTENSION
#define BMFILES_ALL_FILTER              "*.*"
#define BMFILES_SEPARATOR               ";;"

static const auto BMFILES_PORTABLE_FOLDER   = QObject::tr("Portable Folder File", "File Type")         + " (" BMFILES_PORTABLE_FOLDER_FILTER ")";
static const auto BMFILES_PORTABLE_SONG     = QObject::tr("Portable Song File", "File Type")           + " (" BMFILES_PORTABLE_SONG_FILTER ")";
static const auto BMFILES_PORTABLE          = QObject::tr("Portable Song or Folder File", "File Type") + " (" BMFILES_PORTABLE_SONG_FILTER " " BMFILES_PORTABLE_FOLDER_FILTER ")";
static const auto BMFILES_WAVE              = QObject::tr("Wave File", "File Type")                    + " (" BMFILES_WAVE_FILTER ")";
static const auto BMFILES_MIDI              = QObject::tr("Midi File", "File Type")                    + " (" BMFILES_MIDI_FILTER ")";
static const auto BMFILES_SONG_TRACK        = QObject::tr("Track File", "File Type")                   + " (" BMFILES_SONG_TRACK_FILTER ")";
static const auto BMFILES_DRUMSET           = QObject::tr("Drum Set File", "File Type")                + " (" BMFILES_DRUMSET_FILTER ")";
static const auto BMFILES_PROJECT           = QObject::tr("BeatBuddy Project File", "File Type")       + " (" BMFILES_PROJECT_FILTER ")";
static const auto BMFILES_SINGLE_FILE       = QObject::tr("Single File Project File", "File Type")     + " (" BMFILES_SINGLE_FILE_FILTER ")";
static const auto BMFILES_ALL_PROJECT       = QObject::tr("BeatBuddy Project", "File Type")            + " (" BMFILES_PROJECT_FILTER " " BMFILES_SINGLE_FILE_FILTER ")";
static const auto BMFILES_MIDI_TRACK        = QObject::tr("Midi or Track File", "File Type")           + " (" BMFILES_MIDI_FILTER " " BMFILES_SONG_TRACK_FILTER ")";
static const auto BMFILES_ALL_FILES         = QObject::tr("All files", "File Type")                    + " (" BMFILES_ALL_FILTER ")";

/*
 * QFileDialog filter text types
 */
static const auto BMFILES_PORTABLE_FOLDER_DIALOG_FILTER = BMFILES_PORTABLE_FOLDER + BMFILES_SEPARATOR + BMFILES_ALL_FILES;
static const auto BMFILES_PORTABLE_SONG_DIALOG_FILTER   = BMFILES_PORTABLE_SONG   + BMFILES_SEPARATOR + BMFILES_PORTABLE_FOLDER_DIALOG_FILTER;
static const auto BMFILES_PORTABLE_DIALOG_FILTER        = BMFILES_PORTABLE        + BMFILES_SEPARATOR + BMFILES_PORTABLE_SONG_DIALOG_FILTER;
static const auto BMFILES_WAVE_DIALOG_FILTER            = BMFILES_WAVE            + BMFILES_SEPARATOR + BMFILES_ALL_FILES;
static const auto BMFILES_MIDI_DIALOG_FILTER            = BMFILES_MIDI            + BMFILES_SEPARATOR + BMFILES_ALL_FILES;
static const auto BMFILES_SONG_TRACK_DIALOG_FILTER      = BMFILES_SONG_TRACK      + BMFILES_SEPARATOR + BMFILES_ALL_FILES;
static const auto BMFILES_DRUMSET_DIALOG_FILTER         = BMFILES_DRUMSET         + BMFILES_SEPARATOR + BMFILES_ALL_FILES;
static const auto BMFILES_PROJECT_DIALOG_FILTER         = BMFILES_PROJECT         + BMFILES_SEPARATOR + BMFILES_ALL_FILES;
static const auto BMFILES_SINGLE_FILE_DIALOG_FILTER     = BMFILES_SINGLE_FILE     + BMFILES_SEPARATOR + BMFILES_ALL_FILES;
static const auto BMFILES_SAVE_PROJECT_DIALOG_FILTER    = BMFILES_PROJECT         + BMFILES_SEPARATOR + BMFILES_SINGLE_FILE;
static const auto BMFILES_OPEN_PROJECT_DIALOG_FILTER    = BMFILES_ALL_PROJECT     + BMFILES_SEPARATOR + BMFILES_PROJECT + BMFILES_SEPARATOR + BMFILES_SINGLE_FILE + BMFILES_SEPARATOR + BMFILES_ALL_FILES;
static const auto BMFILES_MIDI_TRACK_DIALOG_FILTER      = BMFILES_MIDI_TRACK      + BMFILES_SEPARATOR + BMFILES_ALL_FILES;

#endif // BEATSMODELFILES_H
