/*
 * Copyright 2008 Ralf Schülke, ralf.schuelke@googlemail.com.
 * Copyright 2014 Haiku, Inc. All rights reserved.
 *
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Ralf Schülke, ralf.schuelke@googlemail.com
 *		John Scipione, jscipione@gmail.com
 */


#include "PairsButton.h"


//	#pragma mark - PairsButton


PairsButton::PairsButton(int32 x, int32 y, int32 size, BMessage* message)
	:
	BButton(BRect(x, y, x + size, y + size), "pairs button", "?", message)
{
	SetFontSize(size - 15);
}


PairsButton::~PairsButton()
{
}
