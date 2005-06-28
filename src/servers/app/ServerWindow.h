/*
 * Copyright 2001-2005, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		DarkWyrm <bpmagic@columbus.rr.com>
 *		Adrian Oanca <adioanca@cotty.iren.ro>
 *		Stephan Aßmus <superstippi@gmx.de>
 *		Stefano Ceccherini (burton666@libero.it)
 *		Axel Dörfler, axeld@pinc-software.de
 */
#ifndef _SERVERWIN_H_
#define _SERVERWIN_H_


#include <GraphicsDefs.h>
#include <PortLink.h>
#include <Locker.h>
#include <Message.h>
#include <OS.h>
#include <Rect.h>
#include <String.h>
#include <Window.h>

#include "SubWindowList.h"
#include "TokenSpace.h"

class BString;
class BMessenger;
class BPoint;
class BMessage;

class ServerApp;
class Decorator;
class WinBorder;
class Workspace;
class RootLayer;
class Layer;
class ServerPicture;

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
class ServerWindow : public BLocker {
public:
								ServerWindow(const char *title, ServerApp *app,
									port_id clientPort, port_id looperPort, 
									int32 handlerID, BRect frame, uint32 look,
									uint32 feel, uint32 flags, uint32 workspace);
	virtual						~ServerWindow();

			status_t			InitCheck();
			bool				Run();
			void				Quit();

			void				ReplaceDecorator();
			void				Show();
			void				Hide();

			void				PostMessage(int32 code);

			// methods for sending various messages to client.
			void				NotifyQuitRequested();
			void				NotifyMinimize(bool minimize);
			void				NotifyZoom();
			void				NotifyScreenModeChanged(const BRect frame,
									const color_space cspace);

			// util methods.	
			void				SendMessageToClient(const BMessage* msg,
													int32 target = B_NULL_TOKEN,
													bool usePreferred = false) const;
			
			// TODO: Ouch, that's not exactly a nice name
			inline BMessage		&ClientViewsWithInvalidCoords()
									{ return fClientViewsWithInvalidCoords; };
		
			// to who we belong. who do we own. our title.
	inline	ServerApp*			App() const { return fServerApp; }
	inline	const WinBorder*	GetWinBorder() const { return fWinBorder; }
	inline	const char*			Title() const { return fTitle; }

			// related thread/team_id(s).
	inline	team_id				ClientTeam() const { return fClientTeam; }
	inline	thread_id			Thread() const { return fThread; }

			// server "private" - try not to use.
	inline	int32				ClientToken() const { return fHandlerToken; }

			// ToDo: public??
			SubWindowList	fSubWindowList;

private:
			// methods for retrieving and creating a tree strcture of Layers.
			Layer*				CreateLayerTree(BPrivate::LinkReceiver &link, Layer **_parent);
			void				SetLayerState(Layer *layer, BPrivate::LinkReceiver &link);
			void				SetLayerFontState(Layer *layer, BPrivate::LinkReceiver &link);

			// message handling methods.
			void				_DispatchMessage(int32 code, BPrivate::LinkReceiver &link);
			void				_DispatchGraphicsMessage(int32 code, BPrivate::LinkReceiver &link);
			void				_MessageLooper();

	static	int32				_message_thread(void *_window);

			// TODO: Move me elsewhere
			status_t			PictureToRegion(ServerPicture *picture,
												BRegion &,
												bool inverse,
												BPoint where);

private:
			const char*			fTitle;

			ServerApp*			fServerApp;
			WinBorder*			fWinBorder;

			team_id				fClientTeam;
			thread_id			fThread;

			port_id				fMessagePort;
			port_id				fClientReplyPort;
			port_id				fClientLooperPort;

			BPrivate::PortLink	fLink;
			bool				fQuitting;

			BMessage			fClientViewsWithInvalidCoords;

			int32				fHandlerToken;

			Layer*				fCurrentLayer;
};

#endif	// _SERVERWIN_H_
