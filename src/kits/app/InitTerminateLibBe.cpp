/*
 * Copyright 2001-2005, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Ingo Weinhold (bonefish@users.sf.net)
 */

//!	Global library initialization/termination routines.


#include <stdio.h>

#include <ClipboardPrivate.h>
#include <MessagePrivate.h>
#include <RosterPrivate.h>


// debugging
//#define DBG(x) x
#define DBG(x)
#define OUT	printf


extern "C" void
initialize_before()
{
	DBG(OUT("initialize_before()\n"));

	BMessage::Private::StaticInit();
	BRoster::Private::InitBeRoster();
	BPrivate::init_clipboard();

	DBG(OUT("initialize_before() done\n"));
}


extern "C" void
terminate_after()
{
	DBG(OUT("terminate_after()\n"));

	BRoster::Private::DeleteBeRoster();
	BMessage::Private::StaticCleanup();
	BMessage::Private::StaticCacheCleanup();

	DBG(OUT("terminate_after() done\n"));
}

