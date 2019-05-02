/*
 * Copyright 2019, François Revol, revol@free.fr.
 * Distributed under the terms of the MIT license.
 */


#include <stdio.h>

#include <Application.h>
#include <InterfacePrivate.h>
#include <String.h>

using BPrivate::set_control_look;


int
main(int argc, char** argv)
{
	if (argc < 2) {
		printf("usage: %s /path/to/ControlLook\n", argv[0]);
		printf("\nTells app_server and applications which ControlLook "
			"add-on to load, which defines the look of interface controls.\n");
		return 1;
	}

	BString path(argv[1]);

	BApplication app("application/x-vnd.Haiku-setcontrollook");

	status_t err = set_control_look(path);
	if (err < B_OK) {
		fprintf(stderr, "error setting Control Look: %s\n", strerror(err));
		return 1;
	}

	return 0;
}

