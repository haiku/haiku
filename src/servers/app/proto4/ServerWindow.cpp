#include <AppDefs.h>
#include <iostream.h>
#include <string.h>
#include <stdio.h>
#include "PortLink.h"
#include "ServerWindow.h"
#include "ServerApp.h"
#include "ServerProtocol.h"

ServerWindow::ServerWindow(const char *Title, uint32 Flags, uint32 Desktop,
	const BRect &Rect, ServerApp *App, port_id SendPort)
{
	cout << "ServerWindow " << Title << endl;

	if(Title)
	{
		title=new char[strlen(Title)];
		sprintf((char *)title,Title);
	}
	else
	{
		title=new char[strlen("Window")];
		sprintf((char *)title,"Window");
	}

	// sender is the monitored app's event port
	sender=SendPort;
	winlink=new PortLink(sender);

	if(App!=NULL)
		applink=new PortLink(App->receiver);
	else
		applink=NULL;
	
	// receiver is the port to which the app sends messages for the server
	receiver=create_port(30,title);
	
	cout << "ServerWin port for window " << title << "is at " << (int32)receiver << endl;
	if(receiver==B_NO_MORE_PORTS)
	{
		// uh-oh. We have a serious problem. Tell the app to quit
		if(applink!=NULL)
		{
			applink->SetOpCode(B_QUIT_REQUESTED);
			applink->Flush();
		}
	}
	else
	{
		// Everything checks out, so tell the application
		// where to send future messages
		winlink->SetOpCode(SET_SERVER_PORT);
		winlink->Attach(&receiver,sizeof(port_id));
		winlink->Flush();
	}
	active=false;
	frame.Set(0,0,0,0);
	hidecount=0;

	// Spawn our message monitoring thread
	thread=spawn_thread(MonitorWin,title,B_NORMAL_PRIORITY,this);
	if(thread!=B_NO_MORE_THREADS && thread!=B_NO_MEMORY)
		resume_thread(thread);

}

ServerWindow::~ServerWindow(void)
{
	if(applink!=NULL)
		delete applink;
	delete title;
	delete winlink;
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
	return title;
}

ServerApp *ServerWindow::GetApp(void)
{
	return app;
}

void ServerWindow::Show(void)
{
}

void ServerWindow::Hide(void)
{
}

bool ServerWindow::IsHidden(void)
{
	return (hidecount==0)?true:false;
}

void ServerWindow::SetFocus(bool value)
{
	active=value;
}

bool ServerWindow::HasFocus(void)
{
	return active;
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

void ServerWindow::SetFrame(const BRect &rect)
{
	frame=rect;
}

BRect ServerWindow::Frame(void)
{
	return frame;
}

status_t ServerWindow::Lock(void)
{
	return locker.Lock();
}

void ServerWindow::Unlock(void)
{
	locker.Unlock();
}

bool ServerWindow::IsLocked(void)
{
	return locker.IsLocked();
}

void ServerWindow::DispatchMessage(const void *msg, int nCode)
{
}

void ServerWindow::Loop(void)
{
	// Message-dispatching loop for the ServerWindow

	cout << "Main message loop for " << title << endl;

	int32 msgcode;
	int8 *msgbuffer=NULL;
	ssize_t buffersize,bytesread;
	
	for(;;)
	{
		buffersize=port_buffer_size(receiver);
		
		if(buffersize>0)
		{
			// buffers are PortLink messages. Allocate necessary buffer and
			// we'll cast it as a BMessage.
			msgbuffer=new int8[buffersize];
			bytesread=read_port(receiver,&msgcode,msgbuffer,buffersize);
		}
		else
			bytesread=read_port(receiver,&msgcode,NULL,0);

		if (bytesread != B_BAD_PORT_ID && bytesread != B_TIMED_OUT && bytesread != B_WOULD_BLOCK)
		{
			switch(msgcode)
			{
				default:
					cout << "ServerWin received unexpected code " << (int32)msgcode << endl;
					break;
			}

		}
	
		if(buffersize>0)
			delete msgbuffer;

		if(msgcode==B_QUIT_REQUESTED)
			break;
	}
}

int32 ServerWindow::MonitorWin(void *data)
{
	ServerWindow *win=(ServerWindow *)data;
	win->Loop();
	exit_thread(0);
	return 0;
}
