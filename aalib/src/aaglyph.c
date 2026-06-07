#include <string.h>
#include "utf8.h"
#include "ftfont.h"
#include "aalib.h"
#include "aaglyph.h"

static void
aa_draw_bitmap_cell(const unsigned char *glyph, int cell_w, int cell_h,
                    int px0, int py0, uint32_t fg, uint32_t bg,
                    aa_putpixel_fn putpixel, void *ctx)
{
    int gy, gx;

    for (gy = 0; gy < cell_h; gy++) {
        unsigned char row = glyph[gy];
        for (gx = 0; gx < cell_w; gx++) {
            uint32_t color = (row & (0x80 >> gx)) ? fg : bg;
            putpixel(ctx, px0 + gx, py0 + gy, color);
        }
    }
}

static void
aa_draw_ft_cell(uint32_t cp, int cell_w, int cell_h, int px0, int py0,
                uint32_t fg, uint32_t bg, aa_putpixel_fn putpixel, void *ctx)
{
    unsigned char buf[32 * 32];
    int x, y;

    if (!ftfont_render_glyph(cp, cell_w, cell_h, buf, cell_w))
        return;
    for (y = 0; y < cell_h; y++) {
        for (x = 0; x < cell_w; x++) {
            unsigned char v = buf[y * cell_w + x];
            putpixel(ctx, px0 + x, py0 + y, v ? fg : bg);
        }
    }
}

void
aa_draw_cell(aa_context *c, int px, int py, uint32_t cp,
             int cell_w, int cell_h, uint32_t fg, uint32_t bg,
             aa_putpixel_fn putpixel, void *ctx)
{
    const unsigned char *font;
    int font_h;
    int w;

    if (cp == AA_GLYPH_WIDE_PAD || cp == 0)
        return;
    if (putpixel == NULL || c == NULL || c->params.font == NULL)
        return;

    w = utf8_column_width(cp);
    if (w <= 0)
        return;
    if (w > 1)
        cell_w *= w;

    font = c->params.font->data;
    font_h = c->params.font->height;
    if (font_h <= 0)
        font_h = 8;
    if (cell_h <= 0)
        cell_h = font_h;

    if (cp < 128) {
        const unsigned char *glyph = font + (int) cp * font_h;
        int draw_w = (cell_w > 8) ? 8 : cell_w;
        if (cp == ' ') {
            int gx, gy;
            for (gy = 0; gy < cell_h; gy++)
                for (gx = 0; gx < draw_w * w; gx++)
                    putpixel(ctx, px + gx, py + gy, bg);
            return;
        }
        aa_draw_bitmap_cell(glyph, draw_w, cell_h, px, py, fg, bg, putpixel, ctx);
        return;
    }
    aa_draw_ft_cell(cp, cell_w, cell_h, px, py, fg, bg, putpixel, ctx);
}
