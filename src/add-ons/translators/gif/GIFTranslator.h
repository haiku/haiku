////////////////////////////////////////////////////////////////////////////////
//
//	File: GIFTranslator.h
//
//	Date: December 1999
//
//	Author: Daniel Switkin
//
//	Copyright 2003 (c) by Daniel Switkin. This file is made publically available
//	under the BSD license, with the stipulations that this complete header must
//	remain at the top of the file indefinitely, and credit must be given to the
//	original author in any about box using this software.
//
////////////////////////////////////////////////////////////////////////////////

#ifndef GIFTRANSLATOR_H
#define GIFTRANSLATOR_H

#include <Application.h>
class GIFWindow;

class GIFTranslator : public BApplication {
	public:
		GIFTranslator();
		GIFWindow *gifwindow;
};

#endif

