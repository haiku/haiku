// 100% done
#include "support/StopWatch.h"
#include <OS.h>
#include <stdio.h>

#ifdef USE_OPENBEOS_NAMESPACE
namespace OpenBeOS {
#endif

BStopWatch::BStopWatch(const char *name, bool silent){
	fSilent = silent;
	fName = name;
	Reset();
}

BStopWatch::~BStopWatch(){
	if (!fSilent){
		printf("StopWatch \"%s\": %d usecs.", fName, (int)ElapsedTime() );
	
		if (fLap){
			for (int i=1; i<=fLap; i++){
				if (!((i-1)%4))	printf("\n   ");
				printf("[%d: %d#%d] ", i, (int)(fLaps[i]-fStart), (int)(fLaps[i] - fLaps[i-1]) );
			}
			printf("\n");
		}
	}
}

void BStopWatch::Suspend(){
	if (!fSuspendTime)
		fSuspendTime = system_time();
}

void BStopWatch::Resume(){
	if (fSuspendTime)
		fStart = system_time() - fSuspendTime - fStart;
}

bigtime_t BStopWatch::Lap(){
	if (!fSuspendTime){
		if (fLap<9) fLap++;
		fLaps[fLap] = system_time();
		return (system_time()-fStart);
	}else
		return 0;
}

bigtime_t BStopWatch::ElapsedTime() const{
	if (fSuspendTime)
		return (fSuspendTime-fStart);
	else
		return (system_time()-fStart);
}

void BStopWatch::Reset(){
	fStart = system_time();		// store current time
	fSuspendTime = 0;
	fLap = 0;					// clear laps
	for (int i=0; i<10; i++)
		fLaps[i] = fStart;
}

const char *BStopWatch::Name() const{
	return fName;
}

// just for future binary compatibility
void BStopWatch::_ReservedStopWatch1()	{}
void BStopWatch::_ReservedStopWatch2()	{}

#ifdef USE_OPENBEOS_NAMESPACE
}	// namespace OpenBeOS
#endif
