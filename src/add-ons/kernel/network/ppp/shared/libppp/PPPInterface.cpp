//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//
//  Copyright (c) 2003 Waldemar Kornewald, Waldemar.Kornewald@web.de
//---------------------------------------------------------------------

#include "PPPInterface.h"

#include <unistd.h>
#include <net_stack_driver.h>


PPPInterface::PPPInterface(interface_id ID = PPP_UNDEFINED_INTERFACE_ID)
{
	fFD = open("/dev/net/stack", O_RDWR);
	
	SetTo(ID);
}


PPPInterface::PPPInterface(const PPPInterface& copy)
{
	fFD = open("/dev/net/stack", O_RDWR);
	
	SetTo(copy.ID());
}


PPPInterface::~PPPInterface()
{
	if(fFD >= 0)
		close(fFD);
}


status_t
PPPInterface::InitCheck() const
{
	if(fFD < 0 || fID == PPP_UNDEFINED_INTERFACE_ID)
		return B_ERROR;
	
	return B_OK;
}


status_t
PPPInterface::SetTo(interface_id ID)
{
	if(fFD < 0)
		return B_ERROR;
	
	fID = ID;
	
	return Control(PPPC_GET_INTERFACE_INFO, &fInfo, sizeof(fInfo));
}


status_t
PPPInterface::Control(uint32 op, void *data, size_t length) const
{
	if(InitCheck() != B_OK)
		return B_ERROR;
	
	ppp_control_info control;
	control_net_module_args args;
	
	args.name = PPP_INTERFACE_MODULE_NAME;
	args.op = PPPC_CONTROL_INTERFACE;
	args.data = &control;
	args.length = sizeof(control);
	
	control.index = ID();
	control.op = op;
	control.data = data;
	control.length = length;
	
	return ioctl(fFD, NET_STACK_CONTROL_NET_MODULE, &args);
}


bool
PPPInterface::GetInterfaceInfo(ppp_interface_info *info) const
{
	if(InitCheck() != B_OK || !info)
		return false;
	
	memcpy(info, &fInfo, sizeof(fInfo));
	
	return true;
}


bool
PPPInterface::Up() const
{
	if(InitCheck() != B_OK)
		return false;
	
	int32 id = ID();
	control_net_module_args args;
	args.name = PPP_INTERFACE_MODULE_NAME;
	args.op = PPPC_BRING_INTERFACE_UP;
	args.data = &id;
	args.length = sizeof(id);
	
	return ioctl(fFD, NET_STACK_CONTROL_NET_MODULE, &args) == B_OK;
}


bool
PPPInterface::Down() const
{
	if(InitCheck() != B_OK)
		return false;
	
	int32 id = ID();
	control_net_module_args args;
	args.name = PPP_INTERFACE_MODULE_NAME;
	args.op = PPPC_BRING_INTERFACE_DOWN;
	args.data = &id;
	args.length = sizeof(id);
	
	return ioctl(fFD, NET_STACK_CONTROL_NET_MODULE, &args) == B_OK;
}


bool
PPPInterface::EnableReports(ppp_report_type type, thread_id thread,
	int32 flags = PPP_NO_FLAGS) const
{
	ppp_report_request request;
	request.type = type;
	request.thread = thread;
	request.flags = flags;
	
	return Control(PPPC_ENABLE_REPORTS, &request, sizeof(request)) == B_OK;
}


bool
PPPInterface::DisableReports(ppp_report_type type, thread_id thread) const
{
	ppp_report_request request;
	request.type = type;
	request.thread = thread;
	
	return Control(PPPC_DISABLE_REPORTS, &request, sizeof(request)) == B_OK;
}
