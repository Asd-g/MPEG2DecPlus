cmake_minimum_required(VERSION 3.1)

set(ProjectName "D2VSource")

project(${ProjectName} LANGUAGES CXX)

string(TOLOWER "${ProjectName}" LibName)

find_package (Git)

if (GIT_FOUND)
    execute_process (COMMAND ${GIT_EXECUTABLE} describe --tags --abbrev=0
        OUTPUT_VARIABLE ver
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
else ()
    message (STATUS "GIT not found")
endif ()

find_path(AVS_FOUND avisynth.h HINTS /usr/include /usr/local/include PATH_SUFFIXES avisynth)
if (NOT AVS_FOUND)
    message(FATAL_ERROR "AviSynth+ not found.")
else()
    message(STATUS "AviSynth+ : ${AVS_FOUND}")
endif()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -fPIC -std=c++17 -I${AVS_FOUND} ")

# include_directories("include/")

file(GLOB SOURCES "src/*.cpp")

# special SSE2 option for source files with *_sse2.cpp pattern
file(GLOB_RECURSE SRCS_SSE2 "src/*_sse2.cpp")
set_source_files_properties(${SRCS_SSE2} PROPERTIES COMPILE_FLAGS " -msse2 ")

# special SSSE3 option for source files with *_sse3.cpp pattern
file(GLOB_RECURSE SRCS_SSSE3 "src/*_sse3.cpp")
set_source_files_properties(${SRCS_SSSE3} PROPERTIES COMPILE_FLAGS " -msse3 ")

# special AVX2 option for source files with *_avx2.cpp pattern
file(GLOB_RECURSE SRCS_AVX2 "src/*_avx2.cpp")
set_source_files_properties(${SRCS_AVX2} PROPERTIES COMPILE_FLAGS " -mavx2 -mfma ")

add_library("${LibName}" SHARED ${SOURCES})
set_target_properties("${LibName}" PROPERTIES OUTPUT_NAME "${LibName}-${ver}")

include(GNUInstallDirs)

INSTALL(TARGETS ${LibName}
        LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}/avisynth")
        
# uninstall target
if(NOT TARGET uninstall)
  configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake_uninstall.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
    IMMEDIATE @ONLY)

  add_custom_target(uninstall
    COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake)
endif()
