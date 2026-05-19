cmake_minimum_required(VERSION 3.10)
project(bb VERSION 1.3.0 LANGUAGES C)

include(CheckIncludeFile)
include(CheckFunctionExists)
include(CheckTypeSize)
include(CheckSymbolExists)
include(GNUInstallDirs)

option(SANITIZE_ADDRESS "Enable address sanitizer" OFF)

if(SANITIZE_ADDRESS)
    message(STATUS "Enabling address sanitizer")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=address -g")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fsanitize=address")
endif()

# ── Feature detection ────────────────────────────────────────────────

# Headers
check_include_file(fcntl.h      HAVE_FCNTL_H)
check_include_file(limits.h     HAVE_LIMITS_H)
check_include_file(malloc.h     HAVE_MALLOC_H)
check_include_file(sys/ioctl.h  HAVE_SYS_IOCTL_H)
check_include_file(sys/time.h   HAVE_SYS_TIME_H)
check_include_file(unistd.h     HAVE_UNISTD_H)
check_include_file(inttypes.h   HAVE_INTTYPES_H)
check_include_file(stdint.h     HAVE_STDINT_H)
check_include_file(stdio.h      HAVE_STDIO_H)
check_include_file(stdlib.h     HAVE_STDLIB_H)
check_include_file(strings.h    HAVE_STRINGS_H)
check_include_file(string.h     HAVE_STRING_H)
check_include_file(sys/stat.h   HAVE_SYS_STAT_H)
check_include_file(sys/types.h  HAVE_SYS_TYPES_H)

# Functions
check_function_exists(ftime        HAVE_FTIME)
check_function_exists(gettimeofday HAVE_GETTIMEOFDAY)
check_function_exists(select       HAVE_SELECT)
check_function_exists(strdup       HAVE_STRDUP)
check_function_exists(strstr       HAVE_STRSTR)
check_function_exists(strtol       HAVE_STRTOL)
check_function_exists(alloca       HAVE_ALLOCA)

# alloca.h
check_include_file(alloca.h HAVE_ALLOCA_H)
if(HAVE_ALLOCA_H)
    set(HAVE_ALLOCA 1)
endif()

# Signal return type
include(CheckTypeSize)
check_symbol_exists(signal "signal.h" HAVE_SIGNAL)
if(HAVE_SIGNAL)
    set(RETSIGTYPE void)
else()
    set(RETSIGTYPE int)
endif()

# Time headers
check_symbol_exists(time "time.h" HAVE_TIME_H)
if(HAVE_SYS_TIME_H AND HAVE_TIME_H)
    set(TIME_WITH_SYS_TIME 1)
endif()

# Standard C headers
if(HAVE_STDLIB_H AND HAVE_STDARG_H AND HAVE_STRING_H AND HAVE_FLOAT_H)
    set(STDC_HEADERS 1)
endif()

# ── Required: AA-lib ─────────────────────────────────────────────────

find_path(AALIB_INCLUDE_DIR aalib.h)
find_library(AALIB_LIBRARY NAMES aa)

if(NOT AALIB_INCLUDE_DIR OR NOT AALIB_LIBRARY)
    message(FATAL_ERROR "AA-lib (aalib) >= 1.4.0 not found. Please install libaa-dev.")
endif()

message(STATUS "Found AA-lib: ${AALIB_LIBRARY}")

# ── Optional: libmikmod ─────────────────────────────────────────────

find_path(LIBMIKMOD_INCLUDE_DIR mikmod.h)
find_library(LIBMIKMOD_LIBRARY NAMES mikmod)

if(LIBMIKMOD_INCLUDE_DIR AND LIBMIKMOD_LIBRARY)
    set(HAVE_LIBMIKMOD 1)
    set(SOUNDFILES "bb.s3m;bb2.s3m;bb3.s3m")
    message(STATUS "Found libmikmod: ${LIBMIKMOD_LIBRARY} (sound support enabled)")
else()
    set(HAVE_LIBMIKMOD 0)
    set(SOUNDFILES "")
    message(STATUS "libmikmod not found - sound support disabled")
endif()

# ── Generate config header ───────────────────────────────────────────

configure_file(aconfig.h.cmake aconfig.h)

# ── Build target ─────────────────────────────────────────────────────

set(BB_SOURCES
    # core
    src/core/main.c
    src/core/bb.c
    src/core/image.c
    src/core/timers.c
    src/core/backconv.c
    src/core/credits.c
    src/core/credits2.c
    src/core/messager.c
    src/core/print.c
    src/core/textform.c
    src/core/uncompfn.c
    src/core/scene1.c
    src/core/scene2.c
    src/core/scene3.c
    src/core/scene4.c
    src/core/scene5.c
    src/core/scene7.c
    src/core/scene8.c
    src/core/scene9.c
    # zoom (fractal engine from XaoS)
    src/zoom/zoom.c
    src/zoom/formulas.c
    src/zoom/julia.c
    src/zoom/autopilo.c
    src/zoom/ctrl87.c
    # effects (compressed image data)
    src/effects/fk1.c   src/effects/fk2.c   src/effects/fk3.c   src/effects/fk4.c
    src/effects/hh1.c   src/effects/hh2.c   src/effects/hh3.c   src/effects/hh4.c
    src/effects/kt1.c   src/effects/kt2.c   src/effects/kt3.c   src/effects/kt4.c
    src/effects/ms1.c   src/effects/ms2.c   src/effects/ms3.c   src/effects/ms4.c
    src/effects/zeb.c
    # tex (3D object rendering)
    src/tex/tex.c
    # lzo (MiniLZO compression)
    src/lzo/minilzo.c
)

add_executable(bb ${BB_SOURCES})

target_include_directories(bb PRIVATE
    ${CMAKE_CURRENT_BINARY_DIR}   # for generated aconfig.h
    src/core                      # for config.h, bb.h, image.h, timers.h
    src/zoom                      # for zoom.h, formulas.h, complex.h, autopilo.h, ui.h, ctrl87.h
    src/tex                       # for tex.h
    src/lzo                       # for minilzo.h
    ${AALIB_INCLUDE_DIR}
)

target_link_libraries(bb PRIVATE
    ${AALIB_LIBRARY}
    m
)

if(HAVE_LIBMIKMOD)
    target_include_directories(bb PRIVATE ${LIBMIKMOD_INCLUDE_DIR})
    target_link_libraries(bb PRIVATE ${LIBMIKMOD_LIBRARY})
endif()

target_compile_definitions(bb PRIVATE
    SOUNDDIR="${CMAKE_INSTALL_FULL_DATADIR}/bb"
)

# ── Install ──────────────────────────────────────────────────────────

install(TARGETS bb RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

install(FILES bb.1 DESTINATION ${CMAKE_INSTALL_MANDIR}/man1)

if(SOUNDFILES)
    install(FILES ${SOUNDFILES} DESTINATION ${CMAKE_INSTALL_DATADIR}/bb)
endif()
