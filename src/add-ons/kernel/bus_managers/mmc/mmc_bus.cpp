/*
 * Copyright 2018 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		B Krishnan Iyer, krishnaniyer97@gmail.com
 */
#include "mmc_bus.h"


MMCBus::MMCBus(device_node* node)
	:
	fNode(node),
	fController(NULL),
	fCookie(NULL),
	fStatus(B_OK),
	fDriverCookie(NULL)
{
	CALLED();
	device_node* parent = gDeviceManager->get_parent_node(node);
	fStatus = gDeviceManager->get_driver(parent,
		(driver_module_info**)&fController, &fCookie);
	gDeviceManager->put_node(parent);

	if (fStatus != B_OK) {
		ERROR("Not able to establish the bus %s\n",
			strerror(fStatus));
		return;
	}
}


MMCBus::~MMCBus()
{
	CALLED();
}


status_t
MMCBus::InitCheck()
{
	return fStatus;
}
