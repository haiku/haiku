/*
 * Copyright 2010, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef ACTIVE_PROCESS_INFO_H
#define ACTIVE_PROCESS_INFO_H


#include <OS.h>
#include <String.h>


class ActiveProcessInfo {
public:
								ActiveProcessInfo();

			void				SetTo(pid_t id, pid_t shellID,
									const BString& name,
									const BString& currentDirectory);
			void				Unset();

			bool				IsValid() const			{ return fID >= 0; }

			pid_t				ID() const				{ return fID; }
			pid_t				ShellProcessID() const	{ return fShellID; }

			const BString&		Name() const			{ return fName; }
			const BString&		CurrentDirectory() const
									{ return fCurrentDirectory; }

private:
			pid_t				fID;
			pid_t				fShellID;
			BString				fName;
			BString				fCurrentDirectory;
};



#endif	// ACTIVE_PROCESS_INFO_H
