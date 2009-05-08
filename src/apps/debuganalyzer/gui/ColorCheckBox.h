/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef COLOR_CHECK_BOX_H
#define COLOR_CHECK_BOX_H

#include <CheckBox.h>
#include <GroupView.h>


class BCheckBox;


#include <SpaceLayoutItem.h>

class ColorCheckBox : public BGroupView {
public:
								ColorCheckBox(const char* label,
									const rgb_color& color,
									BMessage* message = NULL);

			BCheckBox*			CheckBox() const;

			void				SetTarget(const BMessenger& target);

	virtual	void				Draw(BRect updateRect);

private:
			BCheckBox*			fCheckBox;
			rgb_color			fColor;
};


#endif	// COLOR_CHECK_BOX_H
