/*
	ServerApp.cpp
		Class which works with a BApplication. Handles all messages coming
		from and going to its application.
*/
//#define DEBUG_SERVERAPP_MSGS
//#define DEBUG_SERVERAPP_CURSORS
//#define DEBUG_SERVERAPP_MISC

#include <AppDefs.h>
#include <Cursor.h>
#include <List.h>
#include <String.h>
#include "Desktop.h"
#include "DisplayDriver.h"
//#include "Layer.h"
#include "ServerApp.h"
#include "ServerWindow.h"
#include "ServerBitmap.h"
#include "ServerProtocol.h"
#include "ServerCursor.h"
#include "PortLink.h"
#include <stdio.h>
#include <iostream.h>
#include <string.h>
#include "DebugTools.h"


ServerApp::ServerApp(port_id msgport, char *signature)
{
	// need to copy the signature because the message buffer
	// owns the copy which we are passed as a parameter.
	app_sig=(signature)?signature:"Application";

	// sender is the monitored app's event port
	sender=msgport;
	applink=new PortLink(sender);

	port_info pinfo;
	get_port_info(sender,&pinfo);
	target_id=pinfo.team;
	
	// receiver is the port to which the app sends messages for the server
	receiver=create_port(30,app_sig.String());

#ifdef DEBUG_SERVERAPP_MISC
printf("ServerApp port for app %s is at %ld\n",app_sig.String(),receiver);
#endif

	if(receiver==B_NO_MORE_PORTS)
	{
		// uh-oh. We have a serious problem. Tell the app to quit
		applink->SetOpCode(B_QUIT_REQUESTED);
		applink->Flush();
	}

	// No longer necessary. This message is sent from the app_server thread
	// now because of changes in the application structure from previous
	// prototypes.
/*	else
	{
		// Everything checks out, so tell the application
		// where to send future messages
		applink->SetOpCode(SET_SERVER_PORT);
		applink->Attach(&receiver,sizeof(port_id));
		applink->Flush();
	}
*/
	applink->SetPort(sender);
	winlist=new BList(0);
	bmplist=new BList(0);
	driver=get_gfxdriver();
	isactive=false;
	cursor=new ServerCursor();

	// This exists for testing the graphics access module and will disappear
	// when layers are implemented
	high.red=255;
	high.green=255;
	high.blue=255;
	high.alpha=255;
	low.red=0;
	low.green=0;
	low.blue=0;
	low.alpha=255;
}

ServerApp::~ServerApp(void)
{
#ifdef DEBUG_SERVERAPP_MISC
printf("%s: ~ServerApp\n",app_sig.String());
#endif
	int32 i;
	ServerWindow *tempwin;
	for(i=0;i<winlist->CountItems();i++)
	{
		tempwin=(ServerWindow*)winlist->ItemAt(i);
		if(tempwin)
			delete tempwin;
	}
	winlist->MakeEmpty();
	delete winlist;

	ServerBitmap *tempbmp;
	for(i=0;i<bmplist->CountItems();i++)
	{
		tempbmp=(ServerBitmap*)bmplist->ItemAt(i);
		if(tempbmp)
			delete tempbmp;
	}
	bmplist->MakeEmpty();
	delete bmplist;

	delete applink;
	applink=NULL;
	delete cursor;

	// Kill the monitor thread if it exists
	thread_info info;
	if(get_thread_info(monitor_thread,&info)==B_OK)
		kill_thread(monitor_thread);
}

bool ServerApp::Run(void)
{
#ifdef DEBUG_SERVERAPP_MISC
printf("%s: Run()\n",app_sig.String());
#endif
	// Unlike a BApplication, a ServerApp is *supposed* to return immediately
	// when its Run function is called.
	monitor_thread=spawn_thread(MonitorApp,app_sig.String(),B_NORMAL_PRIORITY,this);
	if(monitor_thread==B_NO_MORE_THREADS || monitor_thread==B_NO_MEMORY)
		return false;
	resume_thread(monitor_thread);
	return true;
}

bool ServerApp::PingTarget(void)
{
	// This function is called by the app_server thread to ensure that
	// the target app still exists. We do this not by sending a message
	// but by calling get_port_info. We don't want to send ping messages
	// just because the app might simply be hung. If this is the case, it
	// should be up to the user to kill it. If the app has been killed, its
	// ports will be invalid. Thus, if get_port_info returns an error, we
	// tell the app_server to delete the respective ServerApp.
	
	team_info tinfo;
	if(get_team_info(target_id,&tinfo)==B_BAD_TEAM_ID)
	{
		port_id serverport=find_port(SERVER_PORT_NAME);
		if(serverport==B_NAME_NOT_FOUND)
		{
			printf("PANIC: ServerApp %s could not find the app_server port in PingTarget()!\n",app_sig.String());
			return false;
		}
		applink->SetPort(serverport);
		applink->SetOpCode(DELETE_APP);
		applink->Attach(&monitor_thread,sizeof(thread_id));
		applink->Flush();
#ifdef DEBUG_SERVERAPP_MISC
printf("%s: target application is dead. ServerApp is quitting\n",app_sig.String());
#endif
		return false;
	}
	return true;
}

int32 ServerApp::MonitorApp(void *data)
{
	ServerApp *app=(ServerApp *)data;
	app->Loop();
	exit_thread(0);
	return 0;
}

void ServerApp::Loop(void)
{
#ifdef DEBUG_SERVERAPP_MISC
printf("%s: Loop()\n",app_sig.String());
#endif
	// Message-dispatching loop for the ServerApp
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
// -------------- Messages received from the Application ------------------
				case B_QUIT_REQUESTED:
				{
					// Our BApplication sent us this message when it quit.
					// We need to ask the app_server to delete our monitor
					port_id serverport=find_port(SERVER_PORT_NAME);
					if(serverport==B_NAME_NOT_FOUND)
					{
						printf("PANIC: ServerApp %s could not find the app_server port!\n",app_sig.String());
						break;
					}
					applink->SetPort(serverport);
					applink->SetOpCode(DELETE_APP);
					applink->Attach(&monitor_thread,sizeof(thread_id));
					applink->Flush();
					break;
				}

// -------------- Messages received from the Server ------------------------
				// Mouse messages simply get passed onto the active app for now
				case B_MOUSE_UP:
				case B_MOUSE_DOWN:
				case B_MOUSE_MOVED:
				{
					// everything is formatted as it should be, so just call
					// write_port. Eventually, this will be replaced by a
					// BMessage
					write_port(sender, msgcode, msgbuffer, buffersize);
					break;
				}
				case QUIT_APP:
				{
					// This message is received from the app_server thread
					// because the server was asked to quit. Thus, we
					// ask all apps to quit. This is NOT the same as system
					// shutdown and will happen only in testing
										
#ifdef DEBUG_SERVERAPP_MSGS
printf("ServerApp asking %s to quit\n",app_sig.String());
#endif
					applink->SetOpCode(B_QUIT_REQUESTED);
					applink->Flush();
					break;
				}
				default:
				{

#ifdef DEBUG_SERVERAPP_MSGS
printf("ServerApp received unrecognized code: ");
PrintMessageCode(msgcode);
#endif

					DispatchMessage(msgcode, msgbuffer);
					break;
				}
			}

		}
	
		if(buffersize>0)
			delete msgbuffer;

		if(msgcode==B_QUIT_REQUESTED)
			break;
	}
}

void ServerApp::DispatchMessage(int32 code, int8 *buffer)
{
#ifdef DEBUG_SERVERAPP_MSGS
printf("ServerApp ");
PrintMessageCode(code);
#endif

int8 *index=buffer;
	switch(code)
	{
		case CREATE_WINDOW:
		{
			// Create the ServerWindow to node monitor a new OBWindow
			
			// Attached data:
			// 1) port_id reply port
			// 2) BRect window frame
			// 3) uint32 window flags
			// 4) port_id window's message port
			// 5) uint32 workspace index
			// 6) const char * title

			// Find the necessary data
			port_id reply_port=*((port_id*)index); index+=sizeof(port_id);
			BRect rect=*((BRect*)index); index+=sizeof(BRect);

			uint32 winlook=*((uint32*)index); index+=sizeof(uint32);
			uint32 winfeel=*((uint32*)index); index+=sizeof(uint32);
			uint32 winflags=*((uint32*)index); index+=sizeof(uint32);

			port_id win_port=*((port_id*)index); index+=sizeof(port_id);
			uint32 workspace=*((uint32*)index); index+=sizeof(uint32);

			// Create the ServerWindow object for this window
			ServerWindow *newwin=new ServerWindow(rect,(const char *)index,
				winlook, winfeel, winflags,this,win_port,workspace);
			winlist->AddItem(newwin);

			// Window looper is waiting for our reply. Send back the
			// ServerWindow's message port
			PortLink *replylink=new PortLink(reply_port);
			replylink->SetOpCode(SET_SERVER_PORT);
			replylink->Attach((int32)newwin->receiver);
			replylink->Flush();

			delete replylink;

#ifdef DEBUG_SERVERAPP_MSGS
printf("ServerApp: Create Window()\n");
printf("Frame: "); rect.PrintToStream();
printf("Title: %s\n",(const char*)index);
printf("SenderPort: %ld\n",win_port);
printf("Look: 0x%lx\n",winlook);
printf("Feel: 0x%lx\n",winfeel);
printf("Flags: 0x%lx\n",winflags);
printf("Workspace: 0x%lx\n",workspace);
#endif

			break;

		}
		case DELETE_WINDOW:
		{
			// Received from a ServerWindow when its window quits
			
			// Attached data:
			// 1) thread_id  ServerWindow ID
			thread_id winid;
			ServerWindow *w;
			for(int32 i=0;i<winlist->CountItems();i++)
			{
				w=(ServerWindow*)winlist->ItemAt(i);
				if(w->thread==winid)
				{
					winlist->RemoveItem(w);
					delete w;
					break;
				}
			}
#ifdef DEBUG_SERVERAPP_MSGS
printf("ServerApp: Delete Window()\n");
printf("Thread ID: %ld",winid);
#endif
			break;
		}
		case GFX_SET_SCREEN_MODE:
		{
			// Attached data
			// 1) int32 workspace #
			// 2) uint32 screen mode
			// 3) bool make default
			int32 workspace=*((int32*)index); index+=sizeof(int32);
			uint32 mode=*((uint32*)index); index+=sizeof(uint32);

			SetScreenSpace(workspace,mode,*((bool*)index));
#ifdef DEBUG_SERVERAPP_MSGS
printf("ServerApp: SetScreenMode(%ld,%lu,%s)\n",workspace,
			mode,(*((bool*)index)==true)?"true":"false");
#endif
			break;
		}
		case GFX_ACTIVATE_WORKSPACE:
		{
			// Attached data
			// 1) int32 workspace index
			ActivateWorkspace(*((int32*)index));
#ifdef DEBUG_SERVERAPP_MSGS
printf("ServerApp: ActivateWorkspace(%ld)\n",*((int32*)index));
#endif
			break;
		}
		case SHOW_CURSOR:
		{
			driver->ShowCursor();
			break;
		}
		case HIDE_CURSOR:
		{
			driver->HideCursor();
			break;
		}
		case OBSCURE_CURSOR:
		{
			driver->ObscureCursor();
			break;
		}
		case SET_CURSOR_DATA:
		{
			// Attached data: 68 bytes of cursor data
			
			// Get the data, update the app's cursor, and update the
			// app's cursor if active.
			int8 cdata[68];
			memcpy(cdata, buffer, 68);
			cursor->SetCursor(cdata);
			driver->SetCursor(cursor);
#ifdef DEBUG_SERVERAPP_CURSOR
printf("ServerApp: SetCursor(%d,%d,%d,%d)\n",r,g,b,a);
#endif
			break;
		}
		case SET_CURSOR_BCURSOR:
			// Not yet implemented
			break;
			
		case SET_CURSOR_BBITMAP:
			// Not yet implemented
			break;

		// Graphics messages
		case GFX_SET_HIGH_COLOR:
		{
			// Attached data:
			// 1) uint8 red
			// 2) uint8 green
			// 3) uint8 blue
			// 4) uint8 alpha
			
			uint8 r,g,b,a;
			r=*((uint8*)index); index+=sizeof(uint8);
			g=*((uint8*)index); index+=sizeof(uint8);
			b=*((uint8*)index); index+=sizeof(uint8);
			a=*((uint8*)index);

#ifdef DEBUG_SERVERAPP_MSGS
printf("ServerApp: SetHighColor(%d,%d,%d,%d)\n",r,g,b,a);
#endif
			driver->SetHighColor(r,g,b,a);
			break;
		}
		case GFX_SET_LOW_COLOR:
		{
			// Attached data:
			// 1) uint8 red
			// 2) uint8 green
			// 3) uint8 blue
			// 4) uint8 alpha
			
			uint8 r,g,b,a;
			r=*((uint8*)index); index+=sizeof(uint8);
			g=*((uint8*)index); index+=sizeof(uint8);
			b=*((uint8*)index); index+=sizeof(uint8);
			a=*((uint8*)index);
#ifdef DEBUG_SERVERAPP_MSGS
printf("ServerApp: SetHighColor(%d,%d,%d,%d)\n",r,g,b,a);
#endif
			driver->SetLowColor(r,g,b,a);
			break;
		}
		case GFX_MOVEPENBY:
		{
			// Attached data:
			// 1) BPoint pt
			
			// Default functionality is MoveTo, so get the current position
			// from the driver and compensate
			BPoint *ptindex,passed_pt;

			ptindex=(BPoint*)index;
			passed_pt=driver->PenPosition();
			passed_pt+=*ptindex;
#ifdef DEBUG_SERVERAPP_MSGS
printf("ServerApp MovePenBy: (%f,%f)\n",ptindex->x,ptindex->y);
printf("Pen moved to (%f,%f)\n",passed_pt.x,passed_pt.y);
#endif
			driver->MovePenTo(passed_pt);
			break;
		}
		case GFX_MOVEPENTO:
		{
			// Attached data:
			// 1) BPoint pt
			BPoint *pt;
			pt=(BPoint*)index;
#ifdef DEBUG_SERVERAPP_MSGS
printf("ServerApp MovePenTo: (%f,%f)\n",pt->x,pt->y);
#endif
			driver->MovePenTo(*pt);
			break;
		}
		case GFX_SETPENSIZE:
		{
			// Attached data:
			// 1) float pen size
#ifdef DEBUG_SERVERAPP_MSGS
printf("ServerApp SetPenSize(%f)\n",*((float*)index));
#endif
			driver->SetPenSize(*((float*)index));
			break;
		}
		case GFX_DRAW_STRING:
		{
			// Attached data:
			// 1) uint16 string length
			// 2) char * string
			// 3) BPoint point

			uint16 length=*((uint16*)index); index+=sizeof(uint16);
//			char *string=new char[length];
//			strcpy(string, (const char *)index); index+=length;
			BString string=(char *)index; index+=length;
			BPoint point=*((BPoint*)index);

#ifdef DEBUG_SERVERAPP_MSGS
printf("ServerApp DrawString(%s, %d, BPoint(%f,%f))\n",string.String(),length,point.x,point.y);
#endif
			driver->DrawString((char*)string.String(), length, point);
//			delete string;
			break;
		}
		case GFX_SET_FONT_SIZE:
			break;
		case GFX_STROKE_ARC:
		{
			// Attached data:
			// 1) float center x
			// 2) float center y
			// 3) float x radius
			// 4) float y radius
			// 5) float starting angle
			// 6) float span (arc length in degrees)
			// 7) uint8[8] pattern
			float cx,cy,rx,ry,angle,span;
			
			cx=*((float*)index); index+=sizeof(float);
			cy=*((float*)index); index+=sizeof(float);
			rx=*((float*)index); index+=sizeof(float);
			ry=*((float*)index); index+=sizeof(float);
			angle=*((float*)index); index+=sizeof(float);
			span=*((float*)index); index+=sizeof(float);
#ifdef DEBUG_SERVERAPP_MSGS
printf("ServerApp StrokeArc: ");
printf("Center: %f,%f\n",cx,cy);
printf("Radii: %f,%f\n",rx,ry);
printf("Angle, Span: %f,%f\n",angle, span);
printf("pattern: {%d,%d,%d,%d,%d,%d,%d,%d}\n",index[0],index[1],index[2],index[3],index[4],index[5],index[6],index[7]);
#endif
			driver->StrokeArc(cx,cy,rx,ry,angle,span,(uint8*)index);
			break;
		}
		case GFX_STROKE_BEZIER:
		{
			// Attached data:
			// 1) BPoint #1
			// 2) BPoint #2
			// 3) BPoint #3
			// 4) BPoint #4
			// 5) uint8[8] pattern
			
#ifdef DEBUG_SERVERAPP_MSGS
BPoint *pt=(BPoint*)index;
printf("ServerApp StrokeBezier:\n");
printf("Point 1: "); pt[0].PrintToStream();
printf("Point 2: "); pt[1].PrintToStream();
printf("Point 3: "); pt[2].PrintToStream();
printf("Point 4: "); pt[3].PrintToStream();
printf("pattern: {%d,%d,%d,%d,%d,%d,%d,%d}\n",index[0],index[1],index[2],index[3],index[4],index[5],index[6],index[7]);
#endif
			driver->StrokeBezier((BPoint*)index,(uint8*)(index+(sizeof(BPoint)*4)));
			break;
		}
		case GFX_STROKE_ELLIPSE:
		{
			// Attached data:
			// 1) float center x
			// 2) float center y
			// 3) float x radius
			// 4) float y radius
			// 5) uint8[8] pattern
			float cx,cy,rx,ry;
			
			cx=*((float*)index); index+=sizeof(float);
			cy=*((float*)index); index+=sizeof(float);
			rx=*((float*)index); index+=sizeof(float);
			ry=*((float*)index); index+=sizeof(float);
#ifdef DEBUG_SERVERAPP_MSGS
printf("ServerApp StrokeEllipse: ");
printf("Center: %f,%f\n",cx,cy);
printf("Radii: %f,%f\n",rx,ry);
printf("pattern: {%d,%d,%d,%d,%d,%d,%d,%d}\n",index[0],index[1],index[2],index[3],index[4],index[5],index[6],index[7]);
#endif
			driver->StrokeEllipse(cx,cy,rx,ry,(uint8*)index);
			break;
		}
		case GFX_STROKE_LINE:
		{
			// Attached data:
			// 1) BPoint endpoint
			// 2) uint8[8] pattern
			BPoint *pt;
			pt=(BPoint*)index;	index+=sizeof(BPoint);
			
#ifdef DEBUG_SERVERAPP_MSGS
printf("ServerApp StrokeLineTo: (%f,%f)\n",pt->x,pt->y);
printf("pattern: {%d,%d,%d,%d,%d,%d,%d,%d}\n",index[0],index[1],index[2],index[3],index[4],index[5],index[6],index[7]);
#endif
			driver->StrokeLine(*pt,(uint8*)index);
			break;
		}
		case GFX_STROKE_POLYGON:
			break;
		case GFX_STROKE_RECT:
		{
			// Attached data:
			// 1) BRect rect
			// 2) uint8[8] pattern
			BRect rect;
			rect=*((BRect*)index); index+=sizeof(BRect);
#ifdef DEBUG_SERVERAPP_MSGS
printf("ServerApp StrokeRect: "); rect.PrintToStream();
printf("pattern: {%d,%d,%d,%d,%d,%d,%d,%d}\n",index[0],index[1],index[2],index[3],index[4],index[5],index[6],index[7]);
#endif
			driver->StrokeRect(rect, (uint8*)index);
			break;
		}
		case GFX_STROKE_ROUNDRECT:
		{
			// Attached data:
			// 1) BRect rect
			// 2) float x_radius
			// 3) float y_radius
			// 4) uint8[8] pattern
			BRect rect;
			float x,y;
			
			rect=*((BRect*)index); index+=sizeof(BRect);
			x=*((float*)index); index+=sizeof(float);
			y=*((float*)index); index+=sizeof(float);
#ifdef DEBUG_SERVERAPP_MSGS
printf("ServerApp StrokeRoundRect: "); rect.PrintToStream();
printf("Radii: %f,%f\n",x,y);
printf("pattern: {%d,%d,%d,%d,%d,%d,%d,%d}\n",index[0],index[1],index[2],index[3],index[4],index[5],index[6],index[7]);
#endif
			driver->StrokeRoundRect(rect, x,y,(uint8*)index);
			break;
		}
		case GFX_STROKE_SHAPE:
			break;
		case GFX_STROKE_TRIANGLE:
		{
			// Attached data
			// 1) BPoint #1
			// 2) BPoint #2
			// 3) BPoint #3
			// 4) BRect invalid rect
			// 5) uint8[8] pattern
			BPoint pt1,pt2,pt3;
			BRect rect;
			pt1=*((BPoint*)index); index+=sizeof(BPoint);
			pt2=*((BPoint*)index); index+=sizeof(BPoint);
			pt3=*((BPoint*)index); index+=sizeof(BPoint);
			rect=*((BRect*)index); index+=sizeof(BRect);
#ifdef DEBUG_SERVERAPP_MSGS
printf("ServerApp StrokeTriangle: ");
printf("Point 1: "); pt1.PrintToStream();
printf("Point 2: "); pt2.PrintToStream();
printf("Point 3: "); pt3.PrintToStream();
printf("Rectangle: "); rect.PrintToStream();
printf("pattern: {%d,%d,%d,%d,%d,%d,%d,%d}\n",index[0],index[1],index[2],index[3],index[4],index[5],index[6],index[7]);
#endif
			driver->StrokeTriangle(pt1,pt2,pt3,rect,(uint8*)index);
			break;
		}
		case GFX_FILL_ARC:
		{
			// Attached data:
			// 1) float center x
			// 2) float center y
			// 3) float x radius
			// 4) float y radius
			// 5) float starting angle
			// 6) float span (arc length in degrees)
			// 7) uint8[8] pattern
			float cx,cy,rx,ry,angle,span;
			
			cx=*((float*)index); index+=sizeof(float);
			cy=*((float*)index); index+=sizeof(float);
			rx=*((float*)index); index+=sizeof(float);
			ry=*((float*)index); index+=sizeof(float);
			angle=*((float*)index); index+=sizeof(float);
			span=*((float*)index); index+=sizeof(float);
#ifdef DEBUG_SERVERAPP_MSGS
printf("ServerApp FillArc: ");
printf("Center: %f,%f\n",cx,cy);
printf("Radii: %f,%f\n",rx,ry);
printf("Angle, Span: %f,%f\n",angle, span);
printf("pattern: {%d,%d,%d,%d,%d,%d,%d,%d}\n",index[0],index[1],index[2],index[3],index[4],index[5],index[6],index[7]);
#endif
			driver->FillArc(cx,cy,rx,ry,angle,span,(uint8*)index);
			break;
		}
		case GFX_FILL_BEZIER:
		{
			// Attached data:
			// 1) BPoint #1
			// 2) BPoint #2
			// 3) BPoint #3
			// 4) BPoint #4
			// 5) uint8[8] pattern
			
#ifdef DEBUG_SERVERAPP_MSGS
BPoint *pt=(BPoint*)index;
printf("ServerApp FillBezier:\n");
printf("Point 1: "); pt[0].PrintToStream();
printf("Point 2: "); pt[1].PrintToStream();
printf("Point 3: "); pt[2].PrintToStream();
printf("Point 4: "); pt[3].PrintToStream();
printf("pattern: {%d,%d,%d,%d,%d,%d,%d,%d}\n",index[0],index[1],index[2],index[3],index[4],index[5],index[6],index[7]);
#endif
			driver->FillBezier((BPoint*)index,(uint8*)(index+(sizeof(BPoint)*4)));
			break;
		}
		case GFX_FILL_ELLIPSE:
		{
			// Attached data:
			// 1) float center x
			// 2) float center y
			// 3) float x radius
			// 4) float y radius
			// 5) uint8[8] pattern
			float cx,cy,rx,ry;
			
			cx=*((float*)index); index+=sizeof(float);
			cy=*((float*)index); index+=sizeof(float);
			rx=*((float*)index); index+=sizeof(float);
			ry=*((float*)index); index+=sizeof(float);
#ifdef DEBUG_SERVERAPP_MSGS
printf("ServerApp FillEllipse: ");
printf("Center: %f,%f\n",cx,cy);
printf("Radii: %f,%f\n",rx,ry);
printf("pattern: {%d,%d,%d,%d,%d,%d,%d,%d}\n",index[0],index[1],index[2],index[3],index[4],index[5],index[6],index[7]);
#endif
			driver->FillEllipse(cx,cy,rx,ry,(uint8*)index);
			break;
		}
		case GFX_FILL_POLYGON:
			break;
		case GFX_FILL_RECT:
		{
			// Attached data:
			// 1) BRect rect
			// 2) uint8[8] pattern
			BRect rect;
			rect=*((BRect*)index); index+=sizeof(BRect);
#ifdef DEBUG_SERVERAPP_MSGS
printf("ServerApp FillRect: "); rect.PrintToStream();
printf("pattern: {%d,%d,%d,%d,%d,%d,%d,%d}\n",index[0],index[1],index[2],index[3],index[4],index[5],index[6],index[7]);
#endif
			driver->FillRect(rect, (uint8*)index);
			break;
		}
		case GFX_FILL_REGION:
			break;
		case GFX_FILL_ROUNDRECT:
		{
			// Attached data:
			// 1) BRect rect
			// 2) float x_radius
			// 3) float y_radius
			// 4) uint8[8] pattern
			BRect rect;
			float x,y;
			
			rect=*((BRect*)index); index+=sizeof(BRect);
			x=*((float*)index); index+=sizeof(float);
			y=*((float*)index); index+=sizeof(float);
#ifdef DEBUG_SERVERAPP_MSGS
printf("ServerApp FillRoundRect: "); rect.PrintToStream();
printf("Radii: %f,%f\n",x,y);
printf("pattern: {%d,%d,%d,%d,%d,%d,%d,%d}\n",index[0],index[1],index[2],index[3],index[4],index[5],index[6],index[7]);
#endif
			driver->FillRoundRect(rect, x,y,(uint8*)index);
			break;
		}
		case GFX_FILL_SHAPE:
			break;
		case GFX_FILL_TRIANGLE:
		{
			// Attached data
			// 1) BPoint #1
			// 2) BPoint #2
			// 3) BPoint #3
			// 4) BRect invalid rect
			// 5) uint8[8] pattern
			BPoint pt1,pt2,pt3;
			BRect rect;
			pt1=*((BPoint*)index); index+=sizeof(BPoint);
			pt2=*((BPoint*)index); index+=sizeof(BPoint);
			pt3=*((BPoint*)index); index+=sizeof(BPoint);
			rect=*((BRect*)index); index+=sizeof(BRect);
#ifdef DEBUG_SERVERAPP_MSGS
printf("ServerApp FillTriangle: ");
printf("Point 1: "); pt1.PrintToStream();
printf("Point 2: "); pt2.PrintToStream();
printf("Point 3: "); pt3.PrintToStream();
printf("Rectangle: "); rect.PrintToStream();
printf("pattern: {%d,%d,%d,%d,%d,%d,%d,%d}\n",index[0],index[1],index[2],index[3],index[4],index[5],index[6],index[7]);
#endif
			driver->FillTriangle(pt1,pt2,pt3,rect,(uint8*)index);
			break;
		}
		default:
		{
#ifdef DEBUG_SERVERAPP_MSGS
cout << "DispatchMessage::Unrecognized code: ";
TranslateMessageCodeToStream(code);
cout << endl << flush;
#endif
			break;
		}
	}
}

// Prevent any preprocessor problems in case I screw something up
#ifdef DEBUG_SERVERAPP_MSGS
#undef DEBUG_SERVERAPP_MSGS
#endif

#ifdef DEBUG_SERVERAPP_CURSORS
#undef DEBUG_SERVERAPP_CURSORS
#endif
