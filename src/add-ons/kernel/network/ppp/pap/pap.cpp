//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//
//  Copyright (c) 2003 Waldemar Kornewald, Waldemar.Kornewald@web.de
//---------------------------------------------------------------------

#include <KernelExport.h>
#include <driver_settings.h>
#include <core_funcs.h>
#include <net_module.h>

#include <KPPPInterface.h>
#include <KPPPModule.h>
#include <LockerHelper.h>

#include "Protocol.h"


#ifdef _KERNEL_MODE
	#define spawn_thread spawn_kernel_thread
	#define printf dprintf
#else
	#include <cstdio>
#endif


#define PAP_MODULE_NAME		NETWORK_MODULES_ROOT "ppp/pap"

struct core_module_info *core = NULL;
status_t std_ops(int32 op, ...);


static
bool
add_to(PPPInterface& mainInterface, PPPInterface *subInterface,
	driver_parameter *settings, ppp_module_key_type type)
{
	if(type != PPP_AUTHENTICATOR_KEY_TYPE)
		return B_ERROR;
	
	PAP *pap;
	bool success;
	if(subInterface) {
		pap = new PAP(*subInterface, settings);
		success = subInterface->AddProtocol(pap);
	} else {
		pap = new PAP(mainInterface, settings);
		success = mainInterface.AddProtocol(pap);
	}
	
#if DEBUG
	printf("PAP: add_to(): %s\n",
		success && pap && pap->InitCheck() == B_OK ? "OK" : "ERROR");
#endif
	
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
	switch(op) {
		case B_MODULE_INIT:
			if(get_module(NET_CORE_MODULE_NAME, (module_info**) &core) != B_OK)
				return B_ERROR;
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
	(module_info*) &pap_module,
	NULL
};
