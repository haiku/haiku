//------------------------------------------------------------------------------
//	main.cpp
//
//------------------------------------------------------------------------------

// Standard Includes -----------------------------------------------------------
#include <stdio.h>

// System Includes -------------------------------------------------------------
#include <Application.h>
#include <Looper.h>
#include <Message.h>
#include <OS.h>

// Project Includes ------------------------------------------------------------

// Local Includes --------------------------------------------------------------

// Local Defines ---------------------------------------------------------------
#ifdef USE_BAPP
	#define DO_EXIT	be_app_messenger.SendMessage(B_QUIT_REQUESTED)
#else
	#define DO_EXIT release_sem(gThreadLock)
#endif

// Globals ---------------------------------------------------------------------
#ifndef USE_BAPP
sem_id gThreadLock = create_sem(0, "gThreadLock");
#endif

class TLooper1 : public BLooper
{
	public:
		TLooper1() : BLooper() {;}
		void MessageReceived(BMessage* msg)
		{
			switch (msg->what)
			{
				case '2345':
					printf("Got message '2345' in %s\n", __PRETTY_FUNCTION__);
					DO_EXIT;
					break;
				default:
					BLooper::MessageReceived(msg);
			}
		}
};

class TLooper2 : public BLooper
{
	public:
		TLooper2(BMessenger target) : BLooper(), fTarget(target) {;}
		void MessageReceived(BMessage* msg)
		{
			switch (msg->what)
			{
				case '1234':
					printf("Got message '1234' in %s\n", __PRETTY_FUNCTION__);
					fTarget.SendMessage('2345');
					break;
				default:
					BLooper::MessageReceived(msg);
					break;
			}
		}

	private:
		BMessenger fTarget;
};

int main()
{
	BLooper*	fLooper1 = new TLooper1;
	BLooper*	fLooper2 = new TLooper2(fLooper1);
	fLooper1->Run();
	fLooper2->Run();
	printf("Sending message '1234' in %s\n", __PRETTY_FUNCTION__);
	BMessenger(fLooper2).SendMessage('1234');

	// Wait for loopers to finish
	acquire_sem(gThreadLock);

	return 0;
}

#if 0
class TestApp : public BApplication
{
	public:
		TestApp() : BApplication("application/x-vnd.FirstMessageTestApp")
		{
			fLooper1 = new TLooper1;
			fLooper2 = new TLooper2(fLooper1);
			fLooper1->Run();
			fLooper2->Run();
		}
		void ReadyToRun()
		{
			printf("Sending message '1234' in %s\n", __PRETTY_FUNCTION__);
			BMessenger(fLooper2).SendMessage('1234');
		}

	private:
		BLooper*	fLooper1;
		BLooper*	fLooper2;
};

int main()
{
	new TestApp;
	be_app->Run();
	delete be_app;
	return 0;
}
#endif

/*
 * $Log $
 *
 * $Id  $
 *
 */

