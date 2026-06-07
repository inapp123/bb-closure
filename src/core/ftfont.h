#ifndef BB_FTFONT_H
#define BB_FTFONT_H

#include <stdint.h>

int  ftfont_init(const char *path);
void ftfont_uninit(void);
int  ftfont_ready(void);
int  ftfont_render_glyph(uint32_t cp, int cell_w, int cell_h,
                         unsigned char *out, int stride);
int  ftfont_render_glyph_scaled(uint32_t cp, int out_w, int out_h,
                                unsigned char *out, int stride);

#endif
