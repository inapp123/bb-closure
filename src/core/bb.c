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
#include "image.h"
#include <aalib.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

int finish_stuff, starttime, endtime;
int dual = 0;
static int quitnow = 0;
int loopmode;
aa_context *context;
aa_renderparams *params;
int TIME;
tl_timer *scenetimer;
struct font *font;

double getwidth(double size) {
  double height = aa_imgheight(context) / size;
  double width = height * (double)aa_imgwidth(context) * 0.75 /
                 aa_imgheight(context) * aa_mmheight(context) /
                 aa_mmwidth(context);
  return (width);
}

void centerprint(int x, int y, double size, int color, char *text, int mode) {
  if (!dual || !mode) {
    double height = aa_imgheight(context) / size;
    double width = height * (double)aa_imgwidth(context) * 0.75 /
                   aa_imgheight(context) * aa_mmheight(context) /
                   aa_mmwidth(context);
    print(x - (width * strlen(text)) / 2, y - height / 2, width, height, font,
          color, text);
  } else {
    if (mode & 1) {
      double height = aa_imgheight(context) / size;
      double width = height * (double)aa_imgwidth(context) * 0.75 /
                     aa_imgheight(context) * aa_mmheight(context) /
                     aa_mmwidth(context);
      print(x / 2 - (width * strlen(text)) / 2, y - height / 2, width, height,
            font, color, text);
    }
    if (mode & 2) {
      double height = aa_imgheight(context) / size;
      double width = height * (double)aa_imgwidth(context) * 0.75 /
                     aa_imgheight(context) * aa_mmheight(context) /
                     aa_mmwidth(context);
      print(aa_imgwidth(context) / 2 + x / 2 - (width * strlen(text)) / 2,
            y - height / 2, width, height, font, color, text);
    }
  }
}

void centerprinth(int x, int y, double size, int color, char *text, int mode) {
  if (!mode || !dual) {
    double width = aa_imgwidth(context) / size;
    double height = width * (double)aa_imgheight(context) * 1.333 /
                    aa_imgwidth(context) * aa_mmwidth(context) /
                    aa_mmheight(context);
    print(x - (width * strlen(text)) / 2, y - height / 2, width, height, font,
          color, text);
  } else {
    if (mode & 1) {
      double width = aa_imgwidth(context) / size / 2;
      double height = width * (double)aa_imgheight(context) * 1.333 /
                      aa_imgwidth(context) * aa_mmwidth(context) /
                      aa_mmheight(context);
      print(x / 2 - (width * strlen(text)) / 2, y - height / 2, width, height,
            font, color, text);
    }
    if (mode & 1) {
      double width = aa_imgwidth(context) / size / 2;
      double height = width * (double)aa_imgheight(context) * 1.333 /
                      aa_imgwidth(context) * aa_mmwidth(context) /
                      aa_mmheight(context);
      print(aa_imgwidth(context) / 2 + x / 2 - (width * strlen(text)) / 2,
            y - height / 2, width, height, font, color, text);
    }
  }
}

static void (*control1)(int);
static int called = 0;

static void mycontrol(void *data, int i) {
  called = 1;
  if (control1 != NULL)
    control1(i);
}
static void mycontrol2(void *data, int i) { ((void (*)(int))data)(i); }

int bbupdate() {
  int ch;
  tl_update_time();
  TIME = tl_lookup_timer(scenetimer);
  tl_process_group(syncgroup, NULL);
  ch = aa_getkey(context, 0);
  switch (ch) {
  case 's':
  case 'S':
  case AA_BACKSPACE:
    finish_stuff = 1;
    break;
  case AA_ESC:
  case 'q':
    finish_stuff = 1, quitnow = 1;
  }
  return (ch);
}

void timestuff(int rate, void (*control)(int), void (*draw)(void),
               int maxtime) {
  int waitmode = 0, t;
  tl_timer *timer;
  bbupdate();
  /*starttime = TIME; */
  endtime = starttime + maxtime;
  timer = tl_create_timer();
  if (control == NULL) {
    rate = -40;
  }
  if (rate < 0) {
    waitmode = 1, rate = -rate;
    control1 = control;
    tl_set_multihandler(timer, mycontrol, NULL);
  } else
    tl_set_multihandler(timer, mycontrol2, control);
  tl_set_interval(timer, 1000000 / rate);
  tl_add_timer(syncgroup, timer);
  tl_reset_timer(timer);
  tl_slowdown_timer(timer, starttime - TIME);
  if (control != NULL)
    control(1);
  while (!finish_stuff && TIME < endtime) {
    called = 0;
    bbupdate();
    t = tl_process_group(syncgroup, NULL);
    if (TIME > endtime)
      break;
    if (!called && waitmode)
      tl_sleep(t);
    else {
      if (draw != NULL)
        draw();
    }
  }
  starttime = endtime;
  tl_free_timer(timer);
}

void bbwait(int maxtime) {
  int wait;
  if (finish_stuff)
    return;
  bbupdate();
  endtime = starttime + maxtime;

  wait = endtime - TIME;
  while (wait > 0) {
    int t;
    bbupdate();
    t = tl_process_group(syncgroup, NULL);
    wait = endtime - TIME;
    if (wait < t)
      t = wait;
    tl_sleep(t);
  }
  starttime = endtime;
}

void bbflushwait(int maxtime) {
  int wait;
  if (finish_stuff)
    return;
  bbupdate();
  wait = maxtime + starttime - TIME;
  if (wait > 0) {
    aa_flush(context);
  }
  bbwait(maxtime);
}

static int stage = 0;

static int parse_stage_arg(const char *arg) {
  char *end;
  long n;

  if (!isdigit((unsigned char)arg[0]))
    return -1;
  n = strtol(arg, &end, 10);
  if (*end != '\0' || n < 0 || n > 10 || n == 9)
    return -1;
  return (int)n;
}

int bbinit(int argc, char **argv) {
  //  aa_defparams.supported|= AA_NORMAL_MASK | AA_BOLD_MASK | AA_DIM_MASK;
  aa_parseoptions(NULL, NULL, &argc, argv);
  if (argc != 1 && (argc != 2 ||
                    (strcmp(argv[1], "-loop") && parse_stage_arg(argv[1]) < 0))) {
    printf("Usage: bb [aaoptions] [-loop] [scene]\n\n");
    printf("Options:\n"
           "  -loop          play demo in infinite loop\n"
           "  scene          0 or omitted: full demo; 1-8, 10: that scene only\n\n"
           "AAlib options:\n%s\n",
           aa_help);
    exit(1);
  }
  context = aa_autoinit(&aa_defparams);
  if (!context) {
    printf("Failed to initialize aalib\n");
    exit(2);
  }
  if (!aa_autoinitkbd(context, 0)) {
    aa_close(context);
    printf("Failed to initialize keyboard\n");
    exit(3);
  }
  aa_resizehandler(context, (void (*)(aa_context *))aa_resize);
  if (argc == 2 && !strcmp(argv[1], "-loop"))
    loopmode = 1;
  else if (argc == 2)
    stage = parse_stage_arg(argv[1]);
  aa_hidecursor(context);
  return 1;
}

static void play_scene(int scene) {
  finish_stuff = 0;
  bb_reset_draw_state();
  load_song("bb.s3m");
  bbupdate();
  starttime = endtime = TIME;
  aa_resize(context);
  switch (scene) {
  case 1:
    scene1();
    break;
  case 2:
    scene2();
    break;
  case 3:
    scene3();
    break;
  case 4:
    scene4();
    break;
  case 5:
    scene5();
    break;
  case 6:
    scene6();
    break;
  case 7:
    scene7();
    break;
  case 8:
    scene8();
    break;
  case 10:
    scene10();
    break;
  }
}

static void play_full_demo(void) {
  load_song("bb.s3m");
  bbupdate();
  starttime = endtime = TIME;

  aa_resize(context);
  scene1();
  aa_resize(context);
  scene3();
  if (quitnow)
    return;
  aa_resize(context);
  play_image_carousel(&kal1, &kal2, &kal3, &kal4);
  messager("KAL'TSIT known as Kal'tsit, Ama-10, Mon3tr's keeper\n"
           "birth: classified, Rhodes Island, sex: female\n"
           "\n"
           "???? - Walked the plain before Terra had a name for it\n"
           "???? - Co-founded Babel; stood when Theresa fell\n"
           "1097 - Turned ruins into Rhodes Island's medical mandate\n"
           "1098 - Refused every physical exam offered to her\n"
           "\n"
           "2021 - Released to the roster as a six-star Medic\n"
           "\n"
           "Contact address: via Doctor, or deploy Mon3tr");
  section_transition_2();
  aa_resize(context);
  scene4();
  aa_resize(context);
  scene2();
  aa_resize(context);
  if (quitnow)
    return;
  play_image_carousel(&ami1, &ami2, &ami3, &ami4);
  messager("AMIYA known as Amiya, Rhodes Island's public face\n"
           "birth: Dec 23, Rim Billiton, sex: female\n"
           "\n"
           "???? - Taken in by Theresa; learned to lead by watching\n"
           "1094 - Stood when Babel fell; chose Rhodes Island's path\n"
           "1097 - Inherited the crown and highest executive power\n"
           "2020 - Reunited with the Doctor at Chernobog\n"
           "\n"
           "2021 - Still asking the Doctor to show her the way forward\n"
           "\n"
           "Contact address: the bridge, or leave a note for Doctor");
  section_transition_3();
  aa_resize(context);
  scene8();
  aa_resize(context);
  scene6();
  aa_resize(context);
  if (quitnow)
    return;
  aa_resize(context);
  play_image_carousel(&doc1, &doc2, &doc3, &doc4);
  messager("THE DOCTOR known as Doctor, Rhodes Island field commander\n"
           "birth: classified, unknown, sex: unknown\n"
           "\n"
           "???? - Commanded Babel's most decisive counterattacks\n"
           "???? - Slept while history moved on without memory\n"
           "2020 - Woken in Chernobog; escorted back to the ship\n"
           "2021 - Still cannot recall what mattered most\n"
           "\n"
           "2026 - Kal'tsit still schedules the physical exams\n"
           "\n"
           "Contact address: command bridge; no restrictions on channel");
  bbupdate();
  starttime = endtime = TIME;
  section_transition_1();
  aa_resize(context);
  if (quitnow)
    return;
  aa_resize(context);
  scene7();
  if (quitnow)
    return;
  aa_resize(context);
  scene5();
  if (quitnow)
    return;
  aa_resize(context);
  scene10();
  play_image_carousel(&clo1, &clo2, &clo3, &clo4);
  messager("CLOSURE known as Closure, Chief Engineer, Procurement boss\n"
           "birth: Dec 10, Kazdel, sex: female\n"
           "\n"
           "???? - Built a network in a Kazdel attic, alone\n"
           "???? - Dragged out by Theresa to fix impossible things\n"
           "1089 - Rebuilt the hull they would call Rhodes Island\n"
           "1094 - Took charge when Kal'tsit was away too long\n"
           "\n"
           "2021 - Still selling snacks through special channels\n"
           "\n"
           "Contact address: Engineering Dept.; pay your invoices first");
  aa_resize(context);
  section_transition_4();
  if (quitnow)
    return;
  aa_resize(context);
  credits();
  if (quitnow)
    return;
  if (loopmode)
    return;
  aa_resize(context);
  credits2();
}

int bb(void) {
  if (stage == 0) {
    aa_gotoxy(context, 0, 0);
    introscreen();
  }
  params = aa_getrenderparams();
  aa_render(context, params, 0, 0, 1, 1);
  font = uncompressfont(/*context->params.font */ &aa_font16);
  scenetimer = tl_create_timer();
  srand(time(NULL));
  do {
    if (stage == 0)
      play_full_demo();
    else
      play_scene(stage);
  } while (loopmode && !quitnow);
quit:;
  aa_close(context);
  return (0);
}
