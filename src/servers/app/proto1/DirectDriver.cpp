#include <stdio.h>
#include "DirectDriver.h"

OBAppServerScreen::OBAppServerScreen(status_t *error)
	: BWindowScreen("DirectDriver",B_32_BIT_800x600,error,true)
{
	redraw=true;
}

OBAppServerScreen::~OBAppServerScreen(void)
{
}

void OBAppServerScreen::MessageReceived(BMessage *msg)
{
	msg->PrintToStream();
	switch(msg->what)
	{
		case B_KEY_DOWN:
			Disconnect();
			QuitRequested();
			break;
		default:
			BWindowScreen::MessageReceived(msg);
			break;
	}
}

bool OBAppServerScreen::QuitRequested(void)
{
	be_app->PostMessage(B_QUIT_REQUESTED);
	return true;
}

int32 OBAppServerScreen::Draw(void *data)
{
	OBAppServerScreen *screen=(OBAppServerScreen *)data;
	for(;;)
	{
		if(screen->connected && screen->redraw)
		{
			// Draw a sample background
			screen->Clear(128,128,128);
			screen->SimWin(BRect(100,100,250,250));
			screen->redraw=false;
		}
	}
	return 1;
}

void OBAppServerScreen::ScreenConnected(bool ok)
{
	if(ok==true)
		connected=true;
	else
		connected=false;
}

void OBAppServerScreen::Clear(uint8 red,uint8 green,uint8 blue)
{
	if(!connected) return;
}

void OBAppServerScreen::SimWin(BRect rect)
{
	if(!connected) return;
}

DirectDriver::DirectDriver(void)
{
	printf("DirectDriver()\n");
	old_space=B_32_BIT_800x600;
}

DirectDriver::~DirectDriver(void)
{
	printf("~DirectDriver()\n");
	winscreen->Quit();
}

void DirectDriver::Initialize(void)
{
	printf("DirectDriver::Initialize()\n");
	status_t stat;
	winscreen=new OBAppServerScreen(&stat);
	if(stat==B_OK)
	{
		is_initialized=true;
		winscreen->Show();
	}
	else
		delete winscreen;

	if(winscreen->connected==false)
		return;
	ginfo=winscreen->CardInfo();
	for(int8 i=0;i<48;i++)
		ghooks[i]=winscreen->CardHookAt(i);
}

void DirectDriver::SafeMode(void)
{
	printf("DirectDriver::SafeMode()\n");
	Reset();
}

void DirectDriver::Reset(void)
{
	printf("DirectDriver::Reset()\n");
	if(is_initialized)
		winscreen->Clear(51,102,160);
}

void DirectDriver::SetScreen(uint32 space)
{
	printf("DirectDriver::SetScreen(%lu)\n",space);
}