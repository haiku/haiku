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
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <string.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdio.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>

#include <OS.h>

#include "TermConst.h"
#include "Shell.h"
#include "PrefHandler.h"

/* default shell command and options. */
#define SHELL_COMMAND "/bin/sh -login"


const static char *kSpawnAlertMessage = "alert --stop " "'Cannot execute \"%s\":\n"
	"\t%s\n'"
	"'Use Default Shell' 'Abort'";

/*
 * Set environment variable.
 */
#if !defined(__HAIKU__) || defined(HAIKU_TARGET_PLATFORM_LIBBE_TEST)

extern char **environ;

static int
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
				snprintf(environ[envindex], "%s=%s", var, value);
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
 *  spawn_shell(): spawn child process, create pty master/slave device and
 *  execute SHELL program.
 */

/* handshake interface */
typedef struct 
{
	int status;		/* status of child */
	char msg[128];	/* error message */
	int row;		/* terminal rows */
	int col;		/* Terminal columns */
} handshake_t;

/* status of handshake */
#define PTY_OK	0	/* pty open and set termios OK */
#define PTY_NG	1	/* pty open or set termios NG */
#define PTY_WS	2	/* pty need WINSIZE (row and col ) */



static pid_t sShPid;


static status_t
send_handshake_message(thread_id target, const handshake_t& handshake)
{
	return send_data(target, 0, &handshake, sizeof(handshake_t));
}


static void
receive_handshake_message(handshake_t& handshake)
{
	thread_id sender;
	receive_data(&sender, &handshake, sizeof(handshake_t));
}


static int
spawn_shell(int row, int col, const char *command, const char *coding)
{
	signal(SIGTTOU, SIG_IGN);
	
	/*
	 * Get a pseudo-tty. We do this by cycling through files in the
	 * directory. The oparationg system will not allow us to open a master
	 * which is already in use, so we simply go until the open succeeds.
	 */
	char ttyName[B_PATH_NAME_LENGTH];
	int master = -1;	
	DIR *dir = opendir("/dev/pt/");
	if (dir != NULL) {
		struct dirent *dirEntry;
		while ((dirEntry = readdir(dir)) != NULL) { 
			// skip '.' and '..'
			if (dirEntry->d_name[0] == '.')
				continue;

			char ptyName[B_PATH_NAME_LENGTH];
			snprintf(ptyName, sizeof(ptyName), "/dev/pt/%s", dirEntry->d_name);

			master = open(ptyName, O_RDWR);
			if (master >= 0) {
				// Set the tty that corresponds to the pty we found
				snprintf(ttyName, sizeof(ttyName), "/dev/tt/%s", dirEntry->d_name);
				break;
			} else {
				// B_BUSY is a normal case
				if (errno != B_BUSY) 
					fprintf(stderr, "could not open %s: %s\n", ptyName, strerror(errno));
			}
		}
		closedir(dir);
	}

	if (master < 0) {
    		printf("didn't find any available pseudo ttys.");
    		return -1;
	}

   /*
	* Get the modes of the current terminal. We will duplicates these
	* on the pseudo terminal.
	*/

	thread_id terminalThread = find_thread(NULL);

	/* Fork a child process. */
	if ((sShPid = fork()) < 0) {
		close(master);
		return -1;
	}


	handshake_t handshake;

	if (sShPid == 0) {
	    // Now in child process.

		/*
		 * Make our controlling tty the pseudo tty. This hapens because
		 * we cleared our original controlling terminal above.
		 */

		/* Set process session leader */
		if (setsid() < 0) {
			handshake.status = PTY_NG;
			snprintf(handshake.msg, sizeof(handshake.msg),
				"could not set session leader.");
			send_handshake_message(terminalThread, handshake);
			exit(1);
		}

		/* change pty owner and access mode. */
		chown(ttyName, getuid(), getgid());
		chmod(ttyName, S_IRUSR | S_IWUSR);

		/* open slave pty */
		int slave = -1;
		if ((slave = open(ttyName, O_RDWR)) < 0) {
			handshake.status = PTY_NG;
			snprintf(handshake.msg, sizeof(handshake.msg),
				"can't open tty (%s).", ttyName);
			send_handshake_message(terminalThread, handshake);
			exit(1);
		}

		struct termios tio;
	
		/* get tty termios (not necessary).
		 * TODO: so why are we doing it ?
		 */
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

		/* set control characters. */
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
			snprintf(handshake.msg, sizeof(handshake.msg),
				"failed set terminal interface (TERMIOS).");
			send_handshake_message(terminalThread, handshake);
			exit(1);
		}

		/*
		 * set window size.
		 */

		handshake.status = PTY_WS;
		send_handshake_message(terminalThread, handshake);
		receive_handshake_message(handshake);

		if (handshake.status != PTY_WS) {
			handshake.status = PTY_NG;
			snprintf(handshake.msg, sizeof(handshake.msg),
				"mismatch handshake.");
			send_handshake_message(terminalThread, handshake);
			exit(1);
		}

		struct winsize ws;
  	
		ws.ws_row = handshake.row;
		ws.ws_col = handshake.col;

		ioctl(0, TIOCSWINSZ, &ws);

		/*
		 * Set process group ID to process, and Terminal Process group ID
		 * to this process group ID (equal process ID).
		 */

		pid_t processGroup = getpid();
		if (setpgid(processGroup, processGroup) < 0) {
			handshake.status = PTY_NG;
			snprintf(handshake.msg, sizeof(handshake.msg),
				"can't set process group id.");
			send_handshake_message(terminalThread, handshake);
			exit(1);		
		}
		tcsetpgrp(0, processGroup);

		/* pty open and set termios successful. */
		handshake.status = PTY_OK;
		send_handshake_message(terminalThread, handshake);

		/*
		 * setenv TERM and TTY.
		 */
		setenv("TERM", "beterm", true);
		setenv("TTY", ttyName, true);
		setenv("TTYPE", coding, true);

		/*
		 * If don't set command args, exec SHELL_COMMAND.
		 */
		if (command == NULL)
			command = SHELL_COMMAND;
		
		/*
		 * split up the arguments in the command into an artv-like structure.
		 */
		char commandLine[256];
		memcpy(commandLine, command, 256);
		char *ptr = commandLine;
		char *args[16];
		int i = 0;
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

#if 0
		/*
		 * Print Welcome Message.
		 * (But, Only print message when MuTerminal coding is UTF8.)
		 */
		
		time_t now_time_t = time(NULL);
		struct tm *now_time = localtime (&now_time_t);
		int now_hour = 0;
		if (now_time->tm_hour >= 5 && now_time->tm_hour < 11) {
			now_hour = 0;
		} else if (now_time->tm_hour >= 11 && now_time->tm_hour <= 18 ) {
			now_hour = 1;
		} else {
			now_hour = 2;
		}
#endif
		execve(*args, args, environ);

		/*
		 * Exec failed.
		 */
		sleep(1);
		char errorMessage[256];
		snprintf(errorMessage, sizeof(errorMessage), kSpawnAlertMessage, commandLine, strerror(errno));

		if (system(errorMessage) == 0)
			execl("/bin/sh", "/bin/sh", "-login", NULL);

		exit(1);
	}

	/*
	 * In parent Process, Set up the input and output file pointers so 
	 * that they can write and read the pseudo terminal.
	 */

	/*
	 * close parent control tty.
	 */

	int done = 0;
	while (!done) {
		receive_handshake_message(handshake);

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
				send_handshake_message(sShPid, handshake);
				break;
		}
	}
  
	return (done > 0) ? master : -1;
}


static void
close_shell(int fd)
{
	if (fd < 0)
		return;

	close(fd);
	
	int status;
	kill(-sShPid, SIGHUP);
	wait(&status);	
}


Shell::Shell()
	:fFd(-1)
{
}


Shell::~Shell()
{
	Close();
}


status_t
Shell::Open(int row, int col, const char *command, const char *coding)
{
	fFd = spawn_shell(row, col, command, coding);
	if (fFd < 0)
		return fFd;

	return B_OK;
}


void
Shell::Close()
{
	if (fFd >= 0) {
		close_shell(fFd);
		fFd = -1;	
	}
}


const char *
Shell::TTYName() const
{
	return ttyname(fFd);
}


ssize_t
Shell::Read(void *buffer, size_t numBytes)
{
	if (fFd < 0)
		return B_NO_INIT;
	
	return read(fFd, buffer, numBytes);
}


ssize_t
Shell::Write(const void *buffer, size_t numBytes)
{
	if (fFd < 0)
		return B_NO_INIT;
	
	return write(fFd, buffer, numBytes);
}


void
Shell::UpdateWindowSize(int rows, int columns)
{
	struct winsize winSize;
	winSize.ws_row = rows;
	winSize.ws_col = columns;
	ioctl(fFd, TIOCSWINSZ, &winSize);
	Signal(SIGWINCH);
}


void
Shell::Signal(int signal)
{
	kill(-sShPid, signal);
}


status_t
Shell::GetAttr(struct termios &attr)
{
	return tcgetattr(fFd, &attr);
}


status_t
Shell::SetAttr(struct termios &attr)
{
	return tcsetattr(fFd, TCSANOW, &attr);
}


int
Shell::FD() const
{
	return fFd;
}

