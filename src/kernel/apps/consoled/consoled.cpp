/*
 * Copyright 2004-2005, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2002, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */


#include <OS.h>
#include <image.h>
#include <Message.h>
#include <AppDefs.h>
#include <InterfaceDefs.h>
#include <TypeConstants.h>

#include <termios.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "VTkeymap.h"

//#define USE_INPUT_SERVER
	// define this if consoled should use the input_server to
	// get to its keyboard input

struct console {
	int console_fd;
	int keyboard_fd;
	int tty_master_fd;
	int tty_slave_fd;
	int tty_num;
	thread_id keyboard_reader;
	thread_id console_writer;
	sem_id wait_sem;
};


struct console gConsole;


static int32
keyboard_reader(void *arg)
{
	struct console *con = (struct console *)arg;

#ifdef USE_INPUT_SERVER
	area_id appArea;
	void *cursorAddr;
	sem_id cursorSem = 0;	// not initialized?
	thread_id isThread;
	port_id isAsPort;
	port_id isPort;

	appArea = create_area("isCursor", &cursorAddr, B_ANY_ADDRESS, B_PAGE_SIZE, B_FULL_LOCK, B_READ_AREA);

	int32 arg_c = 1;
	char **arg_v = (char **)malloc(sizeof(char *) * (arg_c + 1));
	arg_v[0] = strdup("/system/servers/input_server");
	arg_v[1] = NULL;
	isThread = load_image(arg_c, (const char**)arg_v, (const char **)environ);
	
	free(arg_v[0]);

	int32 tmpbuf[2] = {cursorSem, appArea};
	int32 code = 0;
	send_data(isThread, code, (void *)tmpbuf, sizeof(tmpbuf));

	resume_thread(isThread);
	setpgid(isThread, 0);

	thread_id sender;
	code = receive_data(&sender, (void *)tmpbuf, sizeof(tmpbuf)); 
	isAsPort = tmpbuf[0];
	isPort = tmpbuf[1];
	
	ssize_t	length;
	status_t err;
	const char *string;

	for (;;) {
		length = port_buffer_size(isAsPort);
		char buffer[length];
		err = read_port(isAsPort, &code, buffer, length);
		if(err != length) {
			if (err > 0)
				fprintf(stderr, "consoled: failed to read full packet (read %lu of %lu)\n", err, length);
			else
				fprintf(stderr, "consoled: read_port error: (0x%lx) %s\n", err, strerror(err));
			continue;
		}

		BMessage event;
		if ((err = event.Unflatten(buffer)) < 0) {
			fprintf(stderr, "consoled: Unflatten() error: (0x%lx) %s\n", err, strerror(err));
			continue;
		}

		if (event.what != B_KEY_DOWN) 
			continue;
		event.FindData("bytes", B_STRING_TYPE, (const void**)&string, &length);

		if (length <= 2)
			switch (*string) {
			case B_LEFT_ARROW:
				write(con->tty_master_fd, LEFT_ARROW_KEY_CODE, sizeof(LEFT_ARROW_KEY_CODE));
				break;
			case B_RIGHT_ARROW:
				write(con->tty_master_fd, RIGHT_ARROW_KEY_CODE, sizeof(RIGHT_ARROW_KEY_CODE));
				break;
			case B_UP_ARROW:
				write(con->tty_master_fd, UP_ARROW_KEY_CODE, sizeof(UP_ARROW_KEY_CODE));
				break;
			case B_DOWN_ARROW:
				write(con->tty_master_fd, DOWN_ARROW_KEY_CODE, sizeof(DOWN_ARROW_KEY_CODE));
				break;
			default:
				write(con->tty_master_fd, string, length);
			}	
		else 
			write(con->tty_master_fd, string, length);
	}
#else	// USE_INPUT_SERVER
	char buffer[1024];

	for (;;) {
		ssize_t length = read(con->keyboard_fd, buffer, sizeof(buffer));
		if (length < 0)
			break;

		write(con->tty_master_fd, buffer, length);
	}
#endif	// USE_INPUT_SERVER

	release_sem(con->wait_sem);
	return 0;
}


static int32
console_writer(void *arg)
{
	char buf[1024];
	ssize_t len;
	ssize_t write_len;
	struct console *con = (struct console *)arg;

	for (;;) {
		len = read(con->tty_master_fd, buf, sizeof(buf));
		if (len < 0)
			break;

		write_len = write(con->console_fd, buf, len);
	}

	release_sem(con->wait_sem);
	return 0;
}


static int
start_console(struct console *con)
{
	DIR *dir;

	memset(con, 0, sizeof(struct console));

	con->wait_sem = create_sem(0, "console wait sem");
	if (con->wait_sem < 0)
		return -1;

	con->console_fd = open("/dev/console", 0);
	if (con->console_fd < 0)
		return -2;

#ifndef USE_INPUT_SERVER
	con->keyboard_fd = open("/dev/keyboard", 0);
	if (con->keyboard_fd < 0)
		return -3;
#endif

	dir = opendir("/dev/pt");
	if (dir != NULL) {
		struct dirent *entry;
		char name[64];

		while ((entry = readdir(dir)) != NULL) {
			if (entry->d_name[0] == '.')
				continue;

			sprintf(name, "/dev/pt/%s", entry->d_name);

			con->tty_master_fd = open(name, O_RDWR);
			if (con->tty_master_fd >= 0) {
				sprintf(name, "/dev/tt/%s", entry->d_name);

				con->tty_slave_fd = open(name, O_RDWR);
				if (con->tty_slave_fd < 0) {
					fprintf(stderr, "Could not open tty!\n");
					close(con->tty_master_fd);
				} else {
					// set default mode
					struct termios termios;

					if (tcgetattr(con->tty_slave_fd, &termios) == 0) {
						termios.c_iflag = ICRNL;
						termios.c_oflag = OPOST | ONLCR;
						termios.c_lflag = ISIG | ICANON | ECHO | ECHOE | ECHONL;

						tcsetattr(con->tty_slave_fd, TCSANOW, &termios);
					}
				}
				break;
			}
		}

		setenv("TTY", name, true);
	}

	if (con->tty_master_fd < 0 || con->tty_slave_fd < 0)
		return -4;

	con->keyboard_reader = spawn_thread(&keyboard_reader, "console reader", B_URGENT_DISPLAY_PRIORITY, con);
	if (con->keyboard_reader < 0)
		return -7;

	con->console_writer = spawn_thread(&console_writer, "console writer", B_URGENT_DISPLAY_PRIORITY, con);
	if (con->console_writer < 0)
		return -8;

	resume_thread(con->keyboard_reader);
	resume_thread(con->console_writer);
	setenv("TERM", "beterm", true);

	return 0;
}


static pid_t
start_process(int argc, const char **argv, struct console *con)
{
	int saved_stdin, saved_stdout, saved_stderr;
	pid_t pid;

	saved_stdin = dup(0);
	saved_stdout = dup(1);
	saved_stderr = dup(2);

	dup2(con->tty_slave_fd, 0);
	dup2(con->tty_slave_fd, 1);
	dup2(con->tty_slave_fd, 2);

	pid = load_image(argc, argv, (const char **)environ);
	resume_thread(pid);
	setpgid(pid, 0);

	dup2(saved_stdin, 0);
	dup2(saved_stdout, 1);
	dup2(saved_stderr, 2);
	close(saved_stdin);
	close(saved_stdout);
	close(saved_stderr);

	return pid;
}


int
main(void)
{
	int err;

	err = start_console(&gConsole);
	if (err < 0) {
		printf("consoled: error %d starting console\n", err);
		return err;
	}

	// we're a session leader
	setsid();

	// move our stdin and stdout to the console
	dup2(gConsole.tty_slave_fd, 0);
	dup2(gConsole.tty_slave_fd, 1);
	dup2(gConsole.tty_slave_fd, 2);

	for (;;) {
		pid_t shell_process;
		status_t returnCode;
		const char *argv[3];

		argv[0] = "/bin/sh";
		argv[1] = "--login";
		shell_process = start_process(2, argv, &gConsole);

		wait_for_thread(shell_process, &returnCode);

		puts("Restart shell");
	}

	acquire_sem(gConsole.wait_sem);

	return 0;
}

