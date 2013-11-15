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
			PowerMonitor*		fPowerMonitors[2];
			uint32				fMonitorCount;

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
	fMonitorCount(0),
	fQuitRequested(false)
{
	PowerMonitor* powerButtonMonitor = new PowerButtonMonitor;
	if (powerButtonMonitor->FD() > 0)
		fPowerMonitors[fMonitorCount++] = powerButtonMonitor;
	else
		delete powerButtonMonitor;

	PowerMonitor* lidMonitor = new LidMonitor;
	if (lidMonitor->FD() > 0)
		fPowerMonitors[fMonitorCount++] = lidMonitor;
	else
		delete lidMonitor;

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
	for (uint32 i = 0; i < fMonitorCount; i++)
		delete fPowerMonitors[i];
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
	if (fMonitorCount == 0)
		return;
	object_wait_info info[fMonitorCount];
	for (uint32 i = 0; i < fMonitorCount; i++) {
		info[i].object = fPowerMonitors[i]->FD();
		info[i].type = B_OBJECT_TYPE_FD;
		info[i].events = B_EVENT_READ;
	}
	while (!fQuitRequested) {
		if (wait_for_objects(info, fMonitorCount) < B_OK)
			continue;
		// handle events and reset events
		for (uint32 i = 0; i < fMonitorCount; i++) {
			if (info[i].events & B_EVENT_READ)
				fPowerMonitors[i]->HandleEvent();
			else
				info[i].events = B_EVENT_READ;
		}
	}
}
