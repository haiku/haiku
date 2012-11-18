/*
 * Copyright 2004-2010, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2002, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */


#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

#include <FindDirectory.h>
#include <image.h>
#include <InterfaceDefs.h>
#include <OS.h>

#include <keyboard_mouse_driver.h>
#include <Keymap.h>


struct console;

struct keyboard {
	struct keyboard*	next;
	int					device;
	int					target;
	thread_id			thread;
};

struct console {
	int					console_fd;
	thread_id			console_writer;

	struct keyboard*	keyboards;

	int					tty_master_fd;
	int					tty_slave_fd;
	int					tty_num;
};


struct console gConsole;


void
error(const char* message, ...)
{
	char buffer[2048];

	va_list args;
	va_start(args, message);
	
	vsnprintf(buffer, sizeof(buffer), message, args);

	va_end(args);

	// put it out on stderr as well as to serial/syslog
	fputs(buffer, stderr);
	debug_printf("%s", buffer);
}


void
update_leds(int fd, uint32 modifiers)
{
	char lockIO[3] = {0, 0, 0};

	if ((modifiers & B_NUM_LOCK) != 0)
		lockIO[0] = 1;
	if ((modifiers & B_CAPS_LOCK) != 0)
		lockIO[1] = 1;
	if ((modifiers & B_SCROLL_LOCK) != 0)
		lockIO[2] = 1;

	ioctl(fd, KB_SET_LEDS, &lockIO);
}


static int32
keyboard_reader(void* arg)
{
	struct keyboard* keyboard = (struct keyboard*)arg;
	uint8 activeDeadKey = 0;
	uint32 modifiers = 0;

	BKeymap keymap;
	// Load current keymap from disk (we can't talk to the input server)
	// TODO: find a better way (we shouldn't have to care about the on-disk
	// location)
	char path[PATH_MAX];
	status_t status = find_directory(B_USER_SETTINGS_DIRECTORY, -1, false,
		path, sizeof(path));
	if (status == B_OK) {
		strlcat(path, "/Key_map", sizeof(path));
		status = keymap.SetTo(path);
	}
	if (status != B_OK)
		keymap.SetToDefault();

	for (;;) {
		raw_key_info rawKeyInfo;
		if (ioctl(keyboard->device, KB_READ, &rawKeyInfo) != 0)
			break;

		uint32 keycode = rawKeyInfo.keycode;
		bool isKeyDown = rawKeyInfo.is_keydown;

		if (keycode == 0)
			continue;

		uint32 changedModifiers = keymap.Modifier(keycode);
		bool isLock = (changedModifiers
			& (B_CAPS_LOCK | B_NUM_LOCK | B_SCROLL_LOCK)) != 0;
		if (changedModifiers != 0 && (!isLock || isKeyDown)) {
			uint32 oldModifiers = modifiers;

			if ((isKeyDown && !isLock)
				|| (isKeyDown && !(modifiers & changedModifiers)))
				modifiers |= changedModifiers;
			else {
				modifiers &= ~changedModifiers;

				// ensure that we don't clear a combined B_*_KEY when still
				// one of the individual B_{LEFT|RIGHT}_*_KEY is pressed
				if (modifiers & (B_LEFT_SHIFT_KEY | B_RIGHT_SHIFT_KEY))
					modifiers |= B_SHIFT_KEY;
				if (modifiers & (B_LEFT_COMMAND_KEY | B_RIGHT_COMMAND_KEY))
					modifiers |= B_COMMAND_KEY;
				if (modifiers & (B_LEFT_CONTROL_KEY | B_RIGHT_CONTROL_KEY))
					modifiers |= B_CONTROL_KEY;
				if (modifiers & (B_LEFT_OPTION_KEY | B_RIGHT_OPTION_KEY))
					modifiers |= B_OPTION_KEY;
			}

			if (modifiers != oldModifiers) {
				if (isLock)
					update_leds(keyboard->device, modifiers);
			}
		}

		uint8 newDeadKey = 0;
		if (activeDeadKey == 0 || !isKeyDown)
			newDeadKey = keymap.ActiveDeadKey(keycode, modifiers);

		char* string = NULL;
		int32 numBytes = 0;
		if (newDeadKey == 0 && isKeyDown) {
			keymap.GetChars(keycode, modifiers, activeDeadKey, &string,
				&numBytes);
			if (numBytes > 0)
				write(keyboard->target, string, numBytes);

			delete[] string;
		}

		if (newDeadKey == 0) {
			if (isKeyDown && !modifiers && activeDeadKey != 0) {
				// a dead key was completed
				activeDeadKey = 0;
			}
		} else if (isKeyDown) {
			// start of a dead key
			activeDeadKey = newDeadKey;
		}
	}

	return 0;
}


static int32
console_writer(void* arg)
{
	struct console* con = (struct console*)arg;

	for (;;) {
		char buffer[1024];
		ssize_t length = read(con->tty_master_fd, buffer, sizeof(buffer));
		if (length < 0)
			break;

		write(con->console_fd, buffer, length);
	}

	return 0;
}


static void
stop_keyboards(struct console* con)
{
	// close devices

	for (struct keyboard* keyboard = con->keyboards; keyboard != NULL;
			keyboard = keyboard->next) {
		close(keyboard->device);
	}

	// wait for the threads

	for (struct keyboard* keyboard = con->keyboards; keyboard != NULL;) {
		struct keyboard* next = keyboard->next;
		wait_for_thread(keyboard->thread, NULL);

		delete keyboard;
		keyboard = next;
	}

	con->keyboards = NULL;
}


/*!	Opens the all keyboard drivers it finds starting from the given
	location \a start that support the debugger extension.
*/
static struct keyboard*
open_keyboards(int target, const char* start, struct keyboard* previous)
{
	// Wait for the directory to appear, if we're loaded early in boot
	// it may take a while for it to appear while the drivers load.
	DIR* dir;
	int32 tries = 0;
	while (true) {
		dir = opendir(start);
		if (dir != NULL)
			break;
		if(++tries == 10)
			return NULL;
		sleep(1);
	}

	struct keyboard* keyboard = previous;

	while (true) {
		dirent* entry = readdir(dir);
		if (entry == NULL)
			break;
		if (!strcmp(entry->d_name, ".") || !strcmp(entry->d_name, ".."))
			continue;

		char path[PATH_MAX];
		strlcpy(path, start, sizeof(path));
		strlcat(path, "/", sizeof(path));
		strlcat(path, entry->d_name, sizeof(path));

		struct stat stat;
		if (::stat(path, &stat) != 0)
			continue;

		if (S_ISDIR(stat.st_mode)) {
			keyboard = open_keyboards(target, path, keyboard);
			continue;
		}

		// Try to open it as a device
		int fd = open(path, O_RDONLY);
		if (fd >= 0) {
			// Turn on debugger mode
			if (ioctl(fd, KB_SET_DEBUG_READER, NULL, 0) == 0) {
				keyboard = new ::keyboard();
				keyboard->device = fd;
				keyboard->target = target;
				keyboard->thread = spawn_thread(&keyboard_reader, path,
					B_URGENT_DISPLAY_PRIORITY, keyboard);
				if (keyboard->thread < 0) {
					close(fd);
					closedir(dir);
					delete keyboard;
					return NULL;
				}

				if (previous != NULL)
					previous->next = keyboard;

				resume_thread(keyboard->thread);
			} else
				close(fd);
		}
	}

	closedir(dir);
	return keyboard;
}


static int
start_console(struct console* con)
{
	memset(con, 0, sizeof(struct console));
	con->console_fd = -1;
	con->tty_master_fd = -1;
	con->tty_slave_fd = -1;
	con->console_writer = -1;

	con->console_fd = open("/dev/console", O_WRONLY);
	if (con->console_fd < 0)
		return -2;

	DIR* dir = opendir("/dev/pt");
	if (dir != NULL) {
		struct dirent* entry;
		char name[64];

		while ((entry = readdir(dir)) != NULL) {
			if (entry->d_name[0] == '.')
				continue;

			snprintf(name, sizeof(name), "/dev/pt/%s", entry->d_name);
 
			con->tty_master_fd = open(name, O_RDWR);
			if (con->tty_master_fd >= 0) {
				snprintf(name, sizeof(name), "/dev/tt/%s", entry->d_name);

				con->tty_slave_fd = open(name, O_RDWR);
				if (con->tty_slave_fd < 0) {
					error("Could not open tty %s: %s!\n", name,
						strerror(errno));
					close(con->tty_master_fd);
				} else {
					// set default mode
					struct termios termios;
					struct winsize size;

					if (tcgetattr(con->tty_slave_fd, &termios) == 0) {
						termios.c_iflag = ICRNL;
						termios.c_oflag = OPOST | ONLCR;
						termios.c_lflag = ISIG | ICANON | ECHO | ECHOE | ECHONL;

						tcsetattr(con->tty_slave_fd, TCSANOW, &termios);
					}

					if (ioctl(con->console_fd, TIOCGWINSZ, &size,
							sizeof(struct winsize)) == 0) {
						// we got the window size from the console
						ioctl(con->tty_slave_fd, TIOCSWINSZ, &size,
							sizeof(struct winsize));
					}
				}
				break;
			}
		}

		setenv("TTY", name, true);
	}

	if (con->tty_master_fd < 0 || con->tty_slave_fd < 0)
		return -3;

	con->keyboards
		= open_keyboards(con->tty_master_fd, "/dev/input/keyboard", NULL);
	if (con->keyboards == NULL)
		return -4;

	con->console_writer = spawn_thread(&console_writer, "console writer",
		B_URGENT_DISPLAY_PRIORITY, con);
	if (con->console_writer < 0)
		return -5;

	resume_thread(con->console_writer);
	setenv("TERM", "xterm", true);

	return 0;
}


static void
stop_console(struct console* con)
{
	// close TTY FDs; this will also unblock the threads
	close(con->tty_master_fd);
	close(con->tty_slave_fd);

	// close console and keyboards
	close(con->console_fd);
	wait_for_thread(con->console_writer, NULL);

	stop_keyboards(con);
}


static pid_t
start_process(int argc, const char** argv, struct console* con)
{
	int savedInput = dup(0);
	int savedOutput = dup(1);
	int savedError = dup(2);

	dup2(con->tty_slave_fd, 0);
	dup2(con->tty_slave_fd, 1);
	dup2(con->tty_slave_fd, 2);

	pid_t pid = load_image(argc, argv, (const char**)environ);
	resume_thread(pid);
	setpgid(pid, 0);
	tcsetpgrp(con->tty_slave_fd, pid);

	dup2(savedInput, 0);
	dup2(savedOutput, 1);
	dup2(savedError, 2);
	close(savedInput);
	close(savedOutput);
	close(savedError);

	return pid;
}


int
main(int argc, char** argv)
{
	// we're a session leader
	setsid();

	int err = start_console(&gConsole);
	if (err < 0) {
		error("consoled: error %d starting console.\n", err);
		return err;
	}

	// move our stdin and stdout to the console
	dup2(gConsole.tty_slave_fd, 0);
	dup2(gConsole.tty_slave_fd, 1);
	dup2(gConsole.tty_slave_fd, 2);

	if (argc > 1) {
		// a command was given: we run it only once

		// get the command argument vector
		int commandArgc = argc - 1;
		const char** commandArgv = new const char*[commandArgc + 1];
		for (int i = 0; i < commandArgc; i++)
			commandArgv[i] = argv[i + 1];

		commandArgv[commandArgc] = NULL;

		// start the process
		pid_t process = start_process(commandArgc, commandArgv, &gConsole);

		status_t returnCode;
		wait_for_thread(process, &returnCode);
	} else {
		// no command given: start a shell in an endless loop
		for (;;) {
			pid_t shellProcess;
			status_t returnCode;
			const char* shellArgv[] = { "/bin/sh", "--login", NULL };

			shellProcess = start_process(2, shellArgv, &gConsole);

			wait_for_thread(shellProcess, &returnCode);

			puts("Restart shell");
		}
	}

	stop_console(&gConsole);

	return 0;
}

