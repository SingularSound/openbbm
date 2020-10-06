QT += core gui widgets multimedia network

CONFIG += c++11 debug


TARGET = BBManagerLean
QMAKE_MACOSX_DEPLOYMENT_TARGET = 10.12

macx {
   LIBS += -framework AppKit

#   ICON_FILES.files += ./mac_deployment/Bundle_src/Content/Resources/Application_Icon.icns
#   ICON_FILES.files += ./mac_deployment/Bundle_src/Content/Resources/BeatBuddy_Project_Folder.icns
#   ICON_FILES.files += ./mac_deployment/Bundle_src/Content/Resources/BBP.icns
#   ICON_FILES.files += ./mac_deployment/Bundle_src/Content/Resources/BBZ.icns
#   ICON_FILES.files += ./mac_deployment/Bundle_src/Content/Resources/DRM.icns
#   ICON_FILES.files += ./mac_deployment/Bundle_src/Content/Resources/BBS.icns
#   ICON_FILES.path  += Contents/Resources
#   QMAKE_BUNDLE_DATA += ICON_FILES
#   QMAKE_INFO_PLIST   = ./mac_deployment/Bundle_src/Content/Info.plist

   LIBS       += -L$$PWD/libs/quazip/macx/release/ -lquazip
   LIBS       += -L$$PWD/libs/minIni/macx/release/ -lminIni
   DEPENDPATH += $$PWD/./libs/quazip/macx/release
   DEPENDPATH += $$PWD/./libs/minIni/macx/release

   HEADERS += ./src/platform/macosx/macosxplatform.h

   OBJECTIVE_SOURCES += ./src/platform/macosx/macosxplatform.mm

} else:linux {
    LIBS       += -lquazip -lminIni
} else:win32{
   LIBS       += -L$$PWD/./libs/quazip/msvc_64/release/ -lquazip
   LIBS       += -L$$PWD/./libs/minIni/msvc_64/release/ -lminIni
   DEPENDPATH += $$PWD/./libs/quazip/msvc_64/release
   DEPENDPATH += $$PWD/./libs/minIni/msvc_64/release

}else {
   message( "UNDEFINED BUILD ENVIRONMENT" )
}

INCLUDEPATH += ./src \
               ./libs/quazip/includes \
               ./libs/minIni/includes

INCLUDEPATH += $$[QT_INSTALL_HEADERS]/QtZlib

SOURCES +=./src/main.cpp\
    ./src/treeitem.cpp \
    ./src/stylesheethelper.cpp \
    ./src/beatspanel/beatspanel.cpp \
    ./src/beatspanel/songfolderview.cpp \
    ./src/beatspanel/songwidget.cpp \
    ./src/beatspanel/songpartwidget.cpp \
    ./src/beatspanel/songfolderviewitem.cpp \
    ./src/beatspanel/movehandlewidget.cpp \
    ./src/beatspanel/deletehandlewidget.cpp \
    ./src/beatspanel/partcolumnwidget.cpp \
    ./src/beatspanel/beatfilewidget.cpp \
    ./src/beatspanel/songtitlewidget.cpp \
    ./src/beatspanel/newsongwidget.cpp \
    ./src/beatspanel/newpartwidget.cpp \
    ./src/mainwindow.cpp \
    ./src/playbackpanel.cpp \
    ./src/pexpanel/projectexplorerpanel.cpp \
    ./src/model/filegraph/songfilemodel.cpp \
    ./src/model/filegraph/abstractfilepartmodel.cpp \
    ./src/model/filegraph/fileheadermodel.cpp \
    ./src/crc32.cpp \
    ./src/model/filegraph/fileoffsettablemodel.cpp \
    ./src/model/filegraph/songtracksmodel.cpp \
    ./src/model/filegraph/songtrackmetaitem.cpp \
    ./src/model/filegraph/songtrackdataitem.cpp \
    ./src/model/filegraph/songtrackindexitem.cpp \
    ./src/model/filegraph/songtrack.cpp \
    ./src/model/filegraph/filepartcollection.cpp \
    ./src/model/filegraph/songpartmodel.cpp \
    ./src/model/filegraph/songmodel.cpp \
    ./src/model/filegraph/songfilemeta.cpp \
    ./src/model/tree/abstracttreeitem.cpp \
    ./src/model/tree/project/beatsprojectmodel.cpp \
    ./src/model/tree/song/trackarrayitem.cpp \
    ./src/model/tree/song/trackptritem.cpp \
    ./src/model/tree/song/filepartitem.cpp \
    ./src/model/tree/song/songfileitem.cpp \
    ./src/model/tree/song/songpartitem.cpp \
    ./src/model/tree/song/songfoldertreeitem.cpp \
    ./src/model/tree/project/foldertreeitem.cpp \
    ./src/model/tree/project/contentfoldertreeitem.cpp \
    ./src/model/tree/project/csvconfigfile.cpp \
    ./src/model/filegraph/trackindexcollection.cpp \
    ./src/model/filegraph/trackmetacollection.cpp \
    ./src/model/filegraph/trackdatacollection.cpp \
    ./src/model/tree/project/effectfoldertreeitem.cpp \
    ./src/model/tree/project/effectfileitem.cpp \
    ./src/model/tree/song/effectptritem.cpp \
    ./src/model/filegraph/midiparser.cpp \
    ./src/model/tree/project/drmfoldertreeitem.cpp \
    ./src/model/tree/project/drmfileitem.cpp \
    ./src/utils/dirlistallsubfilesmodal.cpp \
    ./src/utils/dircleanupmodal.cpp \
    ./src/utils/dircopymodal.cpp \
    ./src/utils/wavfile.cpp \
    ./src/utils/utils.cpp \
    ./src/workspace/workspace.cpp \
    ./src/workspace/contentlibrary.cpp \
    ./src/workspace/libcontent.cpp \
    ./src/utils/filecompare.cpp \
    ./src/player/player.cpp \
    ./src/player/soundManager.c \
    ./src/player/mixer.c \
    ./src/player/songPlayer.cpp \
    ./src/model/tree/project/paramsfoldertreemodel.cpp \
    ./src/workspace/settings.cpp \
    ./src/model/tree/project/songsfoldertreeitem.cpp \
    ./src/drmmaker/DrumSetExtractor.cpp \
    ./src/drmmaker/DrumSetMaker.cpp \
    ./src/drmmaker/UI_Elements/DropLineEdit.cpp \
    ./src/drmmaker/UI_Elements/Instrument.cpp \
    ./src/drmmaker/UI_Elements/velocity.cpp \
    ./src/drmmaker/UI_Elements/Xbutton.cpp \
    ./src/drmmaker/UI_Elements/Dialogs/DrumsetNameDialog.cpp \
    ./src/drmmaker/UI_Elements/Dialogs/instrumentconfigdialog.cpp \
    ./src/drmmaker/UI_Elements/Labels/ClickableLabel.cpp \
    ./src/drmmaker/UI_Elements/Labels/DrumsetLabel.cpp \
    ./src/drmmaker/UI_Elements/Labels/InstrumentLabel.cpp \
    ./src/drmmaker/DrumsetPanel.cpp \
    ./src/drmmaker/Model/drmmakermodel.cpp \
    ./src/pexpanel/drmlistmodel.cpp \
    ./src/model/tree/project/songfolderproxymodel.cpp \
    ./src/platform/platform.cpp \
    ./src/utils/webdownloadmanager.cpp \
    ./src/drmmaker/UI_Elements/drumsetsizeprogressbar.cpp \
    ./src/dialogs/aboutdialog.cpp \
    ./src/utils/extractzipmodal.cpp \
    ./src/utils/compresszipmodal.cpp \
    ./src/drmmaker/Utils/myqsound.cpp \
    ./src/newprojectdialog.cpp \
    ./src/model/selectionlinker.cpp \
    ./src/vm/vmscreen.cpp \
    ./src/vm/virtualmachinepanel.cpp \
    ./src/dialogs/optionsdialog.cpp \
    ./src/bbmanagerapplication.cpp \
    ./src/utils/midifilewriter.cpp \
    ./src/utils/filedownloader.cpp \
    ./src/dialogs/supportdialog.cpp \
    ./src/model/index.cpp \
    ./src/dialogs/loopCountDialog.cpp \
    ./src/dialogs/nameAndIdDialog.cpp \
    ./src/dialogs/autopilotsettingsdialog.cpp \
    ./src/model/filegraph/autopilotdatapartmodel.cpp \
    ./src/model/filegraph/autopilotdatamodel.cpp \
    ./src/model/filegraph/autopilotdatafillmodel.cpp \
    ./src/model/filegraph/autopilot.cpp \
    ./src/debug.cpp \
    ./src/versioninfo.cpp \
    src/update/update.cpp \
    src/update/updatedialog.cpp \
    src/midi_editor/editorvelocitycolorizer.cpp \
    src/midi_editor/editor.cpp \
    src/midi_editor/editordata.cpp \
    src/midi_editor/editorcelldelegate.cpp \
    src/midi_editor/editorverticalheader.cpp \
    src/midi_editor/editorhorizontalheader.cpp

HEADERS  += \
    ./src/treeitem.h \
    ./src/stylesheethelper.h \
    ./src/beatspanel/beatspanel.h \
    ./src/beatspanel/songfolderview.h \
    ./src/beatspanel/songwidget.h \
    ./src/beatspanel/songpartwidget.h \
    ./src/beatspanel/songfolderviewitem.h \
    ./src/beatspanel/movehandlewidget.h \
    ./src/beatspanel/deletehandlewidget.h \
    ./src/beatspanel/partcolumnwidget.h \
    ./src/beatspanel/beatfilewidget.h \
    ./src/beatspanel/songtitlewidget.h \
    ./src/beatspanel/newsongwidget.h \
    ./src/beatspanel/newpartwidget.h \
    ./src/copypastable.h \
    ./src/mainwindow.h \
    ./src/playbackpanel.h \
    ./src/pexpanel/projectexplorerpanel.h \
    ./src/model/filegraph/songfilemodel.h \
    ./src/model/filegraph/songfile.h \
    ./src/model/filegraph/song.h \
    ./src/model/filegraph/abstractfilepartmodel.h \
    ./src/model/filegraph/fileheadermodel.h \
    ./src/crc32.h \
    ./src/model/filegraph/fileoffsettablemodel.h \
    ./src/model/filegraph/songtracksmodel.h \
    ./src/model/filegraph/songtrackmetaitem.h \
    ./src/model/filegraph/songtrackdataitem.h \
    ./src/model/filegraph/songtrackindexitem.h \
    ./src/model/filegraph/songtrack.h \
    ./src/model/filegraph/filepartcollection.h \
    ./src/model/filegraph/songpartmodel.h \
    ./src/model/filegraph/songmodel.h \
    ./src/model/filegraph/songfilemeta.h \
    ./src/model/tree/abstracttreeitem.h \
    ./src/model/tree/project/beatsprojectmodel.h \
    ./src/model/tree/song/trackarrayitem.h \
    ./src/model/tree/song/trackptritem.h \
    ./src/model/tree/song/filepartitem.h \
    ./src/model/tree/song/songfileitem.h \
    ./src/model/tree/song/songpartitem.h \
    ./src/model/tree/song/songfoldertreeitem.h \
    ./src/model/tree/project/foldertreeitem.h \
    ./src/model/tree/project/contentfoldertreeitem.h \
    ./src/model/tree/project/csvconfigfile.h \
    ./src/model/filegraph/trackindexcollection.h \
    ./src/model/filegraph/trackmetacollection.h \
    ./src/model/filegraph/trackdatacollection.h \
    ./src/model/tree/project/effectfoldertreeitem.h \
    ./src/model/tree/project/effectfileitem.h \
    ./src/model/tree/song/effectptritem.h\
    ./src/model/filegraph/midiparser.h \
    ./src/model/tree/project/drmfoldertreeitem.h \
    ./src/model/tree/project/drmfileitem.h \
    ./src/utils/dirlistallsubfilesmodal.h \
    ./src/utils/dircleanupmodal.h \
    ./src/utils/dircopymodal.h \
    ./src/utils/wavfile.h \
    ./src/utils/utils.h \
    ./src/workspace/workspace.h \
    ./src/workspace/contentlibrary.h \
    ./src/workspace/libcontent.h \
    ./src/utils/filecompare.h \
    ./src/player/settings.h \
    ./src/player/player.h \
    ./src/player/mixer.h \
    ./src/player/songPlayer.h \
    ./src/player/button.h \
    ./src/player/soundManager.h \
    ./src/model/tree/project/paramsfoldertreemodel.h \
    ./src/workspace/settings.h \
    ./src/model/tree/project/songsfoldertreeitem.h \
    ./src/drmmaker/DrumSetExtractor.h \
    ./src/drmmaker/DrumSetMaker.h \
    ./src/drmmaker/UI_Elements/DropLineEdit.h \
    ./src/drmmaker/UI_Elements/Instrument.h \
    ./src/drmmaker/UI_Elements/velocity.h \
    ./src/drmmaker/UI_Elements/Xbutton.h \
    ./src/drmmaker/UI_Elements/Dialogs/DrumsetNameDialog.h \
    ./src/drmmaker/UI_Elements/Dialogs/instrumentconfigdialog.h \
    ./src/drmmaker/UI_Elements/Labels/ClickableLabel.h \
    ./src/drmmaker/UI_Elements/Labels/DrumsetLabel.h \
    ./src/drmmaker/UI_Elements/Labels/InstrumentLabel.h \
    ./src/drmmaker/Utils/Common.h \
    ./src/drmmaker/DrumsetPanel.h \
    ./src/drmmaker/Model/drmmakermodel.h \
    ./src/pexpanel/drmlistmodel.h \
    ./src/model/tree/project/songfolderproxymodel.h \
    ./src/platform/platform.h \
    ./src/utils/webdownloadmanager.h \
    ./src/drmmaker/UI_Elements/drumsetsizeprogressbar.h \
    ./src/model/tree/project/pedalsettingsdefinitions.h \
    ./src/version.h \
    ./src/version_constants.h \
    ./src/dialogs/aboutdialog.h \
    ./src/utils/extractzipmodal.h \
    ./src/utils/compresszipmodal.h \
    ./src/model/beatsmodelfiles.h \
    ./src/drmmaker/Utils/myqsound.h \
    ./src/newprojectdialog.h \
    ./src/model/selectionlinker.h \
    ./src/vm/vmscreen.h \
    ./src/vm/virtualmachinepanel.h \
    ./src/model/tree/song/portablesongfile.h \
    ./src/pragmapack.h \
    ./src/dialogs/optionsdialog.h \
    ./src/bbmanagerapplication.h \
    ./src/model/filegraph/trackfile.h \
    ./src/utils/filedownloader.h \
    ./src/utils/midifilewriter.h \
    ./src/dialogs/supportdialog.h \
    ./src/model/index.h \
    ./src/dialogs/loopCountDialog.h \
    ./src/dialogs/autopilotsettingsdialog.h \
    ./src/model/filegraph/autopilotdatapartmodel.h \
    ./src/model/filegraph/autopilotdatamodel.h \
    ./src/model/filegraph/autopilotdatafillmodel.h \
    ./src/model/filegraph/autopilot.h \
    ./src/model/filegraph/midiParser.h \
    ./src/dialogs/nameAndIdDialog.h \
    ./src/debug.h \
    ./src/versioninfo.h \
    src/update/update.h \
    src/update/updatedialog.h \
    src/midi_editor/editorvelocitycolorizer.h \
    src/midi_editor/editor.h \
    src/midi_editor/editorhorizontalheader.h \
    src/midi_editor/editordata.h \
    src/midi_editor/editorcolumnscolorizer.h \
    src/midi_editor/util.h \
    src/midi_editor/editorverticalheader.h \
    src/midi_editor/editorcelldelegate.h \
    src/midi_editor/editordialog.h \
    src/midi_editor/undoable_commands.h

FORMS += \
    ./src/newprojectdialog.ui \
    ./src/dialogs/aboutdialog.ui \
    ./src/dialogs/autopilotsettingsdialog.ui \
    ./src/dialogs/optionsdialog.ui \
    ./src/dialogs/supportdialog.ui \
    ./src/dialogs/updatebbmdialog.ui \
    src/update/updatedialog.ui

RESOURCES += \
    ./res/resources.qrc

DEFINES+= MININI_ANSI
DEFINES+= QT_MESSAGELOGCONTEXT
