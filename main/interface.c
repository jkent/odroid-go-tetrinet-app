#include <errno.h>
#include <stdio.h>
#include <sys/select.h>
#include "io.h"
#include "tetrinet.h"
#include "client.h"

static int wait_for_input(int msec)
{
    fd_set fds;
    struct timeval tv;

    FD_ZERO(&fds);
    FD_SET(sock, &fds);
    tv.tv_sec = msec / 1000;
    tv.tv_usec = (msec * 1000) % 1000000;
    while (select(sock + 1, &fds, NULL, NULL, msec < 0 ? NULL : &tv) < 0) {
        if (errno != EINTR) {
            perror("Warning: select() failed");
        }
    }
    if (FD_ISSET(sock, &fds)) {
        return -1;
    } else {
        return -2; /* out of time */
    }
}

static void screen_setup(void)
{
    /* do nothing */
}

static void screen_refresh(void)
{
    /* do nothing */
}

static void screen_redraw(void)
{
    /* do nothing */
}

static void draw_text(int bufnum, const char *s)
{
    /* do nothing */
}

static void clear_text(int bufnum)
{
    /* do nothing */
}

static void setup_fields(void)
{
    /* do nothing */
}

static void draw_own_field(void)
{
    /* do nothing */
}

static void draw_other_field(int player)
{
    /* do nothing */
}

static void draw_status(void)
{
    /* do nothing */
}

static void draw_specials(void)
{
    /* do nothing */
}

static void draw_attdef(const char *type, int from, int to)
{
    /* do nothing */
}

static void draw_gmsg_input(const char *s, int pos)
{
    /* do nothing */
}

static void clear_gmsg_input(void)
{
    /* do nothing */
}

static void setup_partyline(void)
{
    /* do nothing */
}

static void draw_partyline_input(const char *s, int pos)
{
    /* do nothing */
}

static void setup_winlist(void)
{
    /* do nothing */
}

Interface odroid_go_interface = {
    wait_for_input,

    screen_setup,
    screen_refresh,
    screen_redraw,

    draw_text,
    clear_text,

    setup_fields,
    draw_own_field,
    draw_other_field,
    draw_status,
    draw_specials,
    draw_attdef,
    draw_gmsg_input,
    clear_gmsg_input,

    setup_partyline,
    draw_partyline_input,

    setup_winlist,
};
