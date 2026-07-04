/*
 * Copyright 2026, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */

#include <net_protocol.h>
#include <net_stack.h>

extern net_address_module_info gBluetoothAddressModule;
net_stack_module_info* gStackModule;
static net_domain* sDomain;


status_t
bluetooth_std_ops(int32 op, ...)
{
	switch (op) {
		case B_MODULE_INIT:
		{
			status_t status = gStackModule->register_domain(AF_BLUETOOTH, "bluetooth", NULL,
				&gBluetoothAddressModule, &sDomain);
			if (status != B_OK)
				return status;

			return B_OK;
		}

		case B_MODULE_UNINIT:
			gStackModule->unregister_domain(sDomain);
			return B_OK;

		default:
			return B_ERROR;
	}
}


module_dependency module_dependencies[] = {
	{NET_STACK_MODULE_NAME, (module_info**)&gStackModule},
	{}
};

module_info* modules[] = {
	(module_info*)&gBluetoothAddressModule,
	NULL
};
