/*
 * Copyright (c) 2001-2008, Haiku, Inc.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Marc Flerackers (mflerackers@androme.be)
 */

//!	Functions and class to manage input devices.

#include <stdlib.h>
#include <string.h>
#include <new>

#include <Input.h>
#include <List.h>
#include <Message.h>

#include <input_globals.h>
#include <InputServerTypes.h>


static BMessenger *sInputServer = NULL;


BInputDevice *
find_input_device(const char *name)
{
	BMessage command(IS_FIND_DEVICES);
	BMessage reply;

	command.AddString("device", name);

	status_t err = _control_input_server_(&command, &reply);

	if (err != B_OK)
		return NULL;

	BInputDevice *dev = new (std::nothrow) BInputDevice;
	if (dev == NULL)
		return NULL;

	const char *device;
	int32 type;

	reply.FindString("device", &device);
	reply.FindInt32("type", &type);

	dev->_SetNameAndType(device, (input_device_type)type);

	return dev;
}


status_t
get_input_devices(BList *list)
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

	while (reply.FindString("device", i, &name) == B_OK) {
		reply.FindInt32("type", i++, &type);

		BInputDevice *dev = new (std::nothrow) BInputDevice;
		if (dev != NULL) {
			dev->_SetNameAndType(name, (input_device_type)type);
			list->AddItem(dev);
		}
	}

	return err;
}


status_t
watch_input_devices(BMessenger target, bool start)
{
	BMessage command(IS_WATCH_DEVICES);
	BMessage reply;

	command.AddMessenger("target", target);
	command.AddBool("start", start);

	return _control_input_server_(&command, &reply);
}


BInputDevice::~BInputDevice()
{
	free(fName);
}


const char *
BInputDevice::Name() const
{
	return fName;
}


input_device_type
BInputDevice::Type() const
{
	return fType;
}


bool
BInputDevice::IsRunning() const
{
	if (!fName)
		return false;

	BMessage command(IS_IS_DEVICE_RUNNING);
	BMessage reply;

	command.AddString("device", fName);

	return _control_input_server_(&command, &reply) == B_OK;
}


status_t
BInputDevice::Start()
{
	if (!fName)
		return B_ERROR;

	BMessage command(IS_START_DEVICE);
	BMessage reply;

	command.AddString("device", fName);

	return _control_input_server_(&command, &reply);
}


status_t
BInputDevice::Stop()
{
	if (!fName)
		return B_ERROR;

	BMessage command(IS_STOP_DEVICE);
	BMessage reply;

	command.AddString("device", fName);

	return _control_input_server_(&command, &reply);
}


status_t
BInputDevice::Control(uint32 code, BMessage *message)
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


status_t
BInputDevice::Start(input_device_type type)
{
	BMessage command(IS_START_DEVICE);
	BMessage reply;

	command.AddInt32("type", type);

	return _control_input_server_(&command, &reply);
}


status_t
BInputDevice::Stop(input_device_type type)
{
	BMessage command(IS_STOP_DEVICE);
	BMessage reply;

	command.AddInt32("type", type);

	return _control_input_server_(&command, &reply);
}


status_t
BInputDevice::Control(input_device_type type, uint32 code, BMessage *message)
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


BInputDevice::BInputDevice()
	:
	fName(NULL),
	fType(B_UNDEFINED_DEVICE)
{
}


void
BInputDevice::_SetNameAndType(const char *name, input_device_type type)
{
	if (fName) {
		free(fName);
		fName = NULL;
	}

	if (name)
		fName = strdup(name);

	fType = type;
}


status_t
_control_input_server_(BMessage *command, BMessage *reply)
{
	if (!sInputServer) {
		sInputServer = new (std::nothrow) BMessenger;
		if (!sInputServer)
			return B_NO_MEMORY;
	}

	if (!sInputServer->IsValid())
		*sInputServer = BMessenger("application/x-vnd.Be-input_server", -1, NULL);

	status_t err = sInputServer->SendMessage(command, reply);

	if (err != B_OK)
		return err;

	if (reply->FindInt32("status", &err) != B_OK)
		return B_ERROR;

	return err;
}
