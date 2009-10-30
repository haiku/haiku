/*
 * Copyright 1999-2009 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Jeremy Friesner
 */


#ifndef ResizableButton_h
#define ResizableButton_h

#include <Message.h>
#include <Rect.h>
#include <View.h>
#include <Button.h>

//Just like a regular BButton, but with a handy resize method added
class ResizableButton : public BButton {
public:
						ResizableButton(BRect parentFrame, BRect frame, 
							const char* name, const char* label, 
							BMessage* msg);

	virtual	void		ChangeToNewSize(float newWidth, float newHeight);
private:
			BRect 		fPercentages;
};

#endif
