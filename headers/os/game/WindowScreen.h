/*
 *  Copyright 2020, Haiku, Inc. All Rights Reserved.
 *  Distributed under the terms of the MIT License.
 */
#ifndef	_WINDOW_SCREEN_H
#define	_WINDOW_SCREEN_H


#include <Accelerant.h>
#include <GraphicsCard.h>
#include <OS.h>
#include <SupportDefs.h>
#include <Window.h>
#include <kernel/image.h>


void set_mouse_position(int32 x, int32 y);

enum {
	B_ENABLE_VIEW_DRAWING	= 0x0001,
	B_ENABLE_DEBUGGER		= 0x0002
};

class BWindowScreen : public BWindow {
public:
						BWindowScreen(const char* title, uint32 space,
							status_t* _error, bool debugMode = false);
	        			BWindowScreen(const char* title, uint32 space,
	        				uint32 attributes, status_t* _error);
	virtual				~BWindowScreen();

	virtual	void		Quit();
	virtual	void		ScreenConnected(bool active);
    		void		Disconnect();

	virtual	void		WindowActivated(bool active);
	virtual	void		WorkspaceActivated(int32 workspace, bool active);
	virtual	void		ScreenChanged(BRect screenSize, color_space depth);

	virtual	void		Hide();
	virtual	void		Show();

			void		SetColorList(rgb_color* list, int32 firstIndex = 0,
							int32 lastIndex = 255);
			status_t	SetSpace(uint32 space);

			bool		CanControlFrameBuffer();
			status_t	SetFrameBuffer(int32 width, int32 height);
			status_t	MoveDisplayArea(int32 x, int32 y);
	rgb_color*			ColorList();
	frame_buffer_info*	FrameBufferInfo();
	graphics_card_hook	CardHookAt(int32 index);
	graphics_card_info*	CardInfo();
			void		RegisterThread(thread_id thread);
	virtual	void		SuspensionHook(bool active);
			void		Suspend(char* label);

private:
	virtual status_t	Perform(perform_code d, void* arg);
	virtual	void		_ReservedWindowScreen1();
	virtual	void		_ReservedWindowScreen2();
	virtual	void		_ReservedWindowScreen3();
	virtual	void		_ReservedWindowScreen4();
			status_t	_InitData(uint32 space, uint32 attributes);
			void		_DisposeData();
			status_t	_LockScreen(bool lock);
			status_t	_Activate();
			status_t	_Deactivate();
			status_t	_SetupAccelerantHooks();
			void		_ResetAccelerantHooks();
			status_t	_GetCardInfo();
			void		_Suspend();
			void		_Resume();
			status_t	_GetModeFromSpace(uint32 space, display_mode* mode);
			status_t	_InitClone();
			status_t	_AssertDisplayMode(display_mode* mode);

private:
			uint16		_reserved0;
			bool		_reserved1;
			bool		fWorkState;
			bool		fWindowState;
			bool		fActivateState;
			int32		fLockState;
			int32		fWorkspaceIndex;

	display_mode*		fOriginalDisplayMode;
	display_mode*		fDisplayMode;
			sem_id		fDebugSem;
			image_id	fAddonImage;
			uint32		fAttributes;

			rgb_color	fPalette[256];

	graphics_card_info	fCardInfo;
	frame_buffer_info	fFrameBufferInfo;

			char*		fDebugFrameBuffer;
			bool		fDebugState;
			bool		fDebugFirst;
			int32		fDebugWorkspace;
			int32		fDebugThreadCount;
			thread_id*	fDebugThreads;
			uint32		fModeCount;
	display_mode*		fModeList;

	GetAccelerantHook	fGetAccelerantHook;
	wait_engine_idle	fWaitEngineIdle;

			uint32		_reserved[163];
};

#endif