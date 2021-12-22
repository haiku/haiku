/*
 * ffm.cpp
 * (c) 2002, Carlos Hasan, for Haiku.
 */


#include <strings.h>

#include <Application.h>
#include <InterfaceDefs.h>


int
main(int argc, char *argv[])
{
	BApplication app("application/x-vnd.Haiku-ffm");
	bool follow;

	if (argc == 2) {
		if (strcasecmp(argv[1], "yes") == 0 || strcasecmp(argv[1], "on") == 0)
			follow = true;
		else
			follow = false;
	} else {
		follow = true;
	}

	set_focus_follows_mouse(follow);
	return 0;
}

