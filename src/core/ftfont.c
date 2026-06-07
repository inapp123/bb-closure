/*
 * FreeType glyph cache for BB+AA-lib UTF-8 rendering.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ftfont.h"

#ifdef HAVE_FTFONT
#include <ft2build.h>
#include FT_FREETYPE_H
#endif

#define FTFONT_CACHE_SIZE 512
#define FTFONT_CELL_MAX   32

typedef struct ftfont_entry {
    uint32_t cp;
    int cell_w;
    int cell_h;
    unsigned char *bitmap;
    int bmp_w;
    int bmp_h;
    struct ftfont_entry *next;
} ftfont_entry;

#ifdef HAVE_FTFONT
static FT_Library ft_lib;
static FT_Face ft_face;
#endif
static ftfont_entry *ft_cache[FTFONT_CACHE_SIZE];
static int ft_enabled;

static unsigned
ftfont_hash(uint32_t cp, int cw, int ch)
{
    return (unsigned) (cp ^ (uint32_t) (cw * 131 + ch * 17)) % FTFONT_CACHE_SIZE;
}

static ftfont_entry *
ftfont_lookup(uint32_t cp, int cell_w, int cell_h)
{
    ftfont_entry *e = ft_cache[ftfont_hash(cp, cell_w, cell_h)];

    while (e != NULL) {
        if (e->cp == cp && e->cell_w == cell_w && e->cell_h == cell_h)
            return e;
        e = e->next;
    }
    return NULL;
}

static void
ftfont_store(ftfont_entry *entry)
{
    unsigned h = ftfont_hash(entry->cp, entry->cell_w, entry->cell_h);

    entry->next = ft_cache[h];
    ft_cache[h] = entry;
}

static const char *
ftfont_default_path(void)
{
    const char *env = getenv("BB_FONT");
    if (env != NULL && env[0] != '\0')
        return env;
    env = getenv("AA_FONT");
    if (env != NULL && env[0] != '\0')
        return env;
    return "./WQY_bitmap_12px.ttf";
}

int
ftfont_init(const char *path)
{
#ifndef HAVE_FTFONT
    (void) path;
    return 0;
#else
    const char *fontpath = path;

    if (ft_enabled)
        return 1;
    if (fontpath == NULL || fontpath[0] == '\0')
        fontpath = ftfont_default_path();
    if (FT_Init_FreeType(&ft_lib) != 0)
        return 0;
    if (FT_New_Face(ft_lib, fontpath, 0, &ft_face) != 0) {
        fprintf(stderr, "bb: cannot load font '%s' (set BB_FONT)\n", fontpath);
        FT_Done_FreeType(ft_lib);
        ft_lib = NULL;
        return 0;
    }
    ft_enabled = 1;
    return 1;
#endif
}

void
ftfont_uninit(void)
{
#ifndef HAVE_FTFONT
    return;
#else
    int i;

    for (i = 0; i < FTFONT_CACHE_SIZE; i++) {
        ftfont_entry *e = ft_cache[i];
        while (e != NULL) {
            ftfont_entry *next = e->next;
            free(e->bitmap);
            free(e);
            e = next;
        }
        ft_cache[i] = NULL;
    }
    if (ft_face != NULL) {
        FT_Done_Face(ft_face);
        ft_face = NULL;
    }
    if (ft_lib != NULL) {
        FT_Done_FreeType(ft_lib);
        ft_lib = NULL;
    }
    ft_enabled = 0;
#endif
}

int
ftfont_ready(void)
{
    return ft_enabled;
}

static unsigned char
ftfont_sample_bitmap(const FT_Bitmap *bmp, int x, int y)
{
    unsigned char v;

    if (bmp->pixel_mode == FT_PIXEL_MODE_MONO) {
        v = bmp->buffer[y * bmp->pitch + (x >> 3)];
        return (v & (0x80 >> (x & 7))) ? 255 : 0;
    }
    v = bmp->buffer[y * bmp->pitch + x];
    return v >= 128 ? 255 : 0;
}

static int
ftfont_rasterize(uint32_t cp, int cell_w, int cell_h, ftfont_entry *entry)
{
#ifdef HAVE_FTFONT
    FT_GlyphSlot slot;
    FT_Bitmap *bmp;
    int x, y;
    int pen_x, pen_y;
    int ascender;

    if (!ft_enabled)
        return 0;
    if (cell_w <= 0 || cell_h <= 0 || cell_w > FTFONT_CELL_MAX
        || cell_h > FTFONT_CELL_MAX)
        return 0;

    FT_Set_Pixel_Sizes(ft_face, (FT_UInt) cell_w, (FT_UInt) cell_h);
    if (FT_Load_Char(ft_face, cp, FT_LOAD_RENDER) != 0)
        return 0;

    slot = ft_face->glyph;
    bmp = &slot->bitmap;
    entry->bmp_w = cell_w;
    entry->bmp_h = cell_h;

    entry->bitmap = calloc((size_t) cell_w * (size_t) cell_h, 1);
    if (entry->bitmap == NULL)
        return 0;

    ascender = (int) (ft_face->size->metrics.ascender / 64);
    pen_x = (cell_w - (int) bmp->width) / 2;
    pen_y = ascender - slot->bitmap_top;
    if (pen_x < 0)
        pen_x = 0;
    if (pen_y < 0)
        pen_y = 0;

    for (y = 0; y < (int) bmp->rows; y++) {
        for (x = 0; x < (int) bmp->width; x++) {
            int dx = pen_x + x;
            int dy = pen_y + y;
            unsigned char v;
            if (dx < 0 || dy < 0 || dx >= cell_w || dy >= cell_h)
                continue;
            v = ftfont_sample_bitmap(bmp, x, y);
            if (v)
                entry->bitmap[dy * cell_w + dx] = v;
        }
    }
    return 1;
#else
    (void) cp;
    (void) cell_w;
    (void) cell_h;
    (void) entry;
    return 0;
#endif
}

static ftfont_entry *
ftfont_get(uint32_t cp, int cell_w, int cell_h)
{
    ftfont_entry *entry;

    entry = ftfont_lookup(cp, cell_w, cell_h);
    if (entry != NULL)
        return entry;

    entry = calloc(1, sizeof(*entry));
    if (entry == NULL)
        return NULL;
    entry->cp = cp;
    entry->cell_w = cell_w;
    entry->cell_h = cell_h;
    if (!ftfont_rasterize(cp, cell_w, cell_h, entry)) {
        free(entry);
        return NULL;
    }
    ftfont_store(entry);
    return entry;
}

int
ftfont_render_glyph(uint32_t cp, int cell_w, int cell_h,
                    unsigned char *out, int stride)
{
    ftfont_entry *entry;
    int y;

    if (out == NULL || stride <= 0)
        return 0;
    memset(out, 0, (size_t) stride * (size_t) cell_h);
    if (!ft_enabled)
        return 0;
    entry = ftfont_get(cp, cell_w, cell_h);
    if (entry == NULL)
        return 0;
    for (y = 0; y < cell_h; y++)
        memcpy(out + y * stride, entry->bitmap + y * cell_w, (size_t) cell_w);
    return 1;
}

int
ftfont_render_glyph_scaled(uint32_t cp, int out_w, int out_h,
                           unsigned char *out, int stride)
{
    return ftfont_render_glyph(cp, out_w, out_h, out, stride);
}
