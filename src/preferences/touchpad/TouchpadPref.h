#ifndef TouchpadPref_h
#define TouchpadPref_h

#define DEBUG 1
#include <Debug.h>

#include "touchpad_settings.h"
#include <Input.h>
#include <Path.h>

#if DEBUG
#	define LOG(text...) PRINT((text))
#else
#	define LOG(text...)
#endif


// draw a touchpad
class TouchpadPref
{
	public:
							TouchpadPref();
							~TouchpadPref();
				
		void				Revert();
		void				Defaults();
		
		BPoint 				WindowPosition(){return fWindowPosition;}
		void				SetWindowPosition(BPoint position){
								fWindowPosition = position;}
		
		touchpad_settings *	GetSettings(){return &fSettings;}
		status_t			UpdateSettings();
	private:
		status_t			GetSettingsPath(BPath &path);
		status_t			LoadSettings();
		status_t			SaveSettings();
		
		status_t			ConnectToTouchPad();
		
		bool 				fConnected;
		BInputDevice * 		fTouchPad;
		
		touchpad_settings	fSettings;
		touchpad_settings	fStartSettings;
		BPoint				fWindowPosition;
};


#endif