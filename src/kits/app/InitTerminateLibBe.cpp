//------------------------------------------------------------------------------
//	Copyright (c) 2001-2002, OpenBeOS
//
//	Permission is hereby granted, free of charge, to any person obtaining a
//	copy of this software and associated documentation files (the "Software"),
//	to deal in the Software without restriction, including without limitation
//	the rights to use, copy, modify, merge, publish, distribute, sublicense,
//	and/or sell copies of the Software, and to permit persons to whom the
//	Software is furnished to do so, subject to the following conditions:
//
//	The above copyright notice and this permission notice shall be included in
//	all copies or substantial portions of the Software.
//
//	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
//	FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
//	DEALINGS IN THE SOFTWARE.
//
//	File Name:		InitTerminateLibBe.cpp
//	Author(s):		Ingo Weinhold (bonefish@users.sf.net)
//	Description:	Global library initialization/termination routines.
//------------------------------------------------------------------------------
#include <stdio.h>

#include <ClipboardPrivate.h>
#include <MessagePrivate.h>
#include <RosterPrivate.h>

// debugging
//#define DBG(x) x
#define DBG(x)
#define OUT	printf

// initialize_before
extern "C"
void
initialize_before()
{
DBG(OUT("initialize_before()\n"));

	BMessage::Private::StaticInit();
	BRoster::Private::InitBeRoster();
	BPrivate::init_clipboard();

DBG(OUT("initialize_before() done\n"));
}

// terminate_after
extern "C"
void
terminate_after()
{
DBG(OUT("terminate_after()\n"));

	BRoster::Private::DeleteBeRoster();
	BMessage::Private::StaticCleanup();
	BMessage::Private::StaticCacheCleanup();

DBG(OUT("terminate_after() done\n"));
}

