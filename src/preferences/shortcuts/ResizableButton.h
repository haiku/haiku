/*
 * Copyright 1999-2009 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Jeremy Friesner
 */
#ifndef ResizableButton_h
#define ResizableButton_h


#include <Button.h>


class ResizableButton : public BButton {
public:
							ResizableButton(BRect parentFrame, BRect frame, 
								const char* name, const char* label, 
								BMessage* message);

	virtual	void			ChangeToNewSize(float newWidth, float newHeight);
private:
			BRect			fPercentages;
};


#endif

