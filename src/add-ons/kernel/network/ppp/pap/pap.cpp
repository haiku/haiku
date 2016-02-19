/*
 * Copyright 2003-2004, Waldemar Kornewald <wkornew@gmx.net>
 * Distributed under the terms of the MIT License.
 */

#include <KernelExport.h>
#include <driver_settings.h>

#include <net_buffer.h>
#include <net_stack.h>

#include <KPPPInterface.h>
#include <KPPPModule.h>

#include "Protocol.h"

#define PAP_MODULE_NAME		NETWORK_MODULES_ROOT "/ppp/pap"

net_stack_module_info *gStackModule = NULL;
net_buffer_module_info *gBufferModule = NULL;
status_t std_ops(int32 op, ...);

static bool
add_to(KPPPInterface& mainInterface, KPPPInterface *subInterface,
	driver_parameter *settings, ppp_module_key_type type)
{
	if (type != PPP_AUTHENTICATOR_KEY_TYPE)
		return B_ERROR;

	PAP *pap;
	bool success;
	if (subInterface) {
		pap = new PAP(*subInterface, settings);
		success = subInterface->AddProtocol(pap);
	} else {
		pap = new PAP(mainInterface, settings);
		success = mainInterface.AddProtocol(pap);
	}

	TRACE("PAP: add_to(): %s\n",
		success && pap && pap->InitCheck() == B_OK ? "OK" : "ERROR");

	return success && pap && pap->InitCheck() == B_OK;
}


static ppp_module_info pap_module = {
	{
		PAP_MODULE_NAME,
		0,
		std_ops
	},
	NULL,
	add_to
};


_EXPORT
status_t
std_ops(int32 op, ...)
{
	switch (op) {
		case B_MODULE_INIT:
			if (get_module(NET_STACK_MODULE_NAME, (module_info**) &gStackModule) != B_OK)
				return B_ERROR;
			if (get_module(NET_BUFFER_MODULE_NAME,
				(module_info **)&gBufferModule) != B_OK) {
				put_module(NET_STACK_MODULE_NAME);
				return B_ERROR;
			}
		return B_OK;

		case B_MODULE_UNINIT:
			put_module(NET_BUFFER_MODULE_NAME);
			put_module(NET_STACK_MODULE_NAME);
		break;

		default:
			return B_ERROR;
	}

	return B_OK;
}


_EXPORT
module_info *modules[] = {
	(module_info*) &pap_module,
	NULL
};
