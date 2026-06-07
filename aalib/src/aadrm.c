#include "config.h"
#ifdef DRM_DRIVER
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>
#include <xf86drm.h>
#include <xf86drmMode.h>
#include <drm.h>
#include <drm_mode.h>
#include "utf8.h"
#include "aalib.h"
#include "aaint.h"
#include "aaglyph.h"

struct drm_state
{
  int fd;
  uint32_t conn_id;
  uint32_t crtc_id;
  drmModeModeInfo mode;
  uint32_t fb_id;
  uint32_t handle;
  unsigned char *fb;
  size_t fb_len;
  uint32_t pitch;
  uint32_t width;
  uint32_t height;
  int chars_w;
  int chars_h;
  int font_h;
  int cursor_x;
  int cursor_y;
  int cursor_visible;
  drmModeCrtc *saved_crtc;
};

static uint32_t
drm_pack_rgb (uint8_t r, uint8_t g, uint8_t b)
{
  return ((uint32_t) r << 16) | ((uint32_t) g << 8) | (uint32_t) b;
}

static void
drm_putpixel (struct drm_state *st, int x, int y, uint32_t rgb)
{
  uint32_t *dst;

  if (x < 0 || y < 0 || (uint32_t) x >= st->width || (uint32_t) y >= st->height)
    return;

  dst = (uint32_t *) (st->fb + (size_t) y * st->pitch + (size_t) x * 4);
  *dst = rgb;
}

static void
drm_pick_colors (int attr, uint32_t *fg, uint32_t *bg)
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

  *fg = drm_pack_rgb (fgl, fgl, fgl);
  *bg = drm_pack_rgb (bgl, bgl, bgl);
}

static int
drm_find_connector (int fd, uint32_t *conn_id, drmModeModeInfo *mode)
{
  drmModeRes *res;
  int i;

  res = drmModeGetResources (fd);
  if (res == NULL)
    return 0;

  for (i = 0; i < res->count_connectors; i++)
    {
      drmModeConnector *conn =
          drmModeGetConnector (fd, res->connectors[i]);
      if (conn == NULL)
        continue;
      if (conn->connection == DRM_MODE_CONNECTED && conn->count_modes > 0)
        {
          *conn_id = conn->connector_id;
          *mode = conn->modes[0];
          drmModeFreeConnector (conn);
          drmModeFreeResources (res);
          return 1;
        }
      drmModeFreeConnector (conn);
    }

  drmModeFreeResources (res);
  return 0;
}

static uint32_t
drm_find_crtc (int fd, uint32_t conn_id, const drmModeModeInfo *mode)
{
  drmModeConnector *conn;
  drmModeEncoder *enc;
  uint32_t crtc_id = 0;

  conn = drmModeGetConnector (fd, conn_id);
  if (conn == NULL)
    return 0;

  if (conn->encoder_id)
    {
      enc = drmModeGetEncoder (fd, conn->encoder_id);
      if (enc != NULL)
        {
          crtc_id = enc->crtc_id;
          drmModeFreeEncoder (enc);
        }
    }
  else
    {
      drmModeRes *res = drmModeGetResources (fd);
      int i, j;
      if (res != NULL)
        {
          for (i = 0; i < conn->count_encoders; i++)
            {
              enc = drmModeGetEncoder (fd, conn->encoders[i]);
              if (enc == NULL)
                continue;
              for (j = 0; j < res->count_crtcs; j++)
                {
                  if (enc->possible_crtcs & (1 << j))
                    {
                      crtc_id = res->crtcs[j];
                      break;
                    }
                }
              drmModeFreeEncoder (enc);
              if (crtc_id)
                break;
            }
          drmModeFreeResources (res);
        }
    }

  (void) mode;
  drmModeFreeConnector (conn);
  return crtc_id;
}

static int
drm_try_open (const char *path, int *fd_out, uint32_t *conn_id,
              drmModeModeInfo *mode)
{
  int fd;
  uint64_t cap;

  fd = open (path, O_RDWR | O_CLOEXEC);
  if (fd < 0)
    return 0;

  if (drmGetCap (fd, DRM_CAP_DUMB_BUFFER, &cap) < 0 || cap == 0)
    {
      close (fd);
      return 0;
    }

  if (!drm_find_connector (fd, conn_id, mode))
    {
      close (fd);
      return 0;
    }

  *fd_out = fd;
  return 1;
}

static int
drm_open_device (struct drm_state *st)
{
  const char *path;
  char devpath[32];
  int i;

  path = getenv ("AADRM");
  if (path != NULL && path[0] != '\0')
    return drm_try_open (path, &st->fd, &st->conn_id, &st->mode);

  for (i = 0; i < 16; i++)
    {
      snprintf (devpath, sizeof (devpath), "/dev/dri/card%d", i);
      if (drm_try_open (devpath, &st->fd, &st->conn_id, &st->mode))
        return 1;
    }

  return 0;
}

static int
drm_init (__AA_CONST struct aa_hardware_params *p, __AA_CONST void *none,
          struct aa_hardware_params *dest, void **params)
{
  struct drm_state *st;
  struct drm_mode_create_dumb create;
  struct drm_mode_map_dumb map;
  __AA_CONST struct aa_font *font;
  uint32_t x, y;

  (void) none;
  st = calloc (1, sizeof (*st));
  if (st == NULL)
    return 0;
  st->fd = -1;

  if (!drm_open_device (st))
    goto fail;

  st->crtc_id = drm_find_crtc (st->fd, st->conn_id, &st->mode);
  if (st->crtc_id == 0)
    goto fail;

  st->saved_crtc = drmModeGetCrtc (st->fd, st->crtc_id);
  if (st->saved_crtc == NULL)
    goto fail;

  memset (&create, 0, sizeof (create));
  create.width = st->mode.hdisplay;
  create.height = st->mode.vdisplay;
  create.bpp = 32;
  if (drmIoctl (st->fd, DRM_IOCTL_MODE_CREATE_DUMB, &create) < 0)
    goto fail;

  st->handle = create.handle;
  st->pitch = create.pitch;
  st->width = create.width;
  st->height = create.height;
  st->fb_len = create.size;

  memset (&map, 0, sizeof (map));
  map.handle = create.handle;
  if (drmIoctl (st->fd, DRM_IOCTL_MODE_MAP_DUMB, &map) < 0)
    goto fail;

  st->fb = (unsigned char *) mmap (NULL, st->fb_len, PROT_READ | PROT_WRITE,
                                   MAP_SHARED, st->fd, map.offset);
  if (st->fb == MAP_FAILED)
    goto fail;

  if (drmModeAddFB (st->fd, st->width, st->height, 24, 32, st->pitch,
                    st->handle, &st->fb_id)
      < 0)
    goto fail;

  if (drmSetMaster (st->fd) < 0)
    goto fail;

  if (drmModeSetCrtc (st->fd, st->crtc_id, st->fb_id, 0, 0, &st->conn_id, 1,
                      &st->mode)
      < 0)
    goto fail;

  for (y = 0; y < st->height; y++)
    for (x = 0; x < st->width; x++)
      drm_putpixel (st, (int) x, (int) y, drm_pack_rgb (0, 0, 0));

  font = (p != NULL && p->font != NULL) ? p->font : &aa_font8;
  if (font->height <= 0)
    goto fail;

  st->font_h = font->height;
  st->chars_w = (int) st->width / 8;
  st->chars_h = (int) st->height / st->font_h;
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
  if (st->fb_id)
    drmModeRmFB (st->fd, st->fb_id);
  if (st->handle)
    {
      struct drm_mode_destroy_dumb destroy;
      memset (&destroy, 0, sizeof (destroy));
      destroy.handle = st->handle;
      drmIoctl (st->fd, DRM_IOCTL_MODE_DESTROY_DUMB, &destroy);
    }
  if (st->saved_crtc != NULL)
    {
      drmModeSetCrtc (st->fd, st->saved_crtc->crtc_id,
                      st->saved_crtc->buffer_id, st->saved_crtc->x,
                      st->saved_crtc->y, &st->conn_id, 1,
                      &st->saved_crtc->mode);
      drmModeFreeCrtc (st->saved_crtc);
    }
  if (st->fd >= 0)
    close (st->fd);
  free (st);
  return 0;
}

static void
drm_uninit (aa_context *c)
{
  struct drm_state *st = (struct drm_state *) c->driverdata;
  if (st == NULL)
    return;

  if (st->saved_crtc != NULL)
    {
      drmModeSetCrtc (st->fd, st->saved_crtc->crtc_id,
                      st->saved_crtc->buffer_id, st->saved_crtc->x,
                      st->saved_crtc->y, &st->conn_id, 1,
                      &st->saved_crtc->mode);
      drmModeFreeCrtc (st->saved_crtc);
    }
  if (st->fb != NULL && st->fb != MAP_FAILED)
    munmap (st->fb, st->fb_len);
  if (st->fb_id)
    drmModeRmFB (st->fd, st->fb_id);
  if (st->handle)
    {
      struct drm_mode_destroy_dumb destroy;
      memset (&destroy, 0, sizeof (destroy));
      destroy.handle = st->handle;
      drmIoctl (st->fd, DRM_IOCTL_MODE_DESTROY_DUMB, &destroy);
    }
  if (st->fd >= 0)
    {
      drmDropMaster (st->fd);
      close (st->fd);
    }
  free (st);
  c->driverdata = NULL;
}

static void
drm_getsize (aa_context *c, int *width, int *height)
{
  struct drm_state *st = (struct drm_state *) c->driverdata;
  *width = st->chars_w;
  *height = st->chars_h;
}

static void
drm_gotoxy (aa_context *c, int x, int y)
{
  struct drm_state *st = (struct drm_state *) c->driverdata;
  st->cursor_x = x;
  st->cursor_y = y;
}

static void
drm_cursor (aa_context *c, int mode)
{
  struct drm_state *st = (struct drm_state *) c->driverdata;
  st->cursor_visible = mode;
}

static void
drm_putpixel_cb (void *ctx, int px, int py, uint32_t rgb)
{
  drm_putpixel ((struct drm_state *) ctx, px, py, rgb);
}

static void
drm_flush (aa_context *c)
{
  struct drm_state *st = (struct drm_state *) c->driverdata;
  int cw = aa_scrwidth (c);
  int ch = aa_scrheight (c);
  int x, y;

  for (y = 0; y < ch; y++)
    {
      for (x = 0; x < cw; x++)
        {
          int idx = x + y * cw;
          uint32_t cp = c->glyphbuffer[idx];
          int attr = c->attrbuffer[idx];
          uint32_t fg, bg;

          drm_pick_colors (attr, &fg, &bg);
          if (cp == AA_GLYPH_WIDE_PAD)
            continue;
          aa_draw_cell (c, x * 8, y * st->font_h, cp, 8, st->font_h, fg, bg,
                        drm_putpixel_cb, st);
        }
    }

  if (st->cursor_visible && st->cursor_x >= 0 && st->cursor_x < cw
      && st->cursor_y >= 0 && st->cursor_y < ch)
    {
      int py = st->cursor_y * st->font_h + (st->font_h - 1);
      int gx;
      uint32_t fg = drm_pack_rgb (255, 255, 255);
      for (gx = 0; gx < 8; gx++)
        drm_putpixel (st, st->cursor_x * 8 + gx, py, fg);
    }
}

__AA_CONST struct aa_driver drm_d = {
  "drm", "Linux DRM/KMS driver 1.0", drm_init, drm_uninit,
  drm_getsize, NULL, NULL, drm_gotoxy, drm_flush, drm_cursor
};
#endif
