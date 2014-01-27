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
#ifndef PAIRS_BUTTON_H
#define PAIRS_BUTTON_H


#include <Button.h>


class PairsButton : public BButton {
public:
								PairsButton(int32 x, int32 y, int32 size,
									BMessage* message);
	virtual						~PairsButton();
};


#endif	// PAIRS_BUTTON_H
