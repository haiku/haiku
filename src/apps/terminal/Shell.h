/*
 * Copyright 2007 Haiku, Inc.
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
