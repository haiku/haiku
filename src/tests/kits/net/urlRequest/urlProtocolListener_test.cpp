#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <iostream>

#include <AppKit.h>
#include <Debug.h>
#include <KernelKit.h>
#include <NetworkKit.h>


class TestSyncListener : public BUrlProtocolListener {
public:
	void	ConnectionOpened(BUrlProtocol* caller) {
				printf("Thread<#%5d> ", find_thread(NULL));
				printf("TestSyncListener::ConnectionOpened(%p)\n", caller);
			}
};


class TestAsyncListener : public BUrlProtocolAsynchronousListener {
public:
			TestAsyncListener(bool transparent)
			:
			BUrlProtocolAsynchronousListener(transparent)
			{ }
			
	void	ConnectionOpened(BUrlProtocol* caller) {
				printf("Thread<#%5d> ", find_thread(NULL));
				printf("TestAsyncListener::ConnectionOpened(%p)\n", caller);
			}
};


class Test {
	// Synchronous listener
	TestSyncListener s;
	
	// Asynchronous listener with dispatcher not embedded
	TestAsyncListener a;
	BUrlProtocolDispatchingListener a_sync;
	
	// Asynchronous listener with embedded dispatcher
	TestAsyncListener a_transparent;
	
public:
			Test()
			:
			a(false),
			a_sync(&a),
			a_transparent(true)
			{ }
			
	void testListener(BUrlProtocolListener* listener)
	{
		listener->ConnectionOpened((BUrlProtocol*)0xdeadbeef);
	}
	
	
	void DoTest() {
		// Tests
		printf("Launching test from thread #%5ld\n", find_thread(NULL));
		testListener(&s);
		testListener(&a_sync);
		testListener(a_transparent.SynchronousListener());
	}
};


class TestThread {
	Test t;
	thread_id fThreadId;

public:	
	thread_id Run() {			
		fThreadId = spawn_thread(&TestThread::_ThreadEntry, "TestThread", 
			B_NORMAL_PRIORITY, this);
		
		if (fThreadId < B_OK)
			return fThreadId;
			
		status_t launchErr = resume_thread(fThreadId);
		
		if (launchErr < B_OK)
			return launchErr;
			
		return fThreadId;
	}
	
	static status_t _ThreadEntry(void* ptr) {
		TestThread* parent = (TestThread*)ptr;
		
		parent->t.DoTest();
		return B_OK;
	}
};


class TestApp : public BApplication {
	Test t;
	TestThread t2;
	
public:
			TestApp()
			:
			BApplication("application/x-vnd.urlprotocollistener-test")
			{ }
			
	void	ReadyToRun() {	
		t2.Run();
		t.DoTest();
		SetPulseRate(1000000);
	}
	
	void 	Pulse() {
		static int count = 0;
		count++;
		
		if (count == 1) {
			Quit();
		}
	}
};


int
main(int, char**)
{	
	new TestApp();
	
	// Let the async calls be handled
	be_app->Run();
	
	delete be_app;
	return EXIT_SUCCESS;
}
