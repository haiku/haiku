#ifndef _BLUE_STOP_WATCH_H
#define _BLUE_STOP_WATCH_H

#include <BeBuild.h>
#include "support/SupportDefs.h"

#ifdef USE_OPENBEOS_NAMESPACE
namespace OpenBeOS {
#endif

class BStopWatch {
  public:
					BStopWatch(const char *name, bool silent = false);
	virtual			~BStopWatch();

	void			Suspend();
	void			Resume();
	bigtime_t		Lap();
	bigtime_t		ElapsedTime() const;
	void			Reset();
	const char		*Name() const;

  private:
	virtual	void	_ReservedStopWatch1();
	virtual	void	_ReservedStopWatch2();

	bigtime_t		fStart;
	bigtime_t		fSuspendTime;
	bigtime_t		fLaps[10];
	int32			fLap;
	const char		*fName;
	uint32			_reserved[2];	// these are for fName to be initiased
	bool			fSilent;
};

#ifdef USE_OPENBEOS_NAMESPACE
}	// namespace OpenBeOS
using namespace OpenBeOS;
#endif

#endif /* _STOP_WATCH_H */
