/*
 * Copyright 2011, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Philippe Houdoin
 */
#ifndef _PRINTERS_TESTPAGEVIEW_H
#define _PRINTERS_TESTPAGEVIEW_H


#include <View.h>

class PrinterItem;

class TestPageView : public BView {
public:
							TestPageView(BRect rect, PrinterItem* printer);

		void				AttachedToWindow();
		void				DrawAfterChildren(BRect rect);

private:
		PrinterItem*		fPrinter;
};

#endif
