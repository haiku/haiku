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

			bool				HasChanged();
			void				Revert();
			void				Update();

			const BString&		Attribute() const
									{ return fAttribute; }

private:
			BString				fAttribute;
			BString				fOriginalValue;
};

#endif // ATTRIBUTE_TEXT_CONTROL_H
