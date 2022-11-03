#-------------------------------------------------
#
# Project created by QtCreator 2012-12-24T22:18:46
#
#-------------------------------------------------

CONFIG -= qt

TARGET = midi_processing
TEMPLATE = lib
CONFIG += staticlib

QMAKE_CXXFLAGS += -std=c++0x
macx:QMAKE_CXXFLAGS += -mmacosx-version-min=10.7 -stdlib=libc++

SOURCES += \
    midi_processor_xmi.cpp \
    midi_processor_syx.cpp \
    midi_processor_standard_midi.cpp \
    midi_processor_riff_midi.cpp \
    midi_processor_mus.cpp \
    midi_processor_mids.cpp \
    midi_processor_lds.cpp \
    midi_processor_hmp.cpp \
    midi_processor_hmi.cpp \
    midi_processor_helpers.cpp \
    midi_processor_gmf.cpp \
    midi_container.cpp

HEADERS += \
    midi_processor.h \
    midi_container.h
unix:!symbian {
    maemo5 {
        target.path = /opt/usr/lib
    } else {
        target.path = /usr/lib
    }
    INSTALLS += target
}
