/*****************************************************************************/
// DirectWindow.h
//
// A "client window class for direct screen access."
//
//
// Copyright (c) 2001 OpenBeOS Project
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense, 
// and/or sell copies of the Software, and to permit persons to whom the 
// Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included 
// in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
// OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL 
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.
/*****************************************************************************/

#ifndef GAME_KIT_DIRECT_WINDOW
#define GAME_KIT_DIRECT_WINDOW

#include <Region.h>
#include <Window.h>

class BDirectDriver;

// State of the direct access when called back through DirectConnected.
enum direct_buffer_state 
{
	B_DIRECT_MODE_MASK	= 15,

	B_DIRECT_START		= 0,
	B_DIRECT_STOP		= 1,
	B_DIRECT_MODIFY		= 2,
	
	B_CLIPPING_MODIFIED = 16,
	B_BUFFER_RESIZED 	= 32,
	B_BUFFER_MOVED 		= 64,
	B_BUFFER_RESET	 	= 128
};

// State of the direct driver and its hooks functions.
enum direct_driver_state 
{
	B_DRIVER_CHANGED	= 0x0001,
	B_MODE_CHANGED		= 0x0002
};

// Frame buffer access descriptor.
typedef struct 
{
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


class BDirectWindow : public BWindow
{
public:
	// Construction and destruction.
	BDirectWindow( BRect frame, const char* pTitle, window_type	type,
		uint32 flags, uint32 workspace = B_CURRENT_WORKSPACE );

    BDirectWindow( BRect frame, const char* pTitle, window_look	look,
		window_feel feel, uint32 flags, uint32 workspace = B_CURRENT_WORKSPACE );

	virtual ~BDirectWindow();

	// Save and restore.
	static BArchivable* Instantiate( BMessage* pData );
	virtual	status_t 	Archive( BMessage *data, bool deep = true ) const;


	virtual void 	DirectConnected( direct_buffer_info* pInfo );
	status_t 		GetClippingRegion( BRegion* pRegion, BPoint* pOrigin = NULL ) const;
	status_t 		SetFullScreen( bool enable );
	bool 			IsFullScreen() const;
		
	static bool SupportsWindowMode( screen_id = B_MAIN_SCREEN_ID );

	// Defined for future extension (fragile base class). Otherwise
	// identical to BWindow.
	virtual void Quit();
	virtual	void DispatchMessage( BMessage* pMessage, BHandler* pHandler );
	virtual	void MessageReceived( BMessage* pMessage );
	virtual	void FrameMoved( BPoint new_position );
	virtual void WorkspacesChanged( uint32 old_workspace, uint32 new_workspace );
	virtual void WorkspaceActivated( int32 workspace, bool state );
	virtual	void FrameResized( float new_width, float new_height );
	virtual void Minimize( bool minimize );
	virtual void Zoom( BPoint rec_position, float rec_width, float rec_height );
	virtual void ScreenChanged( BRect screen_size, color_space depth );
	virtual	void MenusBeginning();
	virtual	void MenusEnded();
	virtual	void WindowActivated( bool state );
	virtual	void Show();
	virtual	void Hide();
	virtual status_t GetSupportedSuites( BMessage* pData );
	virtual status_t Perform( perform_code performCode, void* pArg );
	virtual BHandler * ResolveSpecifier( BMessage* pMsg, int32 index,
		BMessage* pSpecifier, int32 form, const char* pProperty );

private:
	// No copying allowed.
	BDirectWindow();
	BDirectWindow( BDirectWindow& );
	BDirectWindow& operator=( BDirectWindow& );

private:
	// Fill in all the particulars relating to our implementation,
	// once we know what they're going to be, here.
}

#endif // GAME_KIT_DIRECT_WINDOW