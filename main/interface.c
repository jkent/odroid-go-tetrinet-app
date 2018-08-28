#include <errno.h>
#include <stdio.h>
#include <sys/select.h>

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

#include "client.h"
#include "display.h"
#include "event.h"
#include "graphics.h"
#include "io.h"
#include "OpenSans_Regular_11X12.h"
#include "tetrinet.h"
#include "tetris.h"
#include "tiles_image.h"
#include "tf.h"

static void tf_draw_str_shadow(gbuf_t *g, tf_t *tf, const char *s, point_t p, short shadow_depth)
{
    p.x += shadow_depth;
    p.x += shadow_depth;
    tf->color = ~tf->color; 
    tf_draw_str(fb, tf, s, p);
    p.x -= shadow_depth;
    p.y -= shadow_depth;
    tf->color = ~tf->color; 
    tf_draw_str(fb, tf, s, p);
}

static void draw_border(gbuf_t *g, rect_t r, bool flat)
{
    if (flat) {
        draw_rectangle(fb, r, DRAW_STYLE_SOLID, 0xFFFF);
    } else {
        draw_rectangle3d(fb, r, 0x632c, 0xce79);
    }

    r.x += 1;
    r.y += 1;
    r.width -= 2;
    r.height -= 2;
    if (flat) {
        draw_rectangle(fb, r, DRAW_STYLE_SOLID, 0xFFFF);
    } else {
        draw_rectangle3d(fb, r, 0x3186, 0x9cd3);
    }
}

static void screen_setup(void)
{
    rect_t r = {
        .x = 0,
        .y = 0,
        .width = fb->width,
        .height = fb->height,
    };
    fill_rectangle(fb, r, 0x7bef);
    display_update();
}

static void draw_text(int bufnum, const char *s)
{
    /* do nothing */
}

static void clear_text(int bufnum)
{
    /* do nothing */
}

static bool skip_update = false;
static int selected_player = 0;

static void draw_own_field(void);
static void draw_other_field(int player);
static void draw_status(void);
static void draw_specials(void);

static void setup_fields(void)
{
    tf_t *tf = tf_new(&font_OpenSans_Regular_11X12, 0xFFFF, 0, TF_ELIDE);
    rect_t r;

    skip_update = true;

    /* Own field */
    r.x = 8;
    r.y = 1;
    r.width = 10 * FIELD_WIDTH + 4,
    r.height = tf->font->height + 1;
    fill_rectangle(fb, r, 0x7bef);

    point_t p = {
        .x = r.x,
        .y = r.y + 1,
    };
    tf->width = r.width - 1;
    tf_draw_str_shadow(fb, tf, players[my_playernum - 1], p, 1);

    r.y += 13;
    r.height = 10 * FIELD_HEIGHT + 4;
    draw_border(fb, r, selected_player == my_playernum);

    draw_own_field();

    /* Other fields */
    for (int player = 1; player <= 6; player++) {
        if (player == my_playernum) {
            continue;
        }

        int field = player - 1;
        if (field >= my_playernum) {
            field -= 1;
        }

        r.x = 140 + (field % 3) * 60 + (field / 3) * 60;
        r.y = 1 + (field / 3) * 108;
        r.width = 4 * FIELD_WIDTH + 4;
        r.height = tf->font->height + 1;
        fill_rectangle(fb, r, 0x7bef);

        p.x = r.x;
        p.y = r.y + 1;
        tf->width = r.width - 1;
        tf_draw_str_shadow(fb, tf, players[player - 1], p, 1);

        r.y += 13;
        r.height = 4 * FIELD_HEIGHT + 4;
        draw_border(fb, r, selected_player == player);

        draw_other_field(player);
    }

    /* Next box */
    p.x = 140;
    p.y = 110;
    tf->width = 51;
    tf_draw_str_shadow(fb, tf, "Next:", p, 1);

    r.x = p.x;
    r.y = p.y + tf->font->height;
    r.width = 52;
    r.height = 52;
    draw_border(fb, r, false);

    draw_status();

    /* Specials box */
    p.x = 139;
    p.y = 212;
    tf->width = 138;
    tf_draw_str_shadow(fb, tf, "Specials:", p, 1);

    r.x = p.x;
    r.y = p.y + tf->font->height;
    r.width = 174;
    r.height = 14;
    draw_border(fb, r, false);

    draw_specials();
    skip_update = false;

    tf_free(tf);
    display_update();
}

static void draw_own_field(void)
{
    rect_t r = {
        .x = 10,
        .y = 16,
        .width = 10,
        .height = 10 * FIELD_HEIGHT,
    };
    for (int i = 0; i < FIELD_WIDTH; i++) {
        fill_rectangle(fb, r, i % 2 ? 0x18C3 : 0x0000);
        r.x += 10;
    }

    Field *f = &fields[my_playernum - 1];
    for (int y = 0; y < FIELD_HEIGHT; y++) {
        for (int x = 0; x < FIELD_WIDTH; x++) {
            if ((*f)[y][x]) {
                rect_t dst_r = {
                    .x = 10 + x * 10,
                    .y = 16 + y * 10,
                    .width = 10,
                    .height = 10,
                };
                rect_t src_r = {
                    .x = ((*f)[y][x] - 1) * 10,
                    .y = 0,
                    .width = 10,
                    .height = 10,
                };
                blit(fb, dst_r, &tiles_image, src_r);
            }
        }
    }

    r.x = 10;
    r.width = 10 * FIELD_WIDTH;

    if (!skip_update) {
        display_update_rect(r);
    }
}

static void draw_other_field(int player)
{
    int field = player - 1;
    if (field >= my_playernum) {
        field -= 1;
    }

    rect_t r = {
        .x = 142 + (field % 3) * 60 + (field / 3) * 60,
        .y = 16 + (field / 3) * 108,
        .width = 4 * FIELD_WIDTH,
        .height = 4 * FIELD_HEIGHT,
    };
    fill_rectangle(fb, r, 0x0000);

    Field *f = &fields[player - 1];
    for (int y = 0; y < FIELD_HEIGHT; y++) {
        for (int x = 0; x < FIELD_WIDTH; x++) {
            if ((*f)[y][x]) {
                rect_t dst_r = {
                    .x = 142 + (field % 3 + field / 3) * 60 + x * 4,
                    .y = 16 + (field / 3) * 108 + y * 4,
                    .width = 4,
                    .height = 4,
                };
                rect_t src_r = {
                    .x = ((*f)[y][x] - 1) * 4,
                    .y = 10,
                    .width = 4,
                    .height = 4,
                };
                blit(fb, dst_r, &tiles_image, src_r);
            }
        }
    }

    if (!skip_update) {
        display_update_rect(r);
    }
}

static void draw_status(void)
{
    rect_t r = {
        .x = 142,
        .y = 124,
        .width = 48,
        .height = 48,
    };
    fill_rectangle(fb, r, 0x0000);

    r.x = 140;
    r.y = 174;
    r.width = 52,
    r.height = 37;
    fill_rectangle(fb, r, 0x7bef);

    if (!playing) {
        return;
    }

    tf_t *tf = tf_new(&font_OpenSans_Regular_11X12, 0xFFFF, 0, TF_ELIDE);
    point_t p = {
        .x = r.x,
        .y = r.y + 6,
    };
    tf->width = r.width - 1;
    char buf[12];
    snprintf(buf, sizeof(buf), "Lines: %d", lines > 9999 ? 9999 : lines);
    tf_draw_str_shadow(fb, tf, buf, p, 1);
    p.y += tf->font->height;
    snprintf(buf, sizeof(buf), "Level: %d", levels[my_playernum]);
    tf_draw_str_shadow(fb, tf, buf, p, 1);

    char shape[4][4];
    if (get_shape(next_piece, 0, shape) == 0) {
        for (int y = 0; y < 4; y++) {
            for (int x = 0; x < 4; x++) {
                if (shape[y][x]) { 
                    rect_t dst_r = {
                        .x = 146 + x * 10,
                        .y = 128 + y * 10,
                        .width = 10,
                        .height = 10,
                    };
                    rect_t src_r = {
                        .x = (shape[y][x] - 1) * 10,
                        .y = 0,
                        .width = 10,
                        .height = 10,
                    };
                    blit(fb, dst_r, &tiles_image, src_r);
                }
            }
        }
    }
    tf_free(tf);

    r.x = 140;
    r.y = 124;
    r.width = 52;
    r.height = 89;

    if (!skip_update) {
        display_update_rect(r);
    }
}

static void draw_specials(void)
{
    rect_t r = {
        .x = 141,
        .y = 226,
        .width = 170,
        .height = 10,
    };
    fill_rectangle(fb, r, 0x0000);

    if (!playing) {
        return;
    }

    for (int i = 0; i < special_capacity && specials[i] >= 0 && i < 17; i++) {
        rect_t dst_r = {
            .x = 141 + i * 10,
            .y = 226,
            .width = 10,
            .height = 10,
        };
        rect_t src_r = {
            .x = (specials[i] + 5) * 10,
            .y = 0,
            .width = 10,
            .height = 10,
        };
        blit(fb, dst_r, &tiles_image, src_r);
    }

    if (!skip_update) {
        display_update_rect(r);
    }
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
