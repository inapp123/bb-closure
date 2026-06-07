
#include <stdio.h>
#include "config.h"
#include "utf8.h"
#include "aalib.h"
#include "aaint.h"

#ifdef CURSES_DRIVER
#ifdef USE_NCURSES
#include <ncurses.h>
#else
#include <curses.h>
#endif
extern __AA_CONST struct aa_driver curses_d;
#endif

#define HIDEMOUSE if(!hidden&&cursor&&c->mousedriver!=NULL&&(c->mousedriver->flags&AA_HIDECURSOR)) \
	   aa_hidemouse(c),hidden=1;
static void aa_display(aa_context * c, int x1, int y1, int x2, int y2)
{
    int x, y, pos, attr, p;
    unsigned char str[256];
    int cursor=c->mousemode,hidden=0;
    if (x2 < 0 || y2 < 0 || x1 > aa_scrwidth(c) || y1 > aa_scrheight(c))
	return;
    if (x2 >= aa_scrwidth(c))
	x2 = aa_scrwidth(c);
    if (y2 >= aa_scrheight(c))
	y2 = aa_scrheight(c);
    if (x1 < 0)
	x1 = 0;
    if (y1 < 0)
	y1 = 0;
	if (c->driver->print == NULL)
	    return;
	pos = 0;
	for (y = y1; y < y2; y++) {
	    pos = y * aa_scrwidth(c) + x1;
	    c->driver->gotoxy(c, x1, y);
	    for (x = x1; x < x2;) {
		p = 0;
		attr = c->attrbuffer[pos];
		while (p < (int) sizeof(str) - 5 && x < x2
		       && c->attrbuffer[pos] == attr) {
		    uint32_t cp = c->glyphbuffer[pos];
		    int n;
		    if (cp == AA_GLYPH_WIDE_PAD) {
			pos++;
			x++;
			continue;
		    }
		    n = utf8_encode(cp, (char *) str + p);
		    p += n;
		    pos++;
		    x++;
		}
		str[p] = 0;
		HIDEMOUSE
		c->driver->setattr(c, attr);
		c->driver->print(c, (char *)str);
	    }
	c->driver->gotoxy(c, c->cursorx, c->cursory);
    }
        if(hidden&&cursor)
	   aa_showmouse(c);
}
void aa_hidemouse(aa_context *c)
{
  if(c->mousemode) {
  c->mousemode=0;
  if(c->mousedriver!=NULL&&c->mousedriver->cursormode!=NULL) c->mousedriver->cursormode(c,0);
  }
}
void aa_showmouse(aa_context *c)
{
  if(!c->mousemode) {
  c->mousemode=1;
  if(c->mousedriver!=NULL&&c->mousedriver->cursormode!=NULL) c->mousedriver->cursormode(c,1);
  }
}

void aa_flush(aa_context * c)
{
#ifdef CURSES_DRIVER
    if (c->driver == &curses_d)
	clear();
#endif
    if (c->driver->print != NULL)
	aa_display(c, 0, 0, aa_scrwidth(c), aa_scrheight(c));
    if (c->driver->flush != NULL)
    { int cursor=c->mousemode;
        if(cursor&&c->mousedriver!=NULL&&(c->mousedriver->flags&AA_HIDECURSOR))
	   aa_hidemouse(c);
	c->driver->flush(c);
        if(cursor&&c->mousedriver!=NULL&&(c->mousedriver->flags&AA_HIDECURSOR))
	   aa_showmouse(c);
    }
}
