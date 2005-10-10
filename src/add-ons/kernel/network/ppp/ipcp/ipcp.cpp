/*
 * Copyright 2003-2004, Waldemar Kornewald <wkornew@gmx.net>
 * Distributed under the terms of the MIT License.
 */

#include <KernelExport.h>
#include <driver_settings.h>
#include <core_funcs.h>
#include <net_module.h>

#include <KPPPInterface.h>
#include <KPPPModule.h>

#include "Protocol.h"


#define IPCP_MODULE_NAME		NETWORK_MODULES_ROOT "ppp/ipcp"

struct protosw *gProto[IPPROTO_MAX];
struct core_module_info *core = NULL;
status_t std_ops(int32 op, ...);


// XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
// TODO: Remove isascii() (needed for inet_aton()) when our kernel is finished!
// isascii() is not defined in the R5 kernel, thus we must define it here:
extern "C"
int
isascii(char c)
{
	return c & ~0x7f == 0; // If c is a 7 bit value.
}
// XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX


static
bool
add_to(KPPPInterface& mainInterface, KPPPInterface *subInterface,
	driver_parameter *settings, ppp_module_key_type type)
{
	if(type != PPP_PROTOCOL_KEY_TYPE)
		return B_ERROR;
	
	IPCP *ipcp;
	bool success;
	if(subInterface) {
		ipcp = new IPCP(*subInterface, settings);
		success = subInterface->AddProtocol(ipcp);
	} else {
		ipcp = new IPCP(mainInterface, settings);
		success = mainInterface.AddProtocol(ipcp);
	}
	
	TRACE("IPCP: add_to(): %s\n",
		success && ipcp && ipcp->InitCheck() == B_OK ? "OK" : "ERROR");
	
	return success && ipcp && ipcp->InitCheck() == B_OK;
}


static ppp_module_info ipcp_module = {
	{
		IPCP_MODULE_NAME,
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
	switch(op) {
		case B_MODULE_INIT:
			if(get_module(NET_CORE_MODULE_NAME, (module_info**) &core) != B_OK)
				return B_ERROR;
			memset(gProto, 0, sizeof(struct protosw*) * IPPROTO_MAX);
			add_protosw(gProto, NET_LAYER1);
		return B_OK;
		
		case B_MODULE_UNINIT:
			put_module(NET_CORE_MODULE_NAME);
		break;
		
		default:
			return B_ERROR;
	}
	
	return B_OK;
}


_EXPORT
module_info *modules[] = {
	(module_info*) &ipcp_module,
	NULL
};
