#ifndef AAGLYPH_H
#define AAGLYPH_H

#include <stdint.h>
#include "aalib.h"

typedef void (*aa_putpixel_fn)(void *ctx, int px, int py, uint32_t rgb);

void aa_draw_cell(aa_context *c, int px, int py, uint32_t cp,
                  int cell_w, int cell_h, uint32_t fg, uint32_t bg,
                  aa_putpixel_fn putpixel, void *ctx);

#endif
