/********************************************************************************
/
/	File:		Joystick.h
/
/	Description:	Joystick port class header.
/
/	Copyright 1996-98, Be Incorporated, All Rights Reserved.
/
********************************************************************************/


#ifndef	_JOYSTICK_H
#define	_JOYSTICK_H

#include <BeBuild.h>
#include <SupportDefs.h>
#include <OS.h>

class BList;
class BString;
struct entry_ref;
struct _extended_joystick;
class _BJoystickTweaker;
struct _joystick_info;

/* -----------------------------------------------------------------------*/
class BJoystick {
public:
					BJoystick();
virtual				~BJoystick();

		status_t	Open(const char *portName);
		status_t	Open(const char *portName, bool enter_enhanced);
		void		Close(void);

		status_t	Update(void);
		status_t	SetMaxLatency(bigtime_t max_latency);

		bigtime_t	timestamp;
		int16		horizontal;
		int16		vertical;
		
		/* NOTE: true == off */
		bool		button1;
		bool		button2;

		int32		CountDevices();
		status_t	GetDeviceName(int32 n, char * name,
						size_t bufSize = B_OS_NAME_LENGTH);

		bool		EnterEnhancedMode(const entry_ref * ref = NULL);

		int32		CountSticks();

		status_t	GetControllerModule(BString * out_name);
		status_t	GetControllerName(BString * out_name);

		bool		IsCalibrationEnabled();
		status_t	EnableCalibration(bool calibrates = true);

		int32		CountAxes();
		status_t	GetAxisValues(int16 * out_values, int32 for_stick = 0);
		status_t	GetAxisNameAt(int32 index, BString * out_name);
		
		int32		CountHats();
		status_t	GetHatValues(uint8 * out_hats, int32 for_stick = 0);
		status_t	GetHatNameAt(int32 index, BString * out_name);
		
		int32		CountButtons();
		/* NOTE: Buttons() are 1 == on */
		uint32		ButtonValues(int32 for_stick = 0);
		status_t	GetButtonNameAt(int32 index, BString * out_name);

protected:

virtual	void		Calibrate(struct _extended_joystick * reading);

private:
friend class _BJoystickTweaker;

		void		ScanDevices(bool use_disabled = false);
		status_t	GatherEnhanced_info(const entry_ref * ref = NULL);
		status_t	SaveConfig(const entry_ref * ref = NULL);
		
		bool			fBeBoxMode;
		bool			fReservedBool;
		int				ffd;
		BList*			fDevices;
		_joystick_info* fJoystickInfo;
		char* 			fDevName;
#if DEBUG
public:
	static FILE *sLogFile;
#endif
};

#endif
