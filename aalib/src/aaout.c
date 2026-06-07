#include "utf8.h"
#include "aalib.h"
#include "aaint.h"

static void
aa_put_cell(aa_context *c, int *x1, int *y1, uint32_t cp, int attr)
{
    int pos1;

    if (*x1 < 0 || *y1 < 0 || *x1 >= aa_scrwidth(c) || *y1 >= aa_scrheight(c))
	return;
    pos1 = *x1 + *y1 * aa_scrwidth(c);
    c->glyphbuffer[pos1] = cp;
    c->textbuffer[pos1] = (cp < 128) ? (unsigned char) cp : ' ';
    c->attrbuffer[pos1] = attr;
    (*x1)++;
    if (*x1 >= aa_scrwidth(c)) {
	*x1 = 0;
	(*y1)++;
    }
}

void aa_puts_utf8(aa_context *c, int x, int y, enum aa_attribute attr,
		  __AA_CONST char *s)
{
    int x1, y1;
    uint32_t cp;
    int w, i;

    if (x < 0 || y < 0 || x >= aa_scrwidth(c) || y >= aa_scrheight(c))
	return;
    if (c->glyphbuffer == NULL)
	return;
    x1 = x;
    y1 = y;
    while (s != NULL && *s != '\0') {
	s = utf8_next(s, &cp);
	if (cp == 0)
	    break;
	w = utf8_column_width(cp);
	if (w <= 0)
	    continue;
	aa_put_cell(c, &x1, &y1, cp, attr);
	for (i = 1; i < w; i++)
	    aa_put_cell(c, &x1, &y1, AA_GLYPH_WIDE_PAD, attr);
	if (y1 >= aa_scrheight(c))
	    break;
    }
}

void aa_puts(aa_context * c, int x, int y, enum aa_attribute attr, __AA_CONST char *s)
{
    aa_puts_utf8(c, x, y, attr, s);
}

void aa_resizehandler(aa_context * c, void (*handler) (aa_context *))
{
    c->resizehandler = handler;
}

void aa_hidecursor(aa_context * c)
{
    c->cursorstate--;
    if (c->cursorstate == -1 && c->driver->cursormode != NULL)
	c->driver->cursormode(c, 0);
}
void aa_showcursor(aa_context * c)
{
    c->cursorstate++;
    if (c->cursorstate == 0 && c->driver->cursormode != NULL)
	c->driver->cursormode(c, 1);
    aa_gotoxy(c, c->cursorx, c->cursory);
}
void aa_gotoxy(aa_context * c, int x, int y)
{
    if (c->cursorstate >= 0) {
	if (x < 0)
	    x = 0;
	if (y < 0)
	    y = 0;
	if (x >= aa_scrwidth(c))
	    x = aa_scrwidth(c) - 1;
	if (y >= aa_scrheight(c))
	    y = aa_scrheight(c) - 1;
	c->driver->gotoxy(c, x, y);
	c->cursorx = x;
	c->cursory = y;
    }
}
