/*
	AppServer.cpp
		File for the main app_server thread. This particular thread monitors for
		application start and quit messages. It also starts the housekeeping threads
		and initializes most of the server's globals.
*/
#include <Message.h>
#include <AppDefs.h>
#include "scheduler.h"
#include <iostream.h>
#include <stdio.h>
#include <OS.h>
#include <String.h>
#include <Debug.h>
#include "AppServer.h"
#include "Desktop.h"
#include "ServerProtocol.h"
#include "ServerCursor.h"
#include "PortLink.h"
#include "DisplayDriver.h"
#include "DebugTools.h"
#include "ViewDriver.h"
#include "DirectDriver.h"
#include "ScreenDriver.h"

//#define DEBUG_APPSERVER_THREAD

AppServer::AppServer(void) : BApplication("application/x-vnd.obe-OBAppServer")
{
#ifdef DEBUG_APPSERVER_THREAD
	printf("AppServer()\n");
#endif

	mouseport=create_port(30,SERVER_INPUT_PORT);
	messageport=create_port(20,SERVER_PORT_NAME);
#ifdef DEBUG_APPSERVER_THREAD
printf("Server message port: %ld\n",messageport);
printf("Server input port: %ld\n",mouseport);
#endif
	applist=new BList(0);
	quitting_server=false;
	exit_poller=false;

//	driver=new ViewDriver;
//	driver=new DirectDriver;
	driver=new ScreenDriver;
	driver->Initialize();

	// Spawn our input-polling thread
	poller_id = spawn_thread(PollerThread, "Poller", B_NORMAL_PRIORITY, this);
	if (poller_id >= 0)
		resume_thread(poller_id);
}

AppServer::~AppServer(void)
{
	// If these threads are still running, kill them - after this, if exit_poller
	// is deleted, who knows what will happen... These things will just return an
	// error and fail if the threads have already exited.
	kill_thread(poller_id);

	driver->Shutdown();
	delete driver;

}

void AppServer::RunTests(void)
{
	driver->SetScreen(B_32_BIT_800x600);
	driver->Clear(80,85,152);

//	TestScreenStates();
	TestArcs();
	TestBeziers();
	TestEllipses();
	TestLines();
	TestPolygon();
	TestRects();
	TestRegions();
	TestShape();
	TestTriangle();

	TestCursors();
	TestFonts();

}

thread_id AppServer::Run(void)
{
#ifdef DEBUG_APPSERVER_THREAD
printf("AppServer::Run()\n");
#endif	
	RunTests();
	MainLoop();
	return 0;
}

void AppServer::MainLoop(void)
{
	// Main loop (duh). Monitors the message queue and dispatches as appropriate.
	// Input messages are handled by the poller, however.

#ifdef DEBUG_APPSERVER_THREAD
printf("AppServer::MainLoop()\n");
#endif

	int32 msgcode;
	int8 *msgbuffer=NULL;
	ssize_t buffersize,bytesread;
	
	for(;;)
	{
		buffersize=port_buffer_size(messageport);
		if(buffersize>0)
			msgbuffer=new int8[buffersize];
		bytesread=read_port(messageport,&msgcode,msgbuffer,buffersize);

		if (bytesread != B_BAD_PORT_ID && bytesread != B_TIMED_OUT && bytesread != B_WOULD_BLOCK)
		{
			switch(msgcode)
			{
				case CREATE_APP:
				case DELETE_APP:
				case B_QUIT_REQUESTED:
					DispatchMessage(msgcode,msgbuffer);
					break;
				default:
				{
#ifdef DEBUG_APPSERVER_THREAD
printf("Server received unexpected code %ld\n",msgcode);
#endif
					break;
				}
			}

		}

		if(buffersize>0)
			delete msgbuffer;
		if(msgcode==DELETE_APP || msgcode==B_QUIT_REQUESTED)
		{
			if(quitting_server==true && applist->CountItems()==0)
				break;
		}
	}
	// Make sure our polling thread has exited
	if(find_thread("Poller")!=B_NAME_NOT_FOUND)
		kill_thread(poller_id);

}


void AppServer::DispatchMessage(int32 code, int8 *buffer)
{
	switch(code)
	{
		case B_QUIT_REQUESTED:
		{
			quitting_server=true;
			exit_poller=true;
			break;
		}
		default:
			break;
	}
}

void AppServer::Poller(void)
{
	// This thread handles nothing but input messages for mouse and keyboard
	int32 msgcode;
	int8 *msgbuffer=NULL,*index;
	ssize_t buffersize,bytesread;
	
	for(;;)
	{
		buffersize=port_buffer_size(mouseport);
		if(buffersize>0)
			msgbuffer=new int8[buffersize];
		bytesread=read_port(mouseport,&msgcode,msgbuffer,buffersize);

		if (bytesread != B_BAD_PORT_ID && bytesread != B_TIMED_OUT && bytesread != B_WOULD_BLOCK)
		{
			switch(msgcode)
			{
				// Process the cursor and then pass it on
				case B_MOUSE_MOVED:
				{
					// Attached data:
					// 1) int64 - time of mouse click
					// 2) float - x coordinate of mouse click
					// 3) float - y coordinate of mouse click
					// 4) int32 - buttons down
					
					index=msgbuffer;

					// Time sent is not necessary for cursor processing.
					index += sizeof(int64);
					
					float tempx=0,tempy=0;
					tempx=*((float*)index);
					index+=sizeof(float);
					tempy=*((float*)index);
					index+=sizeof(float);

					driver->MoveCursorTo(tempx,tempy);

					break;
				}
				default:
					//printf("Server::Poller received unexpected code %ld\n",msgcode);
					break;
			}

		}

		if(buffersize>0)
			delete msgbuffer;

		if(exit_poller)
			break;
	}
}

int32 AppServer::PollerThread(void *data)
{
#ifdef DEBUG_APPSERVER_THREAD
printf("Starting Poller thread...\n");
#endif
	AppServer *server=(AppServer *)data;
	
	// This won't return until it's told to
	server->Poller();
	
#ifdef DEBUG_APPSERVER_THREAD
printf("Exiting Poller thread...\n");
#endif
	exit_thread(B_OK);
	return B_OK;
}

void AppServer::TestScreenStates(void)
{
	driver->SetScreen(B_8_BIT_640x480);
	snooze(3000000);
	driver->SetScreen(B_8_BIT_800x600);
}

void AppServer::TestArcs(void)
{
	driver->SetHighColor(0,0,255);
	driver->FillArc(275,95,70,45,40,210,(uint8 *)&B_SOLID_HIGH);

	for(int i=0; i<45; i+=5)
	{
		driver->SetHighColor(128+(i*2),128+(i*2),128+(i*2));
		driver->StrokeArc(280+i,100+i,75+i,50+i,45+i,215+i,(uint8 *)&B_SOLID_HIGH);
	}
}

void AppServer::TestBeziers(void)
{
	BPoint	pts[4];
	
	pts[0].Set(500,300),
	pts[1].Set(350,450),
	pts[2].Set(350,300),
	pts[3].Set(500,450);

	driver->SetHighColor(0,255,0);
	driver->StrokeBezier(pts, (uint8 *)&B_SOLID_HIGH);

	// Fill Bezier
	pts[0].Set(250,250),
	pts[1].Set(300,300),
	pts[2].Set(300,250),
	pts[3].Set(300,250);

	driver->SetHighColor(255,0,0);
	driver->FillBezier(pts, (uint8 *)&B_SOLID_HIGH);

}

void AppServer::TestEllipses(void)
{
	driver->SetHighColor(0,0,0);
	driver->FillEllipse(200,200, 10, 5,(uint8*)&B_SOLID_HIGH);

	for(int i=5; i<255; i+=10)
	{
		driver->SetHighColor(i,i,i);
		driver->StrokeEllipse(200,200, 10+i, 5+i,(uint8*)&B_SOLID_HIGH);
	}

}

void AppServer::TestFonts(void)
{
	// Draw String
	BString string("OpenBeOS: The Future is Drawing Near...");
	uint16 length=string.Length();

	driver->SetHighColor(0,0,0);
	driver->DrawString((char*)string.String(),length,BPoint(10,200));
}

void AppServer::TestLines(void)
{
	int32 i;
	rgb_color col;
	col.red=255;
	col.green=0;
	col.blue=0;

	driver->MovePenTo(BPoint(10,150));
	driver->SetHighColor(255,0,0);

	for(i=0;i<100;i++)
	{
		col.green=100+i;
		col.blue=100+i;
		driver->StrokeLine(BPoint(10,10),BPoint(10+i,100),col);
	}

	for(i=0;i<100;i++)
		driver->StrokeLine(BPoint(10+i,200),(uint8*)&B_SOLID_HIGH);

//	BPoint start[100],end[100];
//	rgb_color colarray[100];

/*	for(i=0;i<100;i++)
	{
		start[i].Set(10,10);
		end[i].Set(110,10+i);
		colarray[i].red=100+i;
		colarray[i].green=100+i;
		colarray[i].blue=255;
	}
	driver->DrawLineArray(100,start,end,colarray);
*/
	driver->BeginLineArray(100);
	for(i=0;i<100;i++)
	{
		col.red=100+i;
		col.green=100+i;
		col.blue=255;
		driver->AddLine(BPoint(10,10),BPoint(110,10+i),col);
	}
	driver->EndLineArray();
}

void AppServer::TestPolygon(void)
{
	int x[]={10,20,30,40,50,40,30,20,10};
	int y[]={20,10,20,30,40,50,40,30,25};
	driver->StrokePolygon(x,y,9,false);
}

void AppServer::TestRects(void)
{
	BRect rect(10,270,75,335);

	rgb_color col;
	col.red=255;
	col.green=255;
	col.blue=0;

	driver->SetHighColor(255,0,0);
	driver->StrokeRect(rect,(uint8*)&B_SOLID_HIGH);

	rect.InsetBy(1,1);
	driver->SetHighColor(0,0,0);
	driver->FillRect(rect,(uint8*)&B_SOLID_HIGH);

	rect.InsetBy(1,1);
	driver->StrokeRect(rect,col);
	
	rect.InsetBy(1,1);
	col.red=0;
	driver->FillRect(rect,col);

	rect.OffsetBy(100,10);
	driver->SetHighColor(0,255,0);
	driver->StrokeRoundRect(rect,10,30,(uint8*)&B_SOLID_HIGH);

	rect.OffsetBy(100,10);
	driver->SetHighColor(0,255,255);
	driver->FillRoundRect(rect,10,30,(uint8*)&B_SOLID_HIGH);
}

void AppServer::TestRegions(void)
{
}

void AppServer::TestShape(void)
{
}

void AppServer::TestTriangle(void)
{
	BRect invalid;
	BPoint pt[3];
	pt[0].Set(600,100);
	pt[1].Set(500,200);
	pt[2].Set(550,450);

	invalid.Set(	MIN( MIN(pt[0].x,pt[1].x), pt[2].x),
					MIN( MIN(pt[0].y,pt[1].y), pt[2].y),
					MAX( MAX(pt[0].x,pt[1].x), pt[2].x),
					MAX( MAX(pt[0].y,pt[1].y), pt[2].y)	);

	driver->SetHighColor(228,228,50);
	driver->FillTriangle(pt[0],pt[1],pt[2],invalid,(uint8*)&B_SOLID_HIGH);
	
	
	pt[0].Set(50,50);
	pt[1].Set(200,475);
	pt[2].Set(400,400);
	invalid.Set(	MIN( MIN(pt[0].x,pt[1].x), pt[2].x),
					MIN( MIN(pt[0].y,pt[1].y), pt[2].y),
					MAX( MAX(pt[0].x,pt[1].x), pt[2].x),
					MAX( MAX(pt[0].y,pt[1].y), pt[2].y)	);
	// Stroke Triangle
	driver->SetHighColor(0,128,128);
	driver->StrokeTriangle(pt[0],pt[1],pt[2],invalid,(uint8*)&B_SOLID_HIGH);
}

void AppServer::TestCursors(void)
{
	int8 cross[] = { 16,1,5,5,
	14,0,4,0,4,0,4,0,128,32,241,224,128,32,4,0,
	4,0,4,0,14,0,0,0,0,0,0,0,0,0,0,0,
	14,0,4,0,4,0,4,0,128,32,245,224,128,32,4,0,
	4,0,4,0,14,0,0,0,0,0,0,0,0,0,0,0 };

	ServerCursor *crosscursor=new ServerCursor(cross);
	driver->SetCursor(crosscursor);

	driver->HideCursor();
	driver->ShowCursor();
	driver->ObscureCursor();

	delete crosscursor;
}

int main( int argc, char** argv )
{
	// There can be only one....
	if(find_port(SERVER_PORT_NAME)!=B_NAME_NOT_FOUND)
		return -1;

	AppServer *app_server = new AppServer();
	app_server->Run();
	delete app_server;
	return 0;
}
