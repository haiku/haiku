/*****************************************************************************/
// WindowScreen.h
//
// This is a "client window class for direct screen access."
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

#ifndef GAME_KIT_WINDOW_SCREEN
#define GAME_KIT_WINDOW_SCREEN

#include <kernel/image.h>
#include <Accelerant.h>
#include <BeBuild.h>
#include <GraphicsCard.h>
#include <OS.h>
#include <SupportDefs.h>
#include <Window.h>


// Private struct.
typedef struct 
{
	area_id memory_area;
    area_id io_area;
	char addon_path[64+B_FILE_NAME_LENGTH];

} _direct_screen_info_;

// Private typedef.
typedef int32 (*_add_on_control_)( uint32,void* );

// Global function, allowing to move the position of the mouse when
// the GameKit is in control of the screen.
_IMPEXP_GAME void set_mouse_position( int32 x, int32 y );

//*********************  WARNING - WARNING - WARNING ***********************************
//	This is the current version of WindowScreen. The main purpose of this object is to
//	establish a direct connexion between application and graphic driver, to increase
//	efficiency and abilities. The graphic driver architecture will overcome serious
//	changes in the next release, consequently this could only be a preview version of
//	the real WindowScreen API.
//
//	Question - is what follows below still true?
//  Answer - to be determined.
//
//	"Nevertheless, compatibility will be insured in future releases FOR MOST OF THE API,
//	through the use of a compatibility library (the new game library having a name
//	different from "libgame.so"), or a compatibility class (the new WindowScreen class
//	using a different name). Part of the API for which compatibility will not be insure
//	after the Preview Release are specifically pointed out in the following declarations."
//***************************************************************************************

enum 
{
	B_ENABLE_VIEW_DRAWING = 0x0001,
	B_ENABLE_DEBUGGER = 0x0002
};

class BWindowScreen : public BWindow 
{
public:
	BWindowScreen( const char* pTitle, uint32 space, status_t* pError,
		bool debug_enable = false );
	BWindowScreen( const char *pTitle, uint32 space, uint32 attributes,
		status_t* pError );
		
	virtual ~BWindowScreen();
	
	virtual void Quit();
	virtual void ScreenConnected( bool active );
	void Disconnect();
	virtual	void WindowActivated( bool active );
	virtual void WorkspaceActivated( int32 workspace, bool state );
	virtual void ScreenChanged( BRect screen_size, color_space depth );
	virtual void Hide();
	virtual void Show();
    void SetColorList( rgb_color* pList, int32 first_index = 0, int32 last_index = 255 );
	status_t SetSpace( uint32 space );
	bool CanControlFrameBuffer();
	status_t SetFrameBuffer( int32 width, int32 height );
	status_t MoveDisplayArea( int32 x, int32 y );

	// IOBase will not be supported in a future compatibility version of 
	// WindowScreen. Its features should be replaced by other functions 
	// of the new API.
	void *IOBase();
		
	rgb_color* 			ColorList();
	frame_buffer_info* 	FrameBufferInfo();
	graphics_card_hook 	CardHookAt( int32 index );
	graphics_card_info*	CardInfo();

//******************************* Debugger API Notice ********************************
//  Those three calls, and the debug_enable flag in the constructor, have been added to
//  help debugging a WindowScreen application without a cross-developement platform or
//  the use of serial debug output.
//
//  To use them, you need to :
//  - enable the debug mode by setting the debug_enable flag of the constructor to "true".
//  - register all threads that could be accessing the screen directly at any time. To do
//    that, call RegisterThread after spawning the new thread and before resuming
//	  it (you should never draw from the Window thread itself, neither register it).
//  - launch your application from Terminal. The GameKit will first swicth to another
//    workspace before opening the WindowScreen. If you launch from workspace 0 [Alt-F1],
//	  it will choose the workspace 1 [Alt-F2]. For any other workspace, it will choose
//	  the previous one (for example, for workspace 3 (Alt-F4), it will choose workspace 2
//	  [Alt-F3]).
//
//  Then :
//  - You can use printf(...) anywhere to display informations that will be logged in the
//    Terminal window. You can go back to the Terminal Window at any time (using the
//    correct Alt-F?? key). Switching workspace will automatically suspend all the thread
//	  you registered and save the graphic context. Switching back to the WindowScreen
//	  workspace will restore everything and resume your application.
//  - If you need to save and restore more states when your application is suspended and
//    resumed, you can overwrite the ForceSwitchForDebug() method. This function is called
//	  with active == true just after suspending your app, and active == false just before
//	  resuming it.
//  - You can suspend your application by calling the Debugger() method. This will also
//    switch to the Terminal workspace. Then you can resume by just switchng back to the
//	  WindowScreen workspace (using the good Alt-F?? key). It's better not to call Debugger()
//	  with the WindowScreen locked.
//
//  In case of crash :
//  - Use the safety short-cut : All the left modifiers (Ctrl, Shift, Option, Command, Alt
//    or equivalents) of your keyboard and F12. That should send you back to 640x480, and
//	  the debugger terminal should be visible (and usable) if any.
//  - Then, you can also switch to your Terminal workspace to check the last printf infos.
//  - After you got (we hope) more information about your (or our :-) bug, quit the debugger
//    window. Then you should be able to change your code and run your application again.
//	In any case, check the Error() function immediately after the WindowScreen constructor.
//	A bad error code will mean that the GameKit is in a inconsistent state, and then you
//	have better to reboot your machine. If you don't want to reboot, you can also try to
//	run your application in a different workspace.
//
//	It's far from perfect, but we hope it will still help.
//
//	The Debug API will not be support after the Preview Release (replaced by the debug
//	API in the new WindowScreen class).
//**************************************************************************************

	void RegisterThread( thread_id id );
	virtual	void SuspensionHook( bool active );
	void Suspend( char* pLabel );

	virtual status_t Perform( perform_code d, void* pArg );

private:
	BWindowScreen();
	BWindowScreen( BWindowScreen& );
	BWindowScreen& operator=( BWindowScreen& );
};

#endif // GAME_KIT_WINDOW_SCREEN