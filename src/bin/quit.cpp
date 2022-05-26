/*
 * quit.cpp
 * (c) 2002, Carlos Hasan, for Haiku.
 */

#include <stdio.h>
#include <string.h>
#include <app/Messenger.h>

int main(int argc, char *argv[])
{
	status_t status;

	if (argc != 2) {
		printf("use: %s mime_sig\n", argv[0]);
		return 1;
	}

	BMessenger messenger(argv[1]);

	if (!messenger.IsValid()) {
		printf("could not find running app with sig %s\n", argv[1]);
		return 1;
	}

	if ((status = messenger.SendMessage(B_QUIT_REQUESTED)) != B_OK) {
		printf("could not send message, %s\n", strerror(status));
		return 1;
	}

	return 0;
}

