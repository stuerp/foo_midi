#-------------------------------------------------
#
# Project created by QtCreator 2012-12-26T16:54:29
#
#-------------------------------------------------

QT       -= core gui

TARGET = midisynth
TEMPLATE = lib
CONFIG += staticlib

QMAKE_CXXFLAGS += -std=c++0x
macx:QMAKE_CXXFLAGS += -mmacosx-version-min=10.7 -stdlib=libc++

SOURCES += \
    midisynth.cpp \
    fmmidi.cpp \
    oplmidi.cpp \
    iir_filter.cpp \
    adldata.cpp \
    lanczos_resampler.c \
    dbopl.cpp \
    reverb.cpp

HEADERS += midisynth.hpp \
    fmmidi.hpp \
    oplmidi.hpp \
    midisynth_common.hpp \
    iir_filter.hpp \
    dbopl.hpp \
    adldata.hpp \
    lanczos_resampler.h \
    reverb.hpp
unix:!symbian {
    maemo5 {
        target.path = /opt/usr/lib
    } else {
        target.path = /usr/lib
    }
    INSTALLS += target
}
