/*
 * $Id: background.c,v 1.1 2003/12/07 00:06:33 tom Exp $
 */

#include <test.priv.h>

int
main(int argc GCC_UNUSED, char *argv[]GCC_UNUSED)
{
    short f, b;

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
	bkgdset(' ' | COLOR_PAIR(1));
	printw("RED/BLACK\n");
	getch();

	printw("Initializing pair 2 to white/blue\n");
	init_pair(2, COLOR_WHITE, COLOR_BLUE);
	bkgdset(' ' | COLOR_PAIR(2));
	printw("WHITE/BLUE\n");
	getch();

	printw("Resetting colors to pair 0\n");
	bkgdset(' ' | COLOR_PAIR(0));
	printw("Default Colors\n");
	getch();

	printw("Resetting colors to pair 1\n");
	bkgdset(' ' | COLOR_PAIR(1));
	printw("RED/BLACK\n");
	getch();

	printw("Setting screen to pair 0\n");
	bkgd(' ' | COLOR_PAIR(0));
	getch();

	printw("Setting screen to pair 1\n");
	bkgd(' ' | COLOR_PAIR(1));
	getch();

	printw("Setting screen to pair 2\n");
	bkgd(' ' | COLOR_PAIR(2));
	getch();

	printw("Setting screen to pair 0\n");
	bkgd(' ' | COLOR_PAIR(0));
	getch();

    } else {
	printw("This demo requires a color terminal");
	getch();
    }
    endwin();

    ExitProgram(EXIT_SUCCESS);
}
