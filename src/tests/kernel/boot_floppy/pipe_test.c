/* tests basic pipes functionality */

/* 
** Copyright 2003, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/


#include <OS.h>

#include <unistd.h>
#include <stdio.h>
#include <string.h>


static int32
reader_func(void *data)
{
	char buffer[1024];
	int input = (int)data;
	int bytes;

	while ((bytes = read(input, buffer, sizeof(buffer) - 1)) > 0) {
		buffer[bytes] = '\0';
			// null-terminate, just in case
		printf("reader: (%d) %s\n", bytes, buffer);

		if (buffer[0] == '2') {
			puts("reader: wait a second and let the writer lead (the pipe has to be buffered)");
			snooze(1000000);
		}
		if (strstr(buffer, "QUIT"))
			break;
	}
	puts("reader quits");

	return 0;
}


static int32
writer_func(void *data)
{
	int output = (int)data;
	int i;

	const char *strings[] = {
		"1. If you read this ",
		"2. the pipe implementation ",
		"3. seems to work ",
		"4. at least a bit ",
		"5. QUIT",
		NULL};

	for (i = 0; strings[i] != NULL; i++) {
		snooze(100000);
			// make sure the reader is waiting for us...
			// (needed by the current pipefs implementation :)

		printf("writer: (%ld)\n", write(output, strings[i], strlen(strings[i])));
	}
	puts("writer quits");

	return 0;
}


int
main(int argc, char **argv)
{
	thread_id reader, writer;
	int stream[2];
	status_t returnCode;

	if (pipe(stream) < 0) {
		fprintf(stderr, "pipe creation failed!\n");
		return -1;
	}

	reader = spawn_thread(reader_func, "Reader" , B_NORMAL_PRIORITY, (void *)stream[0]);
	resume_thread(reader);

	writer = spawn_thread(writer_func, "Writer" , B_NORMAL_PRIORITY, (void *)stream[1]);
	resume_thread(writer);

	puts("reader & writer started.");

	// wait until they quit
	wait_for_thread(reader, &returnCode);
	wait_for_thread(writer, &returnCode);

	close(stream[0]);
	close(stream[1]);

	return 0;
}

