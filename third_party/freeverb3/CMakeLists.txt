cmake_minimum_required(VERSION 3.15)
project(freeverb3 LANGUAGES CXX)

file(GLOB FREEVERB3_ALL "${CMAKE_CURRENT_SOURCE_DIR}/freeverb/*.cpp")

# Exclude files that depend on FFTW3 (not available/needed)
set(FREEVERB3_EXCLUDE
    frag.cpp irbase.cpp irmodel1.cpp irmodel2.cpp irmodel2zl.cpp
    irmodel3.cpp irmodel3p.cpp irmodel3w.cpp irmodels.cpp fir3bandsplit.cpp
    blockDelay.cpp revmodel.cpp)

set(FREEVERB3_SOURCES "")
foreach(src ${FREEVERB3_ALL})
    get_filename_component(fname "${src}" NAME)
    if(NOT fname IN_LIST FREEVERB3_EXCLUDE)
        list(APPEND FREEVERB3_SOURCES "${src}")
    endif()
endforeach()

add_library(freeverb3 STATIC ${FREEVERB3_SOURCES})
target_include_directories(freeverb3 PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_compile_definitions(freeverb3 PRIVATE LIBFV3_FLOAT)
set_target_properties(freeverb3 PROPERTIES CXX_STANDARD 14 POSITION_INDEPENDENT_CODE ON)
target_compile_options(freeverb3 PRIVATE -w)
