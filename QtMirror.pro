#-------------------------------------------------
#
# Project created by QtCreator 2012-04-15T15:00:05
#
#-------------------------------------------------

QT       += core gui widgets

TARGET = QtMirror
TEMPLATE = app

ICON = QtMirror.icns

SOURCES += main.cpp\
    CpuArch.c \
    LzFind.c \
    LzFindMt.c \
    LzFindOpt.c \
    LzmaDec_copier.cpp \
    LzmaEnc.c \
    Threads.c \
    dialogeditimagelist.cpp \
    encoder7z.cpp \
    entetefichier.cpp \
        mainwindow.cpp \
    dialogeditcommand.cpp \
    scanner.cpp \
    dialoglistimage.cpp \
    dialogviewlog.cpp \
    reverttorestore.cpp \
    conversions.cpp \
    copyfile.cpp \
    vieweditstudy.cpp

HEADERS  += mainwindow.h \
    7z.h \
    7zTypes.h \
    Compiler.h \
    CpuArch.h \
    LzFind.h \
    LzFindMt.h \
    LzHash.h \
    LzmaEnc.h \
    QtMirror.h \
    Threads.h \
    crc32.h \
    defs.h \
    dialogeditcommand.h \
    dialogeditimagelist.h \
    encoder7z.h \
    entetefichier.h \
    fichier7z.h \
    scanner.h \
    dialoglistimage.h \
    dialogviewlog.h \
    reverttorestore.h \
    conversions.h \
    copyfile.h \
    vieweditstudy.h

FORMS    += mainwindow.ui \
    dialogeditcommand.ui \
    dialogeditimagelist.ui \
    dialoglistimage.ui \
    dialogviewlog.ui \
    vieweditstudy.ui

RC_FILE = qtmirror.rc

