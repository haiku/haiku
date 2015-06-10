/*
 * Copyright 2015 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 */


#include <LaunchRoster.h>

#include <Application.h>
#include <String.h>

#include <launch.h>
#include <LaunchDaemonDefs.h>
#include <LaunchRosterPrivate.h>
#include <MessengerPrivate.h>


using namespace BPrivate;


const BLaunchRoster* be_launch_roster;


BLaunchRoster::Private::Private(BLaunchRoster* roster)
	:
	fRoster(roster)
{
}


BLaunchRoster::Private::Private(BLaunchRoster& roster)
	:
	fRoster(&roster)
{
}


status_t
BLaunchRoster::Private::RegisterSession(const BMessenger& target)
{
	BMessage request(B_REGISTER_LAUNCH_SESSION);
	status_t status = request.AddInt32("user", getuid());
	if (status == B_OK)
		status = request.AddMessenger("target", target);
	if (status != B_OK)
		return status;

	// send the request
	BMessage result;
	status = fRoster->fMessenger.SendMessage(&request, &result);

	// evaluate the reply
	if (status == B_OK)
		status = result.what;

	return status;
}


// #pragma mark -


BLaunchRoster::BLaunchRoster()
{
	_InitMessenger();
}


BLaunchRoster::~BLaunchRoster()
{
}


status_t
BLaunchRoster::InitCheck() const
{
	return fMessenger.Team() >= 0 ? B_OK : B_ERROR;
}


status_t
BLaunchRoster::GetData(BMessage& data)
{
	if (be_app == NULL)
		return B_BAD_VALUE;

	return GetData(be_app->Signature(), data);
}


status_t
BLaunchRoster::GetData(const char* signature, BMessage& data)
{
	if (signature == NULL || signature[0] == '\0')
		return B_BAD_VALUE;

	BMessage request(B_GET_LAUNCH_DATA);
	status_t status = request.AddString("name", signature);
	if (status == B_OK)
		status = request.AddInt32("user", getuid());
	if (status != B_OK)
		return status;

	// send the request
	status = fMessenger.SendMessage(&request, &data);

	// evaluate the reply
	if (status == B_OK)
		status = data.what;

	return status;
}


port_id
BLaunchRoster::GetPort(const char* name)
{
	if (be_app == NULL)
		return B_BAD_VALUE;

	return GetPort(be_app->Signature(), name);
}


port_id
BLaunchRoster::GetPort(const char* signature, const char* name)
{
	BMessage data;
	status_t status = GetData(signature, data);
	if (status == B_OK) {
		BString fieldName;
		if (name != NULL)
			fieldName << name << "_";
		fieldName << "port";

		port_id port = data.GetInt32(fieldName.String(), B_NAME_NOT_FOUND);
		if (port >= 0)
			return port;
	}

	return -1;
}


status_t
BLaunchRoster::StartSession(const char* login, const char* password)
{
	if (login == NULL || password == NULL)
		return B_BAD_VALUE;

	BMessage request(B_LAUNCH_SESSION);
	status_t status = request.AddInt32("user", getuid());
	if (status == B_OK)
		status = request.AddString("login", login);
	if (status == B_OK)
		status = request.AddString("password", password);
	if (status != B_OK)
		return status;

	// send the request
	BMessage result;
	status = fMessenger.SendMessage(&request, &result);

	// evaluate the reply
	if (status == B_OK)
		status = result.what;

	return status;
}


void
BLaunchRoster::_InitMessenger()
{
	// find the launch_daemon port
	port_id daemonPort = BPrivate::get_launch_daemon_port();
	port_info info;
	if (daemonPort >= 0 && get_port_info(daemonPort, &info) == B_OK) {
		BMessenger::Private(fMessenger).SetTo(info.team, daemonPort,
			B_PREFERRED_TOKEN);
	}
}
