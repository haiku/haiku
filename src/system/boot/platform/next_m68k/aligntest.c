#include <stddef.h>
#include <unistd.h>
#include "nextrom.h"
#include "../atari_m68k/toscalls.h"

/* we compile this code as an atari TOS PRG to verify offsets in mon_global */

int main()
{
	int o = offsetof(struct mon_global, mg_board_rev);
	/* let's return the value to the shell. Only on 8bit, so it's %256 */
	/* return doesn't work without using the crt files so we must Pterm() */
	Pterm(o);
	return o;
}

