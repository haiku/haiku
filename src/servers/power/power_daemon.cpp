#include <Application.h>
#include <MessageRunner.h>

#include "power_button_monitor.h"

class PowerManagementDaemon : public BApplication {
	public:
		PowerManagementDaemon();
		virtual ~PowerManagementDaemon();
};

int main(void) {
	new PowerManagementDaemon();
	be_app->Run();
	delete be_app;
	return 0;
}

PowerManagementDaemon::PowerManagementDaemon() : BApplication("application/x-vnd.Haiku-powermanagement") {
	PowerButtonMonitor *pb_mon = new PowerButtonMonitor();
	AddHandler(pb_mon);
	
	new BMessageRunner(BMessenger(pb_mon,this),new BMessage(POLL_POWER_BUTTON_STATUS),5e5 /* twice a second */);
}

PowerManagementDaemon::~PowerManagementDaemon() {}
