// SMReplyTarget.cpp

#include <TestUtils.h>
#include <cppunit/TestAssert.h>

#include "SMReplyTarget.h"
#include "SMLooper.h"

// constructor
SMReplyTarget::SMReplyTarget(bool preferred)
			 : fHandler(NULL),
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
SMReplyTarget::~SMReplyTarget()
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

// Handler
BHandler *
SMReplyTarget::Handler()
{
	return fHandler;
}

// Messenger
BMessenger
SMReplyTarget::Messenger()
{
	return BMessenger(fHandler, fLooper);
}

// ReplySuccess
bool
SMReplyTarget::ReplySuccess()
{
	return fLooper->ReplySuccess();
}


