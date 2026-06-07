#include "config.h"
#include <stdio.h>
#include "utf8.h"
#include "aalib.h"
#include "aaint.h"

static int stdout_init(__AA_CONST struct aa_hardware_params *p,__AA_CONST  void *none, struct aa_hardware_params *dest, void **n)
{
    __AA_CONST static struct aa_hardware_params def={NULL, AA_NORMAL_MASK | AA_EXTENDED};
    *dest=def;
    return 1;
}
static void stdout_uninit(aa_context * c)
{
}
static void stdout_getsize(aa_context * c, int *width, int *height)
{
}

static void stdout_emit_row(aa_context *c, FILE *out)
{
    int x, y;
    char buf[8];

    for (y = 0; y < aa_scrheight(c); y++) {
	for (x = 0; x < aa_scrwidth(c); x++) {
	    int idx = x + y * aa_scrwidth(c);
	    uint32_t cp = c->glyphbuffer[idx];
	    int n;
	    if (cp == AA_GLYPH_WIDE_PAD)
		continue;
	    n = utf8_encode(cp, buf);
	    fwrite(buf, 1, (size_t) n, out);
	}
	putc('\n', out);
    }
}

static void stdout_flush(aa_context * c)
{
    stdout_emit_row(c, stdout);
    putc('\f', stdout);
    putc('\n', stdout);
    fflush(stdout);
}
static void stdout_gotoxy(aa_context * c, int x, int y)
{
}
__AA_CONST struct aa_driver stdout_d =
{
    "stdout", "Standard output driver",
    stdout_init,
    stdout_uninit,
    stdout_getsize,
    NULL,
    NULL,
    stdout_gotoxy,
    stdout_flush,
    NULL
};


static void stderr_flush(aa_context * c)
{
    stdout_emit_row(c, stderr);
    putc('\f', stderr);
    putc('\n', stderr);
    fflush(stderr);
}
__AA_CONST struct aa_driver stderr_d =
{
    "stderr", "Standard error driver",
    stdout_init,
    stdout_uninit,
    stdout_getsize,
    NULL,
    NULL,
    stdout_gotoxy,
    stderr_flush,
    NULL
};
