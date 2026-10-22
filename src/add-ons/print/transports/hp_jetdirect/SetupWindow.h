/*
 * Copyright 2001-2026, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *      Philippe Houdoin
 */

#ifndef SETUPWINDOW_H
#define SETUPWINDOW_H

#include <Window.h>

class BDirectory;
class BTextControl;


extern const char* kOKLabel;
extern const char* kCancelLabel;


class SetupWindow : public BWindow {
public:
					SetupWindow(BDirectory* printerDirectory);

	virtual bool 	QuitRequested();
	virtual void 	MessageReceived(BMessage* message);
	status_t 		Go();

private:
	bool			_CheckSetup();

	status_t		fResult;
	sem_id 			fExitSem;

	BTextControl*	fServerAddress;
	BTextControl*	fQueuePort;
	BDirectory*		fPrinterDirectory;
};

#endif	// SETUPWINDOW_H
