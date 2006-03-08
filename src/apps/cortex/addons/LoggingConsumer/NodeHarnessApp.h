// NodeHarnessApp.h

#ifndef NodeHarnessApp_H
#define NodeHarnessApp_H 1

#include <app/Application.h>

class NodeHarnessApp : public BApplication
{
public:
	NodeHarnessApp(const char* signature);
	void ReadyToRun();
};

#endif
