/*
 * Copyright 2013, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "HyperLink.h"

#include <errno.h>
#include <stdlib.h>

#include "TermConst.h"


HyperLink::HyperLink()
	:
	fAddress(),
	fType(TYPE_URL),
	fOSCRef(0),
	fOSCID(NULL)
{
}


HyperLink::HyperLink(const BString& address, Type type)
	:
	fText(address),
	fAddress(address),
	fType(type),
	fOSCRef(0),
	fOSCID(NULL)
{
}


HyperLink::HyperLink(const BString& text, const BString& address, Type type)
	:
	fText(text),
	fAddress(address),
	fType(type),
	fOSCRef(0),
	fOSCID(NULL)
{
}


HyperLink::HyperLink(const BString& address, uint32 ref, const BString& id)
	:
	fText(NULL),
	fAddress(address),
	fType(TYPE_OSC_URL),
	fOSCRef(ref),
	fOSCID(id)
{
}


status_t
HyperLink::Open()
{
	if (!IsValid())
		return B_BAD_VALUE;

	// open with the "open" program
	BString address(fAddress);
	address.CharacterEscape(kShellEscapeCharacters, '\\');
	BString commandLine;
	commandLine.SetToFormat("/bin/open %s", address.String());
	return system(commandLine) == 0 ? B_OK : errno;
}
