//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//
//  Copyright (c) 2003 Waldemar Kornewald, Waldemar.Kornewald@web.de
//---------------------------------------------------------------------

#include <core_funcs.h>
#include <KernelExport.h>
#include <driver_settings.h>

#include <kernel_cpp.h>

#include <PPPDefs.h>
#include "PPPManager.h"


status_t std_ops(int32 op, ...);

struct core_module_info *core = NULL;
static PPPManager *manager = NULL;


int
ppp_ifnet_stop(ifnet *ifp)
{
	if(manager)
		return manager->Stop(ifp);
	else
		return B_ERROR;
}


int
ppp_ifnet_output(ifnet *ifp, struct mbuf *buf, struct sockaddr *dst,
	struct rtentry *rt0)
{
	if(manager)
		return manager->Output(ifp, buf, dst, rt0);
	else
		return B_ERROR;
}


int
ppp_ifnet_ioctl(ifnet *ifp, ulong cmd, caddr_t data)
{
	if(manager)
		return manager->Control(ifp, cmd, data);
	else
		return B_ERROR;
}


static
int
ppp_start(void *cpp)
{
	if(cpp)
		core = (core_module_info*) cpp;
	
	manager = new PPPManager;
	
	return B_OK;
}


static
int
ppp_stop()
{
	delete manager;
	manager = NULL;
	
	return B_OK;
}


// PPPManager wrappers
static
status_t
ppp_control(uint32 op, void *data, size_t length)
{
	if(manager)
		return manager->Control(op, data, length);
	else
		return B_ERROR;
}


static
interface_id
CreateInterface(const driver_settings *settings, interface_id parent)
{
	if(manager)
		return manager->CreateInterface(settings, parent);
	else
		return PPP_UNDEFINED_INTERFACE_ID;
}


static
void
DeleteInterface(interface_id ID)
{
	if(manager)
		manager->DeleteInterface(ID);
}


static
void
RemoveInterface(interface_id ID)
{
	if(manager)
		manager->RemoveInterface(ID);
}


static
ifnet*
RegisterInterface(interface_id ID)
{
	if(manager)
		return manager->RegisterInterface(ID);
	else
		return NULL;
}


static
bool
UnregisterInterface(interface_id ID)
{
	if(manager)
		return manager->UnregisterInterface(ID);
	else
		return false;
}


static
status_t
ControlInterface(interface_id ID, uint32 op, void *data, size_t length)
{
	if(manager)
		return manager->ControlInterface(ID, op, data, length);
	else
		return B_ERROR;
}


static
int32
GetInterfaces(interface_id *interfaces, int32 count,
	ppp_interface_filter filter = PPP_REGISTERED_INTERFACES)
{
	if(manager)
		return manager->GetInterfaces(interfaces, count, filter);
	else
		return 0;
}


static
int32
CountInterfaces(ppp_interface_filter filter = PPP_REGISTERED_INTERFACES)
{
	if(manager)
		return manager->CountInterfaces(filter);
	else
		return 0;
}


static
void
EnableReports(ppp_report_type type, thread_id thread, int32 flags = PPP_NO_FLAGS)
{
	if(manager)
		manager->ReportManager().EnableReports(type, thread, flags);
}


static
void
DisableReports(ppp_report_type type, thread_id thread)
{
	if(manager)
		manager->ReportManager().DisableReports(type, thread);
}


static
bool
DoesReport(ppp_report_type type, thread_id thread)
{
	if(manager)
		return manager->ReportManager().DoesReport(type, thread);
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
