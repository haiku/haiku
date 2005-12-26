/*******************************************************************************
/
/	File:		WindowScreen.h
/
/	Description:	Client window class for direct screen access.
/
/	Copyright 1993-98, Be Incorporated, All Rights Reserved.
/
*******************************************************************************/


#ifndef	_WINDOW_SCREEN_H
#define	_WINDOW_SCREEN_H

#include <BeBuild.h>
#include <Window.h>
#include <SupportDefs.h>
#include <OS.h>
#include <kernel/image.h>
#include <GraphicsCard.h>
#include <Accelerant.h>

/* private struct */
typedef struct {
    area_id               memory_area;
    area_id               io_area;
	char                  addon_path[64+B_FILE_NAME_LENGTH];
} _direct_screen_info_;

/* private typedef */
typedef int32 (*_add_on_control_)(uint32,void *);

/* Global function, allowing to move the position of the mouse when the GameKit is in
   control of the screen. */
_IMPEXP_GAME
void set_mouse_position(int32 x, int32 y);

/**********************  WARNING - WARNING - WARNING ***********************************
   This is the current version of WindowScreen. The main purpose of this object is to
   establish a direct connexion between application and graphic driver, to increase
   efficiency and abilities. The graphic driver architecture will overcome serious
   changes in the next release, consequently this could only be a preview version of
   the real WindowScreen API.

   Nevertheless, compatibility will be insure in future releases FOR MOST OF THE API,
   through the use of a compatibility library (the new game library having a name
   different from "libgame.so"), or a compatibility class (the new WindowScreen class
   using a different name). Part of the API for which compatibility will not be insure
   after the Preview Release are specifically pointed out in the following declarations.
****************************************************************************************/

enum {
	B_ENABLE_VIEW_DRAWING = 0x0001,
	B_ENABLE_DEBUGGER = 0x0002
};

class BWindowScreen : public BWindow {
public:
	BWindowScreen(const char *title, uint32 space, status_t *error,
						bool debug_enable = false);
        BWindowScreen(const char *title, uint32 space, uint32 attributes,
						status_t *error);
	virtual ~BWindowScreen();

	virtual void Quit();
	virtual void ScreenConnected(bool active);
        	void Disconnect();

	virtual	void WindowActivated(bool active);
	virtual void WorkspaceActivated(int32 ws, bool state);
	virtual void ScreenChanged(BRect screen_size, color_space depth);

	virtual void Hide();
	virtual void Show();

        void SetColorList(rgb_color *list,int32 first_index = 0,int32 last_index = 255);
	status_t SetSpace(uint32 space);
	
	bool CanControlFrameBuffer();
	status_t SetFrameBuffer(int32 width, int32 height);
	status_t MoveDisplayArea(int32 x, int32 y);

	void *IOBase(); // Not supported anymore. It always returns NULL
		
	rgb_color           *ColorList();
	frame_buffer_info   *FrameBufferInfo();
	graphics_card_hook  CardHookAt(int32 index);
	graphics_card_info  *CardInfo();

/******************************* Debugger API Notice ********************************
  Those three calls, and the debug_enable flag in the constructor, have been added to
  help debugging a WindowScreen application without a cross-developement platform or
  the use of serial debug output.

  To use them, you need to :
  - enable the debug mode by setting the debug_enable flag of the constructor to "true".
  - register all threads that could be accessing the screen directly at any time. To do
    that, call RegisterThread after spawning the new thread and before resuming
	it (you should never draw from the Window thread itself, neither register it).
  - launch your application from Terminal. The GameKit will first swicth to another
    workspace before opening the WindowScreen. If you launch from workspace 0 [Alt-F1],
	it will choose the workspace 1 [Alt-F2]. For any other workspace, it will choose
	the previous one (for example, for workspace 3 (Alt-F4), it will choose workspace 2
	[Alt-F3]).

  Then :
  - You can use printf(...) anywhere to display informations that will be logged in the
    Terminal window. You can go back to the Terminal Window at any time (using the
	correct Alt-F?? key). Switching workspace will automatically suspend all the thread
	you registered and save the graphic context. Switching back to the WindowScreen
	workspace will restore everything and resume your application.
  - If you need to save and restore more states when your application is suspended and
    resumed, you can overwrite the ForceSwitchForDebug() method. This function is called
	with active == true just after suspending your app, and active == false just before
	resuming it.
  - You can suspend your application by calling the Debugger() method. This will also
    switch to the Terminal workspace. Then you can resume by just switchng back to the
	WindowScreen workspace (using the good Alt-F?? key). It's better not to call Debugger()
	with the WindowScreen locked.

  In case of crash :
  - Use the safety short-cut : All the left modifiers (Ctrl, Shift, Option, Command, Alt
    or equivalents) of your keyboard and F12. That should send you back to 640x480, and
	the debugger terminal should be visible (and usable) if any.
  - Then, you can also switch to your Terminal workspace to check the last printf infos.
  - After you got (we hope) more information about your (or our :-) bug, quit the debugger
    window. Then you should be able to change your code and run your application again.
	In any case, check the Error() function immediately after the WindowScreen constructor.
	A bad error code will mean that the GameKit is in a inconsistent state, and then you
	have better to reboot your machine. If you don't wnat to reboot, you can also try to
	run your application in a different workspace.

	It's far to be perfect, but we hope it will still help.

	The Debug API will not be support after the Preview Release (replaced by the debug
	API in the new WindowScreen class).
***************************************************************************************/
	void        RegisterThread(thread_id id);		
virtual	void        SuspensionHook(bool active);
	void        Suspend(char *label);

		
virtual status_t	Perform(perform_code d, void *arg);

private:

	typedef BWindow	inherited;

virtual void        _ReservedWindowScreen1();
virtual void        _ReservedWindowScreen2();
virtual void        _ReservedWindowScreen3();
virtual void        _ReservedWindowScreen4();
		
		BWindowScreen();
		BWindowScreen(BWindowScreen &);
		BWindowScreen &operator=(BWindowScreen &);
					
		char                  _unused;
		char                  space_mode;
		bool                  direct_enable;
		bool                  fWorkState;
		bool                  fWindowState;
		bool                  fActivateState;
        	int32                 fLockState;
		int32                 fScreenIndex;
		
		display_mode          *fOldDisplayMode;
		display_mode          *fDisplayMode;
		uint32                space0;
		sem_id                fActivateSem;
		sem_id                fDebugSem;
		image_id              fAddonImage;

		rgb_color             fColorList[256];
		GetAccelerantHook     fGetAccelerantHook;
        	graphics_card_info    fCardInfo;

		graphics_card_hook    hooks[B_HOOK_COUNT]; 
		_direct_screen_info_  info;
		frame_buffer_info     fFrameBufferInfo;

		char                  *fDebugBuffer;
		bool                  fDebugState;
		bool                  fDebugFirst;
		int32                 fDebugListCount;
		int32                 fDebugWorkspace;
	    	thread_id             *fDebugList;

		uint32		      fAttributes;
		uint32                fModeCount;
		display_mode          *fModeList;

		engine_token          *fEngineToken;
		wait_engine_idle      m_wei;
		acquire_engine        m_ae;
		release_engine        m_re;
		fill_rectangle        fill_rect;
		screen_to_screen_blit blit_rect;
		screen_to_screen_transparent_blit trans_blit_rect;
		screen_to_screen_scaled_filtered_blit scaled_filtered_blit_rect;

		uint32                _reserved_[24];
		
	static	BRect		CalcFrame(int32 index, int32 space, display_mode *dmode);
		status_t	InitData(uint32 space, uint32 attributes);
        	status_t    	SetActiveState(int32 state);
		status_t	SetupAccelerantHooks(bool enable);
		status_t    	GetCardInfo();
		void        	Suspend();
		void    	Resume();
		status_t	GetModeFromSpace(uint32 space, display_mode *dmode);
		status_t	InitClone();
		status_t	AssertDisplayMode(display_mode *dmode);
};

#endif











