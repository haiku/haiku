/*
 * Copyright 2005-2011, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Robert Polic
 *
 * Copyright 1999, Be Incorporated.   All Rights Reserved.
 * This file may be used under the terms of the Be Sample Code License.
 */
#ifndef ATTRIBUTE_TEXT_CONTROL_H
#define ATTRIBUTE_TEXT_CONTROL_H

#include <String.h>
#include <TextControl.h>


class AttributeTextControl : public BTextControl {
public:
								AttributeTextControl(const char* label,
									const char* attribute);
	virtual						~AttributeTextControl();

	virtual	void				MouseDown(BPoint);
	virtual	void				MouseMoved(BPoint, uint32, const BMessage*);

			bool				HasChanged();
			void				Revert();
			void				Update();

			const BString&		Attribute() const
									{ return fAttribute; }

private:
			const BString&		_MakeUniformUrl(BString &url) const;
			const BString&		_BuildMimeString(BString &mimeType,
									const BString &url) const;

			bool				_ContainsUrl() const;

			BRect				_VisibleLabelBounds() const;
			void				_HandleLabelClicked(const char*);

private:
			BString				fAttribute;
			BString				fOriginalValue;
};

#endif // ATTRIBUTE_TEXT_CONTROL_H
