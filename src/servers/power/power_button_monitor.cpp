#include <Messenger.h>
#include <Roster.h>

#include "power_button_monitor.h"

#define B_SYSTEM_SHUTDOWN 0x12d
static const char *kRosterSignature = "application/x-vnd.Be-ROST";

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
		
	bool button_pressed;
	read(power_button_fd,&button_pressed,1);
	
	if (button_pressed)
		BMessenger(kRosterSignature).SendMessage(B_SYSTEM_SHUTDOWN);
}
