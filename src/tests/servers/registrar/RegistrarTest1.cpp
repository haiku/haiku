// RegistrarTest1.cpp

#include <stdio.h>

#include <Application.h>

class TestApp : public BApplication {
public:
	TestApp(const char* signature)
		:
		BApplication(signature)
	{
	}

	~TestApp()
	{
	}

	virtual void ArgvReceived(int32 argc, char** argv)
	{
		printf("TestApp::ArgvReceived(%" B_PRId32 ")\n", argc);
		BMessage *message = CurrentMessage();
		message->PrintToStream();
		BMessenger returnAddress(message->ReturnAddress());
		printf("team: %" B_PRId32 "\n", returnAddress.Team());
		for (int32 i = 0; i < argc; i++)
			printf("arg %" B_PRId32 ": `%s'\n", i, argv[i]);
	}

	virtual void RefsReceived(BMessage* message)
	{
		printf("TestApp::RefsReceived()\n");
		message->PrintToStream();
	}

	virtual void ReadyToRun()
	{
		printf("TestApp::ReadyToRun()\n");
//		PostMessage(B_QUIT_REQUESTED);
	}

};

// main
int
main()
{
	TestApp* app = new TestApp("application/x-vnd.Haiku-TestApp1");
	app->Run();
	delete app;

	return 0;
}

