//-----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//
//  Copyright (c) 2003-2004 Waldemar Kornewald, Waldemar.Kornewald@web.de
//-----------------------------------------------------------------------

/*!	\class PPPManager
	\brief Allows controlling the PPP stack.
	
	This class can be used for creating and deleting interfaces. It has methods for
	requesting PPP stack report messages (e.g.: about newly created interfaces).
*/

#include "PPPManager.h"
#include "PPPInterface.h"

#include <cstring>
#include <cstdlib>
#include <cctype>
#include <settings_tools.h>
#include <unistd.h>
#include "_libppputils.h"


//!	Constructor. Does nothing special.
PPPManager::PPPManager()
{
	fFD = open(get_stack_driver_path(), O_RDWR);
}


//!	Destructor.
PPPManager::~PPPManager()
{
	if(fFD >= 0)
		close(fFD);
}


//!	Returns \c B_OK if created successfully and \c B_ERROR otherwise.
status_t
PPPManager::InitCheck() const
{
	if(fFD < 0)
		return B_ERROR;
	else
		return B_OK;
}


/*!	\brief Offers an ioctl()-like interface to all functions of the PPP stack.
	
	\param op Any value of ppp_control_ops.
	\param data Some ops require you to pass a structure or other data using this
		argument.
	\param length Make sure this value is correct (e.g.: size of structure).
	
	If you cannot find the method that fits your needs or if you want to have direct
	access to the complete set of  PPP functions you should use this method. All
	other methods call \c Control(), i.e., they are wrappers around this method.
*/
status_t
PPPManager::Control(uint32 op, void *data, size_t length) const
{
	if(InitCheck() != B_OK)
		return B_ERROR;
	
	control_net_module_args args;
	args.name = PPP_INTERFACE_MODULE_NAME;
	args.op = op;
	args.data = data;
	args.length = length;
	
	return ioctl(fFD, NET_STACK_CONTROL_NET_MODULE, &args);
}


/*!	\brief Controls a specific PPP module.
	
	Use this method if you want to access a PPP module. The PPP stack will load it,
	then call its \c control() function (if it is exported), and finally unload
	the module.
	
	\param name The module name.
	\param op The private control op.
	\param data Some ops require you to pass a structure or other data using this
		argument.
	\param length Make sure this value is correct (e.g.: size of structure).
	
	\return
		- \c B_NAME_NOT_FOUND: The module could not be found.
		- \c B_ERROR: Some error occured.
		- The module's return value.
	
	\sa ppp_module_info::control()
*/
status_t
PPPManager::ControlModule(const char *name, uint32 op, void *data,
	size_t length) const
{
	if(!name)
		return B_ERROR;
	
	control_net_module_args args;
	args.name = name;
	args.op = op;
	args.data = data;
	args.length = length;
	return Control(PPPC_CONTROL_MODULE, &args, sizeof(args));
}


/*!	\brief Creates a nameless interface with the given settings and profile.
	
	Please use \c CreateInterfaceWithName() instead of this method.
	
	\return the new interface's ID or \c PPP_UNDEFINED_INTERFACE_ID on failure.
*/
ppp_interface_id
PPPManager::CreateInterface(const driver_settings *settings,
	const driver_settings *profile = NULL) const
{
	ppp_interface_description_info info;
	info.u.settings = settings;
	info.profile = profile;
	
	if(Control(PPPC_CREATE_INTERFACE, &info, sizeof(info)) != B_OK)
		return PPP_UNDEFINED_INTERFACE_ID;
	else
		return info.interface;
}


/*!	\brief Creates an interface with the given name and profile.
	
	Every PPP interface has a profile. By it looks if the pppidf/profile folder
	contains a profile with the interface's name. Otherwise the interface's settings
	become its profile. This has the advantage that you can put the profile and the
	settings into the same file which simplifies your PPP configuration if you edit
	your PPP interface definitions by hand. In this case PAP, for example, would
	still find the username and password although you specify them in the same
	parameter that loads the PAP module.
	
	\param name The PPP interface description file's name.
	\param profile You may override the default profile.
	
	\return the new interface's ID or \c PPP_UNDEFINED_INTERFACE_ID on failure.
*/
ppp_interface_id
PPPManager::CreateInterfaceWithName(const char *name,
	const driver_settings *profile = NULL) const
{
	ppp_interface_description_info info;
	info.u.name = name;
	info.profile = profile;
	
	if(Control(PPPC_CREATE_INTERFACE_WITH_NAME, &info, sizeof(info)) != B_OK)
		return PPP_UNDEFINED_INTERFACE_ID;
	else
		return info.interface;
}


//!	Deletes the interface with the given \a ID.
bool
PPPManager::DeleteInterface(ppp_interface_id ID) const
{
	if(Control(PPPC_DELETE_INTERFACE, &ID, sizeof(ID)) != B_OK)
		return false;
	else
		return true;
}


/*!	\brief Returns all interface IDs matching a certain filter rule.
	
	ATTENTION: You are responsible for deleting (via \c delete) the returned data!\n
	Use this if you want to iterate over all interfaces. It returns an array of all
	interface IDs.\c
	You can specify a filter rule that can be either of:
		- \c PPP_REGISTERED_INTERFACES (default): Only visible interfaces.
		- \c PPP_UNREGISTERED_INTERFACES: Only invisible interfaces.
		- \c PPP_ALL_INTERFACES: All (visible and invisible) interfaces.
	
	\param count The number of IDs in the returned array is stored here.
	\param filter The filter rule.
	
	\return an array of interface IDs or \c NULL on failure.
*/
ppp_interface_id*
PPPManager::Interfaces(int32 *count,
	ppp_interface_filter filter = PPP_REGISTERED_INTERFACES) const
{
	int32 requestCount;
	ppp_interface_id *interfaces;
	
	// loop until we get all interfaces
	while(true) {
		requestCount = *count = CountInterfaces(filter);
		if(*count == -1)
			return NULL;
		
		requestCount += 10;
			// request some more interfaces in case some are added in the mean time
		interfaces = new ppp_interface_id[requestCount];
		*count = GetInterfaces(interfaces, requestCount, filter);
		if(*count == -1) {
			delete interfaces;
			return NULL;
		}
		
		if(*count < requestCount)
			break;
		
		delete interfaces;
	}
	
	return interfaces;
}


//!	Use \c Interfaces() instead of this method.
int32
PPPManager::GetInterfaces(ppp_interface_id *interfaces, int32 count,
	ppp_interface_filter filter = PPP_REGISTERED_INTERFACES) const
{
	ppp_get_interfaces_info info;
	info.interfaces = interfaces;
	info.count = count;
	info.filter = filter;
	
	if(Control(PPPC_GET_INTERFACES, &info, sizeof(info)) != B_OK)
		return -1;
	else
		return info.resultCount;
}


//!	Returns the ID of the interface with the given settings on success.
ppp_interface_id
PPPManager::InterfaceWithSettings(const driver_settings *settings) const
{
	ppp_interface_description_info info;
	info.u.settings = settings;
	info.interface = PPP_UNDEFINED_INTERFACE_ID;
	
	Control(PPPC_FIND_INTERFACE_WITH_SETTINGS, &info, sizeof(info));
	
	return info.interface;
}


//!	Returns the ID of the interface with the given if_unit (interface unit).
ppp_interface_id
PPPManager::InterfaceWithUnit(int32 if_unit) const
{
	int32 count;
	ppp_interface_id *interfaces = Interfaces(&count, PPP_REGISTERED_INTERFACES);
	
	if(!interfaces)
		return PPP_UNDEFINED_INTERFACE_ID;
	
	ppp_interface_id id = PPP_UNDEFINED_INTERFACE_ID;
	PPPInterface interface;
	ppp_interface_info_t info;
	
	for(int32 index = 0; index < count; index++) {
		interface.SetTo(interfaces[index]);
		if(interface.InitCheck() == B_OK && interface.GetInterfaceInfo(&info)
				&& info.info.if_unit == if_unit) {
			id = interface.ID();
			break;
		}
	}
	
	delete interfaces;
	
	return id;
}


//!	Returns the ID of the interface with the given name.
ppp_interface_id
PPPManager::InterfaceWithName(const char *name) const
{
	if(!name)
		return PPP_UNDEFINED_INTERFACE_ID;
	
	int32 count;
	ppp_interface_id *interfaces = Interfaces(&count, PPP_REGISTERED_INTERFACES);
	
	if(!interfaces)
		return PPP_UNDEFINED_INTERFACE_ID;
	
	ppp_interface_id id = PPP_UNDEFINED_INTERFACE_ID;
	PPPInterface interface;
	ppp_interface_info_t info;
	
	for(int32 index = 0; index < count; index++) {
		interface.SetTo(interfaces[index]);
		if(interface.InitCheck() == B_OK && interface.GetInterfaceInfo(&info)
				&& strlen(info.info.name) > 0 && !strcasecmp(info.info.name, name)) {
			id = interface.ID();
			break;
		}
	}
	
	delete interfaces;
	
	if(id != PPP_UNDEFINED_INTERFACE_ID)
		return id;
	else if(!strncmp(name, "ppp", 3) && strlen(name) > 3 && isdigit(name[3]))
		return InterfaceWithUnit(atoi(name + 3));
	else if(isdigit(name[0]))
		return atoi(name);
	else
		return PPP_UNDEFINED_INTERFACE_ID;
}


//!	Returns the number of existing interfaces or a negative value on error.
int32
PPPManager::CountInterfaces(ppp_interface_filter filter =
	PPP_REGISTERED_INTERFACES) const
{
	return Control(PPPC_COUNT_INTERFACES, &filter, sizeof(filter));
}


/*!	\brief Requests report messages from the PPP stack.
	
	\param type The type of report.
	\param thread Receiver thread.
	\param flags Optional flags.
	
	\return \c true on success \c false otherwise.
*/
bool
PPPManager::EnableReports(ppp_report_type type, thread_id thread,
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
PPPManager::DisableReports(ppp_report_type type, thread_id thread) const
{
	ppp_report_request request;
	request.type = type;
	request.thread = thread;
	
	return Control(PPPC_DISABLE_REPORTS, &request, sizeof(request)) == B_OK;
}
