#ifndef __LEDANIMATION_H__
#define __LEDANIMATION_H__

//! Keyboard LED Animation class.

#include <SupportDefs.h>
#include <OS.h>

class LEDAnimation {
public:
				//! Constructor
							LEDAnimation();
				//! Destructor
							~LEDAnimation();
				//!Start LED animation.
				void		Start();
				//!Stop LED animation.
				void		Stop();
				//! Check animation thread is running.
				bool		IsRunning() const {return fRunning;}
private:
	//!Anination thread.
	static	int32			AnimationThread(void *data);
	//!Set LED on or off.
	static	void			LED(uint32 mod/*!Modifier key*/,bool on/*!If LED on, value is true*/);
	//!Animation thread ID.
		thread_id			fThread;
	//!Thread running flag.
			volatile bool	fRunning;
			uint32			fOrigModifiers;
};

#endif
