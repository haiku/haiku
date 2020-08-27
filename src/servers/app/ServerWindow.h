/*
 * Copyright 2001-2009, Haiku.
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


#include <AutoDeleter.h>
#include <GraphicsDefs.h>
#include <Locker.h>
#include <Message.h>
#include <Messenger.h>
#include <Rect.h>
#include <Region.h>
#include <String.h>
#include <Window.h>

#include <PortLink.h>
#include <TokenSpace.h>

#include "EventDispatcher.h"
#include "MessageLooper.h"


class BString;
class BMessenger;
class BPoint;
class BMessage;

class Desktop;
class ServerApp;
class Decorator;
class Window;
class Workspace;
class View;
class ServerPicture;
class DirectWindowInfo;
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

	virtual port_id				MessagePort() const { return fMessagePort; }

			::EventTarget&		EventTarget() { return fEventTarget; }

	inline	ServerApp*			App() const { return fServerApp; }
			::Desktop*			Desktop() const { return fDesktop; }
			::Window*			Window() const;

			// methods for sending various messages to client.
			void				NotifyQuitRequested();
			void				NotifyMinimize(bool minimize);
			void				NotifyZoom();

			// util methods.
			const BMessenger&	FocusMessenger() const
									{ return fFocusMessenger; }
			const BMessenger&	HandlerMessenger() const
									{ return fHandlerMessenger; }

			void				ScreenChanged(const BMessage* message);
			status_t			SendMessageToClient(const BMessage* message,
									int32 target = B_NULL_TOKEN) const;

	virtual	::Window*			MakeWindow(BRect frame, const char* name,
									window_look look, window_feel feel,
									uint32 flags, uint32 workspace);

			void				SetTitle(const char* newTitle);
	inline	const char*			Title() const { return fTitle; }

			// related thread/team_id(s).
	inline	team_id				ClientTeam() const { return fClientTeam; }

	inline	port_id				ClientLooperPort () const
									{ return fClientLooperPort; }

	inline	int32				ClientToken() const { return fClientToken; }
	inline	int32				ServerToken() const { return fServerToken; }

			void				RequestRedraw();

			void				GetInfo(window_info& info);

			void				HandleDirectConnection(int32 bufferState,
									int32 driverState = 0);
			bool				HasDirectFrameBufferAccess() const
									{ return fDirectWindowInfo.Get() != NULL; }
			bool				IsDirectlyAccessing() const
									{ return fIsDirectlyAccessing; }

			void				ResyncDrawState();

						// TODO: Change this
	inline	void				UpdateCurrentDrawingRegion()
									{ _UpdateCurrentDrawingRegion(); };

private:
			View*				_CreateView(BPrivate::LinkReceiver &link,
									View **_parent);

			void				_Show();
			void				_Hide();

			// message handling methods.
			void				_DispatchMessage(int32 code,
									BPrivate::LinkReceiver &link);
			void				_DispatchViewMessage(int32 code,
									BPrivate::LinkReceiver &link);
			void				_DispatchViewDrawingMessage(int32 code,
									BPrivate::LinkReceiver &link);
			bool				_DispatchPictureMessage(int32 code,
									BPrivate::LinkReceiver &link);
			void				_MessageLooper();
	virtual void				_PrepareQuit();
	virtual void				_GetLooperName(char* name, size_t size);

			void				_ResizeToFullScreen();
			status_t			_EnableDirectWindowMode();
			void				_DirectWindowSetFullScreen(bool set);

			void				_SetCurrentView(View* view);
			void				_UpdateDrawState(View* view);
			void				_UpdateCurrentDrawingRegion();

			bool				_MessageNeedsAllWindowsLocked(
									uint32 code) const;

private:
			char*				fTitle;

			::Desktop*			fDesktop;
			ServerApp*			fServerApp;
			ObjectDeleter< ::Window>
								fWindow;
			bool				fWindowAddedToDesktop;

			team_id				fClientTeam;

			port_id				fMessagePort;
			port_id				fClientReplyPort;
			port_id				fClientLooperPort;
			BMessenger			fFocusMessenger;
			BMessenger			fHandlerMessenger;
			::EventTarget		fEventTarget;

			int32				fRedrawRequested;

			int32				fServerToken;
			int32				fClientToken;

			View*				fCurrentView;
			BRegion				fCurrentDrawingRegion;
			bool				fCurrentDrawingRegionValid;

			ObjectDeleter<DirectWindowInfo>
								fDirectWindowInfo;
			bool				fIsDirectlyAccessing;
};

#endif	// SERVER_WINDOW_H
