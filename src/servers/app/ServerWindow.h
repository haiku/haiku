/*
 * Copyright 2001-2005, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		DarkWyrm <bpmagic@columbus.rr.com>
 *		Adrian Oanca <adioanca@gmail.com>
 *		Stephan Aßmus <superstippi@gmx.de>
 *		Stefano Ceccherini (burton666@libero.it)
 *		Axel Dörfler, axeld@pinc-software.de
 */
#ifndef SERVER_WINDOW_H
#define SERVER_WINDOW_H


#include "EventDispatcher.h"
#include "MessageLooper.h"

#include <PortLink.h>
#include <TokenSpace.h>

#include <GraphicsDefs.h>
#include <Locker.h>
#include <Message.h>
#include <Messenger.h>
#include <Rect.h>
#include <String.h>
#include <Window.h>

class BString;
class BMessenger;
class BPoint;
class BMessage;

class Desktop;
class ServerApp;
class Decorator;
class WindowLayer;
class Workspace;
class RootLayer;
class Layer;
class ServerPicture;
struct dw_data;
struct window_info;

#define AS_UPDATE_DECORATOR 'asud'
#define AS_UPDATE_COLORS 'asuc'
#define AS_UPDATE_FONTS 'asuf'

class ServerWindow : public MessageLooper {
public:
								ServerWindow(const char *title, ServerApp *app,
									port_id clientPort, port_id looperPort, 
									int32 clientToken);
	virtual						~ServerWindow();

			status_t			Init(BRect frame, window_look look,
									window_feel feel, uint32 flags,
									uint32 workspace);
	virtual bool				Run();
	virtual port_id				MessagePort() const { return fMessagePort; }

			::EventTarget&		EventTarget() { return fEventTarget; }

			void				ReplaceDecorator();
			void				Show();
			void				Hide();

			// methods for sending various messages to client.
			void				NotifyQuitRequested();
			void				NotifyMinimize(bool minimize);
			void				NotifyZoom();

			// util methods.
			const BMessenger&	FocusMessenger() const { return fFocusMessenger; }
			const BMessenger&	HandlerMessenger() const { return fHandlerMessenger; }

			status_t			SendMessageToClient(const BMessage* msg,
									int32 target = B_NULL_TOKEN) const;

	virtual	WindowLayer*		MakeWindowLayer(BRect frame, const char* name,
									window_look look, window_feel feel, uint32 flags,
									uint32 workspace);

			// TODO: Ouch, that's not exactly a nice name
			inline BMessage		&ClientViewsWithInvalidCoords()
									{ return fClientViewsWithInvalidCoords; };
		
			// to who we belong. who do we own. our title.
	inline	ServerApp*			App() const { return fServerApp; }
			::Desktop*			Desktop() const { return fDesktop; }
	inline	const WindowLayer*	GetWindowLayer() const { return fWindowLayer; }

			void				SetTitle(const char* newTitle);
	inline	const char*			Title() const { return fTitle; }

			// related thread/team_id(s).
	inline	team_id				ClientTeam() const { return fClientTeam; }
			
			void				HandleDirectConnection(int bufferState = -1, int driverState = -1);

	inline	int32				ClientToken() const { return fClientToken; }
	inline	int32				ServerToken() const { return fServerToken; }

			void				GetInfo(window_info& info);

private:
			// methods for retrieving and creating a tree strcture of Layers.
			Layer*				CreateLayerTree(BPrivate::LinkReceiver &link, Layer **_parent);
			void				SetLayerState(Layer *layer, BPrivate::LinkReceiver &link);
			void				SetLayerFontState(Layer *layer, BPrivate::LinkReceiver &link);

			// message handling methods.
			void				_DispatchMessage(int32 code, BPrivate::LinkReceiver &link);
			void				_DispatchGraphicsMessage(int32 code, BPrivate::LinkReceiver &link);
			void				_MessageLooper();
	virtual void				_PrepareQuit();
	virtual void				_GetLooperName(char* name, size_t size);

			status_t			_EnableDirectWindowMode();
			
			// TODO: Move me elsewhere
			status_t			PictureToRegion(ServerPicture *picture,
												BRegion &,
												bool inverse,
												BPoint where);

private:
			char*				fTitle;

			::Desktop*			fDesktop;
			ServerApp*			fServerApp;
			WindowLayer*		fWindowLayer;

			team_id				fClientTeam;

			port_id				fMessagePort;
			port_id				fClientReplyPort;
			port_id				fClientLooperPort;
			BMessenger			fFocusMessenger;
			BMessenger			fHandlerMessenger;
			::EventTarget		fEventTarget;

			BMessage			fClientViewsWithInvalidCoords;

			int32				fServerToken;
			int32				fClientToken;

			Layer*				fCurrentLayer;
			
			dw_data*			fDirectWindowData;
};

#endif	// SERVER_WINDOW_H
