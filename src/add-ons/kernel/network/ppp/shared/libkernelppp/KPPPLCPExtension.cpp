/*
 * Copyright 2003-2007, Waldemar Kornewald <wkornew@gmx.net>
 * Distributed under the terms of the MIT License.
 */

/*!	\class KPPPLCPExtension
	\brief LCP protocol extension
	
	This class can be used to extend the supported LCP protocol codes.
*/

#include <KPPPLCPExtension.h>
#include <PPPControl.h>


/*!	\brief Constructor.
	
	If an error occurs in the constructor you should set \c fInitStatus.
	
	\param name Name of the extension.
	\param code LCP code that this extension can handle.
	\param interface Owning interface.
	\param settings Settings for this extension.
*/
KPPPLCPExtension::KPPPLCPExtension(const char *name, uint8 code,
		KPPPInterface& interface, driver_parameter *settings)
	: fInitStatus(B_OK),
	fInterface(interface),
	fSettings(settings),
	fCode(code),
	fEnabled(true)
{
	if (name)
		fName = strdup(name);
	else
		fName = NULL;
}


//!	Destructor. Frees the name and unregisters extension from LCP protocol.
KPPPLCPExtension::~KPPPLCPExtension()
{
	Interface().LCP().RemoveLCPExtension(this);
	
	free(fName);
}


//!	Returns \c fInitStatus. May be overridden to return status-dependend errors.
status_t
KPPPLCPExtension::InitCheck() const
{
	return fInitStatus;
}


//!	Allows controlling this extension from userlevel.
status_t
KPPPLCPExtension::Control(uint32 op, void *data, size_t length)
{
	switch (op) {
		case PPPC_GET_SIMPLE_HANDLER_INFO:
		{
			if (length < sizeof(ppp_simple_handler_info_t) || !data)
				return B_ERROR;
			
			ppp_simple_handler_info *info = (ppp_simple_handler_info*) data;
			memset(info, 0, sizeof(ppp_simple_handler_info_t));
			if (Name())
				strncpy(info->name, Name(), PPP_HANDLER_NAME_LENGTH_LIMIT);
			info->isEnabled = IsEnabled();
			break;
		}

		case PPPC_ENABLE:
			if (length < sizeof(uint32) || !data)
				return B_ERROR;

			SetEnabled(*((uint32*)data));
			break;

		default:
			return B_BAD_VALUE;
	}

	return B_OK;
}


//!	Stack ioctl handler.
status_t
KPPPLCPExtension::StackControl(uint32 op, void *data)
{
	switch (op) {
		default:
			return B_BAD_VALUE;
	}

	return B_OK;
}


/*!	\brief Reset internal connection state.
	
	This method is called when: connecting, reconfiguring, or disconnecting.
*/
void
KPPPLCPExtension::Reset()
{
	// do nothing by default
}


/*!	\brief You may override this for periodic tasks.
	
	This method gets called every \c PPP_PULSE_RATE microseconds.
*/
void
KPPPLCPExtension::Pulse()
{
	// do nothing by default
}
