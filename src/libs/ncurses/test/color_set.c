/*
 * $Id: color_set.c,v 1.3 2004/04/10 20:10:28 tom Exp $
 */

#include <test.priv.h>

#ifdef HAVE_COLOR_SET

#define SHOW(n) ((n) == ERR ? "ERR" : "OK")

int
main(int argc GCC_UNUSED, char *argv[]GCC_UNUSED)
{
    short f, b;
    int i;

    initscr();
    cbreak();
    noecho();

    if (has_colors()) {
	start_color();

	pair_content(0, &f, &b);
	printw("pair 0 contains (%d,%d)\n", f, b);
	getch();

	printw("Initializing pair 1 to red/black\n");
	init_pair(1, COLOR_RED, COLOR_BLACK);
	i = color_set(1, NULL);
	printw("RED/BLACK (%s)\n", SHOW(i));
	getch();

	printw("Initializing pair 2 to white/blue\n");
	init_pair(2, COLOR_WHITE, COLOR_BLUE);
	i = color_set(2, NULL);
	printw("WHITE/BLUE (%s)\n", SHOW(i));
	getch();

	printw("Resetting colors to pair 0\n");
	i = color_set(0, NULL);
	printw("Default Colors (%s)\n", SHOW(i));
	getch();

	printw("Resetting colors to pair 1\n");
	i = color_set(1, NULL);
	printw("RED/BLACK (%s)\n", SHOW(i));
	getch();

    } else {
	printw("This demo requires a color terminal");
	getch();
    }
    endwin();

    ExitProgram(EXIT_SUCCESS);
}
#else
int
main(void)
{
    printf("This program requires the curses color_set function\n");
    ExitProgram(EXIT_FAILURE);
}
#endif
