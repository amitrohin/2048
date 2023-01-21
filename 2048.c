#include <locale.h>
#include <stdlib.h>
#include <limits.h>
#include <string.h>
#include <time.h>
#include <ncurses.h>
#include <unistd.h>
#include <errno.h>

#if !defined(lint)
static char const rcsid[] = "$Id: 2048.c,v 1.13 2023/01/16 13:33:21 swp Exp $";
#endif

#if 0
A----+----+----+
|2048|1024|2048|
+----+----+----+
|2048|1024|  4 |
+----+----+----+
|2048|1024|  4 |
+----+----+----B
#endif

#define MIN_CELLWIDTH   3
static int CUR_MAX_VALUE = 0;

static inline int calcdigits(int x) {
    int n;
    if (x < 10)
        n = 1;
    else {
        n = 0;
        do {
            x /= 10;
            n++;
        } while (x);
    }
    return n;
}

static void drawtiles(int n, int m, int a[n][m]) {
    int cw = calcdigits(CUR_MAX_VALUE); /* ширина ячейки */
    if (cw < MIN_CELLWIDTH)
        cw = MIN_CELLWIDTH;

    int Ay = (LINES - (2*n + 1)) / 2;
    int Ax = (COLS - (m*cw + m + 1)) / 2;

    /* верхушка игрового поля */
    mvaddch(Ay, Ax, ACS_ULCORNER);
    for (int j = 0; j < m - 1; j++) {
        for (int x = 0; x < cw; x++) addch(ACS_HLINE);
        addch(ACS_TTEE);
    }
    for (int x = 0; x < cw; x++) addch(ACS_HLINE);
    addch(ACS_URCORNER);

    /* середина игрового поля с промежутками */
    for (int i = 0; i < n; i++) {
        mvaddch(Ay + i*2 + 1, Ax, ACS_VLINE);
        for (int j = 0; j < m; j++) {
            for (int x = 0; x < cw; x++) addch(' ');
            addch(ACS_VLINE);
        }
        if (i < n - 1) {
            /* горизонтальные промежутки между клетками */
            mvaddch(Ay + i*2 + 2, Ax, ACS_LTEE);
            for (int j = 0; j < m-1; j++) {
                for (int x = 0; x < cw; x++) addch(ACS_HLINE);
                addch(ACS_PLUS);
            }
            for (int x = 0; x < cw; x++) addch(ACS_HLINE);
            addch(ACS_RTEE);
        }
    }
    /* низ игрового поля */
    mvaddch(Ay + (n) * 2, Ax, ACS_LLCORNER);
    for (int j = 0; j < m - 1; j++) {
        for (int x = 0; x < cw; x++) addch(ACS_HLINE);
        addch(ACS_BTEE);
    }
    for (int x = 0; x < cw; x++) addch(ACS_HLINE);
    addch(ACS_LRCORNER);

    for (int i = 0; i < n; i++) {
        int y = Ay + 1 + i * 2;
        for (int j = 0; j < m; j++) {
            if (!a[i][j])
                continue;
            int x = Ax + 1 + j * (cw + 1);
            int nw = calcdigits(a[i][j]);
            x += cw - nw - (cw - nw) / 2;
            mvprintw(y, x, "%d", a[i][j]);
        }
    }
}

/* static struct tconf {int value, prob;} tconf[] = {{2,9},{4,3},{8,1}}; */
static struct tconf {int value, prob;} tconf[] = {{2,9},{4,3}};

static int addtiles(int n, int m, int a[n][m], int tn, int ncf, struct tconf *cf) {
    int k = 0;                      /* кол-во свободных ячеек */
    struct { int i, j; } b[n * m];  /* свободные ячейки */
    for (int i = 0; i < n; i++)
        for (int j = 0; j < m; j++)
            if (!a[i][j]) {
                b[k].i = i;
                b[k].j = j;
                k++;
            }

    int prob_max = 0;               /* длина отрезка вероятностей */
    for (int i = 0; i < ncf; i++)
        prob_max += cf[i].prob;

    int t = 0;  /* кол-во установленных новых чисел */
    for (int v, p; k && t < tn; t++) {

        /* разыгрываем число для новой клетки */
        p = random() % prob_max;  /* значение на отрезке вероятностей */
        for (int i = 0, prob = 0; i < ncf; i++) {
            prob += cf[i].prob;
            if (p < prob) {
                v = cf[i].value;
                break;
            }
        }
        if (v > CUR_MAX_VALUE)
            CUR_MAX_VALUE = v;

        p = random() % k;       /* место в таблице пустых клеток */
        a[b[p].i][b[p].j] = v;  /* записываем значение в новую клетку */

        b[p] = b[--k];          /* свободных клеток стало на одну меньше */
    }
    return t;
}

static int a_shl(int n, int m, int a[n][m]) {
    int cnt = 0;
    for (int l = 0; l < n; l++) {
        for (int p = 0, i = 0;; p++) {
            for (;; i++) {
                if (i == m)
                    goto L_nextline;
                if (a[l][i]) {
                    if (p != i) {
                        a[l][p] = a[l][i];
                        a[l][i] = 0;
                        cnt++;
                    }
                    break;
                }
            }
            for (;;) {
                i++;
                if (i == m)
                    goto L_nextline;
                if (a[l][i]) {
                    if (a[l][i] == a[l][p]) {
                        a[l][p] += a[l][i];
                        a[l][i] = 0;
                        i++;
                        if (a[l][p] > CUR_MAX_VALUE)
                            CUR_MAX_VALUE = a[l][p];
                        cnt++;
                    }
                    break;
                }
            }
        }
    L_nextline:
        ;
    }
    return cnt;
}
static int a_shr(int n, int m, int a[n][m]) {
    int cnt = 0;
    for (int l = 0; l < n; l++) {
        for (int p = m-1, i = m-1;; p--) {
            for (;; i--) {
                if (i < 0)
                    goto L_nextline;
                if (a[l][i]) {
                    if (p != i) {
                        a[l][p] = a[l][i];
                        a[l][i] = 0;
                        cnt++;
                    }
                    break;
                }
            }
            for (;;) {
                i--;
                if (i < 0)
                    goto L_nextline;
                if (a[l][i]) {
                    if (a[l][i] == a[l][p]) {
                        a[l][p] += a[l][i];
                        a[l][i] = 0;
                        i--;
                        if (a[l][p] > CUR_MAX_VALUE)
                            CUR_MAX_VALUE = a[l][p];
                        cnt++;
                    }
                    break;
                }
            }
        }
    L_nextline:
        ;
    }
    return cnt;
}
static int a_shu(int n, int m, int a[n][m]) {
    int cnt = 0;
    for (int c = 0; c < m; c++) {
        for (int p = 0, i = 0;; p++) {
            for (;; i++) {
                if (i == n)
                    goto L_nextline;
                if (a[i][c]) {
                    if (p != i) {
                        a[p][c] = a[i][c];
                        a[i][c] = 0;
                        cnt++;
                    }
                    break;
                }
            }
            for (;;) {
                i++;
                if (i == n)
                    goto L_nextline;
                if (a[i][c]) {
                    if (a[i][c] == a[p][c]) {
                        a[p][c] += a[i][c];
                        a[i][c] = 0;
                        i++;
                        if (a[p][c] > CUR_MAX_VALUE)
                            CUR_MAX_VALUE = a[p][c];
                        cnt++;
                    }
                    break;
                }
            }
        }
    L_nextline:
        ;
    }
    return cnt;
}
static int a_shd(int n, int m, int a[n][m]) {
    int cnt = 0;
    for (int c = 0; c < m; c++) {
        for (int p = n-1, i = n-1;; p--) {
            for (;; i--) {
                if (i < 0)
                    goto L_nextline;
                if (a[i][c]) {
                    if (p != i) {
                        a[p][c] = a[i][c];
                        a[i][c] = 0;
                        cnt++;
                    }
                    break;
                }
            }
            for (;;) {
                i--;
                if (i < 0)
                    goto L_nextline;
                if (a[i][c]) {
                    if (a[i][c] == a[p][c]) {
                        a[p][c] += a[i][c];
                        a[i][c] = 0;
                        i--;
                        if (a[p][c] > CUR_MAX_VALUE)
                            CUR_MAX_VALUE = a[p][c];
                        cnt++;
                    }
                    break;
                }
            }
        }
    L_nextline:
        ;
    }
    return cnt;
}

static int isgameover(int n, int m, int a[n][m]) {
    for (int i = 0; i < n; i++)
        for (int j = 0; j < m; j++) {
            if (!a[i][j])
                return 0;
            if (i > 0)
                if (a[i-1][j] == a[i][j])
                    return 0;
            if (i < n-1)
                if (a[i][j] == a[i+1][j])
                    return 0;
            if (j > 0)
                if (a[i][j-1] == a[i][j])
                    return 0;
            if (j < m-1)
                if (a[i][j] == a[i][j+1])
                    return 0;
        }
    return 1;
}

int main(int argc, char *argv[]) {
    do {
        char *p, *prog = argv[0];
        p = strrchr(argv[0], '/');
        if (p)
                prog = p;
        for (int c; (c = getopt(argc, argv, "h")) != -1; ) {
            switch (c) {
                case 'h':
                    printf( "Usage:\n"
                            "   %s -h\n"
                            "   %s N\n"
                            "   %s NxM\n"
                            "where\n"
                            "  -h    - this help.\n"
                            "  N,NxM - playground size.\n",
                            prog, prog, prog);
                    return EXIT_SUCCESS;
                default:
                    return EXIT_FAILURE;
            }
        }
        argc -= optind;
        argv += optind;
    } while (0);
    if (argc > 1) {
        fprintf(stderr, "too many arguments.\n");
        return EXIT_FAILURE;
    }

    long n = 4, m = 4;
    if (argc) {
        char *p;
        n = strtol(*argv, &p, 10);
        if ((!n && errno == EINVAL) || ((n == LONG_MIN || n == LONG_MAX) && errno == ERANGE)) {
            fprintf(stderr, "strtol(%s): %s.\n", *argv, strerror(errno));
            return EXIT_FAILURE;
        }
        m = n;
        if (*p == 'x') {
            p++;
            m = strtol(p, &p, 10);
            if ((!m && errno == EINVAL) || ((m == LONG_MIN || m == LONG_MAX) && errno == ERANGE)) {
                fprintf(stderr, "strtol(%s): %s.\n", *argv, strerror(errno));
                return EXIT_FAILURE;
            }
        } else if (*p) {
            fprintf(stderr, "wrong argument syntax.\n");
            return EXIT_FAILURE;
        }
    }

    int a[n][m];

    setlocale(LC_ALL, "");
    srandom(time(0));

    memset(a, 0, sizeof a);
    addtiles(n, m, a, 2, sizeof tconf/sizeof tconf[0], tconf);

    initscr();
    atexit((void *)endwin);
    cbreak();
    noecho();
    nonl();
    keypad(stdscr, TRUE);

    for (int ch;;) {
        drawtiles(n, m, a);
        move(LINES-1, COLS-1);
        refresh();

        if (isgameover(n, m, a)) {
            getch();
            break;
	}

    L_getch:
        ch = getch();
        if (ch == KEY_F(1) || ch == 'q' || ch == 'Q' || ch == 27)
            break;

        int (*shfn)(int n, int m, int a[n][m]);
        if (ch == KEY_LEFT)
            shfn = a_shl;
        else if (ch == KEY_RIGHT)
            shfn = a_shr;
        else if (ch == KEY_UP)
            shfn = a_shu;
        else if (ch == KEY_DOWN)
            shfn = a_shd;
        else
            continue;

        if (!shfn(n, m, a))
            goto L_getch;

        addtiles(n, m, a, 2, sizeof tconf/sizeof tconf[0], tconf);
    }
    return 0;
}

/* vi: ts=4:sts=4:sw=4:et
 */
