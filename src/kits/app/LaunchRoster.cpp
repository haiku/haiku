/*
 * Copyright 2015-2016 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 */


#include <LaunchRoster.h>

#include <Application.h>
#include <String.h>
#include <StringList.h>

#include <launch.h>
#include <LaunchDaemonDefs.h>
#include <LaunchRosterPrivate.h>
#include <MessengerPrivate.h>


using namespace BPrivate;


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
BLaunchRoster::Private::RegisterSessionDaemon(const BMessenger& daemon)
{
	BMessage request(B_REGISTER_SESSION_DAEMON);
	status_t status = request.AddInt32("user", getuid());
	if (status == B_OK)
		status = request.AddMessenger("daemon", daemon);
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

	return _SendRequest(request, data);
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
BLaunchRoster::Target(const char* name, const BMessage& data,
	const char* baseName)
{
	return Target(name, &data, baseName);
}


status_t
BLaunchRoster::Target(const char* name, const BMessage* data,
	const char* baseName)
{
	if (name == NULL)
		return B_BAD_VALUE;

	BMessage request(B_LAUNCH_TARGET);
	status_t status = request.AddInt32("user", getuid());
	if (status == B_OK)
		status = request.AddString("target", name);
	if (status == B_OK && data != NULL && !data->IsEmpty())
		status = request.AddMessage("data", data);
	if (status == B_OK && baseName != NULL)
		status = request.AddString("base target", baseName);
	if (status != B_OK)
		return status;

	return _SendRequest(request);
}


status_t
BLaunchRoster::StopTarget(const char* name, bool force)
{
	if (name == NULL)
		return B_BAD_VALUE;

	BMessage request(B_STOP_LAUNCH_TARGET);
	status_t status = request.AddInt32("user", getuid());
	if (status == B_OK)
		status = request.AddString("target", name);
	if (status == B_OK)
		status = request.AddBool("force", force);
	if (status != B_OK)
		return status;

	return _SendRequest(request);
}


status_t
BLaunchRoster::Start(const char* name)
{
	if (name == NULL)
		return B_BAD_VALUE;

	BMessage request(B_LAUNCH_JOB);
	status_t status = request.AddInt32("user", getuid());
	if (status == B_OK)
		status = request.AddString("name", name);
	if (status != B_OK)
		return status;

	return _SendRequest(request);
}


status_t
BLaunchRoster::Stop(const char* name, bool force)
{
	if (name == NULL)
		return B_BAD_VALUE;

	BMessage request(B_STOP_LAUNCH_JOB);
	status_t status = request.AddInt32("user", getuid());
	if (status == B_OK)
		status = request.AddString("name", name);
	if (status == B_OK)
		status = request.AddBool("force", force);
	if (status != B_OK)
		return status;

	return _SendRequest(request);
}


status_t
BLaunchRoster::SetEnabled(const char* name, bool enable)
{
	if (name == NULL)
		return B_BAD_VALUE;

	BMessage request(B_ENABLE_LAUNCH_JOB);
	status_t status = request.AddInt32("user", getuid());
	if (status == B_OK)
		status = request.AddString("name", name);
	if (status == B_OK)
		status = request.AddBool("enable", enable);
	if (status != B_OK)
		return status;

	return _SendRequest(request);
}


status_t
BLaunchRoster::StartSession(const char* login)
{
	if (login == NULL)
		return B_BAD_VALUE;

	BMessage request(B_LAUNCH_SESSION);
	status_t status = request.AddInt32("user", getuid());
	if (status == B_OK)
		status = request.AddString("login", login);
	if (status != B_OK)
		return status;

	return _SendRequest(request);
}


status_t
BLaunchRoster::RegisterEvent(const BMessenger& source, const char* name,
	uint32 flags)
{
	return _UpdateEvent(B_REGISTER_LAUNCH_EVENT, source, name, flags);
}


status_t
BLaunchRoster::UnregisterEvent(const BMessenger& source, const char* name)
{
	return _UpdateEvent(B_UNREGISTER_LAUNCH_EVENT, source, name);
}


status_t
BLaunchRoster::NotifyEvent(const BMessenger& source, const char* name)
{
	return _UpdateEvent(B_NOTIFY_LAUNCH_EVENT, source, name);
}


status_t
BLaunchRoster::ResetStickyEvent(const BMessenger& source, const char* name)
{
	return _UpdateEvent(B_RESET_STICKY_LAUNCH_EVENT, source, name);
}


status_t
BLaunchRoster::GetTargets(BStringList& targets)
{
	BMessage request(B_GET_LAUNCH_TARGETS);
	status_t status = request.AddInt32("user", getuid());
	if (status != B_OK)
		return status;

	// send the request
	BMessage result;
	status = _SendRequest(request, result);
	if (status == B_OK)
		status = result.FindStrings("target", &targets);

	return status;
}


status_t
BLaunchRoster::GetTargetInfo(const char* name, BMessage& info)
{
	return _GetInfo(B_GET_LAUNCH_TARGET_INFO, name, info);
}


status_t
BLaunchRoster::GetJobs(const char* target, BStringList& jobs)
{
	BMessage request(B_GET_LAUNCH_JOBS);
	status_t status = request.AddInt32("user", getuid());
	if (status == B_OK && target != NULL)
		status = request.AddString("target", target);
	if (status != B_OK)
		return status;

	// send the request
	BMessage result;
	status = _SendRequest(request, result);
	if (status == B_OK)
		status = result.FindStrings("job", &jobs);

	return status;
}


status_t
BLaunchRoster::GetJobInfo(const char* name, BMessage& info)
{
	return _GetInfo(B_GET_LAUNCH_JOB_INFO, name, info);
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


status_t
BLaunchRoster::_SendRequest(BMessage& request)
{
	BMessage result;
	return _SendRequest(request, result);
}


status_t
BLaunchRoster::_SendRequest(BMessage& request, BMessage& result)
{
	// Send the request, and evaluate the reply
	status_t status = fMessenger.SendMessage(&request, &result);
	if (status == B_OK)
		status = result.what;

	return status;
}


status_t
BLaunchRoster::_UpdateEvent(uint32 what, const BMessenger& source,
	const char* name, uint32 flags)
{
	if (be_app == NULL || name == NULL || name[0] == '\0')
		return B_BAD_VALUE;

	BMessage request(what);
	status_t status = request.AddInt32("user", getuid());
	if (status == B_OK)
		status = request.AddMessenger("source", source);
	if (status == B_OK)
		status = request.AddString("owner", be_app->Signature());
	if (status == B_OK)
		status = request.AddString("name", name);
	if (status == B_OK && flags != 0)
		status = request.AddUInt32("flags", flags);
	if (status != B_OK)
		return status;

	return _SendRequest(request);
}


status_t
BLaunchRoster::_GetInfo(uint32 what, const char* name, BMessage& info)
{
	if (name == NULL || name[0] == '\0')
		return B_BAD_VALUE;

	BMessage request(what);
	status_t status = request.AddInt32("user", getuid());
	if (status == B_OK)
		status = request.AddString("name", name);
	if (status != B_OK)
		return status;

	return _SendRequest(request, info);
}
