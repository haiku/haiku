/*
 * Copyright 2003-2007, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Philippe Houdoin
 *		Simon Gauvin	
 *		Michael Pfeiffer
 *		Hartmut Reh
 */

#ifndef PAGESETUPWINDOW_H
#define PAGESETUPWINDOW_H

#include <InterfaceKit.h>
#include <Message.h>
#include <Messenger.h>
#include <File.h>
#include <FindDirectory.h>
#include <Path.h>
#include <String.h>
#include "InterfaceUtils.h"
#include "Utils.h"

class MarginView;

class PageSetupWindow : public BlockingWindow 
{
public:
	// Constructors, destructors, operators...

							PageSetupWindow(BMessage *msg, const char *printerName = NULL);

	typedef BlockingWindow 		inherited;

	// public constantes
	enum {
		OK_MSG				= 'ok__',
		CANCEL_MSG			= 'cncl',
	};
			
	// Virtual function overrides
public:	
	virtual void 			MessageReceived(BMessage *msg);

	// From here, it's none of your business! ;-)
private:
	BMessage *				fSetupMsg;
	BMenuField *			fPageSizeMenu;
	BMenuField *			fOrientationMenu;
	BTextControl *          fScaleControl;
		
	void					UpdateSetupMessage();

	MarginView * 			fMarginView;
	
	// used for saving settings 
	BString					fPrinterDirName;

	//private class constants
	static const int kMargin = 10;
	static const int kOffset = 200;
};

#endif
