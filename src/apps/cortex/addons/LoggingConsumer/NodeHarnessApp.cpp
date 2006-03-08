// NodeHarnessApp.cpp

#include "NodeHarnessApp.h"
#include "NodeHarnessWin.h"

NodeHarnessApp::NodeHarnessApp(const char *signature)
	: BApplication(signature)
{
}

void 
NodeHarnessApp::ReadyToRun()
{
	BWindow* win = new NodeHarnessWin(BRect(100, 200, 210, 330), "NodeLogger");
	win->Show();
}
