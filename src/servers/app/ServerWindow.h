//------------------------------------------------------------------------------
//	Copyright (c) 2001-2002, Haiku, Inc.
//
//	Permission is hereby granted, free of charge, to any person obtaining a
//	copy of this software and associated documentation files (the "Software"),
//	to deal in the Software without restriction, including without limitation
//	the rights to use, copy, modify, merge, publish, distribute, sublicense,
//	and/or sell copies of the Software, and to permit persons to whom the
//	Software is furnished to do so, subject to the following conditions:
//
//	The above copyright notice and this permission notice shall be included in
//	all copies or substantial portions of the Software.
//
//	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
//	FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
//	DEALINGS IN THE SOFTWARE.
//
//	File Name:		ServerWindow.h
//	Author:			DarkWyrm <bpmagic@columbus.rr.com>
//	Description:	Shadow BWindow class
//  
//------------------------------------------------------------------------------
#ifndef _SERVERWIN_H_
#define _SERVERWIN_H_

#include <SupportDefs.h>
#include <GraphicsDefs.h>
#include <OS.h>
#include <Locker.h>
#include <Rect.h>
#include <String.h>
#include <Window.h>
#include <LinkMsgReader.h>
#include <LinkMsgSender.h>
#include "TokenSpace.h"
#include "FMWList.h"

class BString;
class BMessenger;
class BPoint;
class BMessage;
class ServerApp;
class Decorator;
class BPortLink;
class WinBorder;
class Workspace;
class RootLayer;
class Layer;

#define AS_UPDATE_DECORATOR 'asud'
#define AS_UPDATE_COLORS 'asuc'
#define AS_UPDATE_FONTS 'asuf'

/*!
	\class ServerWindow ServerWindow.h
	\brief Shadow BWindow class
	
	A ServerWindow handles all the intraserver tasks required of it by its BWindow. There are 
	too many tasks to list as being done by them, but they include handling View transactions, 
	coordinating and linking a window's WinBorder half with its messaging half, dispatching 
	mouse and key events from the server to its window, and other such things.
*/
class ServerWindow
{
public:
								ServerWindow(	const char *string,
												ServerApp *winapp,
												port_id winport,
												port_id looperPort,
												int32 handlerID);
	virtual						~ServerWindow(void);
	
			void				Init(	BRect frame,
										uint32 wlook, uint32 wfeel, uint32 wflags,
										uint32 wwksindex);

			void				ReplaceDecorator(void);
			void				Show(void);
			void				Hide(void);

			status_t			Lock(void);
			void				Unlock(void);
			bool				IsLocked(void) const;

			// methods for sending various messages to client.
			void				Quit(void);
			void				Minimize(bool status);
			void				Zoom(void);
			void				ScreenModeChanged(const BRect frame, const color_space cspace);
	
			// util methods.	
			void				SendMessageToClient(const BMessage* msg,
													int32 target = B_NULL_TOKEN,
													bool usePreferred = false) const;

			// to who we belong. who do we own. our title.
	inline	ServerApp*			App(void) const { return fServerApp; }
	inline	const WinBorder*	GetWinBorder(void) const { return fWinBorder; }
	inline	const char*			Title(void) const { return fName; }

			// related thread/team_id(s).
	inline	team_id				ClientTeamID(void) const { return fClientTeamID; }
	inline	thread_id			ThreadID(void) const { return fMonitorThreadID;}

			// server "private" - try not to use.
	inline	int32				ClientToken(void) const { return fHandlerToken; }
	
			FMWList fWinFMWList;

private:
			// methods for retrieving and creating a tree strcture of Layers.
			Layer*				CreateLayerTree(Layer *localRoot, LinkMsgReader &link);
			void				SetLayerState(Layer *layer, LinkMsgReader &link);
			void				SetLayerFontState(Layer *layer, LinkMsgReader &link);

			// message handle methods.
			void				DispatchMessage(int32 code, LinkMsgReader &link);
			void				DispatchGraphicsMessage(int32 code, LinkMsgReader &link);
	static	int32				MonitorWin(void *data);

			// used by CopyBits and Scrolling
			void				_CopyBits(RootLayer* rootLayer,
										  Layer* layer,
										  BRect& copy,
										  BRect& dirty,
										  int32 xOffset, int32 yOffset) const;

protected:	
	friend class ServerApp;
	friend class WinBorder;
	friend class Screen; 
	friend class Layer;
	
			char				fName[50];
	
			ServerApp*			fServerApp;
			WinBorder*			fWinBorder;
	
			team_id				fClientTeamID;
			thread_id			fMonitorThreadID;
	
			port_id				fMessagePort;
			port_id				fClientWinPort;
			port_id				fClientLooperPort;
	
			LinkMsgReader*		fMsgReader;
			LinkMsgSender*		fMsgSender;

			BLocker				fLocker;

			int32				fHandlerToken;

			// cl is short for currentLayer. We'll use it a lot, that's why it's short :-)
			Layer*				cl;
};

#endif
