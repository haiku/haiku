/*
 * Copyright 2016 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		John Scipione, jscipione@gmail.com
 */
#ifndef _RAINBOW_ITEM_H
#define _RAINBOW_ITEM_H


#include <StringItem.h>


class RainbowItem : public BStringItem {
public:
							RainbowItem(const char* string);

	virtual	void			DrawItem(BView* owner, BRect frame, bool complete);
};


#endif	// _RAINBOW_ITEM_H
