/*
 * Copyright 2009, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Lotz <mmlr@mlotz.ch>
 */

#include <Application.h>
#include <FindDirectory.h>
#include <Path.h>
#include <Screen.h>
#include <Window.h>

#include "RemoteView.h"

#include <new>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>


void
print_usage(const char *app)
{
	printf("usage:\t%s --listenOnly [-p <listenPort>]\n", app);
	printf("\t%s <user@host> [-p <listenPort>] [-s <sshPort>] [-c <command>]\n",
		app);
}


int
main(int argc, char *argv[])
{
	if (argc < 2) {
		print_usage(argv[0]);
		return 1;
	}

	bool listenOnly = false;
	uint32 listenPort = 10900;
	uint32 sshPort = 22;

	for (int32 i = 2; i < argc; i++) {
		if (strcmp(argv[i], "-p") == 0) {
			if (argc < i + 1 || sscanf(argv[i + 1], "%lu", &listenPort) != 1) {
				print_usage(argv[0]);
				return 2;
			}

			i++;
			continue;
		}

		if (strcmp(argv[i], "-s") == 0) {
			if (argc < i + 1 || sscanf(argv[i + 1], "%lu", &sshPort) != 1) {
				print_usage(argv[0]);
				return 2;
			}

			i++;
			continue;
		}

		if (strcmp(argv[i], "-c") == 0) {
			if (argc < i + 1) {
				print_usage(argv[0]);
				return 2;
			}

			i++;
			command = argv[i];
			continue;
		}

		if (strcmp(argv[i], "--listenOnly") == 0) {
			listenOnly = true;
			continue;
		}

		print_usage(argv[0]);
		return 2;
	}

	pid_t sshPID = -1;
	if (!listenOnly) {

		BPath terminalPath;
		if (find_directory(B_SYSTEM_APPS_DIRECTORY, &terminalPath) != B_OK) {
			printf("failed to determine system-apps directory\n");
			return 3;
		}
		if (terminalPath.Append("Terminal") != B_OK) {
			printf("failed to append to system-apps path\n");
			return 3;
		}

		char shellCommand[4096];
		snprintf(shellCommand, sizeof(shellCommand),
			"echo connected; export TARGET_SCREEN=localhost:%lu; %s\n",
			listenPort, terminalPath.Path());

		int pipes[4];
		if (pipe(&pipes[0]) != 0 || pipe(&pipes[2]) != 0) {
			printf("failed to create redirection pipes\n");
			return 3;
		}

		sshPID = fork();
		if (sshPID < 0) {
			printf("failed to fork ssh process\n");
			return 3;
		}

		if (sshPID == 0) {
			// child code, redirect std* and execute ssh
			close(STDOUT_FILENO);
			close(STDIN_FILENO);
			dup2(pipes[1], STDOUT_FILENO);
			dup2(pipes[1], STDERR_FILENO);
			dup2(pipes[2], STDIN_FILENO);
			for (int32 i = 0; i < 4; i++)
				close(pipes[i]);

			char localRedirect[50];
			sprintf(localRedirect, "localhost:%lu:localhost:%lu",
				listenPort + 1, listenPort + 1);

			char remoteRedirect[50];
			sprintf(remoteRedirect, "localhost:%lu:localhost:%lu",
				listenPort, listenPort);

			char portNumber[10];
			sprintf(portNumber, "%lu", sshPort);

			int result = execl("ssh", "-C", "-L", localRedirect,
				"-R", remoteRedirect, "-p", portNumber, argv[1],
				shellCommand, NULL);

			// we don't get here unless there was an error in executing
			printf("failed to execute ssh process in child\n");
			return result;
		} else {
			close(pipes[1]);
			close(pipes[2]);

			char buffer[10];
			read(pipes[0], buffer, sizeof(buffer));
				// block until connected/error message from ssh
		}
	}

	BApplication app("application/x-vnd.Haiku-RemoteDesktop");
	BScreen screen;
	BWindow *window = new(std::nothrow) BWindow(screen.Frame(), "RemoteDesktop",
		B_TITLED_WINDOW, B_QUIT_ON_WINDOW_CLOSE);

	if (window == NULL) {
		printf("no memory to allocate window\n");
		return 4;
	}

	RemoteView *view = new(std::nothrow) RemoteView(window->Bounds(),
		listenPort);
	if (view == NULL) {
		printf("no memory to allocate remote view\n");
		return 4;
	}

	status_t init = view->InitCheck();
	if (init != B_OK) {
		printf("initialization of remote view failed: %s\n", strerror(init));
		delete view;
		return 5;
	}

	window->AddChild(view);
	view->MakeFocus();
	window->Show();
	app.Run();

	if (sshPID >= 0)
		kill(sshPID, SIGINT);

	return 0;
}
