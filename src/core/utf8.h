#ifndef BB_UTF8_H
#define BB_UTF8_H

#include <stdint.h>

/* Unicode codepoint stored in glyphbuffer for wide-char padding column. */
#define AA_GLYPH_WIDE_PAD 0xFFFFFFFEU

const char *utf8_next(const char *s, uint32_t *cp);
int         utf8_encode(uint32_t cp, char *out);
int         utf8_column_width(uint32_t cp);
int         utf8_display_width(const char *s);

#endif
