/*
 * Copyright 2006 Haiku, Inc. All Rights Reserved.
 * Copyright 1997, 1998 R3 Software Ltd. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 *		John Scipione <jscipione@gmail.com>
 *		Timothy Wayper <timmy@wunderbear.com>
 */
#ifndef _CALC_APPLICATION_H
#define _CALC_APPLICATION_H


#include <Application.h>


extern const char* kAppSig;

class BFile;
class CalcWindow;

class CalcApplication : public BApplication {
 public:
								CalcApplication();
	virtual						~CalcApplication();

	virtual	void				ReadyToRun();
	virtual	void				AboutRequested();
	virtual	bool				QuitRequested();

 private:
			void				_LoadSettings(BMessage& settings);
			void				_SaveSettings();
			status_t			_InitSettingsFile(BFile* file, bool write);

			CalcWindow*			fCalcWindow;
};

#endif // _CALC_APPLICATION_H
