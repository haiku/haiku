#include <Messenger.h>
#include <Roster.h>

#include <RosterPrivate.h>

#include "power_button_monitor.h"


PowerButtonMonitor::PowerButtonMonitor() : BHandler ("power_button_monitor") {
	power_button_fd = open("/dev/power/button/power",O_RDONLY);
}

PowerButtonMonitor::~PowerButtonMonitor() {
	if (power_button_fd > 0)
		close(power_button_fd);
}

void PowerButtonMonitor::MessageReceived(BMessage *msg) {
	if (msg->what != POLL_POWER_BUTTON_STATUS)
		return;
		
	if (power_button_fd <= 0)
		return;
		
	uint8 button_pressed;
	read(power_button_fd,&button_pressed,1);
	
	if (button_pressed) {
		BRoster roster;
		BRoster::Private rosterPrivate(roster);

		rosterPrivate.ShutDown(false, false, false);
	}
}
