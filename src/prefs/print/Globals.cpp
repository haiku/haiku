#include "Globals.h"
#include "pr_server.h"

#include <Roster.h>

status_t GetPrinterServerMessenger(BMessenger& msgr)
{
		// If print server is not yet running, start it
	if (!be_roster->IsRunning(PSRV_SIGNATURE_TYPE))
		be_roster->Launch(PSRV_SIGNATURE_TYPE);
	
	msgr = BMessenger(PSRV_SIGNATURE_TYPE);

	return msgr.IsValid() ? B_OK : B_ERROR;
}
