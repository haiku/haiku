/*
 * Copyright 2004-2011 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 */


#include "NetworkSetupAddOn.h"

#include <stdlib.h>


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
