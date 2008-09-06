/*
 * Copyright 2002-2006, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		DarkWyrm (darkwyrm@earthlink.net)
 */
#ifndef COLORWELL_H_
#define COLORWELL_H_

#include <View.h>
#include <Message.h>
#include <Invoker.h>

class ColorWell : public BView
{
public:
	ColorWell(BRect frame, BMessage *msg, 
		uint32 resizingMode = B_FOLLOW_LEFT | B_FOLLOW_TOP, 
		uint32 flags = B_WILL_DRAW);
	~ColorWell(void);
	void SetColor(rgb_color col);
	rgb_color Color(void) const;
	void SetColor(uint8 r,uint8 g, uint8 b);
	virtual void MessageReceived(BMessage *msg);
	virtual void Draw(BRect update);
	virtual void SetTarget(BHandler *tgt);
	virtual void SetEnabled(bool value);
	void SetMode(bool is_rectangle);
protected:
	BInvoker *invoker;	
	bool is_enabled, is_rect;
	rgb_color disabledcol, currentcol;
};

#endif
