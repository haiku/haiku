/*
 * Copyright 2010, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <OS.h>

#include <system_info.h>
#include <util/KMessage.h>


extern const char* __progname;


static void
print_usage(bool error)
{
	fprintf(error ? stderr : stdout,
		"Usage: %s [ <teamID> [ <events> ] ]\n"
		"  teamID  - The team ID of the team to watch or -1 for all teams\n"
		"            (-1 is the default).\n"
		"  events  - A string specifying the events to watch, consisting of:\n"
		"            'C': team creation\n"
		"            'D': team deletion\n"
		"            'c': thread creation\n"
		"            'd': thread deletion\n"
		"            'p': thread properties\n"
		"            The default is all events.\n",
		__progname);
}


static void
print_usage_and_exit(bool error)
{
	print_usage(error);
	exit(error ? 1 : 0);
}


int
main(int argc, const char* const* argv)
{
	int32 watchTeam = -1;
	uint32 watchEvents = B_WATCH_SYSTEM_ALL;

	if (argc > 3)
		print_usage_and_exit(true);

	if (argc > 1) {
		const char* arg = argv[1];
		if (strcmp(arg, "-h") == 0 || strcmp(arg, "--help") == 0)
			print_usage_and_exit(false);

		// first parameter is the team ID
		watchTeam = atol(arg);

		if (argc > 2) {
			// second parameter is the events string
			arg = argv[2];
			watchEvents = 0;
			while (*arg != '\0') {
				switch (*arg) {
					case 'C':
						watchEvents |= B_WATCH_SYSTEM_TEAM_CREATION;
						break;
					case 'D':
						watchEvents |= B_WATCH_SYSTEM_TEAM_DELETION;
						break;
					case 'c':
						watchEvents |= B_WATCH_SYSTEM_THREAD_CREATION;
						break;
					case 'd':
						watchEvents |= B_WATCH_SYSTEM_THREAD_DELETION;
						break;
					case 'p':
						watchEvents |= B_WATCH_SYSTEM_THREAD_PROPERTIES;
						break;
					default:
						print_usage_and_exit(true);
				}
				arg++;
			}
		}
	}

	// create a port
	port_id port = create_port(10, "system watching test");
	if (port < 0) {
		fprintf(stderr, "Failed to create port: %s\n", strerror(port));
		exit(1);
	}

	// start watching
	status_t error = __start_watching_system(watchTeam, watchEvents, port, 0);
	if (error != B_OK) {
		fprintf(stderr, "Failed to start watching: %s\n", strerror(error));
		exit(1);
	}

	// receive notifications
	while (true) {
		// read the message from the port
		char buffer[1024];
		int32 messageCode;
		ssize_t bytesRead = read_port(port, &messageCode, buffer,
			sizeof(buffer));
		if (bytesRead < 0) {
			if (bytesRead == B_INTERRUPTED)
				continue;
			fprintf(stderr, "Failed to read from port: %s\n",
				strerror(bytesRead));
			exit(1);
		}

		// create a KMessage
		KMessage message;
		error = message.SetTo((const void*)buffer, bytesRead);
		if (error != B_OK) {
			fprintf(stderr, "Failed to create message: %s\n", strerror(error));
			continue;
		}

		message.Dump((void(*)(const char*,...))printf);

		// check "what"
		if (message.What() != B_SYSTEM_OBJECT_UPDATE)
			fprintf(stderr, "Unexpected message what!\n");

		// get opcode
		int32 opcode = 0;
		if (message.FindInt32("opcode", &opcode) != B_OK)
			fprintf(stderr, "Failed to get message opcode!\n");

		switch (opcode) {
			case B_TEAM_CREATED:
			{
				printf("B_TEAM_CREATED: ");
				int32 teamID;
				if (message.FindInt32("team", &teamID) == B_OK)
					printf("team: %" B_PRId32 "\n", teamID);
				else
					printf("no \"team\" field\n");

				break;
			}
			case B_TEAM_DELETED:
			{
				printf("B_TEAM_DELETED: ");
				int32 teamID;
				if (message.FindInt32("team", &teamID) == B_OK)
					printf("team: %" B_PRId32 "\n", teamID);
				else
					printf("no \"team\" field\n");

				break;
			}
			case B_TEAM_EXEC:
			{
				printf("B_TEAM_EXEC: ");
				int32 teamID;
				if (message.FindInt32("team", &teamID) == B_OK)
					printf("team: %" B_PRId32 "\n", teamID);
				else
					printf("no \"team\" field\n");

				break;
			}

			case B_THREAD_CREATED:
			{
				printf("B_THREAD_CREATED: ");
				int32 threadID;
				if (message.FindInt32("thread", &threadID) == B_OK)
					printf("thread: %" B_PRId32 "\n", threadID);
				else
					printf("no \"thread\" field\n");

				break;
			}
			case B_THREAD_DELETED:
			{
				printf("B_THREAD_DELETED: ");
				int32 threadID;
				if (message.FindInt32("thread", &threadID) == B_OK)
					printf("thread: %" B_PRId32 "\n", threadID);
				else
					printf("no \"thread\" field\n");

				break;
			}
			case B_THREAD_NAME_CHANGED:
			{
				printf("B_THREAD_NAME_CHANGED: ");
				int32 threadID;
				if (message.FindInt32("thread", &threadID) == B_OK)
					printf("thread: %" B_PRId32 "\n", threadID);
				else
					printf("no \"thread\" field\n");

				break;
			}

			default:
				fprintf(stderr, "Unknown opcode!\n");
				break;
		}
	}

	return 0;
}
