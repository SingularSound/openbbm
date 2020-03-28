#include <AppKit/AppKit.h>
#include <Cocoa/Cocoa.h>
#include <QCoreApplication>
#include <QDir>
#include <QDebug>
#include <QByteArray>
#include "macosxplatform.h"

#define FOLDER_ICON_NAME "BeatBuddy_Project_Folder.icns"

MacOSXPlatform::MacOSXPlatform()
{
}


bool MacOSXPlatform::CreateProjectShellIcon(const QString &path)
{
    // Initialize auto release pool for this code section
    NSAutoreleasePool * p_autoReleasePool = [[NSAutoreleasePool alloc] init];

    // Obtain path towards resource icon in NSString format
    QDir resourcesDir(QCoreApplication::applicationDirPath());
    resourcesDir.cdUp();
    resourcesDir.cdUp(); // Bundle path
    resourcesDir.cd("Contents");
    resourcesDir.cd("Resources");

    if(!resourcesDir.exists()){
        qWarning() << "MacOSXPlatform::CreateProjectShellIcon - ERROR1 - Resources folder does not exist:" << resourcesDir.absolutePath();
        [p_autoReleasePool release];
        return false;
    }

    QFileInfo iconFI(resourcesDir.absoluteFilePath(FOLDER_ICON_NAME));
    if(!iconFI.exists() || !iconFI.isFile()){
        qWarning() << "MacOSXPlatform::CreateProjectShellIcon - ERROR2 - Icon does not exist:" << iconFI.absoluteFilePath();
        [p_autoReleasePool release];
        return false;
    }

    NSString* macIconPath = [NSString stringWithUTF8String:iconFI.absoluteFilePath().toUtf8().data()];

    // Obtain path towards project folder in NSString format
    QFileInfo projectFolderFI(path);

    if(!projectFolderFI.exists() || !projectFolderFI.isDir()){
        qWarning() << "MacOSXPlatform::CreateProjectShellIcon - ERROR3 - Project Folder does not exist:" << projectFolderFI.absoluteFilePath();
        [p_autoReleasePool release];
        return false;
    }

    NSString* macProjectFolderPath = [NSString stringWithUTF8String:projectFolderFI.absoluteFilePath().toUtf8().data()];

    // Create an NSImage with icon
    NSImage* iconImage = [[NSImage alloc] initWithContentsOfFile:macIconPath];
    if(iconImage == nil){
        qWarning() << "MacOSXPlatform::CreateProjectShellIcon - ERROR4 - Unable to create iconImage:" << projectFolderFI.absoluteFilePath();
        [p_autoReleasePool release];
        return false;
    }

    // Use NSWorkspace method to apply image to folder
    if(![[NSWorkspace sharedWorkspace] setIcon:iconImage forFile:macProjectFolderPath options:0]){
        qWarning() << "MacOSXPlatform::CreateProjectShellIcon - ERROR5 - failed to apply image";
        [p_autoReleasePool release];
        return false;
    }

    [p_autoReleasePool release];
    return true;
}
