/*
 * $Id: keynames.c,v 1.7 2005/04/30 20:13:59 tom Exp $
 */

#include <test.priv.h>

int
main(int argc GCC_UNUSED, char *argv[]GCC_UNUSED)
{
    int n;

    /*
     * Get the terminfo entry into memory, and tell ncurses that we want to
     * use function keys.  That will make it add any user-defined keys that
     * appear in the terminfo.
     */
    newterm(getenv("TERM"), stderr, stdin);
    keypad(stdscr, TRUE);
    endwin();

    for (n = -1; n < KEY_MAX + 512; n++) {
	const char *result = keyname(n);
	if (result != 0)
	    printf("%d(%5o):%s\n", n, n, result);
    }
    ExitProgram(EXIT_SUCCESS);
}
