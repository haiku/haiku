#include <stdlib.h>

#include "NetworkSetupAddOn.h"

class StatusAddOn : public NetworkSetupAddOn {
	public:
		const char * Name();
};



const char * StatusAddOn::Name()
{
	return "Status";
}

NetworkSetupAddOn * get_addon()
{
	return new StatusAddOn();
}
