// RegistrarTest1.cpp

#include <stdio.h>

#include <Application.h>

class TestApp : public BApplication {
public:
	TestApp(const char *signature)
		: BApplication(signature)
	{
	}

	~TestApp()
	{
	}
};

// main
int
main()
{
	TestApp *app = new TestApp("application/x-vnd.OBOS-TestApp1");
//	app->Run();
	delete app;
	return 0;
}

