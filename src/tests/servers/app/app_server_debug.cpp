/*
 * Copyright 2010, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include <DesktopLink.h>
#include <ServerProtocol.h>

#include <stdio.h>
#include <stdlib.h>


extern const char* __progname;


status_t
send_debug_message(team_id team, int32 code)
{
	BPrivate::DesktopLink link;

	status_t status = link.InitCheck();
	if (status != B_OK)
		return status;

	// prepare the message
	status = link.StartMessage(code);
	if (status != B_OK)
		return status;

	status = link.Attach(team);
	if (status != B_OK)
		return status;

	// send it
	return link.Flush();
}


int
main(int argc, char** argv)
{
	if (argc == 1) {
		fprintf(stderr, "usage: %s <team-id> [...]\n", __progname);
		return 1;
	}

	for (int32 i = 1; i < argc; i++) {
		team_id team = atoi(argv[i]);
		if (team > 0)
			send_debug_message(team, AS_DUMP_ALLOCATOR);
	}

	return 0;
}
