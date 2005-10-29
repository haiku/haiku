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
struct _extended_joystick;
class _IMPEXP_DEVICE _BJoystickTweaker;

/* -----------------------------------------------------------------------*/
class BJoystick {

public:
					BJoystick();
virtual				~BJoystick();

		status_t	Open(const char *portName);	/* always goes for enhanced mode */
		status_t	Open(const char *portName, bool enter_enhanced);
		void		Close(void);

		status_t	Update(void);
		status_t	SetMaxLatency(
						bigtime_t max_latency);

		bigtime_t	timestamp;
		int16		horizontal;
		int16		vertical;
		bool		button1;	/* NOTE: true == off */
		bool		button2;

		int32		CountDevices();
		status_t	GetDeviceName(int32 n, char * name, 
						size_t bufSize = B_OS_NAME_LENGTH);

		/* if you care about more than just the first two axes/buttons, here's where you go */
		bool		EnterEnhancedMode(
						const entry_ref * ref = NULL);
		int32		CountSticks();
		int32		CountAxes();
		int32		CountHats();
		int32		CountButtons();
		status_t	GetAxisValues(
						int16 * out_values,
						int32 for_stick = 0);	/* CountAxes() elements */
		status_t	GetHatValues(
						uint8 * out_hats,
						int32 for_stick = 0);
		uint32		ButtonValues(
						int32 for_stick = 0);		/* NOTE: Buttons() are 1 == on */
		status_t	GetAxisNameAt(
						int32 index,
						BString * out_name);
		status_t	GetHatNameAt(
						int32 index,
						BString * out_name);
		status_t	GetButtonNameAt(
						int32 index,
						BString * out_name);
		status_t	GetControllerModule(
						BString * out_name);
		status_t	GetControllerName(
						BString * out_name);

		bool			IsCalibrationEnabled();
		status_t	EnableCalibration(
						bool calibrates = true);

/* -----------------------------------------------------------------------*/

protected:

virtual	void			Calibrate(
						struct _extended_joystick * reading);

private:
friend class _BJoystickTweaker;

		struct _joystick_info;

		void		ScanDevices(
						bool use_disabled = false);
		status_t	gather_enhanced_info(
						const entry_ref * ref = NULL);
		status_t	save_config(
						const entry_ref * ref = NULL);

		void		_ReservedJoystick1();
virtual	void		_ReservedJoystick2();
virtual	void		_ReservedJoystick3();

		bool		_mBeBoxMode;
		bool		_mReservedBool;
		int			ffd;
		BList *		_fDevices;
		_joystick_info * m_info;
		char * m_dev_name;
#if !_PR3_COMPATIBLE_
virtual	status_t	_Reserved_Joystick_4(void *, ...);
virtual	status_t	_Reserved_Joystick_5(void *, ...);
virtual	status_t	_Reserved_Joystick_6(void *, ...);
		uint32		_reserved_Joystick_[10];
#endif
};

#endif
