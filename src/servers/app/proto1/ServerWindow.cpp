#include <Rect.h>
#include "ServerWindow.h"

ServerWindow::ServerWindow(const char *Title, uint32 Flags, uint32 Desktop,
	const BRect &Rect, ServerApp *App, port_id SendPort)
{
}

ServerWindow::ServerWindow(ServerApp *App, ServerBitmap *Bitmap)
{
}

ServerWindow::~ServerWindow(void)
{
}

void ServerWindow::PostMessage(BMessage *msg)
{
}

void ServerWindow::ReplaceDecorator(void)
{
}

void ServerWindow::Quit(void)
{
}

const char *ServerWindow::GetTitle(void)
{
	return NULL;
}

ServerApp *ServerWindow::GetApp(void)
{
	return NULL;
}

BMessenger *ServerWindow::GetAppTarget(void)
{
	return NULL;
}

void ServerWindow::Show(void)
{
}

void ServerWindow::Hide(void)
{
}

void ServerWindow::SetFocus(void)
{
}

bool ServerWindow::HasFocus(void)
{
	return false;
}

void ServerWindow::DesktopActivated(int32 NewDesktop, const BPoint Resolution, color_space CSpace)
{
}

void ServerWindow::WindowActivated(bool Active)
{
}

void ServerWindow::ScreenModeChanged(const BPoint Resolustion, color_space CSpace)
{
}

void ServerWindow::SetFrame(const BRect &Frame)
{
}

BRect ServerWindow::Frame(void)
{
	return BRect(0,0,0,0);
}

thread_id ServerWindow::Run(void)
{
	return -1;
}

status_t ServerWindow::Lock(void)
{
	return B_ERROR;
}

status_t ServerWindow::Unlock(void)
{
	return B_ERROR;
}

bool ServerWindow::IsLocked(void)
{
	return false;
}

bool ServerWindow::DispatchMessage(BMessage *msg)
{
	return false;
}

bool ServerWindow::DispatchMessage(const void *msg, int nCode)
{
	return false;
}

void ServerWindow::Loop(void)
{
}
