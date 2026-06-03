#include "config.h"
#ifdef FBDEV_DRIVER
#include <fcntl.h>
#include <linux/fb.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <unistd.h>
#include "aalib.h"
#include "aaint.h"

struct fbdev_state
{
  int fd;
  unsigned char *fb;
  size_t fb_len;
  struct fb_fix_screeninfo finfo;
  struct fb_var_screeninfo vinfo;
  int chars_w;
  int chars_h;
  int font_h;
  int cursor_x;
  int cursor_y;
  int cursor_visible;
};

static uint32_t
fbdev_pack_rgb (const struct fb_var_screeninfo *vinfo, uint8_t r, uint8_t g,
                uint8_t b)
{
  uint32_t value = 0;
  if (vinfo->red.length)
    value |= ((uint32_t) r >> (8 - vinfo->red.length)) << vinfo->red.offset;
  if (vinfo->green.length)
    value |= ((uint32_t) g >> (8 - vinfo->green.length))
             << vinfo->green.offset;
  if (vinfo->blue.length)
    value |= ((uint32_t) b >> (8 - vinfo->blue.length)) << vinfo->blue.offset;
  return value;
}

static void
fbdev_putpixel (struct fbdev_state *st, int x, int y, uint32_t rgb)
{
  size_t off;
  unsigned char *dst;
  int xreal = x + st->vinfo.xoffset;
  int yreal = y + st->vinfo.yoffset;
  int is_white = (rgb != 0);

  if (x < 0 || y < 0 || x >= (int) st->vinfo.xres || y >= (int) st->vinfo.yres)
    return;

  if (st->vinfo.bits_per_pixel == 1)
    {
      uint8_t mask;
      int bit_on;

      off = (size_t) yreal * st->finfo.line_length + (size_t) (xreal >> 3);
      if (off >= st->fb_len)
        return;

      /* MONO01 means black=1, white=0; MONO10 means black=0, white=1.  */
      bit_on = (st->finfo.visual == FB_VISUAL_MONO01) ? !is_white : is_white;
      /* Some mono framebuffers store leftmost pixel in LSB of each byte. */
      mask = (uint8_t) (1u << (xreal & 7));
      if (bit_on)
        st->fb[off] |= mask;
      else
        st->fb[off] &= (uint8_t) ~mask;
      return;
    }

  {
    int bytespp = st->vinfo.bits_per_pixel >> 3;
    off = (size_t) yreal * st->finfo.line_length + (size_t) xreal * bytespp;
    if (off + (size_t) bytespp > st->fb_len)
      return;

    dst = st->fb + off;
    switch (bytespp)
      {
      case 1:
        dst[0] = (uint8_t) rgb;
        break;
      case 2:
        dst[0] = (uint8_t) (rgb & 0xff);
        dst[1] = (uint8_t) ((rgb >> 8) & 0xff);
        break;
      case 3:
        dst[0] = (uint8_t) (rgb & 0xff);
        dst[1] = (uint8_t) ((rgb >> 8) & 0xff);
        dst[2] = (uint8_t) ((rgb >> 16) & 0xff);
        break;
      default:
        dst[0] = (uint8_t) (rgb & 0xff);
        dst[1] = (uint8_t) ((rgb >> 8) & 0xff);
        dst[2] = (uint8_t) ((rgb >> 16) & 0xff);
        dst[3] = (uint8_t) ((rgb >> 24) & 0xff);
        break;
      }
  }
}

static void
fbdev_pick_colors (int attr, uint32_t *fg, uint32_t *bg,
                   const struct fb_var_screeninfo *vinfo)
{
  uint8_t fgl = 220;
  uint8_t bgl = 0;

  switch (attr)
    {
    case AA_DIM:
      fgl = 120;
      break;
    case AA_BOLD:
    case AA_BOLDFONT:
    case AA_SPECIAL:
      fgl = 255;
      break;
    case AA_REVERSE:
      fgl = 0;
      bgl = 255;
      break;
    default:
      break;
    }

  *fg = fbdev_pack_rgb (vinfo, fgl, fgl, fgl);
  *bg = fbdev_pack_rgb (vinfo, bgl, bgl, bgl);
}

static int
fbdev_init (__AA_CONST struct aa_hardware_params *p, __AA_CONST void *none,
            struct aa_hardware_params *dest, void **params)
{
  const char *path;
  struct fbdev_state *st;
  __AA_CONST struct aa_font *font;

  (void) none;
  st = calloc (1, sizeof (*st));
  if (st == NULL)
    return 0;
  st->fd = -1;

  path = getenv ("AAFBDEV");
  if (path == NULL || path[0] == '\0')
    path = "/dev/fb0";

  st->fd = open (path, O_RDWR);
  if (st->fd < 0)
    goto fail;
  if (ioctl (st->fd, FBIOGET_FSCREENINFO, &st->finfo) < 0)
    goto fail;
  if (ioctl (st->fd, FBIOGET_VSCREENINFO, &st->vinfo) < 0)
    goto fail;
  if (st->vinfo.bits_per_pixel != 1 && st->vinfo.bits_per_pixel != 8
      && st->vinfo.bits_per_pixel != 16
      && st->vinfo.bits_per_pixel != 24 && st->vinfo.bits_per_pixel != 32)
    goto fail;

  st->fb_len = st->finfo.smem_len;
  if (st->fb_len == 0)
    st->fb_len = (size_t) st->finfo.line_length * st->vinfo.yres_virtual;
  st->fb
      = (unsigned char *) mmap (NULL, st->fb_len, PROT_READ | PROT_WRITE,
                                MAP_SHARED, st->fd, 0);
  if (st->fb == MAP_FAILED)
    goto fail;

  font = (p != NULL && p->font != NULL) ? p->font : &aa_font8;
  if (font->height <= 0)
    goto fail;

  st->font_h = font->height;
  st->chars_w = (int) st->vinfo.xres / 8;
  st->chars_h = (int) st->vinfo.yres / st->font_h;
  if (st->chars_w <= 0 || st->chars_h <= 0)
    goto fail;

  st->cursor_visible = 1;
  if (p != NULL)
    *dest = *p;
  else
    memset (dest, 0, sizeof (*dest));
  dest->font = font;
  dest->supported = AA_NORMAL_MASK | AA_DIM_MASK | AA_BOLD_MASK
                    | AA_BOLDFONT_MASK | AA_REVERSE_MASK;
  *params = st;
  aa_recommendlowkbd ("stdin");
  return 1;

fail:
  if (st->fb != NULL && st->fb != MAP_FAILED)
    munmap (st->fb, st->fb_len);
  if (st->fd >= 0)
    close (st->fd);
  free (st);
  return 0;
}

static void
fbdev_uninit (aa_context *c)
{
  struct fbdev_state *st = (struct fbdev_state *) c->driverdata;
  if (st == NULL)
    return;
  if (st->fb != NULL && st->fb != MAP_FAILED)
    munmap (st->fb, st->fb_len);
  if (st->fd >= 0)
    close (st->fd);
  free (st);
  c->driverdata = NULL;
}

static void
fbdev_getsize (aa_context *c, int *width, int *height)
{
  struct fbdev_state *st = (struct fbdev_state *) c->driverdata;
  *width = st->chars_w;
  *height = st->chars_h;
}

static void
fbdev_gotoxy (aa_context *c, int x, int y)
{
  struct fbdev_state *st = (struct fbdev_state *) c->driverdata;
  st->cursor_x = x;
  st->cursor_y = y;
}

static void
fbdev_cursor (aa_context *c, int mode)
{
  struct fbdev_state *st = (struct fbdev_state *) c->driverdata;
  st->cursor_visible = mode;
}

static void
fbdev_flush (aa_context *c)
{
  struct fbdev_state *st = (struct fbdev_state *) c->driverdata;
  const unsigned char *font = c->params.font->data;
  int cw = aa_scrwidth (c);
  int ch = aa_scrheight (c);
  int x, y, gy, gx;

  for (y = 0; y < ch; y++)
    {
      for (x = 0; x < cw; x++)
        {
          int idx = x + y * cw;
          int chv = c->textbuffer[idx] & 0xff;
          int attr = c->attrbuffer[idx];
          const unsigned char *glyph = font + chv * st->font_h;
          uint32_t fg, bg;
          fbdev_pick_colors (attr, &fg, &bg, &st->vinfo);

          for (gy = 0; gy < st->font_h; gy++)
            {
              unsigned char row = glyph[gy];
              int py = y * st->font_h + gy;
              for (gx = 0; gx < 8; gx++)
                {
                  int px = x * 8 + gx;
                  uint32_t color = (row & (0x80 >> gx)) ? fg : bg;
                  fbdev_putpixel (st, px, py, color);
                }
            }
        }
    }

  if (st->cursor_visible && st->cursor_x >= 0 && st->cursor_x < cw
      && st->cursor_y >= 0 && st->cursor_y < ch)
    {
      int py = st->cursor_y * st->font_h + (st->font_h - 1);
      uint32_t fg = fbdev_pack_rgb (&st->vinfo, 255, 255, 255);
      for (gx = 0; gx < 8; gx++)
        fbdev_putpixel (st, st->cursor_x * 8 + gx, py, fg);
    }
}

__AA_CONST struct aa_driver fbdev_d = {
  "fbdev", "Linux framebuffer driver 1.0", fbdev_init, fbdev_uninit,
  fbdev_getsize, NULL, NULL, fbdev_gotoxy, fbdev_flush, fbdev_cursor
};
#endif
