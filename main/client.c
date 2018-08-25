/* Tetrinet for Linux, by Andrew Church <achurch@achurch.org>
 * This program is public domain.
 *
 * Tetrinet main program.
 */

/*************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <errno.h>

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

#include "client.h"
#include "event.h"
#include "tetrinet.h"
#include "io.h"
#include "server.h"
#include "sockets.h"
#include "tetris.h"

/*************************************************************************/

int windows_mode = 0;           /* Try to be just like the Windows version? */
int noslide = 0;                /* Disallow piece sliding? */
int tetrifast = 0;              /* TetriFast mode? */
int cast_shadow = 1;            /* Make pieces cast shadow? */

int my_playernum = -1;          /* What player number are we? */
char *my_nick;                  /* And what is our nick? */
static WinInfo winlist[MAXWINLIST]; /* Winners' list from server */
int sock;                       /* Socket for server communication */
char *players[6];               /* Player names (NULL for no such player) */
char *teams[6];                 /* Team names (NULL for not on a team) */
int playing;                    /* Are we currently playing a game? */
int spectating;                 /* Are we currently watching people play a game? */
int paused;                     /* Is the game currently paused? */

Interface *io;                  /* Input/output routines */

/*************************************************************************/
/*************************************************************************/

#ifndef SERVER_ONLY

/*************************************************************************/

/* Parse a line from the server.  Destroys the buffer it's given as a side
 * effect.
 */

void parse(char *buf)
{
    char *cmd, *s, *t, *sp;

    cmd = strtok_r(buf, " ", &sp);

    if (!cmd) {
        return;

    } else if (strcmp(cmd, "noconnecting") == 0) {
        s = strtok_r(NULL, "", &sp);
        if (!s)
            s = "Unknown";
        /* XXX not to stderr, please! -- we need to stay running w/o server */
        fprintf(stderr, "Server error: %s\n", s);
        exit(1);

    } else if (strcmp(cmd, "winlist") == 0) {
        int i = 0;

        while (i < MAXWINLIST && (s = strtok_r(NULL, " ", &sp))) {
            t = strchr(s, ';');
            if (!t)
                break;
            *t++ = 0;
            if (*s == 't')
                winlist[i].team = 1;
            else
                winlist[i].team = 0;
            s++;
            strncpy(winlist[i].name, s, sizeof(winlist[i].name) - 1);
            winlist[i].name[sizeof(winlist[i].name)] = 0;
            winlist[i].points = atoi(t);
            if ((t = strchr(t, ';')) != NULL)
                winlist[i].games = atoi(t + 1);
            i++;
        }
        if (i < MAXWINLIST)
            winlist[i].name[0] = 0;

    } else if (strcmp(cmd, tetrifast ? ")#)(!@(*3" : "playernum") == 0) {
        if ((s = strtok_r(NULL, " ", &sp)))
            my_playernum = atoi(s);
        /* Note: players[my_playernum-1] is set in init() */
        /* But that doesn't work when joining other channel. */
        players[my_playernum - 1] = strdup(my_nick);

    } else if (strcmp(cmd, "playerjoin") == 0) {
        int player;
        char buf[1024];

        s = strtok_r(NULL, " ", &sp);
        t = strtok_r(NULL, "", &sp);
        if (!s || !t)
            return;
        player = atoi(s) - 1;
        if (player < 0 || player > 5)
            return;
        players[player] = strdup(t);
        if (teams[player]) {
            free(teams[player]);
            teams[player] = NULL;
        }
        snprintf(buf, sizeof(buf), "*** %s is Now Playing", t);
        io->draw_text(BUFFER_PLINE, buf);
        io->setup_fields();

    } else if (strcmp(cmd, "playerleave") == 0) {
        int player;
        char buf[1024];

        s = strtok_r(NULL, " ", &sp);
        if (!s)
            return;
        player = atoi(s) - 1;
        if (player < 0 || player > 5 || !players[player])
            return;
        snprintf(buf, sizeof(buf), "*** %s has Left", players[player]);
        io->draw_text(BUFFER_PLINE, buf);
        free(players[player]);
        players[player] = NULL;
        io->setup_fields();

    } else if (strcmp(cmd, "team") == 0) {
        int player;
        char buf[1024];

        s = strtok_r(NULL, " ", &sp);
        t = strtok_r(NULL, "", &sp);
        if (!s)
            return;
        player = atoi(s) - 1;
        if (player < 0 || player > 5 || !players[player])
            return;
        if (teams[player])
            free(teams[player]);
        if (t)
            teams[player] = strdup(t);
        else
            teams[player] = NULL;
        if (t)
            snprintf(buf, sizeof(buf), "*** %s is Now on Team %s",
                     players[player], t);
        else
            snprintf(buf, sizeof(buf), "*** %s is Now Alone", players[player]);
        io->draw_text(BUFFER_PLINE, buf);

    } else if (strcmp(cmd, "pline") == 0) {
        int playernum;
        char buf[1024], *name;

        s = strtok_r(NULL, " ", &sp);
        t = strtok_r(NULL, "", &sp);
        if (!s)
            return;
        if (!t)
            t = "";
        playernum = atoi(s) - 1;
        if (playernum == -1) {
            name = "Server";
        } else {
            if (playernum < 0 || playernum > 5 || !players[playernum])
                return;
            name = players[playernum];
        }
        snprintf(buf, sizeof(buf), "<%s> %s", name, t);
        io->draw_text(BUFFER_PLINE, buf);

    } else if (strcmp(cmd, "plineact") == 0) {
        int playernum;
        char buf[1024], *name;

        s = strtok_r(NULL, " ", &sp);
        t = strtok_r(NULL, "", &sp);
        if (!s)
            return;
        if (!t)
            t = "";
        playernum = atoi(s) - 1;
        if (playernum == -1) {
            name = "Server";
        } else {
            if (playernum < 0 || playernum > 5 || !players[playernum])
                return;
            name = players[playernum];
        }
        snprintf(buf, sizeof(buf), "* %s %s", name, t);
        io->draw_text(BUFFER_PLINE, buf);

    } else if (strcmp(cmd, tetrifast ? "*******" : "newgame") == 0) {
        int i;

        if ((s = strtok_r(NULL, " ", &sp))) {
            /* stack height */ ;
        }
        if ((s = strtok_r(NULL, " ", &sp)))
            initial_level = atoi(s);
        if ((s = strtok_r(NULL, " ", &sp)))
            lines_per_level = atoi(s);
        if ((s = strtok_r(NULL, " ", &sp)))
            level_inc = atoi(s);
        if ((s = strtok_r(NULL, " ", &sp)))
            special_lines = atoi(s);
        if ((s = strtok_r(NULL, " ", &sp)))
            special_count = atoi(s);
        if ((s = strtok_r(NULL, " ", &sp))) {
            special_capacity = atoi(s);
            if (special_capacity > MAX_SPECIALS)
                special_capacity = MAX_SPECIALS;
        }
        if ((s = strtok_r(NULL, " ", &sp))) {
            memset(piecefreq, 0, sizeof(piecefreq));
            while (*s) {
                i = *s - '1';
                if (i >= 0 && i < 7)
                    piecefreq[i]++;
                s++;
            }
        }
        if ((s = strtok_r(NULL, " ", &sp))) {
            memset(specialfreq, 0, sizeof(specialfreq));
            while (*s) {
                i = *s - '1';
                if (i >= 0 && i < 9)
                    specialfreq[i]++;
                s++;
            }
        }
        if ((s = strtok_r(NULL, " ", &sp)))
            level_average = atoi(s);
        if ((s = strtok_r(NULL, " ", &sp)))
            old_mode = atoi(s);
        lines = 0;
        for (i = 0; i < 6; i++)
            levels[i] = initial_level;
        memset(&fields[my_playernum - 1], 0, sizeof(Field));
        specials[0] = -1;
        io->clear_text(BUFFER_GMSG);
        io->clear_text(BUFFER_ATTDEF);
        new_game();
        playing = 1;
        paused = 0;
        io->draw_text(BUFFER_PLINE, "*** The Game Has Started");

    } else if (strcmp(cmd, "ingame") == 0) {
        /* Sent when a player connects in the middle of a game */
        int x, y;
        char buf[1024], *s;

        s = buf + sprintf(buf, "f %d ", my_playernum);
        for (y = 0; y < FIELD_HEIGHT; y++) {
            for (x = 0; x < FIELD_WIDTH; x++) {
                fields[my_playernum - 1][y][x] = rand() % 5 + 1;
                *s++ = '0' + fields[my_playernum - 1][y][x];
            }
        }
        *s = 0;
        sputs(buf, sock);
        playing = 0;
        spectating = 1;

    } else if (strcmp(cmd, "pause") == 0) {
        if ((s = strtok_r(NULL, " ", &sp)))
            paused = atoi(s);
        if (paused) {
            io->draw_text(BUFFER_PLINE, "*** The Game Has Been Paused");
            io->draw_text(BUFFER_GMSG, "*** The Game Has Been Paused");
        } else {
            io->draw_text(BUFFER_PLINE, "*** The Game Has Been Unpaused");
            io->draw_text(BUFFER_GMSG, "*** The Game Has Been Unpaused");
        }

    } else if (strcmp(cmd, "endgame") == 0) {
        playing = 0;
        spectating = 0;
        memset(fields, 0, sizeof(fields));
        specials[0] = -1;
        io->clear_text(BUFFER_ATTDEF);
        io->draw_text(BUFFER_PLINE, "*** The Game Has Ended");
        io->draw_own_field();
        for (int i = 1; i <= 6; i++) {
            if (i != my_playernum)
                io->draw_other_field(i);
        }

    } else if (strcmp(cmd, "playerwon") == 0) {
        /* Syntax: playerwon # -- sent when all but one player lose */

    } else if (strcmp(cmd, "playerlost") == 0) {
        /* Syntax: playerlost # -- sent after playerleave on disconnect
         *     during a game, or when a player loses (sent by the losing
         *     player and from the server to all other players */

    } else if (strcmp(cmd, "f") == 0) { /* field */
        int player, x, y, tile;

        /* This looks confusing, but what it means is, ignore this message
         * if a game isn't going on. */
        if (!playing && !spectating)
            return;
        if (!(s = strtok_r(NULL, " ", &sp)))
            return;
        player = atoi(s);
        player--;
        if (!(s = strtok_r(NULL, "", &sp)))
            return;
        if (*s >= '0') {
            /* Set field directly */
            char *ptr = (char *)fields[player];
            while (*s) {
                if (*s <= '5')
                    *ptr++ = (*s++) - '0';
                else
                    switch (*s++) {
                    case 'a':
                        *ptr++ = 6 + SPECIAL_A;
                        break;
                    case 'b':
                        *ptr++ = 6 + SPECIAL_B;
                        break;
                    case 'c':
                        *ptr++ = 6 + SPECIAL_C;
                        break;
                    case 'g':
                        *ptr++ = 6 + SPECIAL_G;
                        break;
                    case 'n':
                        *ptr++ = 6 + SPECIAL_N;
                        break;
                    case 'o':
                        *ptr++ = 6 + SPECIAL_O;
                        break;
                    case 'q':
                        *ptr++ = 6 + SPECIAL_Q;
                        break;
                    case 'r':
                        *ptr++ = 6 + SPECIAL_R;
                        break;
                    case 's':
                        *ptr++ = 6 + SPECIAL_S;
                        break;
                    }
            }
        } else {
            /* Set specific locations on field */
            tile = 0;
            while (*s) {
                if (*s < '0') {
                    tile = *s - '!';
                } else {
                    x = *s - '3';
                    y = (*++s) - '3';
                    fields[player][y][x] = tile;
                }
                s++;
            }
        }
        if (player == my_playernum - 1)
            io->draw_own_field();
        else
            io->draw_other_field(player + 1);
    } else if (strcmp(cmd, "lvl") == 0) {
        int player;

        if (!(s = strtok_r(NULL, " ", &sp)))
            return;
        player = atoi(s) - 1;
        if (!(s = strtok_r(NULL, "", &sp)))
            return;
        levels[player] = atoi(s);

    } else if (strcmp(cmd, "sb") == 0) {
        int from, to;
        char *type;

        if (!(s = strtok_r(NULL, " ", &sp)))
            return;
        to = atoi(s);
        if (!(type = strtok_r(NULL, " ", &sp)))
            return;
        if (!(s = strtok_r(NULL, " ", &sp)))
            return;
        from = atoi(s);
        do_special(type, from, to);

    } else if (strcmp(cmd, "gmsg") == 0) {
        if (!(s = strtok_r(NULL, "", &sp)))
            return;
        io->draw_text(BUFFER_GMSG, s);

    }
}

/*************************************************************************/
/*************************************************************************/

void help()
{
    fprintf(stderr,
            "Tetrinet - Text-mode tetrinet client\n"
            "\n"
            "Usage: tetrinet [OPTION]... NICK SERVER\n"
            "\n"
            "Options (see README for details):\n"
            "  -fast        Connect to the server in the tetrifast mode.\n"
            "  -noshadow    Do not make the pieces cast shadow.\n"
            "  -noslide     Do not allow pieces to \"slide\" after being dropped\n"
            "               with the spacebar.\n"
            "  -shadow      Make the pieces cast shadow. Can speed up gameplay\n"
            "               considerably, but it can be considered as cheating by\n"
            "               some people since some other tetrinet clients lack this.\n"
            "  -slide       Opposite of -noslide; allows pieces to \"slide\" after\n"
            "               being dropped.  If both -slide and -noslide are given,\n"
            "               -slide takes precedence.\n"
            "  -windows     Behave as much like the Windows version of Tetrinet as\n"
            "               possible. Implies -noslide and -noshadow.\n");
}

int init(int ac, char **av, event_t *event)
{
    int i;
    char *nick = NULL, *server = NULL;
    char buf[1024];
    char nickmsg[1024];
    unsigned char ip[4];
    char iphashbuf[32];
    int len;
    int slide = 0;              /* Do we definitely want to slide? (-slide) */

    io = &odroid_go_interface;

    srand(time(NULL));
    init_shapes();

    for (i = 1; i < ac; i++) {
        if (*av[i] == '-') {
            if (strcmp(av[i], "-noslide") == 0) {
                noslide = 1;
            } else if (strcmp(av[i], "-noshadow") == 0) {
                cast_shadow = 0;
            } else if (strcmp(av[i], "-shadow") == 0) {
                cast_shadow = 1;
            } else if (strcmp(av[i], "-slide") == 0) {
                slide = 1;
            } else if (strcmp(av[i], "-windows") == 0) {
                windows_mode = 1;
                noslide = 1;
                cast_shadow = 0;
            } else if (strcmp(av[i], "-fast") == 0) {
                tetrifast = 1;
            } else {
                fprintf(stderr, "Unknown option %s\n", av[i]);
                help();
                return 1;
            }
        } else if (!nick) {
            my_nick = nick = av[i];
        } else if (!server) {
            server = av[i];
        } else {
            help();
            return 1;
        }
    }
    if (slide)
        noslide = 0;
    if (!server) {
        help();
        return 1;
    }
    if (strlen(nick) > 63)      /* put a reasonable limit on nick length */
        nick[63] = 0;

    if ((sock = conn(server, 31457, (char *)ip)) < 0) {
        fprintf(stderr, "Couldn't connect to server %s: %s\n",
                server, strerror(errno));
        return 1;
    }
    sprintf(nickmsg, "tetri%s %s 1.13", tetrifast ? "faster" : "sstart", nick);
    sprintf(iphashbuf, "%d", ip[0] * 54 + ip[1] * 41 + ip[2] * 29 + ip[3] * 17);
    /* buf[0] does not need to be initialized for this algorithm */
    len = strlen(nickmsg);
    for (i = 0; i < len; i++)
        buf[i + 1] =
            (((buf[i] & 0xFF) +
              (nickmsg[i] & 0xFF)) % 255) ^ iphashbuf[i % strlen(iphashbuf)];
    len++;
    for (i = 0; i < len; i++)
        sprintf(nickmsg + i * 2, "%02X", buf[i] & 0xFF);
    sputs(nickmsg, sock);

    do {
        xQueueReceive(event_queue, event, portMAX_DELAY);
        if (event->type != EVENT_TYPE_SOCKET) {
            continue;
        }
        
        if (event->socket.eof) {
            fprintf(stderr, "Server %s closed connection\n", server);
            disconn(sock);
            sock = -1;
            return 1;
        }
        parse(event->socket.buf);
    } while (my_playernum < 0);
    sockprintf(sock, "team %d ", my_playernum);

    players[my_playernum - 1] = strdup(nick);
    io->screen_setup();
    io->setup_fields();

    return 0;
}

/*************************************************************************/

void client_main(void *arg)
{
    event_t event;

    char *argv[] = {
        "", "ODROID-GO", "127.0.0.1"
    };

    if (init(3, argv, &event) != 0)
        return;

    for (;;) {
        TickType_t timeout;
        if (playing && !paused)
            timeout = tetris_timeout() / portTICK_PERIOD_MS;
        else
            timeout = portMAX_DELAY;
        if (!xQueueReceive(event_queue, &event, timeout)) {
            tetris_timeout_action();
        } else if (event.type == EVENT_TYPE_KEYPAD) {
            tetris_input(&event.keypad);
        } else if (event.type == EVENT_TYPE_SOCKET) {
            if (event.socket.eof) {
                io->draw_text(BUFFER_PLINE, "*** Disconnected from Server");
                break;
            }
            parse(event.socket.buf);
        }
    }

    disconn(sock);
    sock = -1;
    return;
}

/*************************************************************************/

#endif                          /* !SERVER_ONLY */

/*************************************************************************/
