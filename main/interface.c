#include <errno.h>
#include <stdio.h>
#include <sys/select.h>

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

#include "io.h"
#include "client.h"
#include "event.h"
#include "tetrinet.h"


static void screen_setup(void)
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


Interface odroid_go_interface = {
    screen_setup,

    draw_text,
    clear_text,

    setup_fields,
    draw_own_field,
    draw_other_field,
    draw_status,
    draw_specials,
    draw_attdef,
};
