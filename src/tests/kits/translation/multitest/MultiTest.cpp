// MultiTest.cpp

#include "MainControlWindow.h"
#include "MultiTest.h"

int main(int argc, char **argv)
{
	MultiTestApplication app;
	app.Run();
	
	return 0;
}

MultiTestApplication::MultiTestApplication()
	: BApplication("application/x-vnd.MultiTest")
{
	pRoster = new BTranslatorRoster();
	pRoster->AddTranslators();

	MainControlWindow *pWindow;
	pWindow = new MainControlWindow();
	
	// make window visible
	pWindow->Show();
}

MultiTestApplication::~MultiTestApplication()
{
	delete pRoster;
}

BTranslatorRoster *
MultiTestApplication::GetTranslatorRoster() const
{
	return pRoster;
}