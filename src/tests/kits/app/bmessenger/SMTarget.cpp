// SMTarget.cpp

#include <stdio.h>
#include <stdlib.h>
#include <string>

#include <OS.h>
#include <String.h>
#include <TestShell.h>
#include <TestUtils.h>
#include <cppunit/TestAssert.h>

#include "SMTarget.h"
#include "SMLooper.h"
#include "SMRemoteTargetApp.h"


using namespace std;


// SMTarget

// constructor
SMTarget::SMTarget()
{
}

// destructor
SMTarget::~SMTarget()
{
}

// Init
void
SMTarget::Init(bigtime_t unblockTime, bigtime_t replyDelay)
{
}

// Handler
BHandler *
SMTarget::Handler()
{
	return NULL;
}

// Messenger
BMessenger
SMTarget::Messenger()
{
	return BMessenger();
}

// DeliverySuccess
bool
SMTarget::DeliverySuccess()
{
	return false;
}


// LocalSMTarget

// constructor
LocalSMTarget::LocalSMTarget(bool preferred)
			 : SMTarget(),
			   fHandler(NULL),
			   fLooper(NULL)
{
	// create looper and handler
	fLooper = new SMLooper;
	fLooper->Run();
	if (!preferred) {
		fHandler = new SMHandler;
		CHK(fLooper->Lock());
		fLooper->AddHandler(fHandler);
		fLooper->Unlock();
	}
}

// destructor
LocalSMTarget::~LocalSMTarget()
{
	if (fLooper) {
		fLooper->Lock();
		if (fHandler) {
			fLooper->RemoveHandler(fHandler);
			delete fHandler;
		}
		fLooper->Quit();
	}
}

// Init
void
LocalSMTarget::Init(bigtime_t unblockTime, bigtime_t replyDelay)
{
	fLooper->SetReplyDelay(replyDelay);
	fLooper->BlockUntil(unblockTime);
}

// Handler
BHandler *
LocalSMTarget::Handler()
{
	return fHandler;
}

// Messenger
BMessenger
LocalSMTarget::Messenger()
{
	return BMessenger(fHandler, fLooper);
}

// DeliverySuccess
bool
LocalSMTarget::DeliverySuccess()
{
	return fLooper->DeliverySuccess();
}


// RemoteSMTarget

static const char *kSMRemotePortName = "BMessenger_SMRemoteTarget";

// constructor
RemoteSMTarget::RemoteSMTarget(bool preferred)
			  : SMTarget(),
				fLocalPort(-1),
				fRemotePort(-1),
				fTarget()
{
	// create unused port name
	int32 id = atomic_add(&fID, 1);
	string portName(kSMRemotePortName);
	BString idString;
	idString << id;
	portName += idString.String();
	// create local port
	fLocalPort = create_port(5, portName.c_str());
	CHK(fLocalPort >= 0);
	// execute the remote app
	BString unescapedTestDir(BTestShell::GlobalTestDir());
	unescapedTestDir.CharacterEscape(" \t\n!\"'`$&()?*+{}[]<>|", '\\');
	string remoteApp(unescapedTestDir.String());
	remoteApp += "/../kits/app/SMRemoteTargetApp ";
	remoteApp += portName;
	if (preferred)
		remoteApp += " preferred";
	else
		remoteApp += " specific";
	remoteApp += " &";
	system(remoteApp.c_str());
	// wait for the remote app to send its init data
	smrt_init initData;
	CHK(_GetReply(SMRT_INIT, &initData, sizeof(smrt_init)) == B_OK);
	fRemotePort = initData.port;
	fTarget = initData.messenger;
}

// destructor
RemoteSMTarget::~RemoteSMTarget()
{
	_SendRequest(SMRT_QUIT);
	delete_port(fLocalPort);
}

// Init
void
RemoteSMTarget::Init(bigtime_t unblockTime, bigtime_t replyDelay)
{
	smrt_get_ready data;
	data.unblock_time = unblockTime;
	data.reply_delay = replyDelay;
	CHK(_SendRequest(SMRT_GET_READY, &data, sizeof(smrt_get_ready)) == B_OK);
}

// Messenger
BMessenger
RemoteSMTarget::Messenger()
{
	return fTarget;
}

// DeliverySuccess
bool
RemoteSMTarget::DeliverySuccess()
{
	CHK(_SendRequest(SMRT_DELIVERY_SUCCESS_REQUEST) == B_OK);
	smrt_delivery_success data;
	CHK(_GetReply(SMRT_DELIVERY_SUCCESS_REPLY, &data,
				  sizeof(smrt_delivery_success)) == B_OK);
	return data.success;
}

// _SendRequest
status_t
RemoteSMTarget::_SendRequest(int32 code, const void *buffer, size_t size)
{
	return write_port(fRemotePort, code, buffer, size);
}

// _GetReply
status_t
RemoteSMTarget::_GetReply(int32 expectedCode, void *buffer, size_t size)
{
	status_t error = B_OK;
	int32 code;
	ssize_t readSize = read_port(fLocalPort, &code, buffer, size);
	if (readSize < 0)
		error = readSize;
	else if ((uint32)readSize != size)
		error = B_ERROR;
	else if (code != expectedCode)
		error = B_ERROR;
	return error;
}

// ID
int32 RemoteSMTarget::fID = 0;


