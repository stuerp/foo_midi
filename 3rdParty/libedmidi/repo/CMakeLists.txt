cmake_minimum_required (VERSION 3.2...3.10)
project(libEmuDeMIDI VERSION 1.0.0 LANGUAGES C CXX)

include(GNUInstallDirs)
include(CheckCCompilerFlag)

set(CMAKE_C_STANDARD 90)
set(CMAKE_CXX_STANDARD 98)

if(NOT CMAKE_RUNTIME_OUTPUT_DIRECTORY)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
endif()
if(NOT CMAKE_LIBRARY_OUTPUT_DIRECTORY)
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
endif()
if(NOT CMAKE_ARCHIVE_OUTPUT_DIRECTORY)
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
endif()

#===========================================================================================

# Disable bogus MSVC warnings
if(MSVC)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
endif()

if(VITA)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DVITA -DVITA=1 -fcompare-debug-second")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DVITA -DVITA=1 -fcompare-debug-second")
endif()
if(NINTENDO_3DS OR NINTENDO_WII OR NINTENDO_WIIU OR NINTENDO_SWITCH)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fcompare-debug-second")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fcompare-debug-second")
endif()

if(CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX OR CMAKE_C_COMPILER_ID STREQUAL "Clang")
    if(NINTENDO_3DS OR NINTENDO_WII OR NINTENDO_WIIU OR NINTENDO_SWITCH)
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=gnu90")
    else()
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c89")
    endif()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++98")
endif()

if(OPENBSD_LOCALBASE)
    message("OpenBSD Detected!")
    include_directories(${OPENBSD_LOCALBASE}/include)
endif()

if(NOT WIN32 AND CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX)
    check_c_compiler_flag("-no-pie" HAS_NO_PIE)
endif()

function(set_nopie _target)
    if(NOT WIN32 AND HAS_NO_PIE AND CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX)
        set_property(TARGET ${_target} APPEND_STRING PROPERTY LINK_FLAGS " -no-pie")
    endif()
endfunction()

function(set_legacy_standard destTarget)
    if(CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX)
        # Turn on warnings and legacy C/C++ standards to support more compilers
        target_compile_options(${destTarget} PRIVATE
            $<$<COMPILE_LANGUAGE:C>:-Wall -pedantic -std=gnu90 -Wno-long-long>
            $<$<COMPILE_LANGUAGE:CXX>:-Wall -pedantic -std=gnu++98>
        )
    endif()
endfunction()

function(set_visibility_hidden destTarget)
    if(CMAKE_C_COMPILER_ID MATCHES "^(GNU|Clang)$" AND NOT DJGPP)
        if(CMAKE_VERSION VERSION_GREATER 3.2)
            target_compile_options(${destTarget} PRIVATE
              $<$<COMPILE_LANGUAGE:CXX>:-fvisibility-inlines-hidden> -fvisibility=hidden)
        else()
            target_compile_options(${destTarget} PRIVATE -fvisibility=hidden)
        endif()
        target_compile_definitions(${destTarget} PRIVATE LIBEDMIDI_VISIBILITY=)
    endif()
endfunction()
#===========================================================================================


if(NOT EMSCRIPTEN
   AND NOT VITA
   AND NOT NINTENDO_3DS
   AND NOT NINTENDO_WII
   AND NOT NINTENDO_WIIU
   AND NOT NINTENDO_SWITCH
   AND NOT EDMIDI_DOS)
    option(libEDMIDI_STATIC   "Build static library of libEDMIDI" ON)
    option(libEDMIDI_SHARED   "Build shared library of libEDMIDI" OFF)
else()
    set(libEDMIDI_STATIC ON)
    set(libEDMIDI_SHARED OFF)
endif()

option(WITH_MIDIPLAY          "Build also demo MIDI player" OFF)

if(NOT WIN32
   AND NOT VITA
   AND NOT NINTENDO_3DS
   AND NOT NINTENDO_WII
   AND NOT NINTENDO_WIIU
   AND NOT EMSCRIPTEN
   AND NOT EDMIDI_DOS)
    option(ENABLE_FPIC "Build static libraries as position independent code" ON)
else()
    set(ENABLE_FPIC OFF)
endif()

option(ENABLE_ADDRESS_SANITIZER "Enable the Address Sanitizer GCC feature" OFF)

set(EMIDI_SRC
    src/CEnvelope.cpp
    src/device/emu2413.c
    src/device/emu2212.c
    src/device/emu2149.c
    src/CMIDIModule.cpp
    src/CSccDevice.cpp
    src/CPSGDrum.cpp
    src/COpllDevice.cpp
    src/CSMFPlay.cpp
    src/CMIDISequencer.cpp
    src/emu_de_midi.cpp
)

# Make headers being properly visible at IDEs
file(GLOB EDMIDI_HEADERS
    ${libEmuDeMIDI_SOURCE_DIR}/include/*.h
    ${libEmuDeMIDI_SOURCE_DIR}/src/*.h
    ${libEmuDeMIDI_SOURCE_DIR}/src/*.hpp
    ${libEmuDeMIDI_SOURCE_DIR}/src/device/*.h
    ${libEmuDeMIDI_SOURCE_DIR}/src/sequencer/*.hpp
    ${libEmuDeMIDI_SOURCE_DIR}/src/sequencer/*.h
    ${libEmuDeMIDI_SOURCE_DIR}/src/structures/*.hpp
)
list(APPEND EMIDI_SRC ${EDMIDI_HEADERS})

# Static library
if(libEDMIDI_STATIC)
    add_library(EDMIDI_static STATIC ${EMIDI_SRC})
    if(WIN32 AND libEDMIDI_SHARED)
        set_target_properties(EDMIDI_static PROPERTIES OUTPUT_NAME EDMIDI-static)
    else()
        set_target_properties(EDMIDI_static PROPERTIES OUTPUT_NAME EDMIDI)
    endif()
    target_include_directories(EDMIDI_static PUBLIC ${libEmuDeMIDI_SOURCE_DIR}/include)
    set_legacy_standard(EDMIDI_static)
    set_visibility_hidden(EDMIDI_static)

    if(ENABLE_ADDRESS_SANITIZER)
        target_compile_options(EDMIDI_static PRIVATE -fsanitize=address)
    endif()

    # -fPIC thing
    if(ENABLE_FPIC)
        set_target_properties(EDMIDI_static PROPERTIES COMPILE_FLAGS "-fPIC")
    endif()
endif()

# === Shared library ====
if(libEDMIDI_SHARED)
    add_library(EDMIDI_shared SHARED ${EMIDI_SRC})
    set_target_properties(EDMIDI_shared PROPERTIES OUTPUT_NAME EDMIDI)
    set_target_properties(EDMIDI_shared PROPERTIES
        VERSION ${libEmuDeMIDI_VERSION}
        SOVERSION ${libEmuDeMIDI_VERSION_MAJOR}
    )
    target_include_directories(EDMIDI_shared PUBLIC ${libEmuDeMIDI_SOURCE_DIR}/include)
    set_legacy_standard(EDMIDI_shared)
    set_visibility_hidden(EDMIDI_shared)

    if(WIN32)
        target_compile_definitions(EDMIDI_shared PRIVATE EDMIDI_BUILD_DLL)
        if(CMAKE_COMPILER_IS_GNUCXX)
            set_property(TARGET EDMIDI_shared APPEND_STRING PROPERTY LINK_FLAGS " -static-libgcc -static-libstdc++")
            set_property(TARGET EDMIDI_shared APPEND_STRING PROPERTY LINK_FLAGS " -Wl,-Bstatic,--whole-archive -lpthread -Wl,-Bdynamic,--no-whole-archive")
        endif()
    endif()
    if(OPENBSD_LOCALBASE)
        set_property(TARGET EDMIDI_shared APPEND_STRING PROPERTY LINK_FLAGS " -lc")
    endif()

    if(ENABLE_ADDRESS_SANITIZER)
        target_compile_options(EDMIDI_shared PRIVATE -fsanitize=address)
        target_link_options(EDMIDI_shared PRIVATE -fsanitize=address)
    endif()

    set_target_properties(EDMIDI_shared PROPERTIES SOVERSION "1")
endif()

if(NOT libEDMIDI_STATIC AND NOT libEDMIDI_SHARED)
    message(FATAL_ERROR "Both static and shared libEDMIDI builds are disabled!
You must enable at least one of them!")
endif()

set(libEDMIDI_INSTALLS )
foreach(lib EDMIDI_static EDMIDI_shared)
    if(TARGET ${lib})
        list(APPEND libEDMIDI_INSTALLS ${lib})
    endif()
endforeach()

install(TARGETS ${libEDMIDI_INSTALLS}
        RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
        LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
        ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
        INCLUDES DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}")

install(FILES
        include/emu_de_midi.h
        DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}")

file(GLOB DOCTXT_FILES
    "${libADLMIDI_SOURCE_DIR}/LICENSE*.txt"
    README.md AUTHORS
)
install(FILES ${DOCTXT_FILES}
        DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/doc/libEDMIDI")

if(NOT ANDROID)
    configure_file(libEDMIDI.pc.in libEDMIDI.pc @ONLY)
    install(FILES "${CMAKE_CURRENT_BINARY_DIR}/libEDMIDI.pc"
            DESTINATION "${CMAKE_INSTALL_LIBDIR}/pkgconfig")
endif()


if(WITH_MIDIPLAY)
    find_package(SDL2 REQUIRED)
    add_library(ADLMIDIEXAMPLE_SDL2 INTERFACE)
    if(TARGET SDL2::SDL2)
        if(MINGW)
            target_link_libraries(ADLMIDIEXAMPLE_SDL2 INTERFACE mingw32 SDL2::SDL2main SDL2::SDL2)
        else()
            target_link_libraries(ADLMIDIEXAMPLE_SDL2 INTERFACE SDL2::SDL2)
        endif()
        target_include_directories(ADLMIDIEXAMPLE_SDL2 INTERFACE ${SDL2_INCLUDE_DIRS})
    else()
        string(STRIP ${SDL2_LIBRARIES} SDL2_LIBRARIES)
        target_include_directories(ADLMIDIEXAMPLE_SDL2 INTERFACE ${SDL2_INCLUDE_DIRS})
        target_link_libraries(ADLMIDIEXAMPLE_SDL2 INTERFACE ${SDL2_LIBRARIES})
    endif()

    if(libEDMIDI_SHARED)
        add_library(EDMIDI ALIAS EDMIDI_shared)
    else()
        add_library(EDMIDI ALIAS EDMIDI_static)
    endif()

    add_executable(edmidiplay utils/main_sdl.c)
    target_link_libraries(edmidiplay PRIVATE EDMIDI ADLMIDIEXAMPLE_SDL2)
    set_nopie(edmidiplay)

    if(NOT MSVC)
        target_link_libraries(edmidiplay PRIVATE pthread)
    endif()
    if(NOT WIN32)
        target_link_libraries(edmidiplay PRIVATE dl m)
    endif()

    if(ENABLE_ADDRESS_SANITIZER)
        target_compile_options(edmidiplay PRIVATE -fsanitize=address)
        target_link_options(edmidiplay PRIVATE -fsanitize=address)
    endif()

    if(TARGET EDMIDI_shared)
        set_target_properties(edmidiplay PROPERTIES INSTALL_RPATH "$ORIGIN/../lib")
    endif()

    install(TARGETS edmidiplay
            RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}")
endif()
