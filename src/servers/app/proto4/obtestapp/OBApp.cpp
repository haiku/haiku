/*
	OBApp.cpp:	App faker for OpenBeOS app_server prototype #4

	This one happens to be a graphics tester.

	ReadyToRun is set to spawn a thread to post all the messages. Posting from a
	separate thread will be necessary once packetized graphics messaging is done.
	
	Because graphics message packetizing is not done, the code in each function to
	test the graphics will obviously change even though the data format which the
	server will ultimately receive will stay the same.
*/

#include <OS.h>
#include <GraphicsDefs.h>	// for the pattern struct and screen mode defs
#include <Rect.h>
#include <Region.h>
#include <Cursor.h>
#include <Font.h>
#include <Application.h>
#include <Handler.h>
#include <Message.h>
#include <stdio.h>
#include <String.h>
#include "ServerProtocol.h"
#include "PortLink.h"
#include "OBApp.h"

TestApp::TestApp(void)
{
	initialized=true;

	// Receives messages from server and others
	messageport=create_port(50,"msgport");
	if(messageport==B_BAD_VALUE || messageport==B_NO_MORE_PORTS)
	{	printf("TestApp: Couldn't create message port\n");
		initialized=false;
	}
	
	// Find the port for the app_server
	serverport=find_port("OBappserver");
	if(serverport==B_NAME_NOT_FOUND)
	{	printf("TestApp: Couldn't find server port\n");
		initialized=false;

		serverlink=new PortLink(create_port(50,"OBAppServer"));
	}
	else
	{
		serverlink=new PortLink(serverport);
	}

//	signature=new char[255];
	signature=new char[strlen("application/x-vnd.wgp-OBTestApp")];
	sprintf(signature,"application/x-vnd.wgp-OBTestApp");

}

TestApp::~TestApp(void)
{
//	printf("%s::~TestApp()\n",signature);
	delete signature;

	if(initialized==true)
		delete serverlink;
}

void TestApp::MainLoop(void)
{
	printf("TestApp::MainLoop()\n");

	// Notify server of app's existence
	serverlink->SetOpCode(CREATE_APP);
	serverlink->Attach(&messageport,sizeof(port_id));
//	serverlink.Attach(PID);
	serverlink->Attach(signature,strlen(signature));
	serverlink->Flush();
	
	int32 msgcode;
	uint8 *msgbuffer=NULL;
	ssize_t buffersize,bytesread;
	
	for(;;)
	{
		buffersize=port_buffer_size(messageport);
		
		if(buffersize>0)
		{
			// buffers are flattened messages. Allocate necessary buffer and
			// we'll cast it as a BMessage.
			msgbuffer=new uint8[buffersize];
			bytesread=read_port(messageport,&msgcode,msgbuffer,buffersize);
		}
		else
			bytesread=read_port(messageport,&msgcode,NULL,0);
			
		if (bytesread != B_BAD_PORT_ID && bytesread != B_TIMED_OUT && bytesread != B_WOULD_BLOCK)
		{
			switch(msgcode)
			{
				case B_QUIT_REQUESTED:
				{
					// Attached data:
					// None
					printf("%s: Quit requested\n",signature);
					serverlink->SetOpCode(msgcode);
					serverlink->Flush();
					break;
				}
				case SET_SERVER_PORT:
				{
					// This message is received when the server has created
					// the ServerApp object to monitor our application. It is
					// sending us the port to which we send future messages

					// Attached data:
					// port_id - id of the port of the corresponding ServerApp
					if(buffersize<1)
					{
						printf("%s: Set Server Port has no attached data\n",signature);
						break;
					}
					printf("%s: Set Server Port to %ld\n",signature,*((port_id *)msgbuffer));
					serverport=*((port_id *)msgbuffer);
					serverlink->SetPort(serverport);
					ReadyToRun();
					break;
				}
				case B_MOUSE_MOVED:
				case B_MOUSE_DOWN:
				case B_MOUSE_UP:
					DispatchMessage(msgcode, (int8*)msgbuffer);
					break;
				default:
					printf("OBTestApp received unexpected code %ld\n",msgcode);
					break;
			}

			if(msgcode==B_QUIT_REQUESTED)
			{
				printf("Quitting %s\n",signature);
				break;
			}
		}

		if(buffersize>0)
			delete msgbuffer;
	}
}

void TestApp::DispatchMessage(BMessage *msg, BHandler *handler)
{
//	printf("TestApp::DispatchMessage()\n");
}

// For test application purposes only - will be replaced later
void TestApp::DispatchMessage(int32 code, int8 *buffer)
{
	switch(code)
	{
		case B_MOUSE_DOWN:
		case B_MOUSE_UP:
		case B_MOUSE_MOVED:
			break;
		default:
			printf("TestApp %s received unknown code %ld\n",signature,code);
			break;
	}
}

void TestApp::MessageReceived(BMessage *msg)
{
//	printf("TestApp::MessageReceived()\n");
}

void TestApp::ShowCursor(void)
{
	serverlink->SetOpCode(SHOW_CURSOR);
	serverlink->Flush();
}

void TestApp::HideCursor(void)
{
	serverlink->SetOpCode(HIDE_CURSOR);
	serverlink->Flush();
}

void TestApp::ObscureCursor(void)
{
	serverlink->SetOpCode(OBSCURE_CURSOR);
	serverlink->Flush();
}

bool TestApp::IsCursorHidden(void)
{
	return false;
}

void TestApp::SetCursor(const void *cursor)
{
printf("TestApp::SetCursor()\n");
	// attach & send the 68-byte chunk
	serverlink->SetOpCode(SET_CURSOR_DATA);
	serverlink->Attach((void*)cursor, 68);
	serverlink->Flush();
}

void TestApp::SetCursor(const BCursor *cursor, bool sync=true)
{
	// attach & send the BCursor
	serverlink->SetOpCode(SET_CURSOR_BCURSOR);
	serverlink->Attach((BCursor*)cursor, sizeof(BCursor));
	serverlink->Flush();
}

void TestApp::SetCursor(BBitmap *bmp)
{
	// This will be enabled once the BBitmap functionality is better nailed down
	
/*	// attach & send the TestBitmap's area_id
	serverlink->SetOpCode(SET_CURSOR_BCURSOR);
	serverlink->Attach(bmp, sizeof(BCursor));
	serverlink->Flush();
*/
}

void TestApp::ReadyToRun(void)
{
	thread_id tid=spawn_thread(TestGraphics,"testthread",B_NORMAL_PRIORITY,this);
	if(tid!=B_NO_MEMORY && tid!=B_NO_MORE_THREADS)
		resume_thread(tid);
}

int32 TestApp::TestGraphics(void *data)
{
	printf("TestGraphics Spawned\n");
	TestApp *app=(TestApp*)data;
	
	app->TestScreenStates();
	app->TestArcs();
	app->TestBeziers();
	app->TestEllipses();
	app->TestLines();
//	app->TestPolygon();
	app->TestRects();
//	app->TestRegions();
//	app->TestShape();
	app->TestTriangle();
	app->TestCursors();
	app->TestFonts();
	printf("TestGraphics Exited\n");
	exit_thread(0);
	return 0;
}

void TestApp::SetHighColor(uint8 r, uint8 g, uint8 b)
{
	// Server always expects an alpha value
	uint8 a=255;
	serverlink->SetOpCode(GFX_SET_HIGH_COLOR);
	serverlink->Attach(&r,sizeof(uint8));
	serverlink->Attach(&g,sizeof(uint8));
	serverlink->Attach(&b,sizeof(uint8));
	serverlink->Attach(&a,sizeof(uint8));
	serverlink->Flush();
}

void TestApp::SetLowColor(uint8 r, uint8 g, uint8 b)
{
	// Server always expects an alpha value
	uint8 a=255;
	serverlink->SetOpCode(GFX_SET_LOW_COLOR);
	serverlink->Attach(&r,sizeof(uint8));
	serverlink->Attach(&g,sizeof(uint8));
	serverlink->Attach(&b,sizeof(uint8));
	serverlink->Attach(&a,sizeof(uint8));
	serverlink->Flush();
}

void TestApp::TestScreenStates(void)
{
	// This function is here to test all the graphics state code, such as
	// resolution, desktop switching, etc.
	
	// Protocol defined here will be used by the screen-related global functions

	int32 workspace=0;
	uint32 mode=B_32_BIT_640x480;
	bool sticky=false;

	// set_screen_space()
	serverlink->SetOpCode(GFX_SET_SCREEN_MODE);
	serverlink->Attach(&workspace,sizeof(int32));
	serverlink->Attach(&mode,sizeof(int32));
	serverlink->Attach(&sticky,sizeof(bool));
	serverlink->Flush();	
	
	// This code works, I just don't want to mess with it when I'm
	// working on the other tests.
/*
	// activate_workspace()
	workspace=1;
	serverlink->SetOpCode(GFX_ACTIVATE_WORKSPACE);
	serverlink->Attach(&workspace,sizeof(int32));
	serverlink->Flush();	
*/
}

void TestApp::TestArcs(void)
{
	float centerx=275, centery=95,
		radx=70, rady=45,
		angle=40,
		span=210;

	// Fill Arc
	SetHighColor(0,0,255);
	serverlink->SetOpCode(GFX_FILL_ARC);
	serverlink->Attach(&centerx,sizeof(float));
	serverlink->Attach(&centery,sizeof(float));
	serverlink->Attach(&radx,sizeof(float));
	serverlink->Attach(&rady,sizeof(float));
	serverlink->Attach(&angle,sizeof(float));
	serverlink->Attach(&span,sizeof(float));
	serverlink->Attach((pattern *)&B_SOLID_HIGH, sizeof(pattern));
	serverlink->Flush();

	// Stroke Arc
	for(int i=0; i<45; i+=5)
	{
		centerx=280+i;
		centery=100+i;
		radx=75+i;
		rady=50+i;
		angle=45+i;
		span=215+i;
		SetHighColor(128+(i*2),128+(i*2),128+(i*2));
		serverlink->SetOpCode(GFX_STROKE_ARC);
		serverlink->Attach(&centerx,sizeof(float));
		serverlink->Attach(&centery,sizeof(float));
		serverlink->Attach(&radx,sizeof(float));
		serverlink->Attach(&rady,sizeof(float));
		serverlink->Attach(&angle,sizeof(float));
		serverlink->Attach(&span,sizeof(float));
		serverlink->Attach((pattern *)&B_SOLID_HIGH, sizeof(pattern));
		serverlink->Flush();
	}
}

void TestApp::TestBeziers(void)
{
	BPoint	p1(500,300),
			p2(350,450),
			p3(350,300),
			p4(500,450);

	// Stroke Bezier
	SetHighColor(0,255,0);
	serverlink->SetOpCode(GFX_STROKE_BEZIER);
	serverlink->Attach(&p1,sizeof(BPoint));
	serverlink->Attach(&p2,sizeof(BPoint));
	serverlink->Attach(&p3,sizeof(BPoint));
	serverlink->Attach(&p4,sizeof(BPoint));
	serverlink->Attach((pattern *)&B_SOLID_HIGH, sizeof(pattern));
	serverlink->Flush();

	// Fill Bezier
	p1.Set(250,250);
	p2.Set(300,300);
	p3.Set(300,250);
	p4.Set(300,250);
	SetHighColor(255,0,0);
	serverlink->SetOpCode(GFX_FILL_BEZIER);
	serverlink->Attach(&p1,sizeof(BPoint));
	serverlink->Attach(&p2,sizeof(BPoint));
	serverlink->Attach(&p3,sizeof(BPoint));
	serverlink->Attach(&p4,sizeof(BPoint));
	serverlink->Attach((pattern *)&B_SOLID_HIGH, sizeof(pattern));
	serverlink->Flush();

}

void TestApp::TestEllipses(void)
{
	float centerx=200, centery=200,
		radx=10, rady=5;

	// Fill Ellipse
	SetHighColor(0,0,0);
	serverlink->SetOpCode(GFX_FILL_ELLIPSE);
	serverlink->Attach(&centerx,sizeof(float));
	serverlink->Attach(&centery,sizeof(float));
	serverlink->Attach(&radx,sizeof(float));
	serverlink->Attach(&rady,sizeof(float));
	serverlink->Attach((pattern *)&B_SOLID_HIGH, sizeof(pattern));
	serverlink->Flush();

	// Stroke Ellipse
	for(int i=5; i<255; i+=10)
	{
		SetHighColor(i,i,i);
		radx=10+i;
		rady=5+i;
		serverlink->SetOpCode(GFX_STROKE_ELLIPSE);
		serverlink->Attach(&centerx,sizeof(float));
		serverlink->Attach(&centery,sizeof(float));
		serverlink->Attach(&radx,sizeof(float));
		serverlink->Attach(&rady,sizeof(float));
		serverlink->Attach((pattern *)&B_SOLID_HIGH, sizeof(pattern));
		serverlink->Flush();
	}
}

void TestApp::TestFonts(void)
{

	// Can't test SetFont() -- we don't have an active BApplication. :(
	
	// Draw String
	BString string("OpenBeOS: The Future is Drawing Near...");
	uint16 length=string.Length();

	// this will be set to a negative value in the case of not being passed
	// a BPoint
	BPoint pt(10,175);
	
	SetHighColor(0,0,0);
	serverlink->SetOpCode(GFX_DRAW_STRING);

	// String passing protocol for PortLink/AppServer combination:
	// uint16 length of string precedes each string
	serverlink->Attach(&length,sizeof(uint16));
	serverlink->Attach((char *)string.String(),string.Length());
	serverlink->Attach(&pt,sizeof(BPoint));

	// for now, passing escapements is disabled, so none are attached
	serverlink->Flush();
}

void TestApp::TestLines(void)
{
	// Protocol for the usual line method (BPoint,BPoint) will simply
	// result in a MovePenTo call followed by a StrokeLine(BPoint) call

	BPoint pt(100,100);
	float x=50,y=50,size=1;

	// MovePenTo()
	serverlink->SetOpCode(GFX_MOVEPENTO);
	serverlink->Attach(&pt,sizeof(BPoint));
	serverlink->Flush();
	
	// StrokeLine(BPoint)
	pt.Set(10,10);
	SetHighColor(255,0,255);
	serverlink->SetOpCode(GFX_STROKE_LINE);
	serverlink->Attach(&pt,sizeof(BPoint));
	serverlink->Attach((pattern *)&B_SOLID_HIGH, sizeof(pattern));
	serverlink->Flush();
	
	// MovePenBy()
	pt.Set(100,0);
	serverlink->SetOpCode(GFX_MOVEPENBY);
	serverlink->Attach(&x,sizeof(float));
	serverlink->Attach(&y,sizeof(float));
	serverlink->Flush();

	// Eventually, this will become a LineArray test...
	for(int i=0;i<256;i++)
	{	
		// SetPenSize()	
		size=i/10;
		serverlink->SetOpCode(GFX_SETPENSIZE);
		serverlink->Attach(&size,sizeof(float));
		serverlink->Flush();
	
		// StrokeLine(BPoint)
		pt.Set(10,10+i);
		SetHighColor(255,i,255);
		serverlink->SetOpCode(GFX_STROKE_LINE);
		serverlink->Attach(&pt,sizeof(BPoint));
		serverlink->Attach((pattern *)&B_SOLID_HIGH, sizeof(pattern));
		serverlink->Flush();
	}
	// SetPenSize()	
	size=1;
	serverlink->SetOpCode(GFX_SETPENSIZE);
	serverlink->Attach(&size,sizeof(float));
	serverlink->Flush();
}

void TestApp::TestPolygon(void)
{
	// This one's going to REALLY be a pain. Can't be flattened like a BShape.
	// Can't access any of its data like BRegion. I guess it'll have to wait until
	// I've implemented the BView class. I'm starting to get a headache....
}

void TestApp::TestRects(void)
{
	BRect rect(10,270,75,335);
	float xrad=10,yrad=30;

	SetHighColor(255,0,0);

	// Stroke Rect
	serverlink->SetOpCode(GFX_STROKE_RECT);
	serverlink->Attach(&rect,sizeof(BRect));
	serverlink->Attach((pattern *)&B_SOLID_HIGH, sizeof(pattern));
	serverlink->Flush();

	// Fill Rect
	rect.OffsetBy(100,10);
	serverlink->SetOpCode(GFX_FILL_RECT);
	serverlink->Attach(&rect,sizeof(BRect));
	serverlink->Attach((pattern *)&B_SOLID_LOW, sizeof(pattern));
	serverlink->Flush();

	// Stroke Round Rect
	rect.OffsetBy(100,10);

	SetHighColor(0,255,0);
	serverlink->SetOpCode(GFX_STROKE_ROUNDRECT);
	serverlink->Attach(&rect,sizeof(BRect));
	serverlink->Attach(&xrad,sizeof(float));
	serverlink->Attach(&yrad,sizeof(float));
	serverlink->Attach((pattern *)&B_SOLID_HIGH, sizeof(pattern));
	serverlink->Flush();

	// Fill Round Rect
	rect.OffsetBy(100,10);
	SetHighColor(0,255,255);
	serverlink->SetOpCode(GFX_FILL_ROUNDRECT);
	serverlink->Attach(&rect,sizeof(BRect));
	serverlink->Attach(&xrad,sizeof(float));
	serverlink->Attach(&yrad,sizeof(float));
	serverlink->Attach((pattern *)&B_SOLID_HIGH, sizeof(pattern));
	serverlink->Flush();

}

void TestApp::TestRegions(void)
{
/*
Because of the varying size of BRegions, this will be a bit sticky. Size does 
not change with added rectangles because the internally, it uses a BList. We
also must contend with the fact that they very well may not fit into the 
port's buffer, which could have very nasty results.

Possible Solutions:
1) Flatten it like a PortLink flattens data and pass the flattened region to 
	the server via an area. We' can simply clone it and pass the second 
	area_id to the server.
2) Attach it to a BMessage and fire it off.
3) Create an API which operates something like BeginRectArray()
*/

}

void TestApp::TestShape(void)
{
	// BShapes are derived from BArchivable, so we'll flatten it, and fire it to
	// the server via a BMessage. Thus, this will need to be done later...
}

void TestApp::TestTriangle(void)
{
	// In either form of the function, we have to calculate the invaliated 
	// rect which contains the triangle unless the caller gives us one. Thus,
	// the functions will always send a BRect to the server.

	BRect invalid;

	BPoint pt1(600,100), pt2(500,200), pt3(550,450);

	invalid.Set(	MIN( MIN(pt1.x,pt2.x), pt3.x),
					MIN( MIN(pt1.y,pt2.y), pt3.y),
					MAX( MAX(pt1.x,pt2.x), pt3.x),
					MAX( MAX(pt1.y,pt2.y), pt3.y)	);

	// Fill Triangle
	SetHighColor(228,228,50);
	serverlink->SetOpCode(GFX_FILL_TRIANGLE);
	serverlink->Attach(&pt1,sizeof(BPoint));
	serverlink->Attach(&pt2,sizeof(BPoint));
	serverlink->Attach(&pt3,sizeof(BPoint));
	serverlink->Attach(&invalid,sizeof(BRect));
	serverlink->Attach((pattern *)&B_SOLID_HIGH, sizeof(pattern));
	serverlink->Flush();
	
	
	pt1.Set(50,50);
	pt2.Set(200,475);
	pt3.Set(400,400);
	invalid.Set(	MIN( MIN(pt1.x,pt2.x), pt3.x),
					MIN( MIN(pt1.y,pt2.y), pt3.y),
					MAX( MAX(pt1.x,pt2.x), pt3.x),
					MAX( MAX(pt1.y,pt2.y), pt3.y)	);
	// Stroke Triangle
	SetHighColor(0,128,128);
	serverlink->SetOpCode(GFX_STROKE_TRIANGLE);
	serverlink->Attach(&pt1,sizeof(BPoint));
	serverlink->Attach(&pt2,sizeof(BPoint));
	serverlink->Attach(&pt3,sizeof(BPoint));
	serverlink->Attach(&invalid,sizeof(BRect));
	serverlink->Attach((pattern *)&B_SOLID_HIGH, sizeof(pattern));
	serverlink->Flush();
}

void TestApp::TestCursors(void)
{
uint8 cross[] = {16,1,5,5,
14,0,4,0,4,0,4,0,128,32,241,224,128,32,4,0,
4,0,4,0,14,0,0,0,0,0,0,0,0,0,0,0,
14,0,4,0,4,0,4,0,128,32,245,224,128,32,4,0,
4,0,4,0,14,0,0,0,0,0,0,0,0,0,0,0
};

	SetCursor(cross);
	HideCursor();
	ShowCursor();
	ObscureCursor();
}

bool TestApp::Run(void)
{
//	printf("TestApp::Run()\n");
	if(!initialized)
		return false;

	MainLoop();
	return true;
}

int main(void)
{
	TestApp app;
	app.Run();
}