//------------------------------------------------------------------------------
//	Copyright (c) 2001-2002, OpenBeOS
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
//	File Name:		Window.h
//	Author:			Adrian Oanca (e2joseph@hotpop.com)
//	Description:	BWindow is the base class for all windows (graphic areas
//					displayed on-screen).
//
//
//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//	NOTE(!!!)		All private non virtual functions are to be renamed following
//					this scheme: word_another_third <-> wordAnotherThird
//	OBOS Team:	If you use one of BWindow's private functions, please addapt to this code
//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//------------------------------------------------------------------------------

#ifndef	_WINDOW_H
#define	_WINDOW_H

// Standard Includes -----------------------------------------------------------

// System Includes -------------------------------------------------------------
#include <BeBuild.h>
#include <InterfaceDefs.h>
#include <List.h>
#include <Looper.h>
#include <Rect.h>
#include <StorageDefs.h>

// Project Includes ------------------------------------------------------------

// Local Includes --------------------------------------------------------------

// Local Defines ---------------------------------------------------------------

// Globals ---------------------------------------------------------------------


// window definitions ----------------------------------------------------------

enum window_type {
	B_UNTYPED_WINDOW	= 0,
	B_TITLED_WINDOW 	= 1,
	B_MODAL_WINDOW 		= 3,
	B_DOCUMENT_WINDOW	= 11,
	B_BORDERED_WINDOW	= 20,
	B_FLOATING_WINDOW	= 21
};

//----------------------------------------------------------------

enum window_look {
	B_BORDERED_WINDOW_LOOK	= 20,
	B_NO_BORDER_WINDOW_LOOK	= 19,
	B_TITLED_WINDOW_LOOK	= 1,	
	B_DOCUMENT_WINDOW_LOOK	= 11,
	B_MODAL_WINDOW_LOOK		= 3,
	B_FLOATING_WINDOW_LOOK	= 7
};

//----------------------------------------------------------------

enum window_feel {
	B_NORMAL_WINDOW_FEEL			= 0,	
	B_MODAL_SUBSET_WINDOW_FEEL		= 2,
	B_MODAL_APP_WINDOW_FEEL			= 1,
	B_MODAL_ALL_WINDOW_FEEL			= 3,
	B_FLOATING_SUBSET_WINDOW_FEEL	= 5,
	B_FLOATING_APP_WINDOW_FEEL		= 4,
	B_FLOATING_ALL_WINDOW_FEEL		= 6
};

//----------------------------------------------------------------

enum window_alignment {
	B_BYTE_ALIGNMENT	= 0,
	B_PIXEL_ALIGNMENT	= 1
};

//----------------------------------------------------------------

enum {
	B_NOT_MOVABLE				= 0x00000001,
	B_NOT_CLOSABLE				= 0x00000020,
	B_NOT_ZOOMABLE				= 0x00000040,
	B_NOT_MINIMIZABLE			= 0x00004000,
	B_NOT_RESIZABLE				= 0x00000002,
	B_NOT_H_RESIZABLE			= 0x00000004,
	B_NOT_V_RESIZABLE			= 0x00000008,
	B_AVOID_FRONT				= 0x00000080,
	B_AVOID_FOCUS				= 0x00002000,
	B_WILL_ACCEPT_FIRST_CLICK	= 0x00000010,
	B_OUTLINE_RESIZE			= 0x00001000,
	B_NO_WORKSPACE_ACTIVATION	= 0x00000100,
	B_NOT_ANCHORED_ON_ACTIVATE	= 0x00020000,
	B_ASYNCHRONOUS_CONTROLS		= 0x00080000,
	B_QUIT_ON_WINDOW_CLOSE		= 0x00100000
};

#define B_CURRENT_WORKSPACE	0
#define B_ALL_WORKSPACES	0xffffffff

//----------------------------------------------------------------

class BSession;
class BButton;
class BMenuBar;
class BMenuItem;
class BMessage;
class BMessageRunner;
class BMessenger;
class BView;

struct message;
struct _cmd_key_;
struct _view_attr_;

// BWindow class ---------------------------------------------------------------
class BWindow : public BLooper {

public:
						BWindow(BRect frame,
								const char* title, 
								window_type type,
								uint32 flags,
								uint32 workspace = B_CURRENT_WORKSPACE);
						BWindow(BRect frame,
								const char* title, 
								window_look look,
								window_feel feel,
								uint32 flags,
								uint32 workspace = B_CURRENT_WORKSPACE);
virtual					~BWindow();

						BWindow(BMessage* data);
static	BArchivable		*Instantiate(BMessage* data);
virtual	status_t		Archive(BMessage* data, bool deep = true) const;

virtual	void			Quit();
		void			Close(); // Synonym of Quit()

		void			AddChild(BView* child, BView* before = NULL);
		bool			RemoveChild(BView* child);
		int32			CountChildren() const;
		BView			*ChildAt(int32 index) const;

virtual	void			DispatchMessage(BMessage* message, BHandler* handler);
virtual	void			MessageReceived(BMessage* message);
virtual	void			FrameMoved(BPoint new_position);
virtual void			WorkspacesChanged(uint32 old_ws, uint32 new_ws);
virtual void			WorkspaceActivated(int32 ws, bool state);
virtual	void			FrameResized(float new_width, float new_height);
virtual void			Minimize(bool minimize);
virtual void			Zoom(	BPoint rec_position,
								float rec_width,
								float rec_height);
		void			Zoom();
		void			SetZoomLimits(float maxWidth, float maxHeight);				// changed from: SetZoomLimits(float max_h, float max_v);
virtual void			ScreenChanged(BRect screen_size, color_space depth);
		void			SetPulseRate(bigtime_t rate);
		bigtime_t		PulseRate() const;
		void			AddShortcut(	uint32 key,
										uint32 modifiers,
										BMessage* msg);
		void			AddShortcut(	uint32 key,
										uint32 modifiers,
										BMessage* msg,
										BHandler* target);
		void			RemoveShortcut(uint32 key, uint32 modifiers);
		void			SetDefaultButton(BButton* button);
		BButton			*DefaultButton() const;
virtual	void			MenusBeginning();
virtual	void			MenusEnded();
		bool			NeedsUpdate() const;
		void			UpdateIfNeeded();
		BView			*FindView(const char* viewName) const;						// changed from: FindView( const char* view_name );
		BView			*FindView(BPoint) const;
		BView			*CurrentFocus() const;
		void			Activate(bool = true);
virtual	void			WindowActivated(bool state);
		void			ConvertToScreen(BPoint* pt) const;
		BPoint			ConvertToScreen(BPoint pt) const;
		void			ConvertFromScreen(BPoint* pt) const;
		BPoint			ConvertFromScreen(BPoint pt) const;
		void			ConvertToScreen(BRect* rect) const;
		BRect			ConvertToScreen(BRect rect) const;
		void			ConvertFromScreen(BRect* rect) const;
		BRect			ConvertFromScreen(BRect rect) const;
		void			MoveBy(float dx, float dy);
		void			MoveTo(BPoint);
		void			MoveTo(float x, float y);
		void			ResizeBy(float dx, float dy);
		void			ResizeTo(float width, float height);
virtual	void			Show();
virtual	void			Hide();
		bool			IsHidden() const;
		bool			IsMinimized() const;

		void			Flush() const;
		void			Sync() const;

		status_t		SendBehind(const BWindow* window);

		void			DisableUpdates();
		void			EnableUpdates();

		void			BeginViewTransaction();										// referred as OpenViewTransaction() in BeBook
		void			EndViewTransaction();										// referred as CommitViewTransaction() in BeBook

		BRect			Bounds() const;
		BRect			Frame() const;
		const char		*Title() const;
		void			SetTitle(const char* title);
		bool			IsFront() const;
		bool			IsActive() const;
		void			SetKeyMenuBar(BMenuBar* bar);
		BMenuBar		*KeyMenuBar() const;
		void			SetSizeLimits(	float minWidth,								// changed from: SetSizeLimits(float min_h, float max_h, float min_v, float max_v);
										float maxWidth,
										float minHeight,
										float maxHeight);
		void			GetSizeLimits(	float *minWidth,							// changed from: SetSizeLimits(float* min_h, float* max_h, float* min_v, float* max_v);
										float *maxWidth, 
										float *minHeight,
										float *maxHeight);
		uint32			Workspaces() const;
		void			SetWorkspaces(uint32);
		BView			*LastMouseMovedView() const;

virtual BHandler		*ResolveSpecifier(BMessage* msg,
										int32 index,
										BMessage* specifier,
										int32 form,
										const char* property);
virtual status_t		GetSupportedSuites(BMessage* data);

		status_t		AddToSubset(BWindow* window);
		status_t		RemoveFromSubset(BWindow* window);

virtual status_t		Perform(perform_code d, void* arg);

		status_t		SetType(window_type type);
		window_type		Type() const;

		status_t		SetLook(window_look look);
		window_look		Look() const;

		status_t		SetFeel(window_feel feel);
		window_feel		Feel() const;

		status_t		SetFlags(uint32);
		uint32			Flags() const;

		bool			IsModal() const;
		bool			IsFloating() const;

		status_t		SetWindowAlignment(window_alignment mode,
											int32 h,
											int32 hOffset = 0,
											int32 width = 0,
											int32 widthOffset = 0,
											int32 v = 0,
											int32 vOffset = 0,
											int32 height = 0,
											int32 heightOffset = 0);
		status_t		GetWindowAlignment(window_alignment* mode = NULL,
											int32* h = NULL,
											int32* hOffset = NULL,
											int32* width = NULL,
											int32* widthOffset = NULL,
											int32* v = NULL,
											int32* vOffset = NULL,
											int32* height = NULL,
											int32* heightOffset = NULL) const;

virtual	bool			QuitRequested();
virtual thread_id		Run();

// Private or reserved ---------------------------------------------------------
private:

typedef BLooper inherited;

friend class BApplication;
friend class BBitmap;
friend class BScrollBar;
friend class BView;
friend class BMenuItem;
friend class BWindowScreen;
friend class BDirectWindow;
friend class BFilePanel;
friend class BHandler;
friend class _BEventMask;
friend void _set_menu_sem_(BWindow* w, sem_id sem);
friend status_t _safe_get_server_token_(const BLooper* , int32* );

virtual	void			_ReservedWindow1();
virtual	void			_ReservedWindow2();
virtual	void			_ReservedWindow3();
virtual	void			_ReservedWindow4();
virtual	void			_ReservedWindow5();
virtual	void			_ReservedWindow6();
virtual	void			_ReservedWindow7();
virtual	void			_ReservedWindow8();

					BWindow();
					BWindow(BWindow&);
		BWindow		&operator=(BWindow&);
					
					BWindow(BRect frame, color_space depth,			// to be implemented
							uint32 bitmapFlags, int32 rowBytes);
		void		InitData(BRect frame,
							const char* title,
							window_look look,
							window_feel feel,
							uint32 flags,
							uint32 workspace);
		status_t	ArchiveChildren(BMessage* data, bool deep) const;	// call made from within Archive
		status_t	UnarchiveChildren(BMessage* data);		// Instantiate(BMessage* data)->BWindow(BMessage)->UnarchiveChildren(BMessage* data)
		void		BitmapClose();							// to be implemented
virtual	void		task_looper();							// thread function - it's here where app_server messages are received
/*		void		start_drag(	BMessage* msg,
								int32 token,
								BPoint offset,
								BRect track_rect,
								BHandler* reply_to);
		void		start_drag(	BMessage* msg,
								int32 token,
								BPoint offset,
								int32 bitmap_token,
								drawing_mode dragMode,
								BHandler* reply_to);
*/
		void		prepareView(BView* aView);						// changed from view_builder(BView* a_view);
		void		attachView(BView* aView);						// changed from attach_builder(BView* a_view);
		void		detachView(BView* aView);						// changed from detach_builder(BView* a_view);
		//int32		get_server_token() const;
		BMessage	*extract_drop(BMessage* an_event, BHandler* *target);
		//void		movesize(uint32 opcode, float h, float v);
		
		BMessage* 	ReadMessageFromPort(bigtime_t tout = B_INFINITE_TIMEOUT);
		//int32		MessagesWaiting();

		void		handle_activate(BMessage* an_event);
		//void		do_view_frame(BMessage* an_event);
		//void		do_value_change(BMessage* an_event, BHandler* handler);
		//void		do_mouse_down(BMessage* an_event, BView* target);
		//void		do_mouse_moved(BMessage* an_event, BView* target);
		//void		do_key_down(BMessage* an_event, BHandler* handler);
		//void		do_key_up(BMessage* an_event, BHandler* handler);
		void		do_menu_event(BMessage* an_event);
		//void		do_draw_views();
virtual BMessage	*ConvertToMessage(void* raw, int32 code);					// HUGE function - it converts PortLink messages into BMessages
		//_cmd_key_	*allocShortcut(uint32 key, uint32 modifiers);
		//_cmd_key_	*FindShortcut(uint32 key, uint32 modifiers);
		void		AddShortcut(uint32 key,										// !!! - and menu shortcuts to list when a menu is added
								uint32 modifiers,
								BMenuItem* item);
		//void		post_message(BMessage* message);
		//void		SetLocalTitle(const char* new_title);
		//void		enable_pulsing(bool enable);
		BHandler	*determine_target(BMessage* msg, BHandler* target, bool pref);
		//void		kb_navigate();
		//void		navigate_to_next(int32 direction, bool group = false);
		//void		set_focus(BView* focus, bool notify_input_server);		// what does notify_input_server mean??? why???
		bool		InUpdate();
		void		DequeueAll();
		//bool		find_token_and_handler(BMessage* msg, int32* token,	BHandler* *handler);
		window_type	composeType(window_look look,						// changed from: compose_type(...)
								 window_feel feel) const;
		void		decomposeType(window_type type,						// changed from: decompose_type(...)
								   window_look* look,
								   window_feel* feel) const;

		void		SetIsFilePanel(bool yes);
		bool		IsFilePanel() const;
// OBOS BWindow's addon functions
		uint32		WindowLookToInteger(window_look wl);
		uint32		WindowFeelToInteger(window_feel wf);
		void		BuildTopView();
		void		stopConnection();
		void		setFocus(BView *focusView, bool notifyIputServer=false);

		bool		handleKeyDown( uint32 key, uint32 modifiers);

						// message: B_MOUSE_UP, B_MOUSE_DOWN, B_MOUSE_MOVED
		void		sendMessageUsingEventMask( int32 message, BPoint where ); 
		BView*		sendMessageUsingEventMask2( BView* aView, int32 message, BPoint where );
		void		sendPulse( BView* );
		int32		findShortcut(uint32 key, uint32 modifiers);
		bool		findHandler( BView* start, BHandler* handler );
		BView*		findView(BView* aView, const char* viewName) const;
		BView*		findView(BView* aView, BPoint point) const;
		BView*		findView(BView* aView, int32 token);
		
		BView*		findNextView( BView *focus, uint32 flags);
		BView*		findPrevView( BView *focus, uint32 flags);
		BView*		findLastChild(BView *parent);
		bool		handleKeyDown( int32 raw_char, uint32 modifiers);
		void		handleActivation( bool active );
		void		activateView( BView *aView, bool active );

		void		drawAllViews(BView* aView);
		void		drawView(BView* aView, BRect area);

		// Debug
		void		PrintToStream() const;
// END: OBOS addon functions

		// 3 deprecated calls
		//void			AddFloater(BWindow* a_floating_window);
		//void			RemoveFloater(BWindow* a_floating_window);
		//window_type		WindowType() const;

		char			*fTitle;					// used
		int32			server_token;
		bool			fInTransaction;				// used		// changed from: char			fInUpdate;
		bool			fActive;					// used		// changed from: char			f_active;
		short			fShowLevel;					// used
		uint32			fFlags;						// used

		port_id			send_port;					// used
		port_id			receive_port;				// used

		BView			*top_view;					// used
		BView			*fFocus;					// used
		BView			*fLastMouseMovedView;		// used
		BSession		*session;				// used
		BMenuBar		*fKeyMenuBar;				// used
		BButton			*fDefaultButton;			// used
		BList			accelList;					// used
		int32			fTopViewToken;				//			// changed from: int32			top_view_token;
		bool			fPulseEnabled;				// used		// changed from: bool			pulse_enabled;
		bool			fViewsNeedPulse;
		bool			fIsFilePanel;				// used
		bool			fMaskActivated;
		bigtime_t		fPulseRate;					// used		// changed from: bigtime_t		pulse_rate;
		bool			fWaitingForMenu;
		bool			fMinimized;					// used		// changed from: bool			fOffscreen;
		sem_id			fMenuSem;
		float			fMaxZoomHeight;				// used		// changed from: float			fMaxZoomH;
		float			fMaxZoomWidth;				// used		// changed from: float			fMaxZoomV;
		float			fMinWindHeight;				// used		// changed from: float			fMinWindH;
		float			fMinWindWidth;				// used		// changed from: float			fMinWindV;
		float			fMaxWindHeight;				// used		// changed from: float			fMaxWindH;
		float			fMaxWindWidth;				// used		// changed from: float			fMaxWindV;
		BRect			fFrame;						// used
		window_look		fLook;						// used
		_view_attr_		*fCurDrawViewState;
		window_feel		fFeel;						// used
		int32			fLastViewToken;
		uint32			fUnused1;					// changed from: _CEventPort_* 	fEventPort;
		BMessageRunner	*fPulseRunner;				// used
		BRect			fCurrentFrame;

		uint32			_reserved[2];	// was 8
#if !_PR3_COMPATIBLE_
		uint32			_more_reserved[4];
#endif
};

// inline definitions ----------------------------------------------------------
inline void  BWindow::Close()
{
	Quit();
}
//------------------------------------------------------------------------------

#endif	// _WINDOW_H 

/*
 * $Log $
 *
 * $Id  $
 *
 */

