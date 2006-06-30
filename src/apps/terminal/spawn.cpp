/*
 * Copyright (c) 2003-4 Kian Duffy <myob@users.sourceforge.net>
 * Copyright (c) 2004 Daniel Furrer <assimil8or@users.sourceforge.net>
 * Parts Copyright (C) 1998,99 Kazuho Okui and Takashi Murai. 
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files or portions
 * thereof (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so, subject
 * to the following conditions:
 *
 *  * Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 *  * Redistributions in binary form must reproduce the above copyright notice
 *    in the  binary, as well as this list of conditions and the following
 *    disclaimer in the documentation and/or other materials provided with
 *    the distribution.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */

#include <dirent.h>
#include <sys/param.h>
#include <sys/file.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stddef.h>
#include <stdio.h>
#include <signal.h>
#include <errno.h>
#include <termios.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <time.h>
#include <kernel/OS.h>

#include "TermConst.h"
#include "spawn.h"
#include "PrefHandler.h"

extern PrefHandler *gTermPref;

/* default shell command and options. */
#define SHELL_COMMAND "/bin/sh -login"
extern char **environ;

const char *kSpawnAlertMessage = "alert --stop " "'Cannot execute \"%s\":\n"
	"\t%s\n'"
	"'Use Default Shell' 'Abort'";

/*
 * Set environment varriable.
 */

#if !defined(__HAIKU__) || defined(HAIKU_TARGET_PLATFORM_LIBBE_TEST)
int
setenv(const char *var, const char *value, bool overwrite)
{
	int envindex = 0;
	const int len = strlen(var);
	const int val_len = strlen (value);

	while (environ[envindex] != NULL) {
		if (!strncmp(environ[envindex], var, len)) {
			/* found it */
			if (overwrite) {
				environ[envindex] = (char *)malloc((unsigned)len + val_len + 2);
				sprintf(environ[envindex], "%s=%s", var, value);
			}
			return 0;
		}
		envindex++;
	}

	environ[envindex] = (char *)malloc((unsigned)len + val_len + 2);
	sprintf(environ[envindex], "%s=%s", var, value);
	environ[++envindex] = NULL;
	return 0;
}
#endif


/*
 * reapchild. Child process is out there, let's catch its termination.
 */


/*
 *  spawn_shell(): spawn child process, create pty master/slave device and
 *  execute SHELL program.
 */

/* This pipe use handshake on exec. */
int pc_pipe[2]; /* this pipe is used for parent to child transfer */
int cp_pipe[2]; /* this pipe is used for child to parent transfer */

/* handshake interface */
typedef struct 
{
	int status;		/* status of child */
	char msg[256];	/* error message */
	int row;		/* terminal rows */
	int col;		/* Terminal columns */
} handshake_t;

/* status of handshake */
#define PTY_OK	0	/* pty open and set termios OK */
#define PTY_NG	1	/* pty open or set termios NG */
#define PTY_WS	2	/* pty need WINSIZE (row and col ) */

/* global varriables */

pid_t sh_pid;
char tty_name[B_PATH_NAME_LENGTH];


int
spawn_shell(int row, int col, const char *command, const char *coding)
{
	int done = 0;
	pid_t pgrp;
  
	int master = -1;
	int slave;
	struct termios tio;
	struct winsize ws;
  
	handshake_t handshake;

	int i = 0;
	time_t now_time_t;
	struct tm *now_time;
	int now_hour;
  
	char *args[16];
	char com_line[256];
	char *ptr;

	char err_msg[256];

	signal(SIGTTOU, SIG_IGN);

   /*
	* Get a psuedo-tty. We do this by cycling through files in the
	* directory. The oparationg system will not allow us to open a master
	* which is already in use, so we simply go until the open succeeds.
	*/

	DIR *dir = opendir("/dev/pt/");
	if (dir != NULL) {
		struct dirent *dirEntry;
		while ((dirEntry = readdir(dir)) != NULL) { 
			// skip '.' and '..'
			if (dirEntry->d_name[0] == '.')
				continue;

			char ptyName[B_PATH_NAME_LENGTH];
			sprintf(ptyName, "/dev/pt/%s", dirEntry->d_name);

			master = open(ptyName, O_RDWR);
			if (master >= 0) {
				// Set the tty that corresponds to the pty we found
				sprintf(tty_name, "/dev/tt/%s", dirEntry->d_name);
				break;
			} else {
				// B_BUSY is a normal case
				if (errno != B_BUSY) 
					fprintf(stderr, "could not open %s: %s\n", ptyName, strerror(errno));
			}
		}
		closedir(dir);
	}

	// If master is still < 0 then we haven't found a tty we can use
	if (master < 0) {
    	printf("didn't find any available pesudo ttys.");
    	return -1;
	}

   /*
	* Get the modes of the current terminal. We will duplicates these
	* on the pseudo terminal.
	*/

	/* Create pipe that be use to handshake */
	if (pipe(pc_pipe) || pipe(cp_pipe)) {
		printf ("Could not create handshake pipe.");
		return -1;
	}

	/* Fork a child process. */
	if ((sh_pid = fork()) < 0) {
		close(master);
		return -1;
	}

	if (sh_pid == 0) {
	    // Now in child process.

		/*
		 * Make our controlling tty the pseudo tty. This hapens because
		 * we cleared our original controlling terminal above.
		 */

		// ToDo: why two of them in the first place?
		close(cp_pipe[0]);
		close(pc_pipe[1]);

		/* Set process session leader */
		if ((pgrp = setsid()) < 0) {
			handshake.status = PTY_NG;
			sprintf(handshake.msg, "could not set session leader.");
			write(cp_pipe[1], (char *)&handshake, sizeof (handshake));
			exit(1);
		}

		/* change pty owner and asscess mode. */
		chown(tty_name, getuid(), getgid());
		chmod(tty_name, S_IRUSR | S_IWUSR);

		/* open slave pty */
		if ((slave = open(tty_name, O_RDWR)) < 0) {
			handshake.status = PTY_NG;
			sprintf(handshake.msg, "can't open tty (%s).", tty_name);
			write(cp_pipe[1], (char *)&handshake, sizeof (handshake));
			exit(1);
		}

		/* get tty termios (don't necessary). */
		tcgetattr(slave, &tio);

		/* set signal default */
		signal(SIGCHLD, SIG_DFL);
		signal(SIGHUP, SIG_DFL);
		signal(SIGQUIT, SIG_DFL);
		signal(SIGTERM, SIG_DFL);
		signal(SIGINT, SIG_DFL);
		signal(SIGTTOU, SIG_DFL);

		/*
		 * Set Terminal interface.
		 */

		tio.c_line = 0;
		tio.c_lflag |= ECHOE; 

		/* input: nl->nl, cr->nl */
		tio.c_iflag &= ~(INLCR|IGNCR);
		tio.c_iflag |= ICRNL;
		tio.c_iflag &= ~ISTRIP;

		/* output: cr->cr, nl in not retrun, no delays, ln->cr/ln */
		tio.c_oflag &= ~(OCRNL|ONLRET|NLDLY|CRDLY|TABDLY|BSDLY|VTDLY|FFDLY);
		tio.c_oflag |= ONLCR;
		tio.c_oflag |= OPOST;

		/* baud rate is 19200 (equal beterm) */
		tio.c_cflag &= ~(CBAUD);
		tio.c_cflag |= B19200;

		tio.c_cflag &= ~CSIZE;
		tio.c_cflag |= CS8;
		tio.c_cflag |= CREAD;

		tio.c_cflag |= HUPCL;
		tio.c_iflag &= ~(IGNBRK|BRKINT);

		/*
		 * enable signals, canonical processing (erase, kill, etc), echo.
		*/
		tio.c_lflag |= ISIG|ICANON|ECHO|ECHOE|ECHONL;
		tio.c_lflag &= ~(ECHOK | IEXTEN);

		/* set control charactors. */
		tio.c_cc[VINTR]  = 'C' & 0x1f;	/* '^C'	*/
		tio.c_cc[VQUIT]  = CQUIT;		/* '^\'	*/
		tio.c_cc[VERASE] = 0x08;		/* '^H'	*/
		tio.c_cc[VKILL]  = 'U' & 0x1f;	/* '^U'	*/
		tio.c_cc[VEOF]   = CEOF;		/* '^D' */
		tio.c_cc[VEOL]   = CEOL;		/* '^@' */
		tio.c_cc[VMIN]   = 4;
		tio.c_cc[VTIME]  = 0;
		tio.c_cc[VEOL2]  = CEOL;		/* '^@' */
		tio.c_cc[VSWTCH] = CSWTCH;		/* '^@' */
		tio.c_cc[VSTART] = CSTART;		/* '^S' */
		tio.c_cc[VSTOP]  = CSTOP;		/* '^Q' */
		tio.c_cc[VSUSP]  = CSUSP;		/* '^@' */

		/*
		 * change control tty. 
		 */

		dup2(slave, 0);
		dup2(slave, 1);
		dup2(slave, 2);

		/* close old slave fd. */
		if (slave > 2)
			close(slave);

		/*
		 * set terminal interface.
		 */
		if (tcsetattr (0, TCSANOW, &tio) == -1) {
			handshake.status = PTY_NG;
			sprintf(handshake.msg, "failed set terminal interface (TERMIOS).");
			write(cp_pipe[1], (char *)&handshake, sizeof (handshake));
			exit(1);
		}

		/*
		 * set window size.
		 */

		handshake.status = PTY_WS;
		write(cp_pipe[1], (char *)&handshake, sizeof (handshake));
		read(pc_pipe[0], (char *)&handshake, sizeof (handshake));

		if (handshake.status != PTY_WS) {
			handshake.status = PTY_NG;
			sprintf(handshake.msg, "missmatch handshake.");
			write(cp_pipe[1], (char *)&handshake, sizeof (handshake));
			exit(1);
		}

		ws.ws_row = handshake.row;
		ws.ws_col = handshake.col;

		ioctl(0, TIOCSWINSZ, &ws);

		/*
		 * Set process group ID to process, and Terminal Process group ID
		 * to this process group ID (equal process ID).
		 */

		pgrp = getpid();
		setpgid(pgrp, pgrp);
		tcsetpgrp(0, pgrp);

		/* mark the pipes as close on exec */
		fcntl(cp_pipe[1], F_SETFD, 1);
		fcntl(pc_pipe[0], F_SETFD, 1);

		/* pty open and set termios successful. */
		handshake.status = PTY_OK;
		write(cp_pipe[1], (char *)&handshake, sizeof (handshake));

		/*
		 * setenv TERM and TTY.
		 */
		setenv("TERM", "beterm", true);
		setenv("TTY", tty_name, true);
		setenv("TTYPE", coding, true);

		/*
		 * If don't set command args, exec SHELL_COMMAND.
		 */
		if (command == NULL)
			command = SHELL_COMMAND;

		memcpy (com_line, command, 256);
		ptr = com_line;

		/*
		 * split up the arguments in the command into an artv-like structure.
		 */

		while (*ptr) {
			/* Skip white space */
			while ((*ptr == ' ') || (*ptr == '\t'))
				*ptr++ = 0;
				args[i++] = ptr;

			/* Skip over this word to next white space. */
			while ((*ptr != 0) && (*ptr != ' ') && (*ptr != '\t'))
			ptr++;
		}

		args[i] = NULL;

		setenv("SHELL", *args, true);

		/*
		 * Print Welcome Message.
		 * (But, Only print message when MuTerminal coding is UTF8.)
		 */
		now_time_t = time(NULL);
		now_time = localtime (&now_time_t);

		if (now_time->tm_hour >= 5 && now_time->tm_hour < 11) {
			now_hour = 0;
		} else if (now_time->tm_hour >= 11 && now_time->tm_hour <= 18 ) {
			now_hour = 1;
		} else {
			now_hour = 2;
		}

		execve(*args, args, environ);

		/*
		 * Exec failed.
		 */
		sleep(1);
		sprintf(err_msg, kSpawnAlertMessage, com_line, strerror(errno));

		if (system(err_msg) == 0)
			execl("/bin/sh", "/bin/sh", "-login", NULL);

		exit(1);
	}

	/*
	 * In parent Process, Set up the input and output file pointers so 
	 * that they can write and read the pseudo terminal.
	 */

	/* close childs's side of the pipe */
	close(cp_pipe[1]);
	close(pc_pipe[0]);

	/*
	 * close parent control tty.
	 */

	while (!done) {
		read (cp_pipe[0], (char *)&handshake, sizeof (handshake));

		switch (handshake.status) {
			case PTY_OK:
				done = 1;
				break;

			case PTY_NG:
				printf("%s\n", handshake.msg);
				done = -1;
				break;

			case PTY_WS:
				handshake.row = row;
				handshake.col = col;
				handshake.status = PTY_WS;
				write(pc_pipe[1], (char *)&handshake, sizeof (handshake));
				break;
		}
	}
  
	return (done > 0) ? master : -1;
}

