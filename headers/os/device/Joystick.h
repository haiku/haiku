/*
 * Copyright 2009, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef	_JOYSTICK_H
#define	_JOYSTICK_H


#include <OS.h>
#include <SupportDefs.h>


class BList;
class BString;
class _BJoystickTweaker;
struct entry_ref;
struct _extended_joystick;
struct _joystick_info;


class BJoystick {
public:
							BJoystick();
	virtual					~BJoystick();

			status_t		Open(const char* portName);
			status_t		Open(const char* portName, bool enhanced);
			void			Close();

			status_t		Update();
			status_t		SetMaxLatency(bigtime_t maxLatency);

			bigtime_t		timestamp;
			int16			horizontal;
			int16			vertical;

			bool			button1;
			bool			button2;

			int32			CountDevices();
			status_t		GetDeviceName(int32 index, char* name,
								size_t bufSize = B_OS_NAME_LENGTH);

			status_t		RescanDevices();
								// Haiku extension. Updates the list of devices
								// as enumerated by CountDevices() and
								// GetDeviceName() with possibly newly plugged
								// in devices.

			bool			EnterEnhancedMode(const entry_ref* ref = NULL);

			int32			CountSticks();

			status_t		GetControllerModule(BString* outName);
			status_t		GetControllerName(BString* outName);

			bool			IsCalibrationEnabled();
			status_t		EnableCalibration(bool calibrates = true);

			int32			CountAxes();
			status_t		GetAxisValues(int16* outValues,
								int32 forStick = 0);
			status_t		GetAxisNameAt(int32 index,
								BString* outName);

			int32			CountHats();
			status_t		GetHatValues(uint8* outHats,
								int32 forStick = 0);
			status_t		GetHatNameAt(int32 index, BString* outName);

			int32			CountButtons();

			uint32			ButtonValues(int32 forStick = 0);
			status_t		GetButtonNameAt(int32 index,
								BString* outName);

protected:
	virtual	void			Calibrate(struct _extended_joystick*);

private:
friend class _BJoystickTweaker;

			void			ScanDevices(bool useDisabled = false);
			status_t		GatherEnhanced_info(const entry_ref* ref = NULL);
			status_t		SaveConfig(const entry_ref* ref = NULL);

			void            _ReservedJoystick1();
	virtual void            _ReservedJoystick2();
	virtual void            _ReservedJoystick3();
	virtual status_t        _Reserved_Joystick_4(void *, ...);
	virtual status_t        _Reserved_Joystick_5(void *, ...);
	virtual status_t        _Reserved_Joystick_6(void *, ...);

			bool			fBeBoxMode;
			bool			fReservedBool;
			int				fFD;
			BList*			fDevices;
			_joystick_info*	fJoystickInfo;
			BList*			fExtendedJoystick;
			uint32          _reserved_Joystick_[10];
};

#endif // _JOYSTICK_H
