/*
 * Copyright 2013, Jérôme Duval, korli@users.berlios.de.
 * Copyright 2005, Nathan Whitehorn.
 *
 * Distributed under the terms of the MIT License.
 */


#include "lid_monitor.h"
#include "power_button_monitor.h"

#include <Application.h>


class PowerManagementDaemon : public BApplication {
public:
								PowerManagementDaemon();
	virtual 					~PowerManagementDaemon();
private:
			void				_EventLoop();
	static	status_t			_EventLooper(void *arg);

			thread_id			fEventThread;
			PowerButtonMonitor*	fPowerButtonMonitor;
			LidMonitor*			fLidMonitor;

			bool				fQuitRequested;
};


int
main(void)
{
	new PowerManagementDaemon();
	be_app->Run();
	delete be_app;
	return 0;
}


PowerManagementDaemon::PowerManagementDaemon()
	:
	BApplication("application/x-vnd.Haiku-powermanagement"),
	fPowerButtonMonitor(NULL),
	fQuitRequested(false)
{
	fPowerButtonMonitor = new PowerButtonMonitor;
	fLidMonitor = new LidMonitor;
	fEventThread = spawn_thread(_EventLooper, "_power_daemon_event_loop_",
		B_NORMAL_PRIORITY, this);
	if (fEventThread < B_OK)
		return;
	if (resume_thread(fEventThread) < B_OK) {
		kill_thread(fEventThread);
		fEventThread = -1;
		return;
	}
}


PowerManagementDaemon::~PowerManagementDaemon()
{
	fQuitRequested = true;
	delete fPowerButtonMonitor;
	delete fLidMonitor;
	status_t status;
	wait_for_thread(fEventThread, &status);
}


status_t
PowerManagementDaemon::_EventLooper(void* arg)
{
	PowerManagementDaemon* self = (PowerManagementDaemon*)arg;
	self->_EventLoop();
	return B_OK;
}


void
PowerManagementDaemon::_EventLoop()
{
	while (!fQuitRequested) {
		object_wait_info info[] = {
			{ fPowerButtonMonitor->FD(), B_OBJECT_TYPE_FD, B_EVENT_READ },
			{ fLidMonitor->FD(), B_OBJECT_TYPE_FD, B_EVENT_READ }
		};

		if (wait_for_objects_etc(info, 2, 0, 1000000LL) < B_OK)
			continue;
		if (info[0].events & B_EVENT_READ)
			fPowerButtonMonitor->HandleEvent();
		if (info[1].events & B_EVENT_READ)
			fLidMonitor->HandleEvent();
	}
}
