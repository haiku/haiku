// SMTarget.cpp

#include <TestUtils.h>
#include <cppunit/TestAssert.h>

#include "SMTarget.h"
#include "SMLooper.h"

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

