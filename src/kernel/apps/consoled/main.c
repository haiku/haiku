/*
** Copyright 2002, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/


#include <OS.h>
#include <syscalls.h>

#include <termios.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>
#include <stdio.h>


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

struct console theconsole;


static int32
console_reader(void *arg)
{
	struct console *con = (struct console *)arg;
	char buffer[1024];

	for (;;) {
		ssize_t length = read(con->keyboard_fd, buffer, sizeof(buffer));
		if (length < 0)
			break;

		write(con->tty_master_fd, buffer, length);
	}

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

	con->keyboard_fd = open("/dev/keyboard", 0);
	if (con->keyboard_fd < 0)
		return -3;

	dir = opendir("/dev/pt");
	if (dir != NULL) {
		struct dirent *entry;
		char name[64];
	
		while ((entry = readdir(dir)) != NULL) {
			if (entry->d_name[0] == '.')
				continue;
	
			sprintf(name, "/dev/pt/%s", entry->d_name);
			puts(name);
	
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

						tcsetattr(con->tty_slave_fd, TCSETA, &termios);
					}
				}
				break;
			}
		}
	}

	if (con->tty_master_fd < 0 || con->tty_slave_fd < 0)
		return -4;

	con->keyboard_reader = spawn_thread(&console_reader, "console reader", B_URGENT_DISPLAY_PRIORITY, con);
	if (con->keyboard_reader < 0)
		return -7;

	con->console_writer = spawn_thread(&console_writer, "console writer", B_URGENT_DISPLAY_PRIORITY, con);
	if (con->console_writer < 0)
		return -8;

	resume_thread(con->keyboard_reader);
	resume_thread(con->console_writer);

	return 0;
}


static pid_t
start_process(const char *path, const char *name, char **argv, int argc, struct console *con)
{
	int saved_stdin, saved_stdout, saved_stderr;
	pid_t pid;

	saved_stdin = dup(0);
	saved_stdout = dup(1);
	saved_stderr = dup(2);

	dup2(con->tty_slave_fd, 0);
	dup2(con->tty_slave_fd, 1);
	dup2(con->tty_slave_fd, 2);

	// XXX launch
	pid = _kern_create_team(path, name, argv, argc, NULL, 0, 5/*, PROC_FLAG_NEW_PGROUP*/);

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

	err = start_console(&theconsole);
	if (err < 0) {
		printf("consoled: error %d starting console\n", err);
		return err;
	}

	// we're a session leader
	setsid();

	// move our stdin and stdout to the console
	dup2(theconsole.tty_slave_fd, 0);
	dup2(theconsole.tty_slave_fd, 1);
	dup2(theconsole.tty_slave_fd, 2);

	for (;;) {
		pid_t shell_process;
		status_t retcode;
		char *argv[3];

		argv[0] = "/bin/shell";
//		argv[1] = "-s";
//		argv[2] = "/boot/loginscript";
		shell_process = start_process("/bin/shell", "shell", argv, 1, &theconsole);

		_kern_wait_for_team(shell_process, &retcode);
		
		puts("Restart shell");
	}

	acquire_sem(theconsole.wait_sem);

	return 0;
}

