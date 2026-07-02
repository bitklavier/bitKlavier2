cmake_minimum_required(VERSION 3.25)
project(chuck LANGUAGES C CXX)

set(CHUCK_SRC "${CMAKE_CURRENT_SOURCE_DIR}/../chuck/src/core")

# Pre-generated parser (replaces chuck.tab.c + chuck.yy.c from the upstream bison/flex build)
set(CHUCK_C_SOURCES
    "${CHUCK_SRC}/chuck_yacc.c"
    "${CHUCK_SRC}/util_network.c"
    "${CHUCK_SRC}/util_raw.c"
    "${CHUCK_SRC}/util_xforms.c"
    "${CHUCK_SRC}/util_sndfile.c"
    # liblo (OSC) — pre-configured config.h already in lo/
    "${CHUCK_SRC}/lo/address.c"
    "${CHUCK_SRC}/lo/blob.c"
    "${CHUCK_SRC}/lo/bundle.c"
    "${CHUCK_SRC}/lo/message.c"
    "${CHUCK_SRC}/lo/method.c"
    "${CHUCK_SRC}/lo/pattern_match.c"
    "${CHUCK_SRC}/lo/send.c"
    "${CHUCK_SRC}/lo/server.c"
    "${CHUCK_SRC}/lo/server_thread.c"
    "${CHUCK_SRC}/lo/timetag.c"
)

set(CHUCK_CXX_SOURCES
    "${CHUCK_SRC}/chuck.cpp"
    "${CHUCK_SRC}/chuck_absyn.cpp"
    "${CHUCK_SRC}/chuck_carrier.cpp"
    "${CHUCK_SRC}/chuck_compile.cpp"
    "${CHUCK_SRC}/chuck_dl.cpp"
    "${CHUCK_SRC}/chuck_emit.cpp"
    "${CHUCK_SRC}/chuck_errmsg.cpp"
    "${CHUCK_SRC}/chuck_frame.cpp"
    "${CHUCK_SRC}/chuck_globals.cpp"
    "${CHUCK_SRC}/chuck_instr.cpp"
    "${CHUCK_SRC}/chuck_io.cpp"
    "${CHUCK_SRC}/chuck_lang.cpp"
    "${CHUCK_SRC}/chuck_oo.cpp"
    "${CHUCK_SRC}/chuck_otf.cpp"
    "${CHUCK_SRC}/chuck_parse.cpp"
    "${CHUCK_SRC}/chuck_scan.cpp"
    "${CHUCK_SRC}/chuck_shell.cpp"
    "${CHUCK_SRC}/chuck_stats.cpp"
    "${CHUCK_SRC}/chuck_symbol.cpp"
    "${CHUCK_SRC}/chuck_table.cpp"
    "${CHUCK_SRC}/chuck_type.cpp"
    "${CHUCK_SRC}/chuck_ugen.cpp"
    "${CHUCK_SRC}/chuck_vm.cpp"
    "${CHUCK_SRC}/hidio_sdl.cpp"
    "${CHUCK_SRC}/midiio_rtmidi.cpp"
    "${CHUCK_SRC}/rtmidi.cpp"
    "${CHUCK_SRC}/uana_extract.cpp"
    "${CHUCK_SRC}/uana_xform.cpp"
    "${CHUCK_SRC}/ugen_filter.cpp"
    "${CHUCK_SRC}/ugen_osc.cpp"
    "${CHUCK_SRC}/ugen_stk.cpp"
    "${CHUCK_SRC}/ugen_xxx.cpp"
    "${CHUCK_SRC}/ulib_ai.cpp"
    "${CHUCK_SRC}/ulib_doc.cpp"
    "${CHUCK_SRC}/ulib_machine.cpp"
    "${CHUCK_SRC}/ulib_math.cpp"
    "${CHUCK_SRC}/ulib_opsc.cpp"
    "${CHUCK_SRC}/ulib_std.cpp"
    "${CHUCK_SRC}/util_buffers.cpp"
    "${CHUCK_SRC}/util_console.cpp"
    "${CHUCK_SRC}/util_hid.cpp"
    "${CHUCK_SRC}/util_math.cpp"
    "${CHUCK_SRC}/util_opsc.cpp"
    "${CHUCK_SRC}/util_platforms.cpp"
    "${CHUCK_SRC}/util_serial.cpp"
    "${CHUCK_SRC}/util_string.cpp"
    "${CHUCK_SRC}/util_thread.cpp"
)

add_library(chuck STATIC ${CHUCK_C_SOURCES} ${CHUCK_CXX_SOURCES})

# chuck_yacc.c includes "chuck.tab.h" (the bison-generated header), but the repo
# ships only the pre-generated "chuck_yacc.h". Copy it into the build dir under
# the expected name so the include resolves without modifying upstream source.
configure_file(
    "${CHUCK_SRC}/chuck_yacc.h"
    "${CMAKE_CURRENT_BINARY_DIR}/chuck.tab.h"
    COPYONLY
)

target_include_directories(chuck
    PUBLIC
        "${CHUCK_SRC}"
        "${CHUCK_SRC}/lo"
    PRIVATE
        "${CMAKE_CURRENT_BINARY_DIR}"  # for the generated chuck.tab.h
)

# Silence all upstream warnings
target_compile_options(chuck PRIVATE -w)

set_target_properties(chuck PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    C_STANDARD 11
    POSITION_INDEPENDENT_CODE ON
)

if(APPLE)
    target_compile_definitions(chuck PUBLIC __MACOSX_CORE__)
    target_link_libraries(chuck PUBLIC
        "-framework CoreAudio"
        "-framework CoreMIDI"
        "-framework CoreFoundation"
        "-framework IOKit"
        "-framework AppKit"
        "-framework Foundation"
    )
elseif(UNIX)
    target_compile_definitions(chuck PUBLIC __LINUX_ALSA__)
elseif(WIN32)
    target_compile_definitions(chuck PUBLIC __WINDOWS_MM__)
    target_link_libraries(chuck PUBLIC winmm)
endif()
