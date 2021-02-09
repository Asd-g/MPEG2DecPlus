cmake_minimum_required(VERSION 3.16)

project(libd2vsource LANGUAGES CXX)

find_package (Git)
if (GIT_FOUND)
    execute_process (COMMAND ${GIT_EXECUTABLE} describe --tags --abbrev=0
        OUTPUT_VARIABLE ver
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
else ()
    message (STATUS "GIT not found")
endif ()

add_library(d2vsource SHARED
    src/AVISynthAPI.cpp
    src/color_convert.cpp
    src/getbit.cpp
    src/gethdr.cpp
    src/getpic.cpp
    src/global.cpp
    src/idct_ap922_sse2.cpp
    src/idct_llm_float_avx2.cpp
    src/idct_llm_float_sse2.cpp
    src/idct_ref_sse3.cpp
    src/mc.cpp
    src/misc.cpp
    src/MPEG2Decoder.cpp
    src/store.cpp
    src/yv12pict.cpp
)

if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "" FORCE)
endif()

message(STATUS "Build type - ${CMAKE_BUILD_TYPE}")

set_source_files_properties(src/idct_ap922_sse2.cpp PROPERTIES COMPILE_OPTIONS "-mfpmath=sse;-msse2")
set_source_files_properties(src/idct_llm_float_sse2.cpp PROPERTIES COMPILE_OPTIONS "-mfpmath=sse;-msse2")
set_source_files_properties(src/idct_ref_sse3.cpp PROPERTIES COMPILE_OPTIONS "-mssse3")
set_source_files_properties(src/idct_llm_float_avx2.cpp PROPERTIES COMPILE_OPTIONS "-mavx2;-mfma")

target_include_directories(d2vsource PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    /usr/local/include/avisynth
)

set_target_properties(d2vsource PROPERTIES OUTPUT_NAME "d2vsource.${ver}")

target_compile_features(d2vsource PRIVATE cxx_std_17)

include(GNUInstallDirs)

INSTALL(TARGETS d2vsource
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
