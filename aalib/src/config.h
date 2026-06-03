/*
 * aalib config.h — minimal static configuration.
 *
 * Only stdout/stderr display backends and stdin keyboard backend are enabled.
 * All other drivers (X11, curses, slang, linux console, gpm mouse, DOS, OS/2)
 * are intentionally disabled.
 */

#ifndef AALIB_CONFIG_H
#define AALIB_CONFIG_H

#define PACKAGE "aalib"
#define VERSION "1.4.0"

#define RETSIGTYPE void
#define STDC_HEADERS 1
#define TIME_WITH_SYS_TIME 1

#define HAVE_FCNTL_H 1
#define HAVE_LIMITS_H 1
#define HAVE_MALLOC_H 1
#define HAVE_SYS_IOCTL_H 1
#define HAVE_SYS_TIME_H 1
#define HAVE_UNISTD_H 1

#define HAVE_STRDUP 1
#define HAVE_LIBM 1

#endif /* AALIB_CONFIG_H */
