/* Tetrinet for Linux, by Andrew Church <achurch@achurch.org>
 * This program is public domain.
 *
 * Tetrinet main include file.
 */

#ifndef TETRINET_H
#define TETRINET_H

/*************************************************************************/

/* Basic types */

#define FIELD_WIDTH	12
#define FIELD_HEIGHT	22
typedef char Field[FIELD_HEIGHT][FIELD_WIDTH];

typedef struct {
    char name[32];
    int team;	/* 0 = individual player, 1 = team */
    int points;
    int games;	/* Number of games played */
} WinInfo;
#define MAXWINLIST	64	/* Maximum size of winlist */
#define MAXSENDWINLIST	10	/* Maximum number of winlist entries to send
				 *    (this avoids triggering a buffer
				 *    overflow in Windows Tetrinet 1.13) */
#define MAXSAVEWINLIST	32	/* Maximum number of winlist entries to save
				 *    (this allows new players to get into
				 *    a winlist with very high scores) */

/*************************************************************************/

#endif
