#include <malloc.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "config.h"
#ifdef X11_DRIVER
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include "utf8.h"
#include "aalib.h"
#include "aaint.h"
#include "aaxint.h"
#include "aaglyph.h"
__AA_CONST struct aa_driver X11_d;
#define C2 0x68
#define C1 0xB2
#define dr (d->pixmapmode?d->pi:d->wi)
static void X_flush(aa_context * c);

static int font_error;
static int
mygetpixel (XImage * image, int pos, int y)
{
  int width = image->width;
  int i;
  int sum = font_error;
  int start = (pos * width + 4) / 8;
  int end = ((pos + 1) * width + 4) / 8;
  if (start == end)
    {
      if (start == image->width - 1)
	start--;
      else
	end++;
    }
  for (i = start; i < end; i++)
    sum += XGetPixel (image, i, y) != 0;
  if (sum <= (end - start) / 2)
    {
      font_error = sum;
      return 0;
    }
  else
    font_error = -(end - start - sum);
  return 1;
}
static void
X_AllocColors (struct xdriverdata * d)
{
  static XColor c;
  d->black = d->attr.border_pixel = d->attr.background_pixel = d->invertedbold =
    BlackPixel (d->dp, d->screen);
  d->bold = d->invertedblack = WhitePixel (d->dp, d->screen);

  c.red = C1 * 256;
  c.green = C1 * 256;
  c.blue = C1 * 256;
  if (!XAllocColor (d->dp, d->cmap, &c))
     d->normal = d->bold;
  else
     d->normal = c.pixel;
  c.red = 65536 - c.red;
  c.green = 65536 - c.green;
  c.blue = 65536 - c.blue;
  if (!XAllocColor (d->dp, d->cmap, &c))
     d->invertednormal = d->invertedbold, d->normal = d->bold;
  else
     d->invertednormal = c.pixel;

  c.red = C2 * 256;
  c.green = C2 * 256;
  c.blue = C2 * 256;
  if (d->bold == d->dim && !XAllocColor (d->dp, d->cmap, &c))
     d->dim = d->normal;
  else
     d->dim = c.pixel;
  c.red = 65536 - c.red;
  c.green = 65536 - c.green;
  c.blue = 65536 - c.blue;
  if (!XAllocColor (d->dp, d->cmap, &c))
     d->inverteddim = d->invertednormal, d->dim = d->normal;
  else
     d->inverteddim = c.pixel;

  c.red = 0;
  c.green = 0;
  c.blue = 65535UL;
  if (!XAllocColor (d->dp, d->cmap, &c))
     d->special = d->black;
  else
     d->special = c.pixel;
  c.red = 65535UL/2;
  c.green = 65535UL/2;
  c.blue = 65535UL;
  if (!XAllocColor (d->dp, d->cmap, &c))
     d->invertedspecial = d->invertedblack;
  else
     d->invertedspecial = c.pixel;
}
static void
X_setinversionmode (int inverted, struct xdriverdata *d)
{
  d->inverted = inverted;
  if (d->specialGC)
    XFreeGC (d->dp, d->specialGC);
  if (d->normalGC)
    XFreeGC (d->dp, d->normalGC);
  if (d->boldGC)
    XFreeGC (d->dp, d->boldGC);
  if (d->dimGC)
    XFreeGC (d->dp, d->dimGC);
  d->specialGC = XCreateGC (d->dp, d->wi, 0L, NULL);
  XSetForeground (d->dp, d->specialGC, inverted ? d->invertedspecial : d->special);
  XSetFont (d->dp, d->specialGC, d->font);
  d->normalGC = XCreateGC (d->dp, d->wi, 0L, NULL);
  XSetForeground (d->dp, d->normalGC, inverted ? d->invertednormal : d->normal);
  XSetBackground(d->dp, d->normalGC, inverted ? d->invertedblack : d->black);
  XSetFont (d->dp, d->normalGC, d->font);
  d->boldGC = XCreateGC (d->dp, d->wi, 0L, NULL);
  XSetForeground (d->dp, d->boldGC, inverted ? d->invertedbold : d->bold);
  XSetBackground(d->dp, d->boldGC, inverted ? d->invertedblack : d->black);
  XSetFont (d->dp, d->boldGC, d->font);
  d->dimGC = XCreateGC (d->dp, d->wi, 0L, NULL);
  XSetForeground (d->dp, d->dimGC, inverted ? d->inverteddim : d->dim);
  XSetBackground(d->dp, d->dimGC, inverted ? d->invertedblack : d->black);
  XSetFont (d->dp, d->dimGC, d->font);
  d->blackGC = XCreateGC (d->dp, d->wi, 0L, NULL);
  XSetForeground (d->dp, d->blackGC, inverted ? d->invertedblack : d->black);
  XSetBackground(d->dp, d->blackGC, inverted ? d->invertedblack : d->black);
  d->currGC = d->normalGC;

  if (!d->pixmapmode)
    XSetWindowBackground (d->dp, d->wi, inverted ? d->invertedblack : d->black);
  else
    XFillRectangle(d->dp, d->pi, d->blackGC, 0, 0, d->pixelwidth, d->pixelheight);
  XClearWindow (d->dp, d->wi);
  if (d->previoust != NULL)
      free(d->previoust), free(d->previousa);
  d->previoust=NULL;
  d->previousa=NULL;
}

static int X_init(__AA_CONST struct aa_hardware_params *p, __AA_CONST void *none,struct aa_hardware_params *dest, void **driverdata)
{
    const char *font = "8x13bold";
    static int registered;
    static aa_font aafont;
    __AA_CONST static struct aa_hardware_params def=
    {&aa_fontX13B, AA_DIM_MASK | AA_REVERSE_MASK | AA_NORMAL_MASK | AA_BOLD_MASK | AA_BOLDFONT_MASK | AA_EXTENDED,
     0, 0,
     0, 0,
     80, 32,
     0, 0};
    struct xdriverdata *d;
    *dest=def;
    *driverdata=d=calloc(1,sizeof(*d));
    d->previoust=NULL;
    d->previousa=NULL;
    d->cvisible=1;
    d->width=80;
    d->height=32;
    if ((d->dp = XOpenDisplay(NULL)) == NULL)
	return 0;
    d->screen = DefaultScreen(d->dp);
    if (getenv ("AAFont"))
      font = getenv ("AAFont");
    d->font = XLoadFont(d->dp, font);
    if (!d->font) {
	XCloseDisplay(d->dp);
	return 0;
    }
    d->font_s = XQueryFont(d->dp, d->font);
    if (!d->font_s) {
	XCloseDisplay(d->dp);
	return 0;
    }
    d->fontheight = d->font_s->max_bounds.ascent + d->font_s->max_bounds.descent;
    d->fontwidth = d->font_s->max_bounds.rbearing - d->font_s->min_bounds.lbearing;
    d->realfontwidth = d->font_s->max_bounds.width;
    d->cmap = DefaultColormap(d->dp, d->screen);
    /*c.flags=DoRed | DoGreen | DoBlue; */

    X_AllocColors (d);
    if (d->bold == d->normal)
      dest->supported &= ~AA_BOLD_MASK;
    if (d->dim == d->normal)
      dest->supported &= ~AA_DIM_MASK;

    d->attr.event_mask = ExposureMask;
    d->attr.override_redirect = False;
    if (p->width)
	d->width = p->width;
    if (p->height)
	d->height = p->height;
    if (p->maxwidth && d->width > p->maxwidth)
	d->width = p->maxwidth;
    if (p->minwidth && d->width < p->minwidth)
	d->width = p->minwidth;
    if (p->maxheight && d->height > p->maxheight)
	d->height = p->maxheight;
    if (p->minheight && d->height < p->minheight)
	d->height = p->minheight;
    d->wi = XCreateWindow(d->dp, RootWindow(d->dp, d->screen), 0, 0, d->width * d->realfontwidth, d->height * d->fontheight, 0, DefaultDepth(d->dp, d->screen), InputOutput, DefaultVisual(d->dp, d->screen), CWBackPixel | CWBorderPixel | CWEventMask, &d->attr);
    if (!registered) {
	d->pi = XCreatePixmap(d->dp, d->wi, d->fontwidth, d->fontheight * 256, 1);
	if (d->pi) {
	    int i;
	    unsigned char c;
	    unsigned char *data;
	    XImage *image;
	    registered = 1;
	    d->specialGC = XCreateGC(d->dp, d->pi, 0L, NULL);
	    XSetForeground(d->dp, d->specialGC, 0);
	    XSetBackground(d->dp, d->specialGC, 0);
	    XFillRectangle(d->dp, d->pi, d->specialGC, 0, 0, d->fontwidth, 256 * d->fontheight);
	    XSetForeground(d->dp, d->specialGC, 1);
	    XSetFont(d->dp, d->specialGC, d->font);
	    for (i = 0; i < 256; i++) {
		c = i;
		XDrawString(d->dp, d->pi, d->specialGC, 0, (i + 1) * d->fontheight - d->font_s->descent, (char *)&c, 1);
	    }
	    image = XGetImage(d->dp, d->pi, 0, 0, d->fontwidth, 256 * d->fontheight, 1, XYPixmap);
	    if (image != NULL) {
		data = malloc(256 * d->fontheight);
		for (i = 0; i < 256; i++) {
		    int y;
		    font_error = 0;
		    for (y = 0; y < d->fontheight; y++) {
			int o;
			o = ((mygetpixel(image, 0, i * d->fontheight + y) != 0) << 7) +
			    ((mygetpixel(image, 1, i * d->fontheight + y) != 0) << 6) +
			    ((mygetpixel(image, 2, i * d->fontheight + y) != 0) << 5) +
			    ((mygetpixel(image, 3, i * d->fontheight + y) != 0) << 4) +
			    ((mygetpixel(image, 4, i * d->fontheight + y) != 0) << 3) +
			    ((mygetpixel(image, 5, i * d->fontheight + y) != 0) << 2) +
			    ((mygetpixel(image, 6, i * d->fontheight + y) != 0) << 1) +
			    ((mygetpixel(image, 7, i * d->fontheight + y) != 0) << 0);
			data[i * d->fontheight + y] = o;
		    }
		}
		aafont.name = "Font used by X server";
		aafont.shortname = "current";
		aafont.height = d->fontheight;
		aafont.data = data;
		aa_registerfont(&aafont);
		dest->font = &aafont;
	    }
	}
    }
    XStoreName(d->dp, d->wi, "aa for X");
    XMapWindow(d->dp, d->wi);
    X_setinversionmode (getenv ("AAInverted") != NULL, d);
    d->pixelwidth = -1;
    d->pixelheight = -1;
    XSync(d->dp, 0);
    aa_recommendlowkbd("X11");
    return 1;
}
int __aa_X_getsize(struct aa_context *c,struct xdriverdata *d)
{
    unsigned int px, py;
    int tmp;
    Window wtmp;
    XSync(d->dp, 0);
    XGetGeometry(d->dp, d->wi, &wtmp, &tmp, &tmp, &px, &py, (unsigned int *) &tmp, (unsigned int *) &tmp);
    tmp = 0;
    if (px != d->pixelwidth || py != d->pixelheight)
	tmp = 1;
    d->pixelwidth = px;
    d->pixelheight = py;
    if (tmp) {
	d->width = d->pixelwidth / d->realfontwidth;
	d->height = d->pixelheight / d->fontheight;
	if (d->pixmapmode)
	    XFreePixmap(d->dp, d->pi);
	if (!getenv("AABlink"))
	  d->pi = XCreatePixmap(d->dp, d->wi, d->pixelwidth, d->pixelheight, DefaultDepth(d->dp, d->screen));
	else
	  d->pi = BadAlloc;
	if (d->pi == BadAlloc) {
	    d->pixmapmode = 0;
	    XSetWindowBackground(d->dp, d->wi, d->inverted ? d->invertedblack : d->black);
	} else {
	    d->pixmapmode = 1;
	    XFillRectangle(d->dp, d->pi, d->blackGC, 0, 0, d->pixelwidth, d->pixelheight);
	    XSetWindowBackgroundPixmap(d->dp, d->wi, d->pi);
	}
	if (d->previoust != NULL)
	    free(d->previoust), free(d->previousa);
	d->previoust=NULL;
	d->previousa=NULL;
	c->driverparams.mmwidth = DisplayWidthMM(d->dp, d->screen) * d->pixelwidth / DisplayWidth(d->dp, d->screen);
	c->driverparams.mmheight = DisplayHeightMM(d->dp, d->screen) * d->pixelheight / DisplayHeight(d->dp, d->screen);
    }
    XSync(d->dp, 0);
    return (tmp);
}
static void X_uninit(aa_context * c)
{
    struct xdriverdata *d=c->driverdata;
    if (d->previoust != NULL)
	free(d->previoust), free(d->previousa);
    if (d->pixmapmode)
	XFreePixmap(d->dp, d->pi);
    XCloseDisplay(d->dp);
}
static void X_getsize(aa_context * c, int *width1, int *height1)
{
    struct xdriverdata *d=c->driverdata;
    __aa_X_getsize(c,d);
    *width1 = d->width = d->pixelwidth / d->realfontwidth;
    *height1 = d->height = d->pixelheight / d->fontheight;
}
static void X_setattr(struct xdriverdata *d,int attr)
{
    switch (attr) {
    case 0:
    case 4:
	d->currGC = d->normalGC;
	break;
    case 1:
	d->currGC = d->dimGC;
	break;
    case 2:
	d->currGC = d->boldGC;
	break;
    case 3:
	d->currGC = d->blackGC;
	break;
    }
}
/*quite complex but fast drawing routing for X */
#define NATT 5
#define texty(l,a,x) _texty[((l)*NATT+(a))*d->width+(x)]
#define rectangles(a,x) _rectangles[(a)*d->height*d->width+(x)]
static XTextItem *_texty;
static int (*nitem)[NATT];
static int (*startitem)[NATT];
static XRectangle *_rectangles;
static int nrectangles[4];
static int drawed;
static int area;
static void alloctables(struct xdriverdata *d)
{
    _texty = malloc(sizeof(XTextItem) * d->width * NATT * d->height);
    nitem = calloc(sizeof(*nitem) * d->height,1);
    startitem = calloc(sizeof(*startitem) * d->height,1);
    _rectangles = malloc(sizeof(*_rectangles) * d->width * d->height * 4);
}
static void freetables(void)
{
    free(_texty);
    free(nitem);
    free(startitem);
    free(_rectangles);
}

static void MyDrawString(struct xdriverdata *d,int attr, int x, int y, unsigned char *c, int i)
{
    XTextItem *it;
    XRectangle *rect;
    int n, a;
    switch (attr) {
    case AA_NORMAL:
    case AA_DIM:
    case AA_BOLD:
    case AA_BOLDFONT:
    default:
	n = 0;
	break;
    case AA_REVERSE:
	n = 1;
	break;
    case AA_SPECIAL:
	n = 2;
	break;
    }
    switch (attr) {
    default:
    case AA_SPECIAL:
    case AA_NORMAL:
	a = 0;
	break;
    case AA_DIM:
	a = 1;
	break;
    case AA_BOLD:
	a = 2;
	break;
    case AA_REVERSE:
	a = 3;
	break;
    case AA_BOLDFONT:
	a = 4;
	break;
    }
    it = &texty(y, a, nitem[y][a]);
    it->delta = x * d->realfontwidth - startitem[y][a];
    if (!it->delta && x) {
	it--;
	it->nchars += i;
    } else {
	nitem[y][a]++;
	it->chars = (char *)c;
	it->nchars = i;
	it->font = d->font;
	drawed = 1;
    }
    startitem[y][a] = (x + i) * d->realfontwidth;
    rect = &rectangles(n, nrectangles[n]);
    rect->x = x * d->realfontwidth;
    rect->y = (y) * d->fontheight + 1;
    rect->width = i * d->realfontwidth;
    if (nrectangles[n] && (rect - 1)->y == rect->y &&
	(rect - 1)->x + (rect - 1)->width == rect->x)
	nrectangles[n]--, (--rect)->width += i * d->realfontwidth;
    rect->height = d->fontheight;
    nrectangles[n]++;
    rect = &rectangles(n, nrectangles[3]);
    rect->x = x * d->realfontwidth;
    rect->y = (y) * d->fontheight + 1;
    rect->width = i * d->realfontwidth;
    if (nrectangles[3] && (rect - 1)->y == rect->y &&
	(rect - 1)->x + (rect - 1)->width == rect->x)
	nrectangles[3]--, (--rect)->width += i * d->realfontwidth;
    rect->height = d->fontheight;
    nrectangles[3]++;
    area += i;
}
__AA_CONST static int Black[] =
{0, 0, 0, 0, 1, 1};


static void X_invalidate_cache (struct xdriverdata *d)
{
    if (d->previoust != NULL)
	free (d->previoust), free (d->previousa);
    d->previoust = NULL;
    d->previousa = NULL;
}

static void X_sync_screen (aa_context *c, struct xdriverdata *d)
{
    int scr_w = aa_scrwidth (c);
    int scr_h = aa_scrheight (c);

    if (scr_w <= 0 || scr_h <= 0)
	return;
    if (d->previoust != NULL && (scr_w != d->width || scr_h != d->height))
	X_invalidate_cache (d);
    d->width = scr_w;
    d->height = scr_h;
}

struct xdraw_ctx {
    struct xdriverdata *d;
    GC gc;
};

static void
X_pick_colors (struct xdriverdata *d, int attr, uint32_t *fg, uint32_t *bg)
{
    if (d->inverted) {
	switch (attr) {
	case AA_DIM:
	    *fg = d->inverteddim;
	    *bg = d->invertedblack;
	    break;
	case AA_BOLD:
	case AA_BOLDFONT:
	    *fg = d->invertedbold;
	    *bg = d->invertedblack;
	    break;
	case AA_REVERSE:
	    *fg = d->invertednormal;
	    *bg = d->invertedblack;
	    break;
	case AA_SPECIAL:
	    *fg = d->invertedspecial;
	    *bg = d->invertedblack;
	    break;
	default:
	    *fg = d->invertednormal;
	    *bg = d->invertedblack;
	    break;
	}
	return;
    }
    switch (attr) {
    case AA_DIM:
	*fg = d->dim;
	*bg = d->black;
	break;
    case AA_BOLD:
    case AA_BOLDFONT:
	*fg = d->bold;
	*bg = d->black;
	break;
    case AA_REVERSE:
	*fg = d->black;
	*bg = d->normal;
	break;
    case AA_SPECIAL:
	*fg = d->special;
	*bg = d->black;
	break;
    default:
	*fg = d->normal;
	*bg = d->black;
	break;
    }
}

static void
X_putpixel_cb (void *ctx, int px, int py, uint32_t rgb)
{
    struct xdraw_ctx *xc = (struct xdraw_ctx *) ctx;
    XSetForeground (xc->d->dp, xc->gc, (unsigned long) rgb);
    XDrawPoint (xc->d->dp,
                xc->d->pixmapmode ? xc->d->pi : xc->d->wi,
                xc->gc, px, py);
}

static void X_flush(aa_context * c)
{
    struct xdriverdata *d=c->driverdata;
    int x, y;
    int scr_w, scr_h;
    struct xdraw_ctx xctx;
    int cell_h = d->fontheight > 0 ? d->fontheight : 8;

    X_sync_screen (c, d);
    scr_w = aa_scrwidth (c);
    scr_h = aa_scrheight (c);
    if (scr_w <= 0 || scr_h <= 0)
	return;

    XFillRectangle (d->dp, d->pixmapmode ? d->pi : d->wi, d->blackGC, 0, 0,
		    d->pixelwidth, d->pixelheight);

    xctx.d = d;
    xctx.gc = d->normalGC;
    if (c->glyphbuffer != NULL) {
	for (y = 0; y < scr_h; y++) {
	    for (x = 0; x < scr_w; x++) {
		int idx = x + y * scr_w;
		uint32_t cp = c->glyphbuffer[idx];
		int attr = c->attrbuffer[idx];
		uint32_t fg, bg;

		if (cp == AA_GLYPH_WIDE_PAD)
		    continue;
		if (cp == ' ' && attr == AA_NORMAL)
		    continue;
		X_pick_colors (d, attr, &fg, &bg);
		X_setattr (d, attr);
		xctx.gc = d->currGC;
		aa_draw_cell (c, x * d->realfontwidth, y * cell_h + 1, cp,
			      8, cell_h, fg, bg, X_putpixel_cb, &xctx);
	    }
	}
    }
    if (d->cvisible)
	XDrawLine (d->dp, d->pixmapmode ? d->pi : d->wi, d->normalGC,
		   d->Xpos * d->realfontwidth,
		   (d->Ypos + 1) * cell_h - 1,
		   (d->Xpos + 1) * d->realfontwidth - 1,
		   (d->Ypos + 1) * cell_h - 1);
    /* pixmap is the window background; must invalidate window to show updates */
    if (d->pixmapmode)
	XClearWindow (d->dp, d->wi);
    XFlush (d->dp);
}
void __aa_X_redraw(aa_context *c)
{
    struct xdriverdata *d=c->driverdata;
    if (d->pixmapmode && d->previoust != NULL) {
	    XFlush(d->dp);
	    return;
    }
    if (d->previoust != NULL)
	free(d->previoust), free(d->previousa);
    d->previoust=NULL;
    d->previousa=NULL;
    X_flush(c);
    XFlush(d->dp);
}
static void X_gotoxy(aa_context * c, int x, int y)
{
    struct xdriverdata *d=c->driverdata;
    if (d->Xpos != x || d->Ypos != y) {
	if (d->previoust != NULL)
	    d->previoust[d->Ypos * aa_scrwidth (c) + d->Xpos] = 255;
	d->Xpos = x;
	d->Ypos = y;
	X_flush(c);
    }
}
static void X_cursor(aa_context * c, int mode)
{
    struct xdriverdata *d=c->driverdata;
    d->cvisible = mode;
}

__AA_CONST struct aa_driver X11_d =
{
    "X11", "X11 driver 1.1",
    X_init,
    X_uninit,
    X_getsize,
    NULL,
    NULL,
    X_gotoxy,
    X_flush,
    X_cursor
};
#endif
