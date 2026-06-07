cmake_minimum_required(VERSION 3.10)
project(bb VERSION 1.3.0 LANGUAGES C)

include(CheckIncludeFile)
include(CheckFunctionExists)
include(CheckTypeSize)
include(CheckSymbolExists)
include(GNUInstallDirs)

option(SANITIZE_ADDRESS "Enable address sanitizer" OFF)
option(BB_DEBUG "Enable debug build (-g -O0, for gdb/lldb)" OFF)

if(BB_DEBUG)
    message(STATUS "Debug build enabled (-g -O0)")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -O0 -fno-omit-frame-pointer")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -g")
endif()

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

# ── Bundled AA-lib ────────────────────────────────────────────────────
#
# We compile a stripped-down copy of aalib from the in-tree aalib/ directory.
# stdout/stderr display + stdin keyboard are always enabled.
# Other display/kbd/mouse backends are controlled by the AALIB_BACKEND_*
# and AALIB_MOUSE_* options below.

# Display backends
option(AALIB_BACKEND_FBDEV  "Enable Linux fbdev display backend" ON)
option(AALIB_BACKEND_DRM    "Enable Linux DRM/KMS display backend (libdrm)" ON)
option(AALIB_BACKEND_X11    "Enable X11 display backend" ON)
option(AALIB_BACKEND_LINUX  "Enable Linux console display backend" ON)
option(AALIB_BACKEND_SLANG  "Enable Slang display backend" ON)
option(AALIB_BACKEND_CURSES "Enable Curses display backend" ON)

# Mouse backends (keyboard companions for X11/Slang/Curses/Linux follow
# their display backend; GPM is independent)
option(AALIB_MOUSE_GPM "Enable GPM mouse backend (Linux)" OFF)

# Music
option(BB_MUSIC "Enable music" OFF)
option(BB_UTF8 "Enable UTF-8 and FreeType text rendering" ON)

set(AALIB_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/aalib/src)

set(AALIB_SOURCES
    # core
    ${AALIB_SRC_DIR}/aalib.c
    ${AALIB_SRC_DIR}/aaregist.c
    ${AALIB_SRC_DIR}/aakbdreg.c
    ${AALIB_SRC_DIR}/aamoureg.c
    # always-on display + kbd
    ${AALIB_SRC_DIR}/aastdout.c
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
    ${AALIB_SRC_DIR}/aaglyph.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core/utf8.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core/ftfont.c
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

set(AALIB_DEFS "")
set(AALIB_LIBS "")
set(AALIB_ENABLED_BACKENDS "stdout stderr")

# ── fbdev ────────────────────────────────────────────────────────────
if(AALIB_BACKEND_FBDEV)
    if(CMAKE_SYSTEM_NAME STREQUAL "Linux" AND HAVE_LINUX_FB_H)
        list(APPEND AALIB_SOURCES ${AALIB_SRC_DIR}/aafbdev.c)
        list(APPEND AALIB_DEFS FBDEV_DRIVER)
        list(APPEND AALIB_ENABLED_BACKENDS fbdev)
    elseif(AALIB_BACKEND_FBDEV)
        message(WARNING "AALIB_BACKEND_FBDEV requested but linux/fb.h not found; disabled")
    endif()
endif()

# ── DRM/KMS (libdrm) ─────────────────────────────────────────────────
if(AALIB_BACKEND_DRM)
    find_package(PkgConfig QUIET)
    if(PKG_CONFIG_FOUND)
        pkg_check_modules(LIBDRM libdrm)
    endif()
    if(LIBDRM_FOUND)
        list(APPEND AALIB_SOURCES ${AALIB_SRC_DIR}/aadrm.c)
        list(APPEND AALIB_DEFS DRM_DRIVER)
        list(APPEND AALIB_LIBS ${LIBDRM_LIBRARIES})
        list(APPEND AALIB_ENABLED_BACKENDS drm)
    else()
        message(WARNING "AALIB_BACKEND_DRM requested but libdrm not found; disabled")
    endif()
endif()

# ── X11 ──────────────────────────────────────────────────────────────
if(AALIB_BACKEND_X11)
    find_package(X11)
    if(X11_FOUND)
        list(APPEND AALIB_SOURCES
            ${AALIB_SRC_DIR}/aax.c
            ${AALIB_SRC_DIR}/aaxkbd.c
            ${AALIB_SRC_DIR}/aaxmouse.c)
        list(APPEND AALIB_DEFS X11_DRIVER X11_KBDDRIVER X11_MOUSEDRIVER)
        list(APPEND AALIB_LIBS ${X11_LIBRARIES})
        list(APPEND AALIB_ENABLED_BACKENDS x11)
    else()
        message(WARNING "AALIB_BACKEND_X11 requested but X11 not found; disabled")
    endif()
endif()

# ── Linux console ────────────────────────────────────────────────────
if(AALIB_BACKEND_LINUX)
    if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
        list(APPEND AALIB_SOURCES
            ${AALIB_SRC_DIR}/aalinux.c
            ${AALIB_SRC_DIR}/aalinuxkbd.c)
        list(APPEND AALIB_DEFS LINUX_DRIVER LINUX_KBDDRIVER)
        list(APPEND AALIB_ENABLED_BACKENDS linux)
    else()
        message(WARNING "AALIB_BACKEND_LINUX requested but not on Linux; disabled")
    endif()
endif()

# ── Slang ────────────────────────────────────────────────────────────
if(AALIB_BACKEND_SLANG)
    find_library(SLANG_LIBRARY NAMES slang)
    find_path(SLANG_INCLUDE_DIR slang.h)
    if(SLANG_LIBRARY AND SLANG_INCLUDE_DIR)
        list(APPEND AALIB_SOURCES
            ${AALIB_SRC_DIR}/aaslang.c
            ${AALIB_SRC_DIR}/aaslnkbd.c)
        list(APPEND AALIB_DEFS SLANG_DRIVER SLANG_KBDDRIVER)
        list(APPEND AALIB_LIBS ${SLANG_LIBRARY})
        list(APPEND AALIB_ENABLED_BACKENDS slang)
    else()
        message(WARNING "AALIB_BACKEND_SLANG requested but libslang not found; disabled")
    endif()
endif()

# ── Curses ───────────────────────────────────────────────────────────
if(AALIB_BACKEND_CURSES)
    find_package(Curses)
    if(CURSES_FOUND)
        list(APPEND AALIB_SOURCES
            ${AALIB_SRC_DIR}/aacurses.c
            ${AALIB_SRC_DIR}/aacurkbd.c
            ${AALIB_SRC_DIR}/aacurmou.c)
        list(APPEND AALIB_DEFS CURSES_DRIVER CURSES_KBDDRIVER)
        if(CURSES_HAVE_MOUSE)
            list(APPEND AALIB_DEFS CURSES_MOUSEDRIVER)
        endif()
        if(CURSES_NCURSES_VERSION)
            list(APPEND AALIB_DEFS USE_NCURSES)
        else()
            list(APPEND AALIB_DEFS USE_CURSES)
        endif()
        list(APPEND AALIB_LIBS ${CURSES_LIBRARIES})
        list(APPEND AALIB_ENABLED_BACKENDS curses)
    else()
        message(WARNING "AALIB_BACKEND_CURSES requested but curses not found; disabled")
    endif()
endif()

# ── GPM mouse ────────────────────────────────────────────────────────
if(AALIB_MOUSE_GPM)
    find_library(GPM_LIBRARY NAMES gpm)
    if(GPM_LIBRARY AND CMAKE_SYSTEM_NAME STREQUAL "Linux")
        list(APPEND AALIB_SOURCES ${AALIB_SRC_DIR}/aagpm.c)
        list(APPEND AALIB_DEFS GPM_MOUSEDRIVER)
        list(APPEND AALIB_LIBS ${GPM_LIBRARY})
        list(APPEND AALIB_ENABLED_BACKENDS gpm)
    else()
        message(WARNING "AALIB_MOUSE_GPM requested but libgpm not found or not on Linux; disabled")
    endif()
endif()

if(BB_UTF8)
    find_package(Freetype)
    if(NOT Freetype_FOUND)
        find_package(PkgConfig QUIET)
        if(PKG_CONFIG_FOUND)
            pkg_check_modules(FREETYPE2 freetype2)
        endif()
    endif()
    if(Freetype_FOUND OR FREETYPE2_FOUND)
        set(HAVE_FTFONT 1)
        message(STATUS "FreeType found: UTF-8 glyph rendering enabled")
    else()
        message(WARNING "BB_UTF8 requested but FreeType not found; UTF-8 CJK disabled")
        set(HAVE_FTFONT 0)
    endif()
else()
    set(HAVE_FTFONT 0)
endif()

add_library(aa STATIC ${AALIB_SOURCES})
target_include_directories(aa PUBLIC ${AALIB_SRC_DIR})
target_include_directories(aa PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/core)
if(LIBDRM_FOUND)
    target_include_directories(aa PRIVATE ${LIBDRM_INCLUDE_DIRS})
endif()
if(SLANG_INCLUDE_DIR)
    target_include_directories(aa PRIVATE ${SLANG_INCLUDE_DIR})
endif()
if(X11_FOUND)
    target_include_directories(aa PRIVATE ${X11_INCLUDE_DIR})
endif()
if(CURSES_FOUND)
    target_include_directories(aa PRIVATE ${CURSES_INCLUDE_DIR})
endif()
# Silence warnings inside the legacy aalib code so they don't drown out ours.
# Newer gcc promotes a few legacy K&R-style issues to errors; downgrade them.
target_compile_options(aa PRIVATE
    -w
    -Wno-error=implicit-function-declaration
    -Wno-error=implicit-int
    -Wno-error=int-conversion
    -Wno-error=incompatible-pointer-types
    -Wno-error=return-mismatch
)
if(AALIB_DEFS)
    target_compile_definitions(aa PRIVATE ${AALIB_DEFS})
endif()
if(AALIB_LIBS)
    target_link_libraries(aa PUBLIC ${AALIB_LIBS})
endif()
if(HAVE_FTFONT)
    configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/WQY_bitmap_12px.ttf"
        "${CMAKE_CURRENT_BINARY_DIR}/WQY_bitmap_12px.ttf"
        COPYONLY
    )
    configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/WQY_bitmap_16px.ttf"
        "${CMAKE_CURRENT_BINARY_DIR}/WQY_bitmap_16px.ttf"
        COPYONLY
    )
    if(Freetype_FOUND)
        target_link_libraries(aa PUBLIC Freetype::Freetype)
    else()
        target_include_directories(aa PRIVATE ${FREETYPE2_INCLUDE_DIRS})
        target_link_libraries(aa PUBLIC ${FREETYPE2_LIBRARIES})
    endif()
    target_compile_definitions(aa PRIVATE HAVE_FTFONT=1)
endif()

set(AALIB_INCLUDE_DIR ${AALIB_SRC_DIR})
set(AALIB_LIBRARY aa)

list(JOIN AALIB_ENABLED_BACKENDS " " _aalib_backends_str)
message(STATUS "AA-lib display backends: ${_aalib_backends_str}")

# ── Optional: libmikmod ─────────────────────────────────────────────

if(BB_MUSIC)
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

if(HAVE_FTFONT)
    if(Freetype_FOUND)
        target_link_libraries(bb PRIVATE Freetype::Freetype)
    else()
        target_include_directories(bb PRIVATE ${FREETYPE2_INCLUDE_DIRS})
        target_link_libraries(bb PRIVATE ${FREETYPE2_LIBRARIES})
    endif()
    target_compile_definitions(bb PRIVATE HAVE_FTFONT=1)
endif()

target_compile_definitions(bb PRIVATE
    SOUNDDIR="${CMAKE_INSTALL_FULL_DATADIR}/bb"
)

# ── Install ──────────────────────────────────────────────────────────

install(TARGETS bb RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

install(FILES bb.1 DESTINATION ${CMAKE_INSTALL_MANDIR}/man1)

if(HAVE_FTFONT)
    install(FILES WQY_bitmap_12px.ttf DESTINATION ${CMAKE_INSTALL_BINDIR})
endif()

if(SOUNDFILES)
    install(FILES ${SOUNDFILES} DESTINATION ${CMAKE_INSTALL_DATADIR}/bb)
endif()
