/*	
 * Copyright (c) 2000-2008, Ingo Weinhold <ingo_weinhold@gmx.de>,
 * Copyright (c) 2000-2008, Stephan Aßmus <superstippi@gmx.de>,
 * All Rights Reserved. Distributed under the terms of the MIT license.
 */
#include <stdio.h>

#include "Event.h"


Event::Event(bool autoDelete)
	: fTime(0),
	  fAutoDelete(autoDelete)
{
}


Event::Event(bigtime_t time, bool autoDelete)
	: fTime(time),
	  fAutoDelete(autoDelete)
{
}


Event::~Event()
{
}


void
Event::SetTime(bigtime_t time)
{
	fTime = time;
}


bigtime_t
Event::Time() const
{
	return fTime;
}


void
Event::SetAutoDelete(bool autoDelete)
{
	fAutoDelete = autoDelete;
}


void
Event::Execute()
{
	printf("Event::Execute() - %Ld\n", fTime);
}

