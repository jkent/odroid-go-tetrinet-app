/* Tetrinet for Linux, by Andrew Church <achurch@achurch.org>
 * This program is public domain.
 *
 * Input/output interface declaration and constant definitions.
 */

#ifndef IO_H
#define IO_H

/* Text buffers: */
#define BUFFER_PLINE	0
#define BUFFER_GMSG	1
#define BUFFER_ATTDEF	2

typedef struct {

    /**** Input routine. ****/

    /* Wait for input and return either an ASCII code, a K_* value, -1 if
     * server input is waiting, or -2 if we time out. */
    int (*wait_for_input)(int msec);

    /**** Output routines. ****/

    /* Initialize for output. */
    void (*screen_setup)(void);
    /* Redraw the screen. */
    void (*screen_refresh)(void);
    /* Redraw the screen after clearing it. */
    void (*screen_redraw)(void);

    /* Draw text into the given buffer. */
    void (*draw_text)(int bufnum, const char *s);
    /* Clear the given text buffer. */
    void (*clear_text)(int bufnum);

    /* Set up the fields display. */
    void (*setup_fields)(void);
    /* Draw our own field. */
    void (*draw_own_field)(void);
    /* Draw someone else's field. */
    void (*draw_other_field)(int player);
    /* Draw the game status information. */
    void (*draw_status)(void);
    /* Draw specials stuff */
    void (*draw_specials)(void);
    /* Write a text string for usage of a special. */
    void (*draw_attdef)(const char *type, int from, int to);
    /* Draw the game message input window. */
    void (*draw_gmsg_input)(const char *s, int pos);
    /* Clear the game message input window. */
    void (*clear_gmsg_input)(void);

    /* Set up the partyline display. */
    void (*setup_partyline)(void);
    /* Draw the partyline input string with the cursor at the given position. */
    void (*draw_partyline_input)(const char *s, int pos);

    /* Set up the winlist display. */
    void (*setup_winlist)(void);

} Interface;

extern Interface odroid_go_interface;

#endif	/* IO_H */
