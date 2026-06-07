/*
 * BB: The portable demo
 *
 * (C) 1997 by AA-group (e-mail: aa@horac.ta.jcu.cz)
 *
 * 3rd August 1997
 * version: 1.2 [final3]
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public Licences as by published
 * by the Free Software Foundation; either version 2; or (at your option)
 * any later version
 *
 * This program is distributed in the hope that it will entertaining,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILTY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General
 * Publis License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.
 * 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include "bb.h"
#include "utf8.h"
#include "ftfont.h"

/* An hack - but I did aalib, so I can hack :) */
struct parameters
{
  unsigned int p[AA_NPARAMS];
};

static void
backconvert_ascii(int x, int y, int attr)
{
    int n = (int) ' ' + 256 * attr;
    if (context->glyphbuffer != NULL) {
        uint32_t cp = context->glyphbuffer[x + y * aa_scrwidth(context)];
        if (cp != 0 && cp != AA_GLYPH_WIDE_PAD && cp < 128)
            n = (int) cp + 256 * attr;
    } else {
        n = context->textbuffer[x + y * aa_scrwidth(context)] + 256 * attr;
    }
    aa_putpixel(context, x * 2, y * 2, context->parameters[n].p[1]);
    aa_putpixel(context, x * 2 + 1, y * 2, context->parameters[n].p[0]);
    aa_putpixel(context, x * 2, y * 2 + 1, context->parameters[n].p[3]);
    aa_putpixel(context, x * 2 + 1, y * 2 + 1, context->parameters[n].p[2]);
}

static void
backconvert_ft(int x, int y, uint32_t cp, int cols, int attr)
{
    unsigned char buf[32 * 32];
    int mulx = context->mulx > 0 ? context->mulx : 2;
    int muly = context->muly > 0 ? context->muly : 2;
    int gw = cols * mulx;
    int gh = muly;
    int gx, gy;
    int color = context->parameters[(int) ' ' + 256 * attr].p[0];

    if (!ftfont_render_glyph(cp, gw, gh, buf, gw))
        return;
    for (gy = 0; gy < gh; gy++) {
        for (gx = 0; gx < gw; gx++) {
            if (buf[gy * gw + gx])
                aa_putpixel(context, x * mulx + gx, y * muly + gy, color);
        }
    }
}

void backconvert(int x1, int y1, int x2, int y2)
{
    int x, y;
    for (y = y1; y < y2; y++)
	for (x = x1; x < x2; x++) {
            uint32_t cp;
            int cols;
            int attr = context->attrbuffer[x + y * aa_scrwidth(context)];

            if (context->glyphbuffer != NULL) {
                cp = context->glyphbuffer[x + y * aa_scrwidth(context)];
                if (cp == AA_GLYPH_WIDE_PAD)
                    continue;
            } else {
                cp = context->textbuffer[x + y * aa_scrwidth(context)];
            }
            cols = utf8_column_width(cp);
            if (cols <= 0)
                continue;
            if (cp < 128)
                backconvert_ascii(x, y, attr);
            else
                backconvert_ft(x, y, cp, cols, attr);
	}
}
