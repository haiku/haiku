/*
 * MiniTerminal - A basic windowed terminal to allow
 * command-line interaction from within app_server.
 * Based on consoled and MuTerminal.
 *
 * Copyright 2005 Michael Lotz. All rights reserved.
 * Distributed under the Haiku License.
 *
 * Copyright 2004-2005, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2002, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */


#include <OS.h>
#include <image.h>

#include <termios.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

#include "MiniView.h"
#include "Console.h"

#include "VTkeymap.h"

//#define TRACE_MINI_TERMINAL
#ifdef TRACE_MINI_TERMINAL
	#ifdef __HAIKU__
	#define TRACE(x)	debug_printf x
	#else
	#define	TRACE(x)	printf x
	#endif
#else
	#define TRACE(x)
#endif

void
Setenv(const char *var, const char *value)
{
	int envindex = 0;
	const int len = strlen(var);
	const int val_len = strlen (value);
  
	while (environ [envindex] != NULL) {
		if (strncmp (environ [envindex], var, len) == 0) {
			/* found it */
			environ[envindex] = (char *)malloc ((unsigned)len + val_len + 1);
			sprintf (environ [envindex], "%s%s", var, value);
			return;
		}
		envindex ++;
	}
	
	environ [envindex] = (char *) malloc ((unsigned)len + val_len + 1);
	sprintf (environ [envindex], "%s%s", var, value);
	environ [++envindex] = NULL;
}

static int32
ConsoleWriter(void *arg)
{
	char buf[1024];
	ssize_t len;
	MiniView *view = (MiniView *)arg;
	
	for (;;) {
		len = read(view->fMasterFD, buf, sizeof(buf));
		if (len < 0)
			break;
		
		view->fConsole->Write(buf, len);
	}
	
	return 0;
}

static int32
ExecuteShell(void *arg)
{
	MiniView *view = (MiniView *)arg;
	
	for (;;) {
		const char *argv[3];
		argv[0] = "/bin/sh";
		argv[1] = "--login";
		argv[2] = NULL;
		
		int saved_stdin = dup(0);
		int saved_stdout = dup(1);
		int saved_stderr = dup(2);
		
		dup2(view->fSlaveFD, 0);
		dup2(view->fSlaveFD, 1);
		dup2(view->fSlaveFD, 2);
		
		thread_id shell = load_image(2, argv, (const char **)environ);
		setpgid(shell, 0);
		
		status_t return_code;
		wait_for_thread(shell, &return_code);
		
		dup2(saved_stdin, 0);
		dup2(saved_stdout, 1);
		dup2(saved_stderr, 2);
		close(saved_stdin);
		close(saved_stdout);
		close(saved_stderr);
	}
	
	return B_OK;
}

MiniView::MiniView(BRect frame)
	:	ViewBuffer(frame)
{
}

MiniView::~MiniView()
{
}

void
MiniView::Start()
{
	fConsole = new Console(this);
	
	if (OpenTTY() != B_OK) {
		TRACE(("error in OpenTTY\n"));
		return;
	}
	
	// we're a session leader
	setsid();
	
	// move our stdin and stdout to the console
	dup2(fSlaveFD, 0);
	dup2(fSlaveFD, 1);
	dup2(fSlaveFD, 2);
	
	if (SpawnThreads() != B_OK)
		TRACE(("error in SpawnThreads\n"));
}

status_t
MiniView::OpenTTY()
{
	DIR *dir;
	
	dir = opendir("/dev/pt");
	if (dir != NULL) {
		struct dirent *entry;
		char name[64];
		
		while ((entry = readdir(dir)) != NULL) {
			if (entry->d_name[0] == '.') // filter out . and ..
				continue;
			
			sprintf(name, "/dev/pt/%s", entry->d_name);
			
			fMasterFD = open(name, O_RDWR);
			if (fMasterFD >= 0) {
				sprintf(name, "/dev/tt/%s", entry->d_name);
				
				fSlaveFD = open(name, O_RDWR);
				if (fSlaveFD < 0) {
					TRACE(("cannot open tty\n"));
					close(fMasterFD);
				} else {
					struct termios tio;
					tcgetattr(fSlaveFD, &tio);
					
					// set signal default
					signal(SIGCHLD, SIG_DFL);
					signal(SIGHUP, SIG_DFL);
					signal(SIGQUIT, SIG_DFL);
					signal(SIGTERM, SIG_DFL);
					signal(SIGINT, SIG_DFL);
					signal(SIGTTOU, SIG_DFL);
					
					// set terminal interface
					tio.c_line = 0;
					tio.c_lflag |= ECHOE; 
					
					// input: nl->nl, cr->nl
					tio.c_iflag &= ~(INLCR|IGNCR);
					tio.c_iflag |= ICRNL;
					tio.c_iflag &= ~ISTRIP;
					
					// output: cr->cr, nl in not retrun, no delays, ln->cr/ln
					tio.c_oflag &= ~(OCRNL|ONLRET|NLDLY|CRDLY|TABDLY|BSDLY|VTDLY|FFDLY);
					tio.c_oflag |= ONLCR;
					tio.c_oflag |= OPOST;
					
					// baud rate is 19200 (equal to beterm)
					tio.c_cflag &= ~(CBAUD);
					tio.c_cflag |= B19200;
					
					tio.c_cflag &= ~CSIZE;
					tio.c_cflag |= CS8;
					tio.c_cflag |= CREAD;
					
					tio.c_cflag |= HUPCL;
					tio.c_iflag &= ~(IGNBRK|BRKINT);
					
					// enable signals, canonical processing (erase, kill, etc), echo
					tio.c_lflag |= ISIG|ICANON|ECHO|ECHOE|ECHONL;
					tio.c_lflag &= ~ECHOK;
					
					tio.c_lflag &= ~IEXTEN;
					
					// set control charactors
					tio.c_cc[VINTR]  = 'C' & 0x1f;	/* '^C'	*/
					tio.c_cc[VQUIT]  = '\\'& 0x1f;	/* '^\'	*/
					tio.c_cc[VERASE] = 0x08;		/* '^H'	*/
					tio.c_cc[VKILL]  = 'U' & 0x1f;	/* '^U'	*/
					tio.c_cc[VEOF]   = 'D' & 0x1f;	/* '^D' */
					tio.c_cc[VEOL]   = 0;			/* '^@' */
					tio.c_cc[VMIN]   = 4;
					tio.c_cc[VTIME]  = 0;
					tio.c_cc[VEOL2]  = 0;			/* '^@' */
					tio.c_cc[VSWTCH] = 0;			/* '^@' */
					tio.c_cc[VSTART] = 'S' & 0x1f;	/* '^S' */
					tio.c_cc[VSTOP]  = 'Q' & 0x1f;	/* '^Q' */
					tio.c_cc[VSUSP]  = '@' & 0x1f;	/* '^@' */
					
					// set terminal interface
					tcsetattr(fSlaveFD, TCSANOW, &tio);
					
					// set window size (currently disabled)
					/*ws.ws_row = rows;
					ws.ws_col = cols;

					ioctl(con->tty_slave_fd, TIOCSWINSZ, &ws);*/
				}
				break;
			}
		}
		
		
		Setenv("TTY", name);
	}
	
	if (fMasterFD < 0 || fSlaveFD < 0) {
		TRACE(("could not open master or slave fd\n"));
		return B_ERROR;
	}
	
	Setenv("TERM", "beterm");
	return B_OK;
}

void
MiniView::KeyDown(const char *bytes, int32 numBytes)
{
	// TODO: add interrupt char handling
	uint32 mod = modifiers();
	if (numBytes == 1) {
		if (mod & B_OPTION_KEY) {
			char c = bytes[0] | 0x80;
			write(fMasterFD, &c, 1);
		} else {
			switch (bytes[0]) {
				case B_LEFT_ARROW:
					write(fMasterFD, LEFT_ARROW_KEY_CODE, sizeof(LEFT_ARROW_KEY_CODE));
					break;
				case B_RIGHT_ARROW:
					write(fMasterFD, RIGHT_ARROW_KEY_CODE, sizeof(RIGHT_ARROW_KEY_CODE));
					break;
				case B_UP_ARROW:
					write(fMasterFD, UP_ARROW_KEY_CODE, sizeof(UP_ARROW_KEY_CODE));
					break;
				case B_DOWN_ARROW:
					write(fMasterFD, DOWN_ARROW_KEY_CODE, sizeof(DOWN_ARROW_KEY_CODE));
					break;
				default:
					write(fMasterFD, bytes, numBytes);
			}
		}
	} else
		write(fMasterFD, bytes, numBytes);
}

status_t
MiniView::SpawnThreads()
{
	fConsoleWriter = spawn_thread(&ConsoleWriter, "console writer", B_URGENT_DISPLAY_PRIORITY, this);
	if (fConsoleWriter < 0)
		return B_ERROR;
	TRACE(("console writer thread is: %d\n", fConsoleWriter));
	
	fShellProcess = spawn_thread(&ExecuteShell, "shell process", B_URGENT_DISPLAY_PRIORITY, this);
	if (fShellProcess < 0)
		return B_ERROR;
	TRACE(("shell process thread is: %d\n", fShellProcess));
	
	resume_thread(fConsoleWriter);
	resume_thread(fShellProcess);
	return B_OK;
}
