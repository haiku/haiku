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
//	File Name:		DirectWindow.h
//	Author:			Christopher ML Zumwalt May (zummy@users.sf.net)
//	Description:	BFileGameSound allow the developer to directly draw to the
//					app_server's frame buffer.
//------------------------------------------------------------------------------

#ifndef _DIRECTWINDOW_H
#define _DIRECTWINDOW_H

// Standard Includes -----------------------------------------------------------

// System Includes -------------------------------------------------------------
#include <Region.h>
#include <Window.h>

// Project Includes ------------------------------------------------------------

// Local Includes --------------------------------------------------------------

// Local Defines ---------------------------------------------------------------

// Globals ---------------------------------------------------------------------
enum direct_buffer_state {
	B_DIRECT_MODE_MASK	= 15,

	B_DIRECT_START		= 0,
	B_DIRECT_STOP		= 1,
	B_DIRECT_MODIFY		= 2,
	
	B_CLIPPING_MODIFIED = 16,
	B_BUFFER_RESIZED 	= 32,
	B_BUFFER_MOVED 		= 64,
	B_BUFFER_RESET	 	= 128
};

enum direct_driver_state {
	B_DRIVER_CHANGED	= 0x0001,
	B_MODE_CHANGED		= 0x0002
};

typedef struct {
	direct_buffer_state	buffer_state;
	direct_driver_state	driver_state;
	void				*bits;
	void				*pci_bits;
	int32				bytes_per_row;
	uint32				bits_per_pixel;
	color_space			pixel_format;
	buffer_layout		layout;
	buffer_orientation	orientation;
	uint32				_reserved[9];
	uint32				_dd_type_;
	uint32				_dd_token_;
	uint32				clip_list_count;
	clipping_rect		window_bounds;
	clipping_rect		clip_bounds;
	clipping_rect		clip_list[1];
} direct_buffer_info;

// PushGameSound ---------------------------------------------------------------
class BDirectWindow : public BWindow 
{
public:
							BDirectWindow(BRect		frame,
										const char	*title, 
										window_type	type,
										uint32		flags,
										uint32		workspace = B_CURRENT_WORKSPACE);
        					BDirectWindow(BRect		frame,
										const char	*title, 
										window_look	look,
										window_feel feel,
										uint32		flags,
										uint32 		workspace = B_CURRENT_WORKSPACE);
	virtual 				~BDirectWindow();
	
	static	BArchivable		*Instantiate(BMessage *data);
	virtual	status_t		Archive(BMessage *data, bool deep = true) const;
	virtual void        	Quit(void);
	virtual	void			DispatchMessage(BMessage *message, BHandler *handler);
	virtual	void			MessageReceived(BMessage *message);
	virtual	void			FrameMoved(BPoint new_position);
	virtual void			WorkspacesChanged(uint32 old_ws, uint32 new_ws);
	virtual void			WorkspaceActivated(int32 ws, bool state);
	virtual	void			FrameResized(float new_width, float new_height);
	virtual void			Minimize(bool minimize);
	virtual void			Zoom(BPoint rec_position,
								float rec_width,
								float rec_height);
	virtual void			ScreenChanged(BRect screen_size, color_space depth);
	virtual	void			MenusBeginning();
	virtual	void			MenusEnded();
	virtual	void			WindowActivated(bool state);
	virtual	void			Show();
	virtual	void			Hide();
	virtual BHandler		*ResolveSpecifier(BMessage *msg,
										int32 index,
										BMessage *specifier,
										int32 form,
										const char *property);
	virtual status_t		GetSupportedSuites(BMessage *data);
	virtual status_t		Perform(perform_code d, void *arg);

private:
	virtual	void			task_looper();
	virtual BMessage		*ConvertToMessage(void *raw, int32 code);

public:
	// BDirectWindow API
	virtual void        		DirectConnected(direct_buffer_info *info);
			status_t		GetClippingRegion(BRegion *region, BPoint *origin = NULL) const;
			status_t		SetFullScreen(bool enable);
			bool			IsFullScreen() const;

	static	bool			SupportsWindowMode(screen_id = B_MAIN_SCREEN_ID);

private:
	
	typedef	BWindow					inherited;

	virtual void 		       	_ReservedDirectWindow1();
	virtual void        		_ReservedDirectWindow2();
	virtual void        		_ReservedDirectWindow3();
	virtual void        		_ReservedDirectWindow4();	

							BDirectWindow();
							BDirectWindow(BDirectWindow &);
							BDirectWindow &operator=(BDirectWindow &);

			void				InitData(BRect frame);

			void				DoDirectConnected();
			void				Connect();
			void				Disconnect();

			void				CreateClippingList();
			void				GetFrameBuffer();

			bool				fIsFullScreen;
			bool				fIsConnected;
			direct_buffer_info	*fBufferInfo;
			uint32			_reserved_[31];
};

#endif

