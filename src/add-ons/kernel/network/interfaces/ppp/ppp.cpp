//-----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//
//  Copyright (c) 2003-2004 Waldemar Kornewald, Waldemar.Kornewald@web.de
//-----------------------------------------------------------------------

#include <core_funcs.h>
#include <KernelExport.h>
#include <driver_settings.h>

#include <PPPDefs.h>
#include "PPPManager.h"


status_t std_ops(int32 op, ...);

struct core_module_info *core = NULL;
static PPPManager *sManager = NULL;


int
ppp_ifnet_stop(ifnet *ifp)
{
	if(sManager)
		return sManager->Stop(ifp);
	else
		return B_ERROR;
}


int
ppp_ifnet_output(ifnet *ifp, struct mbuf *buf, struct sockaddr *dst,
	struct rtentry *rt0)
{
	if(sManager)
		return sManager->Output(ifp, buf, dst, rt0);
	else
		return B_ERROR;
}


int
ppp_ifnet_ioctl(ifnet *ifp, ulong cmd, caddr_t data)
{
	if(sManager)
		return sManager->Control(ifp, cmd, data);
	else
		return B_ERROR;
}


static
int
ppp_start(void *cpp)
{
	if(cpp)
		core = (core_module_info*) cpp;
	
	sManager = new PPPManager;
	
	return B_OK;
}


static
int
ppp_stop()
{
	delete sManager;
	sManager = NULL;
	
	return B_OK;
}


// PPPManager wrappers
static
status_t
ppp_control(uint32 op, void *data, size_t length)
{
	if(sManager)
		return sManager->Control(op, data, length);
	else
		return B_ERROR;
}


static
ppp_interface_id
CreateInterface(const driver_settings *settings, ppp_interface_id parent)
{
	if(sManager)
		return sManager->CreateInterface(settings, parent);
	else
		return PPP_UNDEFINED_INTERFACE_ID;
}


static
bool
DeleteInterface(ppp_interface_id ID)
{
	if(sManager)
		return sManager->DeleteInterface(ID);
	else
		return false;
}


static
bool
RemoveInterface(ppp_interface_id ID)
{
	if(sManager)
		return sManager->RemoveInterface(ID);
	else
		return false;
}


static
ifnet*
RegisterInterface(ppp_interface_id ID)
{
	if(sManager)
		return sManager->RegisterInterface(ID);
	else
		return NULL;
}


static
bool
UnregisterInterface(ppp_interface_id ID)
{
	if(sManager)
		return sManager->UnregisterInterface(ID);
	else
		return false;
}


static
status_t
ControlInterface(ppp_interface_id ID, uint32 op, void *data, size_t length)
{
	if(sManager)
		return sManager->ControlInterface(ID, op, data, length);
	else
		return B_ERROR;
}


static
int32
GetInterfaces(ppp_interface_id *interfaces, int32 count,
	ppp_interface_filter filter = PPP_REGISTERED_INTERFACES)
{
	if(sManager)
		return sManager->GetInterfaces(interfaces, count, filter);
	else
		return 0;
}


static
int32
CountInterfaces(ppp_interface_filter filter = PPP_REGISTERED_INTERFACES)
{
	if(sManager)
		return sManager->CountInterfaces(filter);
	else
		return 0;
}


static
void
EnableReports(ppp_report_type type, thread_id thread, int32 flags = PPP_NO_FLAGS)
{
	if(sManager)
		sManager->ReportManager().EnableReports(type, thread, flags);
}


static
void
DisableReports(ppp_report_type type, thread_id thread)
{
	if(sManager)
		sManager->ReportManager().DisableReports(type, thread);
}


static
bool
DoesReport(ppp_report_type type, thread_id thread)
{
	if(sManager)
		return sManager->ReportManager().DoesReport(type, thread);
	else
		return false;
}


ppp_interface_module_info ppp_interface_module = {
	{
		{
			PPP_INTERFACE_MODULE_NAME,
			0,
			std_ops
		},
		ppp_start,
		ppp_stop,
		ppp_control
	},
	CreateInterface,
	DeleteInterface,
	RemoveInterface,
	RegisterInterface,
	UnregisterInterface,
	ControlInterface,
	GetInterfaces,
	CountInterfaces,
	EnableReports,
	DisableReports,
	DoesReport
};


_EXPORT
status_t
std_ops(int32 op, ...) 
{
	switch(op) {
		case B_MODULE_INIT:
			if(get_module(NET_CORE_MODULE_NAME, (module_info**)&core) != B_OK)
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
	(module_info*) &ppp_interface_module,
	NULL
};
