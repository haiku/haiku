#ifndef _POWER_BUTTON_MONITOR_H
#define _POWER_BUTTON_MONITOR_H

#include <Handler.h>

#define POLL_POWER_BUTTON_STATUS 'ppbs'

class PowerButtonMonitor : public BHandler {
	public:
		PowerButtonMonitor();
		virtual ~PowerButtonMonitor();
		
		virtual void MessageReceived(BMessage *msg);
		
	private:
		int power_button_fd;
};

#endif // _POWER_BUTTON_MONITOR_H
