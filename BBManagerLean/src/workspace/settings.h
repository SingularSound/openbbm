#ifndef SETTINGS_H
#define SETTINGS_H

#include <QtCore>
#include <QRgb>

#define KEY_UUID "general/uuid"
#define KEY_AUTOPILOT "general/autopilot"
#define KEY_LINKED_PRJ_UUID "general/linked_project_uuid"

#define KEY_MAXIMIZED "window/maximized"
#define KEY_X "window/x"
#define KEY_Y "window/y"
#define KEY_W "window/w"
#define KEY_H "window/h"

#define KEY_BUFFERING_TIME "player_buffering_time"

#define KEY_DND_W "color_dnd_withdraw"
#define KEY_DND_C "color_dnd_copy"
#define KEY_DND_T "color_dnd_target"
#define KEY_DND_A "color_dnd_append"

#define KEY_USE_MIDI_ID "use_midi_id"
#define KEY_HELP_INDEX "general/help_index"
#define KEY_HELP_PDF "general/help_pdf"

#define KEY_LAST_PROJECT "BBWorkspace/user_lib/projects/last_project"
#define KEY_SD_CARD "BBWorkspace/user_lib/projects/sd_card_dir"

#define KEY_WORKSPACE_LOCATION "BBWorkspace/default_path"
#define WORKSPACE_FOLDER_NAME "BBWorkspace"
#define KEY_WORKSPACE_NAME "BBWorkspace"

/* Rest of known settings keys:
BBWorkspace
BBWorkspace/default_path
BBWorkspace/user_lib/projects/sd_card_dir

BBWorkspace/default_lib/current_path
BBWorkspace/default_lib/default_path
BBWorkspace/default_lib/drum_sets/current_path
BBWorkspace/default_lib/drum_sets/default_path
BBWorkspace/default_lib/midi_sources/current_path
BBWorkspace/default_lib/midi_sources/default_path
BBWorkspace/default_lib/projects/current_path
BBWorkspace/default_lib/projects/default_path
BBWorkspace/default_lib/songs/current_path
BBWorkspace/default_lib/songs/default_path
BBWorkspace/default_lib/wave_sources/current_path
BBWorkspace/default_lib/wave_sources/default_path

BBWorkspace/user_lib/current_path
BBWorkspace/user_lib/default_path
BBWorkspace/user_lib/drum_sets/current_path
BBWorkspace/user_lib/drum_sets/default_path
BBWorkspace/user_lib/midi_sources/current_path
BBWorkspace/user_lib/midi_sources/default_path
BBWorkspace/user_lib/projects/current_path
BBWorkspace/user_lib/projects/default_path
BBWorkspace/user_lib/projects/last_project
BBWorkspace/user_lib/songs/current_path
BBWorkspace/user_lib/songs/default_path
BBWorkspace/user_lib/wave_sources/current_path
BBWorkspace/user_lib/wave_sources/default_path
*/


class Settings
{
   Settings();

public:

   static bool softwareUuidExists();
   static QUuid getSoftwareUuid();
   static void setSoftwareUuid(const QUuid &uuid);

   static bool linkedPrjUuidExists();
   static QUuid getLinkedPrjUuid();
   static void setLinkedPrjUuid(const QUuid &uuid);

   static bool lastProjectExists();
   static QString getLastProject();
   static void setLastProject(const QString &path);

   static bool lastSdCardDirExists();
   static QString getLastSdCardDir();
   static void setLastSdCardDir(const QString &path);

   static bool bufferingTime_msExists();
   static int getBufferingTime_ms();
   static void setBufferingTime_ms(int bufferingTime_ms);

   static bool helpIndexExists();
   static QString getHelpIndexDir();

   static bool helpPdfExists();
   static QString getHelpPdfDir();

   static int getCurrentVersion();
   static int getUpdateVersion();
   static void setUpdateVersion(int);

   static bool colorDnDWithdrawExists();
   static QRgb getColorDnDWithdraw();
   static void setColorDnDWithdraw(QRgb value);

   static bool colorDnDCopyExists();
   static QRgb getColorDnDCopy();
   static void setColorDnDCopy(QRgb value);

   static bool colorDnDTargetExists();
   static QRgb getColorDnDTarget();
   static void setColorDnDTarget(QRgb value);

   static bool colorDnDAppendExists();
   static QRgb getColorDnDAppend();
   static void setColorDnDAppend(QRgb value);


   static bool windowMaximized();
   static bool getWindowMaximized();
   static void setWindowMaximized(bool value);

   static bool windowXExists();
   static int  getWindowX();
   static void setWindowX(int value);

   static bool windowYExists();
   static int  getWindowY();
   static void setWindowY(int value);

   static bool windowHExists();
   static int  getWindowH();
   static void setWindowH(int value);

   static bool windowWExists();
   static int  getWindowW();
   static void setWindowW(int value);
   static void toggleMidiId();
   static bool midiIdEnabled();

   static QDir getWorkspaceLocation();
   static void setWorkspaceLocation(QString location);

   static void setWorkspaceName(QString path);
   static void setAutoPilot(bool value);

};

#endif // SETTINGS_H
