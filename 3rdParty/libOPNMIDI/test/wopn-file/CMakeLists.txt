
set(CMAKE_CXX_STANDARD 11)

include_directories (${CMAKE_CURRENT_SOURCE_DIR}/../common
                     ${CMAKE_SOURCE_DIR}/include
                     ${CMAKE_SOURCE_DIR}/src)

add_executable(WopnFile
               wopn_file.cpp
               ${libOPNMIDI_SOURCE_DIR}/src/wopn/wopn_file.c
               $<TARGET_OBJECTS:Catch-objects>)

set_target_properties(WopnFile PROPERTIES COMPILE_DEFINITIONS "GSL_THROW_ON_CONTRACT_VIOLATION")
add_test(NAME WopnFileTest COMMAND WopnFile WORKING_DIRECTORY "${libOPNMIDI_SOURCE_DIR}")
