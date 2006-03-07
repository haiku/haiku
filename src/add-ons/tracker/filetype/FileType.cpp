/*
 * Copyright 2006, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include <Roster.h>


/*!
	\brief Tracker add-on entry
*/
extern "C" void
process_refs(entry_ref dir, BMessage* refs, void* /*reserved*/)
{
	be_roster->Launch("application/x-vnd.haiku-filetypes", refs);
}
