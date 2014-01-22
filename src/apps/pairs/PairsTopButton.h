/*
 * Copyright 2008 Ralf Schülke, ralf.schuelke@googlemail.com.
 * Copyright 2014 Haiku, Inc. All rights reserved.
 *
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		John Scipione, jscipione@gmail.com
 */
#ifndef PAIRS_TOP_BUTTON_H
#define PAIRS_TOP_BUTTON_H

#include <OS.h>

class BButton;

class TopButton : public BButton {
public:
			TopButton(int x, int y, BMessage* message);
			virtual ~TopButton();
};

#endif	// PAIRS_TOP_BUTTON_H
