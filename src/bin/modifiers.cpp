/*
 * modifiers - asserts (un)pressed modifier keys
 * (c) 2002, Francois Revol, revol@free.fr
 * for OpenBeOS
 * compile with:
 * LDFLAGS=-lbe make modifiers
 */

#include <OS.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <InterfaceDefs.h>


int32 modifier_bits[] = {
	B_SHIFT_KEY,
	B_COMMAND_KEY,
	B_CONTROL_KEY,
	B_CAPS_LOCK,
	B_SCROLL_LOCK,
	B_NUM_LOCK,
	B_OPTION_KEY,
	B_MENU_KEY,
	B_LEFT_SHIFT_KEY,
	B_RIGHT_SHIFT_KEY,
	B_LEFT_COMMAND_KEY,
	B_RIGHT_COMMAND_KEY,
	B_LEFT_CONTROL_KEY,
	B_RIGHT_CONTROL_KEY,
	B_LEFT_OPTION_KEY,
	B_RIGHT_OPTION_KEY,
	0
};


const char *modifier_names[] = { "shift", "command", "control", "capslock", "scrolllock", "numlock", "option", "menu", "lshift", "rshift", "lcommand", "rcommand", "lcontrol", "rcontrol", "loption", "roption", NULL };


int
usage(char *progname, int error)
{
	FILE *outstr;
	outstr = error?stderr:stdout;
	fprintf(outstr, "Usage: %s [-help] [-list] [-/+][[|l|r][shift|control|command|option]|capslock|scrolllock|numlock|menu]\n", progname);
	fprintf(outstr, "\t- asserts unpressed modifier,\n");
	fprintf(outstr, "\t+ asserts pressed modifier,\n");
	return error;
}


int
list_modifiers(int mods)
{
	int i;
	int gotone = 0;
	for (i = 0; modifier_names[i]; i++) {
		if (mods & modifier_bits[i]) {
			if (gotone)
				printf(",");
			printf("%s", modifier_names[i]);
			gotone = 1;
		}
		if (modifier_names[i+1] == NULL)
			printf("\n");
	}
	return 0;
}


int
main(int argc, char **argv)
{
	int32 mods;
	int32 mask_xor = 0x0;
	int32 mask_and = 0x0;
	int i, j;

	mods = modifiers();
	if (argc == 1)
		return usage(argv[0], 1);
	for (i=1; i<argc; i++) {
		if (!strncmp(argv[i], "-h", 2))
			return usage(argv[0], 0);
		else if (!strcmp(argv[i], "-list"))
			return list_modifiers(mods);
		else if (strlen(argv[i]) > 1 && (*argv[i] == '-' || *argv[i] ==  '+')) {
			for (j=0; modifier_names[j]; j++) {
				if (!strcmp(argv[i]+1, modifier_names[j])) {
					if (*argv[i] == '-')
						mask_and |= modifier_bits[j];
					else
						mask_xor |= modifier_bits[j];
					break;
				}
			}
			if (modifier_names[j] == NULL)
				return usage(argv[0], 1);
		} else
			return usage(argv[0], 1);
	}
	mask_and |= mask_xor;
	return (((mods & mask_and) ^ mask_xor) != 0);
}

