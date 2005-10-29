/*******************************************************************************
/
/	File:			Screen.h
/
/   Description:    BScreen provides information about a screen's current
/                   display settings.  It also lets you set the Desktop color.
/
/	Copyright 1993-98, Be Incorporated, All Rights Reserved
/
*******************************************************************************/

#ifndef _SCREEN_H
#define _SCREEN_H

#include <BeBuild.h>
#include <Accelerant.h>
#include <GraphicsDefs.h>
#include <Rect.h>
#include <OS.h>

/*----------------------------------------------------------------*/
/*----- BScreen structures and declarations ----------------------*/

class BWindow;
class BPrivateScreen;

/*----------------------------------------------------------------*/
/*----- BScreen class --------------------------------------------*/

class BScreen {
public:  
        BScreen( screen_id id=B_MAIN_SCREEN_ID );
        BScreen( BWindow *win );
        ~BScreen();

        bool   			IsValid();
        status_t		SetToNext();
  
        color_space		ColorSpace();
        BRect			Frame();
        screen_id		ID();

        status_t		WaitForRetrace();
        status_t		WaitForRetrace(bigtime_t timeout);
  
        uint8			IndexForColor( rgb_color rgb );
        uint8			IndexForColor( uint8 r, uint8 g, uint8 b, uint8 a=255 );
        rgb_color		ColorForIndex( const uint8 index );
        uint8			InvertIndex( uint8 index );

const   color_map*		ColorMap();

		status_t		GetBitmap(	BBitmap		**screen_shot,
									bool		draw_cursor = true,
									BRect		*bound = NULL); 
		status_t		ReadBitmap(	BBitmap		*buffer,
									bool		draw_cursor = true,
									BRect		*bound = NULL); 
        
        rgb_color		DesktopColor();
        rgb_color		DesktopColor(uint32 index);
        void			SetDesktopColor( rgb_color rgb, bool stick=true );
        void			SetDesktopColor( rgb_color rgb, uint32 index, bool stick=true );

		status_t		ProposeMode(display_mode *target, const display_mode *low, const display_mode *high);
		status_t		GetModeList(display_mode **mode_list, uint32 *count);
		status_t		GetMode(display_mode *mode);
		status_t		GetMode(uint32 workspace, display_mode *mode);
		status_t		SetMode(display_mode *mode, bool makeDefault = false);
		status_t		SetMode(uint32 workspace, display_mode *mode, bool makeDefault = false);
		status_t		GetDeviceInfo(accelerant_device_info *adi);
		status_t		GetPixelClockLimits(display_mode *mode, uint32 *low, uint32 *high);
		status_t		GetTimingConstraints(display_timing_constraints *dtc);
		status_t		SetDPMS(uint32 dpms_state);
		uint32			DPMSState(void);
		uint32			DPMSCapabilites(void);

        BPrivateScreen*	private_screen();

/*----- Private or reserved -----------------------------------------*/
private:
		status_t		ProposeDisplayMode(display_mode *target, const display_mode *low, const display_mode *high);
		BScreen			&operator=(const BScreen &screen);
						BScreen(const BScreen &screen);
        void*			BaseAddress();
        uint32			BytesPerRow();

        BPrivateScreen 	*screen;
};


/*----------------------------------------------------------------*/
/*----- inline definitions ---------------------------------------*/

inline uint8 BScreen::IndexForColor( rgb_color rgb )
  { return IndexForColor(rgb.red,rgb.green,rgb.blue,rgb.alpha); }

/*-------------------------------------------------------------*/
/*-------------------------------------------------------------*/

#endif /* _SCREEN_H */
