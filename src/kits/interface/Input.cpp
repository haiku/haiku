//------------------------------------------------------------------------------
//	Copyright (c) 2001-2002, OpenBeOS
//
//	Permission is hereby granted, free of charge, to any person obtaining a
//	copy of this software and associated documentation files (the "Software"),
//	to deal in the Software without restriction, including without limitation
//	the rights to use, copy, modify, merge, publish, distribute, sublicense,
//	and/or sell copies of the Software, and to permit persons to whom the
//	Software is furnished to do so, subject to the following conditions:
//
//	The above copyright notice and this permission notice shall be included in
//	all copies or substantial portions of the Software.
//
//	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
//	FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
//	DEALINGS IN THE SOFTWARE.
//
//	File Name:		Input.cpp
//	Author:			Marc Flerackers (mflerackers@androme.be)
//	Description:	Functions and class to manage input devices.
//------------------------------------------------------------------------------

// Standard Includes -----------------------------------------------------------
#include <stdlib.h>
#include <string.h>

// System Includes -------------------------------------------------------------
#include <Input.h>
#include <Message.h>
#include <Errors.h>
#include <List.h>

// Project Includes ------------------------------------------------------------
#include "InputServerTypes.h"

// Local Includes --------------------------------------------------------------

// Local Defines ---------------------------------------------------------------

// Globals ---------------------------------------------------------------------

BMessenger *input_server=NULL;

_IMPEXP_BE status_t _control_input_server_(BMessage *command, BMessage *reply);

/*static bool PrintNameAndDelete(void *ptr)
{
   if(ptr)
   {
      BInputDevice *dev = (BInputDevice *)ptr;
	  printf("* %s : %s\n", dev->Name(), dev->IsRunning() ? "running" : "not running");
      delete dev;
      return false;
   }

   return true;
}

void PrintDevices()
{
	BList devices;

	status_t err = get_input_devices(&devices);

	if(err != B_OK)
		printf("Error while getting input devices\n");
	else
	{
	   devices.DoForEach(PrintNameAndDelete);
	   devices.MakeEmpty();
	}
}*/

//------------------------------------------------------------------------------
BInputDevice *find_input_device(const char *name)
{
	BMessage command(IS_FIND_DEVICES);
	BMessage reply;

	command.AddString("device", name);

	status_t err = _control_input_server_(&command, &reply);

	if (err != B_OK)
		return NULL;

	BInputDevice *dev = new BInputDevice;

	const char *device;
	int32 type;

	reply.FindString("device", &device);
	reply.FindInt32("type", &type);
	
	dev->set_name_and_type(device, (input_device_type)type);

	return dev;
}
//------------------------------------------------------------------------------
status_t get_input_devices(BList *list)
{
	list->MakeEmpty();

	BMessage command(IS_FIND_DEVICES);
	BMessage reply;

	status_t err = _control_input_server_(&command, &reply);

	if (err != B_OK)
		return err;

	const char *name;
	int32 type;
	int32 i = 0;
	
	while (reply.FindString("device", i, &name) == B_OK)
	{
		reply.FindInt32("type", i++, &type);

		BInputDevice *dev = new BInputDevice;

		dev->set_name_and_type(name, (input_device_type)type);

		list->AddItem(dev);
	}

	return err;
}
//------------------------------------------------------------------------------
status_t watch_input_devices(BMessenger target, bool start)
{
	BMessage command(IS_WATCH_DEVICES);
	BMessage reply;

	command.AddMessenger("target", target);
	command.AddBool("start", start);

	return _control_input_server_(&command, &reply);
}
//------------------------------------------------------------------------------
BInputDevice::~BInputDevice()
{
	if (fName)
		free (fName);
}
//------------------------------------------------------------------------------
const char *BInputDevice::Name() const
{
	return fName;
}
//------------------------------------------------------------------------------
input_device_type BInputDevice::Type() const
{
	return fType;
}
//------------------------------------------------------------------------------
bool BInputDevice::IsRunning() const
{
	if (!fName)
		return false;

	BMessage command(IS_IS_DEVICE_RUNNING);
	BMessage reply;

	command.AddString("device", fName);

	return _control_input_server_(&command, &reply) == B_OK;
}
//------------------------------------------------------------------------------
status_t BInputDevice::Start()
{
	if (!fName)
		return B_ERROR;

	BMessage command(IS_START_DEVICE);
	BMessage reply;

	command.AddString("device", fName);

	return _control_input_server_(&command, &reply);
}
//------------------------------------------------------------------------------
status_t BInputDevice::Stop()
{
	if (!fName)
		return B_ERROR;

	BMessage command(IS_STOP_DEVICE);
	BMessage reply;

	command.AddString("device", fName);

	return _control_input_server_(&command, &reply);
}
//------------------------------------------------------------------------------
status_t BInputDevice::Control(uint32 code, BMessage *message)
{
	if (!fName)
		return B_ERROR;

	BMessage command(IS_CONTROL_DEVICES);
	BMessage reply;

	command.AddString("device", fName);
	command.AddInt32("code", code);
	command.AddMessage("message", message);

	message->MakeEmpty();

	status_t err = _control_input_server_(&command, &reply);

	if (err == B_OK)
		reply.FindMessage("message", message);

	return err;
}
//------------------------------------------------------------------------------
status_t BInputDevice::Start(input_device_type type)
{
	BMessage command(IS_START_DEVICE);
	BMessage reply;

	command.AddInt32("type", type);

	return _control_input_server_(&command, &reply);
}
//------------------------------------------------------------------------------
status_t BInputDevice::Stop(input_device_type type)
{
	BMessage command(IS_STOP_DEVICE);
	BMessage reply;

	command.AddInt32("type", type);

	return _control_input_server_(&command, &reply);
}
//------------------------------------------------------------------------------
status_t BInputDevice::Control(input_device_type type, uint32 code,
									  BMessage *message)
{
	BMessage command(IS_CONTROL_DEVICES);
	BMessage reply;

	command.AddInt32("type", type);
	command.AddInt32("code", code);
	command.AddMessage("message", message);

	message->MakeEmpty();

	status_t err = _control_input_server_(&command, &reply);

	if (err == B_OK)
		reply.FindMessage("message", message);

	return err;
}
//------------------------------------------------------------------------------
BInputDevice::BInputDevice()
{
	fName = NULL;
	fType = B_UNDEFINED_DEVICE;
}
//------------------------------------------------------------------------------
void BInputDevice::set_name_and_type(const char *name, input_device_type type)
{
	if (fName)
	{
		free (fName);
		fName = NULL;
	}

	if (name)
		fName = strdup(name);

	fType = type;
}
//------------------------------------------------------------------------------
status_t _control_input_server_(BMessage *command, BMessage *reply)
{
	if (!input_server)
		input_server = new BMessenger;

	if (!input_server->IsValid())
		*input_server = BMessenger("Application/x-vnd.Be-input_server", -1, NULL);

	status_t err = input_server->SendMessage(command, reply);

	if (err != B_OK)
		return err;

	if (reply->FindInt32("status", err) != B_OK)
		return B_ERROR;

	return err;
}
//------------------------------------------------------------------------------
