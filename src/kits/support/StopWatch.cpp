//------------------------------------------------------------------------------
//	Copyright (c) 2001-2004, Haiku
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
//	File Name:		StopWatch.cpp
//	Author(s):		unknown
//					
//	Description:	Timer Class, mostly useful for debugging 
//	
//					
//
//------------------------------------------------------------------------------

#include <OS.h>		// for system_time()
#include <StopWatch.h>

#include <stdio.h>


BStopWatch::BStopWatch(const char *name, bool silent)
	:
	fName(name),
	fSilent(silent)
{
	Reset();
}


BStopWatch::~BStopWatch()
{
	if (!fSilent){
		printf("StopWatch \"%s\": %d usecs.\n", fName, (int)ElapsedTime());
	
		if (fLap) {
			for (int i = 1; i <= fLap; i++){
				if (!((i-1)%4))
					printf("\n   ");
				printf("[%d: %d#%d] ", i, (int)(fLaps[i]-fStart), (int)(fLaps[i] - fLaps[i -1 ]));
			}
			printf("\n");
		}
	}
}


void
BStopWatch::Suspend()
{
	if (!fSuspendTime)
		fSuspendTime = system_time();
}


void
BStopWatch::Resume()
{
	if (fSuspendTime) {
		fStart += system_time() - fSuspendTime;
		fSuspendTime = 0;
	}
}


bigtime_t
BStopWatch::Lap()
{
	if (!fSuspendTime){
		if (fLap<9)
			fLap++;
		fLaps[fLap] = system_time();
		
		return system_time() - fStart;
	} else
		return 0;
}


bigtime_t
BStopWatch::ElapsedTime() const
{
	if (fSuspendTime)
		return fSuspendTime - fStart;
	else
		return system_time() - fStart;
}


void
BStopWatch::Reset()
{
	fStart = system_time();		// store current time
	fSuspendTime = 0;
	fLap = 0;					// clear laps
	for (int i = 0; i < 10; i++)
		fLaps[i] = fStart;
}


const char *
BStopWatch::Name() const
{
	return fName != NULL ? fName : "";
}


// just for future binary compatibility
void BStopWatch::_ReservedStopWatch1()	{}
void BStopWatch::_ReservedStopWatch2()	{}

