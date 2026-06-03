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
check_include_file(linux/fb.h   HAVE_LINUX_FB_H)
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

# ── Bundled AA-lib (stdout/stderr + optional fbdev backend) ───────────
#
# We compile a stripped-down copy of aalib from the in-tree aalib/ directory.
# The always-on backends are stdout/stderr display + stdin keyboard.
# On Linux with <linux/fb.h>, we also compile a native fbdev backend that
# uses only kernel ioctls + mmap (no extra user-space dependencies).
# Other backends (X11, curses, slang, Linux console, gpm mouse, DOS, OS/2)
# are intentionally excluded so that bb has no external aalib dependency.

set(AALIB_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/aalib/src)

set(AALIB_SOURCES
    # core
    ${AALIB_SRC_DIR}/aalib.c
    ${AALIB_SRC_DIR}/aaregist.c      # display driver registry (stdout + stderr)
    ${AALIB_SRC_DIR}/aakbdreg.c      # kbd driver registry (stdin)
    # display backends (only stdout / stderr)
    ${AALIB_SRC_DIR}/aastdout.c
    ${AALIB_SRC_DIR}/aafbdev.c
    # kbd backend
    ${AALIB_SRC_DIR}/aastdin.c
    # in-memory / save drivers
    ${AALIB_SRC_DIR}/aamem.c
    ${AALIB_SRC_DIR}/aasave.c
    # helpers / rendering core
    ${AALIB_SRC_DIR}/aaattrs.c
    ${AALIB_SRC_DIR}/aacurrfnt.c
    ${AALIB_SRC_DIR}/aaedit.c
    ${AALIB_SRC_DIR}/aafastre.c
    ${AALIB_SRC_DIR}/aaflush.c
    ${AALIB_SRC_DIR}/aafont.c
    ${AALIB_SRC_DIR}/aafonts.c
    ${AALIB_SRC_DIR}/aahelp.c
    ${AALIB_SRC_DIR}/aaimage.c
    ${AALIB_SRC_DIR}/aaimgheight.c
    ${AALIB_SRC_DIR}/aaimgwidth.c
    ${AALIB_SRC_DIR}/aain.c
    ${AALIB_SRC_DIR}/aammheight.c
    ${AALIB_SRC_DIR}/aammwidth.c
    ${AALIB_SRC_DIR}/aamktabl.c
    ${AALIB_SRC_DIR}/aaout.c
    ${AALIB_SRC_DIR}/aaparse.c
    ${AALIB_SRC_DIR}/aaprintf.c
    ${AALIB_SRC_DIR}/aaputpixel.c
    ${AALIB_SRC_DIR}/aarec.c
    ${AALIB_SRC_DIR}/aarecfunc.c
    ${AALIB_SRC_DIR}/aarender.c
    ${AALIB_SRC_DIR}/aascrheight.c
    ${AALIB_SRC_DIR}/aascrwidth.c
    ${AALIB_SRC_DIR}/aatext.c
    ${AALIB_SRC_DIR}/aavyhen.c
    # fonts
    ${AALIB_SRC_DIR}/font8.c
    ${AALIB_SRC_DIR}/font9.c
    ${AALIB_SRC_DIR}/font14.c
    ${AALIB_SRC_DIR}/font16.c
    ${AALIB_SRC_DIR}/fontcour.c
    ${AALIB_SRC_DIR}/fontgl.c
    ${AALIB_SRC_DIR}/fontline.c
    ${AALIB_SRC_DIR}/fontx13.c
    ${AALIB_SRC_DIR}/fontx13b.c
    ${AALIB_SRC_DIR}/fontx16.c
)

add_library(aa STATIC ${AALIB_SOURCES})
target_include_directories(aa PUBLIC ${AALIB_SRC_DIR})
# Silence warnings inside the legacy aalib code so they don't drown out ours.
# Newer gcc promotes a few legacy K&R-style issues to errors; downgrade them.
target_compile_options(aa PRIVATE
    -w
    -Wno-error=implicit-function-declaration
    -Wno-error=implicit-int
    -Wno-error=int-conversion
    -Wno-error=incompatible-pointer-types
)
if(CMAKE_SYSTEM_NAME STREQUAL "Linux" AND HAVE_LINUX_FB_H)
    target_compile_definitions(aa PRIVATE FBDEV_DRIVER)
endif()

set(AALIB_INCLUDE_DIR ${AALIB_SRC_DIR})
set(AALIB_LIBRARY aa)

if(CMAKE_SYSTEM_NAME STREQUAL "Linux" AND HAVE_LINUX_FB_H)
    message(STATUS "Using bundled AA-lib (stdout/stderr + fbdev backends)")
else()
    message(STATUS "Using bundled AA-lib (stdout/stderr backends only)")
endif()

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
    # src/effects/fk1.c   src/effects/fk2.c   src/effects/fk3.c   src/effects/fk4.c
    # src/effects/hh1.c   src/effects/hh2.c   src/effects/hh3.c   src/effects/hh4.c
    # src/effects/kt1.c   src/effects/kt2.c   src/effects/kt3.c   src/effects/kt4.c
    # src/effects/ms1.c   src/effects/ms2.c   src/effects/ms3.c   src/effects/ms4.c

    src/effects/clo1.c src/effects/clo2.c src/effects/clo3.c src/effects/clo4.c
    src/effects/kal1.c src/effects/kal2.c src/effects/kal3.c src/effects/kal4.c
    src/effects/doc1.c src/effects/doc2.c src/effects/doc3.c src/effects/doc4.c
    src/effects/ami1.c src/effects/ami2.c src/effects/ami3.c src/effects/ami4.c

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
