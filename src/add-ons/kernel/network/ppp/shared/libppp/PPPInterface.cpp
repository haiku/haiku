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


/*!	\brief Sets the interface to \a ID.
	
	\param ID The ID of the new interface.
*/
PPPInterface::PPPInterface(ppp_interface_id ID = PPP_UNDEFINED_INTERFACE_ID)
{
	fFD = open(get_stack_driver_path(), O_RDWR);
	
	SetTo(ID);
}


//!	Copy constructor.
PPPInterface::PPPInterface(const PPPInterface& copy)
{
	fFD = open(get_stack_driver_path(), O_RDWR);
	
	SetTo(copy.ID());
}


//!	Destructor.
PPPInterface::~PPPInterface()
{
	if(fFD >= 0)
		close(fFD);
}


/*!	\brief Checks if object was created correctly.
	
	You should always call this method after you constructed a PPPInterface object.
	
	\return
		- \c B_OK: Object could be initialized successfully and the interface exists.
		- \c B_BAD_INDEX: The interface does not exist.
		- \c B_ERROR: The PPP stack could not be loaded.
*/
status_t
PPPInterface::InitCheck() const
{
	if(fFD < 0)
		return B_ERROR;
	if(fID == PPP_UNDEFINED_INTERFACE_ID)
		return B_BAD_INDEX;
	
	return B_OK;
}


/*!	\brief Changes the current interface.
	
	The interface's info structure is cached in this object. If you want to update
	its values you must call \c SetTo() again or use \c Control() with an op of
	\c PPPC_GET_INTERFACE_INFO.\n
	If this fails it will set the interface's \a ID to \c PPP_UNDEFINED_INTERFACE_ID.
	
	\param ID The ID of the new interface.
	
	\return
		- \c B_OK: Object could be initialized successfully and the interface exists.
		- \c B_BAD_INDEX: The interface does not exist.
		- any other value: The PPP stack could not be loaded.
	
	\sa Control()
*/
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


/*!	\brief Use this method for accessing additional PPP features.
	
	\param op Any value of ppp_control_ops.
	\param data Some ops require you to pass a structure or other data using this
		argument.
	\param length Make sure this value is correct (e.g.: size of structure).
	
	\return
		- \c B_OK: Operation was successful.
		- \c B_BAD_INDEX: The interface does not exist.
		- any other value: The PPP stack could not be loaded.
*/
status_t
PPPInterface::Control(uint32 op, void *data, size_t length) const
{
	if(InitCheck() != B_OK)
		return InitCheck();
	
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


/*!	\brief Find BEntry to the interface settings that this object represents.
	
	\param entry The entry gets stored in this argument.
	
	\return
		- \c B_OK: The settings file was saved in \a entry.
		- \c B_BAD_INDEX: The interface does not exist.
		- \c B_BAD_VALUE: Either \a entry was \c NULL or the interface is anonymous.
		- any other value: The interface could not be found.
*/
status_t
PPPInterface::GetSettingsEntry(BEntry *entry) const
{
	if(InitCheck() != B_OK)
		return InitCheck();
	else if(!entry || strlen(Name()) == 0)
		return B_BAD_VALUE;
	
	BDirectory directory(PPP_INTERFACE_SETTINGS_PATH);
	return directory.FindEntry(Name(), entry, true);
}


/*!	\brief Get the cached ppp_interface_info_t structure.
	
	\param info The info structure is copied into this argument.
	
	\return \c true on success, \c false otherwise.
*/
bool
PPPInterface::GetInterfaceInfo(ppp_interface_info_t *info) const
{
	if(InitCheck() != B_OK || !info)
		return false;
	
	memcpy(info, &fInfo, sizeof(fInfo));
	
	return true;
}


//!	Compares interface's settings to given driver_settings structure.
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


/*!	\brief Changes the current interface profile.
	
	You may change the interface's profile at any time. The changes take effect when
	the interface connects.
	
	\param profile The new profile.
*/
void
PPPInterface::SetProfile(const driver_settings *profile) const
{
	if(InitCheck() != B_OK || !profile)
		return;
	
	Control(PPPC_SET_PROFILE, const_cast<driver_settings*>(profile), 0);
}


//!	Brings the interface up.
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


//!	Brings the interface down which causes the deletion of the interface.
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


/*!	\brief Requests report messages from the interface.
	
	\param type The type of report.
	\param thread Receiver thread.
	\param flags Optional flags.
	
	\return \c true on success \c false otherwise.
*/
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


/*!	\brief Removes thread from list of report requestors of this interface.
	
	\param type The type of report.
	\param thread Receiver thread.
	
	\return \c true on success \c false otherwise.
*/
bool
PPPInterface::DisableReports(ppp_report_type type, thread_id thread) const
{
	ppp_report_request request;
	request.type = type;
	request.thread = thread;
	
	return Control(PPPC_DISABLE_REPORTS, &request, sizeof(request)) == B_OK;
}
