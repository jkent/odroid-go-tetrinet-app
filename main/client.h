/* Tetrinet for Linux, by Andrew Church <achurch@achurch.org>
 * This program is public domain.
 *
 * Tetrinet main include file.
 */

#ifndef CLIENT_H
#define CLIENT_H

#ifndef IO_H
# include "io.h"
#endif

/*************************************************************************/

/* Key definitions for input.  We use K_* to avoid conflict with ncurses */

#define K_UP		0x100
#define K_DOWN		0x101
#define K_LEFT		0x102
#define K_RIGHT		0x103
#define K_F1		0x104
#define K_F2		0x105
#define K_F3		0x106
#define K_F4		0x107
#define K_F5		0x108
#define K_F6		0x109
#define K_F7		0x10A
#define K_F8		0x10B
#define K_F9		0x10C
#define K_F10		0x10D
#define K_F11		0x10E
#define K_F12		0x10F

/* For function keys that don't correspond to something above, i.e. that we
 * don't care about: */
#define K_INVALID	0x7FFF

/*************************************************************************/

/* Externs */

extern int windows_mode;
extern int noslide;
extern int tetrifast;
extern int cast_shadow;

extern int my_playernum;
extern int sock;
extern char *players[6];
extern char *teams[6];
extern int  playing;
extern int  spectating;
extern int  paused;

extern Interface *io;

/*************************************************************************/

extern void client_main(void *arg);

/*************************************************************************/

#endif
