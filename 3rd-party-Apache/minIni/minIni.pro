#-------------------------------------------------
#
# Project created by QtCreator 2014-10-07T14:31:23
#
#-------------------------------------------------

TEMPLATE = lib
QT       -= core gui

TARGET = minIni
TEMPLATE = lib

DEFINES += MININI_LIBRARY
DEFINES += MININI_ANSI

SOURCES += \
    minIni.c

HEADERS +=\
    minGlue.h \
    minGlue-ccs.h \
    minGlue-efsl.h \
    minGlue-FatFs.h \
    minGlue-ffs.h \
    minGlue-mdd.h \
    minGlue-stdio.h \
    minIni.h \
    minini_global.h \
    wxMinIni.h


unix {
    target.path = /usr/lib
    INSTALLS += target
}
