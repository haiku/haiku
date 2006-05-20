/*******************************************************************************
//
//	File:		DirectWindow.h
//
//	Description:	Client window class for direct screen access.
//
//	Copyright 1998, Be Incorporated, All Rights Reserved.
//
*******************************************************************************/


#ifndef	_DIRECT_WINDOW_H
#define	_DIRECT_WINDOW_H

#include <Region.h>
#include <Window.h>

class BDirectDriver;
					
/* State of the direct access when called back through DirectConnected */
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

/* State of the direct driver and its hooks functions */
enum direct_driver_state {
	B_DRIVER_CHANGED	= 0x0001,
	B_MODE_CHANGED		= 0x0002
};

/* Integer rect used to define a cliping rectangle. All bounds are included */
/* Moved to Region.h */

/* Frame buffer access descriptor */
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

/* BDirectWindow class */
class BDirectWindow : public BWindow {
public:
		BDirectWindow(	BRect		frame,
						const char	*title, 
						window_type	type,
						uint32		flags,
						uint32		workspace = B_CURRENT_WORKSPACE);
        BDirectWindow(	BRect		frame,
						const char	*title, 
						window_look	look,
						window_feel feel,
						uint32		flags,
						uint32 		workspace = B_CURRENT_WORKSPACE);
virtual ~BDirectWindow();
static	BArchivable		*Instantiate(BMessage *data);
virtual	status_t		Archive(BMessage *data, bool deep = true) const;

/* defined for future extension (fragile base class). Identical to BWindow */
virtual void        	Quit(void);
virtual	void			DispatchMessage(BMessage *message, BHandler *handler);
virtual	void			MessageReceived(BMessage *message);
virtual	void			FrameMoved(BPoint new_position);
virtual void			WorkspacesChanged(uint32 old_ws, uint32 new_ws);
virtual void			WorkspaceActivated(int32 ws, bool state);
virtual	void			FrameResized(float new_width, float new_height);
virtual void			Minimize(bool minimize);
virtual void			Zoom(	BPoint rec_position,
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

/* new APIs */

public:
virtual void        	DirectConnected(direct_buffer_info *info);
		status_t		GetClippingRegion(BRegion *region, BPoint *origin = NULL) const;
		status_t		SetFullScreen(bool enable);
		bool			IsFullScreen() const;
		
static	bool			SupportsWindowMode(screen_id = B_MAIN_SCREEN_ID);

/* private */
private:

		typedef BWindow	inherited;

virtual void        	_ReservedDirectWindow1();
virtual void        	_ReservedDirectWindow2();
virtual void        	_ReservedDirectWindow3();
virtual void        	_ReservedDirectWindow4();
		

		BDirectWindow();
		BDirectWindow(BDirectWindow &);
		BDirectWindow &operator=(BDirectWindow &);

		bool				fDaemonKiller;
		bool				fConnectionEnable;
		bool				fIsFullScreen;
		bool				fDirectDriverReady;
		bool				fInDirectConnect;
		
		int32				fDirectLock;				
		sem_id				fDirectSem;
		uint32				fDirectLockCount;
		thread_id			fDirectLockOwner;
		char				*fDirectLockStack;
		
		sem_id				fDisableSem;
		sem_id				fDisableSemAck;
		
		uint32				fInitStatus;
		uint32				fInfoAreaSize;
		
		uint32				fDirectDriverType;
		uint32				fDirectDriverToken;
		
		area_id				fClonedClippingArea;
		area_id				fSourceClippingArea;
		thread_id			fDirectDaemonId;
		direct_buffer_info		*fBufferDesc;
		
		BDirectDriver			*direct_driver;
		struct priv_ext			*extension;
		uint32				_reserved_[15];

	static	int32				_DaemonStarter(void *arg);
		int32				DirectDaemonFunc();
		bool				LockDirect() const;
		void				UnlockDirect() const;
		void				InitData();
		void				DisposeData();
		status_t			DriverSetup() const;
};

#endif
