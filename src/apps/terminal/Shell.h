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


#include "ShellInfo.h"


class ActiveProcessInfo;
class ShellParameters;
// TODO: Maybe merge TermParse and Shell classes ?
class TerminalBuffer;
class TermParse;


class Shell {
public:
								Shell();
	virtual						~Shell();

			status_t			Open(int row, int col,
									const ShellParameters& parameters);
			void				Close();

			const char*			TTYName() const;

			ssize_t				Read(void* buffer, size_t numBytes) const;
			ssize_t				Write(const void* buffer, size_t numBytes);

			status_t			UpdateWindowSize(int row, int columns);

			status_t			GetAttr(struct termios& attr) const;
			status_t			SetAttr(const struct termios& attr);

			int					FD() const;
			pid_t				ProcessID() const
									{ return fShellInfo.ProcessID(); }
			const ShellInfo&	Info() const
									{ return fShellInfo; }

			bool				HasActiveProcesses() const;
			bool				GetActiveProcessInfo(
									ActiveProcessInfo& _info) const;

	virtual	status_t			AttachBuffer(TerminalBuffer* buffer);
	virtual void				DetachBuffer();

private:
			status_t			_Spawn(int row, int col,
									const ShellParameters& parameters);

private:
			ShellInfo			fShellInfo;
			int					fFd;
			pid_t				fProcessID;
			TermParse*			fTermParse;
			bool				fAttached;
};


#endif // _SHELL_H
