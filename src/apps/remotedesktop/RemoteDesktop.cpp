/*
 * Copyright 2009-2017, Haiku, Inc.
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
	printf("usage:\t%s <host> [-p <port>] [-w <width>] [-h <height>]\n", app);
	printf("usage:\t%s <user@host> -s [<sshPort>] [-p <port>] [-w <width>]"
		" [-h <height>] [-c <command>]\n", app);
	printf("\t%s --help\n\n", app);

	printf("Connect to & run applications from a different computer\n\n");
	printf("Arguments available for use:\n\n");
	printf("\t-p\t\tspecify the port to communicate on (default 10900)\n");
	printf("\t-c\t\tsend a command to the other computer (default Terminal)\n");
	printf("\t-s\t\tuse SSH, optionally specify the SSH port to use (22)\n");
	printf("\t-w\t\tmake the virtual desktop use the specified width\n");
	printf("\t-h\t\tmake the virtual desktop use the specified height\n");
	printf("\nIf no width and height are specified, the window is opened with"
		" the size of the the local screen.\n");
}


int
main(int argc, char *argv[])
{
	if (argc < 2 || strcmp(argv[1], "--help") == 0) {
		print_usage(argv[0]);
		return 1;
	}

	uint16 port = 10900;
	uint16 sshPort = 22;
	int32 width = -1;
	int32 height = -1;
	bool useSSH = false;
	const char *command = NULL;
	const char *host = argv[1];

	for (int32 i = 2; i < argc; i++) {
		if (strcmp(argv[i], "-p") == 0) {
			if (argc <= i + 1 || sscanf(argv[i + 1], "%" B_SCNu16, &port) != 1) {
				print_usage(argv[0]);
				return 2;
			}

			i++;
			continue;
		}

		if (strcmp(argv[i], "-w") == 0) {
			if (argc <= i + 1 || sscanf(argv[i + 1], "%" B_SCNd32, &width) != 1) {
				print_usage(argv[0]);
				return 2;
			}

			i++;
			continue;
		}

		if (strcmp(argv[i], "-h") == 0) {
			if (argc <= i + 1 || sscanf(argv[i + 1], "%" B_SCNd32, &height) != 1) {
				print_usage(argv[0]);
				return 2;
			}

			i++;
			continue;
		}

		if (strcmp(argv[i], "-s") == 0) {
			if((i + 1 < argc) && sscanf(argv[i + 1], "%" B_SCNu16, &sshPort) == 1) {
				i++;
			}

			useSSH = true;
			continue;
		}

		if (strcmp(argv[i], "-c") == 0) {
			if (argc <= i + 1) {
				print_usage(argv[0]);
				return 2;
			}

			i++;
			command = argv[i];
			continue;
		}

		print_usage(argv[0]);
		return 2;
	}

	if (command != NULL && !useSSH) {
		print_usage(argv[0]);
		return 2;
	}

	pid_t sshPID = -1;
	if (useSSH) {
		BPath terminalPath;
		if (command == NULL) {
			if (find_directory(B_SYSTEM_APPS_DIRECTORY, &terminalPath)
					!= B_OK) {
				printf("failed to determine system-apps directory\n");
				return 3;
			}
			if (terminalPath.Append("Terminal") != B_OK) {
				printf("failed to append to system-apps path\n");
				return 3;
			}
			command = terminalPath.Path();
		}

		char shellCommand[4096];
		snprintf(shellCommand, sizeof(shellCommand),
			"echo connected; export TARGET_SCREEN=%" B_PRIu16 "; %s\n", port,
			command);

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
			sprintf(localRedirect, "localhost:%" B_PRIu16 ":localhost:%"
				B_PRIu16, port, port);

			char portNumber[10];
			sprintf(portNumber, "%" B_PRIu16, sshPort);

			int result = execl("ssh", "-C", "-L", localRedirect,
				"-p", portNumber, "-o", "ExitOnForwardFailure=yes", host,
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

			host = "localhost";
		}
	}

	BApplication app("application/x-vnd.Haiku-RemoteDesktop");
	BRect windowFrame = BRect(0, 0, width - 1, height - 1);
	if (!windowFrame.IsValid()) {
		BScreen screen;
		windowFrame = screen.Frame();
	}

	BWindow *window = new(std::nothrow) BWindow(windowFrame, "RemoteDesktop",
		B_TITLED_WINDOW, B_QUIT_ON_WINDOW_CLOSE);

	if (window == NULL) {
		printf("no memory to allocate window\n");
		return 4;
	}

	RemoteView *view = new(std::nothrow) RemoteView(window->Bounds(), host,
		port);
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
		kill(sshPID, SIGHUP);

	return 0;
}
