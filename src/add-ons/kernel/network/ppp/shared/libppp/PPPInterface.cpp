/*
 * Copyright 2003-2007, Waldemar Kornewald <wkornew@gmx.net>
 * Distributed under the terms of the MIT License.
 */

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
PPPInterface::PPPInterface(ppp_interface_id ID)
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
	if (fFD >= 0)
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
	if (fFD < 0)
		return B_ERROR;
	if (fID == PPP_UNDEFINED_INTERFACE_ID)
		return B_BAD_INDEX;
	
	return B_OK;
}


/*!	\brief Changes the current interface.
	
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
	if (fFD < 0)
		return B_ERROR;
	
	ppp_interface_info_t info;
	if (GetInterfaceInfo(&info)) {
		fName = info.info.name;
		fID = ID;
	} else {
		fName = "";
		fID = PPP_UNDEFINED_INTERFACE_ID;
		return B_ERROR;
	}
	
	return B_OK;
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
	if (InitCheck() != B_OK)
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


//!	Sets the username used for authentication.
bool
PPPInterface::SetUsername(const char *username) const
{
	if (InitCheck() != B_OK || !username)
		return false;
	
	return Control(PPPC_SET_USERNAME, const_cast<char*>(username), strlen(username))
		== B_OK;
}


//!	Sets the password used for authentication.
bool
PPPInterface::SetPassword(const char *password) const
{
	if (InitCheck() != B_OK || !password)
		return false;
	
	return Control(PPPC_SET_PASSWORD, const_cast<char*>(password), strlen(password))
		== B_OK;
}


//!	Sets whether a request window should be shown before connecting.
bool
PPPInterface::SetAskBeforeConnecting(bool askBeforeConnecting) const
{
	if (InitCheck() != B_OK)
		return false;
	
	uint32 value = askBeforeConnecting ? 1 : 0;
	return Control(PPPC_SET_ASK_BEFORE_CONNECTING, &value, sizeof(value)) == B_OK;
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
	if (InitCheck() != B_OK)
		return InitCheck();
	else if (!entry || strlen(Name()) == 0)
		return B_BAD_VALUE;
	
	BDirectory directory(PTP_INTERFACE_SETTINGS_PATH);
	return directory.FindEntry(Name(), entry, true);
}


/*!	\brief Get the ppp_interface_info_t structure.
	
	\return \c true on success, \c false otherwise.
*/
bool
PPPInterface::GetInterfaceInfo(ppp_interface_info_t *info) const
{
	if (InitCheck() != B_OK || !info)
		return false;
	
	return Control(PPPC_GET_INTERFACE_INFO, &info, sizeof(ppp_interface_info_t))
		== B_OK;
}


/*!	\brief Get transfer statistics for this interface.
	
	\param statistics The structure is copied into this argument.
	
	\return \c true on success, \c false otherwise.
*/
bool
PPPInterface::GetStatistics(ppp_statistics *statistics) const
{
	if (!statistics)
		return false;
	
	return Control(PPPC_GET_STATISTICS, statistics, sizeof(ppp_statistics)) == B_OK;
}


//!	Compares interface's settings to given driver_settings structure.
bool
PPPInterface::HasSettings(const driver_settings *settings) const
{
	if (InitCheck() != B_OK || !settings)
		return false;
	
	if (Control(PPPC_HAS_INTERFACE_SETTINGS, const_cast<driver_settings*>(settings),
			sizeof(driver_settings)) == B_OK)
		return true;
	
	return false;
}


//!	Brings the interface up.
bool
PPPInterface::Up() const
{
	if (InitCheck() != B_OK)
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
	if (InitCheck() != B_OK)
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
	int32 flags) const
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
