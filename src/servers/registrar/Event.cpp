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
//	File Name:		Event.cpp
//	Author:			Ingo Weinhold (bonefish@users.sf.net)
//					YellowBites (http://www.yellowbites.com)
//	Description:	Base class for events as handled by EventQueue.
//------------------------------------------------------------------------------

#include "Event.h"

// constructor
Event::Event(bool autoDelete)
	: fTime(0),
	  fAutoDelete(autoDelete)
{
}

// constructor
Event::Event(bigtime_t time, bool autoDelete)
	: fTime(time),
	  fAutoDelete(autoDelete)
{
}

// destructor
Event::~Event()
{
}

// SetTime
void
Event::SetTime(bigtime_t time)
{
	fTime = time;
}

// Time
bigtime_t
Event::Time() const
{
	return fTime;
}

// SetAutoDelete
void
Event::SetAutoDelete(bool autoDelete)
{
	fAutoDelete = autoDelete;
}

// IsAutoDelete
bool
Event::IsAutoDelete() const
{
	return fAutoDelete;
}

// Do
bool
Event::Do()
{
	return fAutoDelete;
}

