#define _POSIX_C_SOURCE 199309L
#define _XOPEN_SOURCE
#include <stdio.h>
#include <ncurses.h>
#include <time.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <ctype.h>
#include <signal.h>

/*#define MS(X) X ## 000000L*/
#define MS(X) X*1000000

#define NELEM(X) (sizeof(X)/sizeof(X[0]))

#define KEY_ESC 27
#define MAX_ACTIVE 20

#define FPS 30

typedef struct {
    const char * s;
    int row;
    int col;
} active_word_t;

struct {
    int frame;
    int speed;
    int score;
    int game_over;
    size_t num_active_words;
    active_word_t active_words[MAX_ACTIVE];
    size_t bl;
} G;

const struct timespec sleep_dur = {.tv_sec=0, .tv_nsec=MS(1000/FPS)};
int nrows, ncols;
char buffer[128] = "";

size_t nwords = 0;
size_t words_cap = 0;
char ** words;

int g_argc;
char ** g_argv;

static void
readwords(const char * filename)
{
    FILE * fp = fopen(filename, "r");
    if (fp == NULL) {
        fprintf(stderr, "error: cannot open file %s\n", filename);
        exit(1);
    }
    while (fgets(buffer, sizeof(buffer), fp) != NULL) {
        size_t l = strlen(buffer);
        assert(l < sizeof(buffer));
        buffer[l-1] = '\0'; /* remove newline */
        words[nwords] = malloc(sizeof(char)*(l+1));
        strcpy(words[nwords], buffer);
        nwords++;
        if (nwords >= words_cap) {
            words_cap *= 2;
            words = realloc(words, sizeof(*words)*words_cap);
            assert(words);
        }
    }
    fclose(fp);
}

static void
chooseword(size_t n)
{
    size_t i;
    int row;
    const char * s;
    while (1) {
        s = words[rand() % nwords];
        int valid = 1;
        /* chosen word must not already be active */
        for (i = 0; i < G.num_active_words; i++) {
            if (s == G.active_words[i].s) {
                valid = 0;
                break;
            }
        }
        if (valid) {
            break;
        }
    }
    G.active_words[n].s = s;
    G.active_words[n].col = rand() % 15;

    /* make sure each word is on a unique row */
    while (1) {
        int valid = 1;
        row = rand () % nrows;
        for (i = 0; i < G.num_active_words; i++) {
            if (G.active_words[i].row == row) {
                valid = 0;
                break;
            }
        }
        if (valid)
            break;
    }
    G.active_words[n].row = row;
}

static void
fpe_handler()
{
    endwin();
    fprintf(stderr, "error: floating point exception\n");
    exit(1);
}

static void
segv_handler()
{
    endwin();
    fprintf(stderr, "error: segmentation fault\n");
    exit(1);
}

static void
init()
{
    size_t i;
    /* install a signal handler for segfaults to reset terminal */
    const char * words_filename = (g_argc >= 2) ? g_argv[1] : "./data/words";
    struct sigaction act;

    act.sa_handler = segv_handler;
    if (sigaction(SIGSEGV, &act, NULL) != 0) {
        perror("error");
        exit(1);
    }
    act.sa_handler = fpe_handler;
    if (sigaction(SIGFPE, &act, NULL) != 0) {
        perror("error");
        exit(1);
    }

    words_cap = 10;
    words = malloc(sizeof(*words)*words_cap);
    readwords(words_filename);

    putenv("ESCDELAY=0");
    srand(time(NULL));

    initscr();
    raw();
    keypad(stdscr, TRUE);
    noecho();

    getmaxyx(stdscr, nrows, ncols);
    nodelay(stdscr, TRUE);

    G.frame = 0;
    G.speed = 1;
    G.score = 0;
    G.game_over = 0;
    G.num_active_words = 5;
    G.bl = 0;

    for (i = 0 ; i < MAX_ACTIVE; i++) {
        G.active_words[i].s = NULL;
    }

    for (i = 0; i < G.num_active_words; i++) {
        chooseword(i);
    }

    buffer[0] = '\0';
}

static void
process_input()
{
    size_t i;
    int ch = getch();

    if (ch == KEY_ESC) {
        G.game_over = 1;
    } else if (ch == ERR) {
    } else if (ch == KEY_BACKSPACE) {
        if (G.bl > 0) {
            buffer[--G.bl] = '\0';
        }
    } else if (ch == '\n') {
        for (i = 0; i < G.num_active_words; i++) {
            if (strcmp(buffer, G.active_words[i].s) == 0) {
                chooseword(i);
                G.score++;
                if (G.score % 10 == 0) {
                    G.speed++;
                }
                if (G.num_active_words < MAX_ACTIVE) {
                    chooseword(G.num_active_words);
                    G.num_active_words++;
                }
                break;
            }
        }
        buffer[0] = '\0';
        G.bl = 0;
    } else if (isalpha(ch)) {
        buffer[G.bl++] = ch;
        buffer[G.bl] = '\0';
    } else {
        mvprintw(nrows-2, 1, "%c %d\n", ch, ch);
    }
}

static void
update()
{
    size_t i;
    if (G.frame % (FPS/G.speed) == 0) {
        for (i = 0; i < G.num_active_words; i++) {
            if (G.active_words[i].col++ >= ncols) {
                G.game_over = 1;
                break;
            }
        }
    }
}

static void
render()
{
    size_t i;
    clear();
    for (i = 0; i < G.num_active_words; i++) {
        mvprintw(G.active_words[i].row, G.active_words[i].col, "%s\n", G.active_words[i].s);
    }
    mvprintw(nrows-1, 1, "%s\n", buffer);
#if 0
    size_t i;
    for (i = 0; i < G.bl; i++) {
        mvprintw(nrows-2, i*4, "%d ", buffer[i]);
    }
#endif
    refresh();
}

static void
loop()
{
    while (1) {
        G.frame++;
        process_input();
        update();
        if (G.game_over)
            break;
        render();
        nanosleep(&sleep_dur, NULL);
    }
}

int main(int argc, char * argv[])
{
    g_argc = argc;
    g_argv = argv;
    init();
    loop();
    endwin();
    return 0;
}
