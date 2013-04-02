/*
 * Copyright 2010, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef SHELL_INFO_H
#define SHELL_INFO_H


#include <OS.h>
#include <String.h>


class ShellInfo {
public:
								ShellInfo();

			pid_t				ProcessID() const
									{ return fProcessID; }
			void				SetProcessID(pid_t processID)
									{ fProcessID = processID; }

			bool				IsDefaultShell() const
									{ return fIsDefaultShell; }
			void				SetDefaultShell(bool isDefault)
									{ fIsDefaultShell = isDefault; }

			int					Encoding() const
									{ return fEncoding; }
	const	BString&			EncodingName() const
									{ return fEncodingName; }
			void				SetEncoding(int encoding);

private:
			pid_t				fProcessID;
			bool				fIsDefaultShell;
			int					fEncoding;
			BString				fEncodingName;
};


#endif	// SHELL_INFO_H
