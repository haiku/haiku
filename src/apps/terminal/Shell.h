/*
 * Copyright 2007 Haiku, Inc.
 * Copyright (c) 2003-4 Kian Duffy <myob@users.sourceforge.net>
 * Copyright (c) 2004 Daniel Furrer <assimil8or@users.sourceforge.net>
 * Parts Copyright (C) 1998,99 Kazuho Okui and Takashi Murai. 
 * Distributed under the terms of the MIT License.
 * Authors:
 *		Stefano Ceccherini <stefano.ceccherini@gmail.com>
 *		Kian Duffy <myob@users.sourceforge.net>
 *		Kazuho Okui
 *		Takashi Murai 
 */
#ifndef _SHELL_H
#define _SHELL_H

#include <SupportDefs.h>

// TODO: Maybe merge TermParse and Shell classes ?
class TermParse;
class TermView;
class Shell {
public:
			Shell();
	virtual		~Shell();

	status_t	Open(int row, int col, const char *command, const char *coding);
	void		Close();
	
	const char *	TTYName() const;

	ssize_t		Read(void *buffer, size_t numBytes);
	ssize_t		Write(const void *buffer, size_t numBytes);

	status_t	UpdateWindowSize(int row, int columns);
	status_t	Signal(int signal);

	status_t	GetAttr(struct termios &attr);
	status_t	SetAttr(struct termios &attr);

	int		FD() const;	

	virtual	void	ViewAttached(TermView *view);
	virtual void	ViewDetached();
	
private:
	int		fFd;
	pid_t		fProcessID;
	TermParse	*fTermParse;
	bool		fAttached;

	status_t	_Spawn(int row, int col, const char *command, const char *coding);
};

#endif // _SHELL_H
