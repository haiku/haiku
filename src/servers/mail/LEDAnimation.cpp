

#include "LEDAnimation.h"

#include <InterfaceDefs.h>

#include <stdio.h>

#define SNOOZE_TIME 150000


LEDAnimation::LEDAnimation()
	:
	fThread(-1),
	fRunning(false),
	fOrigModifiers(::modifiers())
{
}


LEDAnimation::~LEDAnimation()
{
	Stop();
}


void
LEDAnimation::Start()
{
	// don't do anything if the thread is already running
	if (fThread >= 0)
		return;

	fOrigModifiers = ::modifiers();
	::set_keyboard_locks(0);
	fRunning = true;
	fThread = ::spawn_thread(AnimationThread,"LED thread",B_NORMAL_PRIORITY,this);
	::resume_thread(fThread);
}


void
LEDAnimation::Stop()
{
	// don't do anything if the thread doesn't run
	if (fThread < 0)
		return;

	fRunning = false;
	status_t result;
	::wait_for_thread(fThread,&result);

	::set_keyboard_locks(fOrigModifiers);
}


int32
LEDAnimation::AnimationThread(void* data)
{
	LEDAnimation *anim = (LEDAnimation*)data;

	while (anim->fRunning) {
		LED(B_NUM_LOCK,true);
		LED(B_NUM_LOCK,false);

		LED(B_CAPS_LOCK,true);
		LED(B_CAPS_LOCK,false);

		LED(B_SCROLL_LOCK,true);
		LED(B_SCROLL_LOCK,false);

		LED(B_CAPS_LOCK,true);
		LED(B_CAPS_LOCK,false);
	}
	anim->fThread = -1;
	return 0;
}


void
LEDAnimation::LED(uint32 mod,bool on)
{
	uint32 current_modifiers = ::modifiers();
	if (on)
		current_modifiers |= mod;
	else
		current_modifiers &= ~mod;
	::set_keyboard_locks(current_modifiers);
	if (on)
		::snooze(SNOOZE_TIME);
}
