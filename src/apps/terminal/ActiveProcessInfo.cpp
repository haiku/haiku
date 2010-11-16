/*
 * Copyright 2010, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "ActiveProcessInfo.h"


ActiveProcessInfo::ActiveProcessInfo()
	:
	fID(-1),
	fShellID(-1),
	fName(),
	fCurrentDirectory()
{
}


void
ActiveProcessInfo::SetTo(pid_t id, pid_t shellID, const BString& name,
	const BString& currentDirectory)
{
	fID = id;
	fShellID = shellID;
	fName = name;
	fCurrentDirectory = currentDirectory;
}


void
ActiveProcessInfo::Unset()
{
	fID = -1;
	fShellID = -1;
	fName = BString();
	fCurrentDirectory = BString();
}
