//-----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//
//  Copyright (c) 2003-2004 Waldemar Kornewald, Waldemar.Kornewald@web.de
//-----------------------------------------------------------------------

/*!	\class PPPInterface
	\brief This class represents a PPP interface living in kernel space.
	
	Use this class to control PPP interfaces and get the current interface settings.
	You can also use it to enable/disable report messages.
*/

#include "PPPInterface.h"

#include <cstring>
#include <unistd.h>
#include "_libppputils.h"

#include <Path.h>
#include <Directory.h>
#include <Entry.h>


PPPInterface::PPPInterface(ppp_interface_id ID = PPP_UNDEFINED_INTERFACE_ID)
{
	fFD = open(get_stack_driver_path(), O_RDWR);
	
	SetTo(ID);
}


PPPInterface::PPPInterface(const PPPInterface& copy)
{
	fFD = open(get_stack_driver_path(), O_RDWR);
	
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
PPPInterface::SetTo(ppp_interface_id ID)
{
	if(fFD < 0)
		return B_ERROR;
	
	fID = ID;
	
	status_t error = Control(PPPC_GET_INTERFACE_INFO, &fInfo, sizeof(fInfo));
	if(error != B_OK) {
		memset(&fInfo, 0, sizeof(fInfo));
		fID = PPP_UNDEFINED_INTERFACE_ID;
	}
	
	return error;
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


status_t
PPPInterface::GetSettingsEntry(BEntry *entry) const
{
	if(InitCheck() != B_OK || !entry || strlen(Name()) == 0)
		return B_ERROR;
	
	BDirectory directory(PPP_INTERFACE_SETTINGS_PATH);
	return directory.FindEntry(Name(), entry, true);
}


bool
PPPInterface::GetInterfaceInfo(ppp_interface_info_t *info) const
{
	if(InitCheck() != B_OK || !info)
		return false;
	
	memcpy(info, &fInfo, sizeof(fInfo));
	
	return true;
}


bool
PPPInterface::HasSettings(const driver_settings *settings) const
{
	if(InitCheck() != B_OK || !settings)
		return false;
	
	if(Control(PPPC_HAS_INTERFACE_SETTINGS, const_cast<driver_settings*>(settings),
			sizeof(driver_settings)) == B_OK)
		return true;
	
	return false;
}


void
PPPInterface::SetProfile(const driver_settings *profile) const
{
	if(InitCheck() != B_OK || !profile)
		return;
	
	Control(PPPC_SET_PROFILE, const_cast<driver_settings*>(profile), 0);
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
