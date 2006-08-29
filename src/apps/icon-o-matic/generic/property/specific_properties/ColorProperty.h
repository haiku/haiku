/*
 * Copyright 2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#ifndef COLOR_PROPERTY_H
#define COLOR_PROPERTY_H

#include <GraphicsDefs.h>

#include "Property.h"

class ColorProperty : public Property {
 public:
								ColorProperty(uint32 identifier);
								ColorProperty(uint32 identifier,
											  rgb_color color);
								ColorProperty(const ColorProperty& other);
								ColorProperty(BMessage* archive);

	virtual						~ColorProperty();

	virtual	status_t			Archive(BMessage* archive,
										bool deep = true) const;
	static	BArchivable*		Instantiate(BMessage* archive);

	virtual	Property*			Clone() const;

	virtual	type_code			Type() const
									{ return B_RGB_COLOR_TYPE; }

	virtual	bool				SetValue(const char* value);
	virtual	bool				SetValue(const Property* other);
	virtual	void				GetValue(BString& string);

	virtual	bool				InterpolateTo(const Property* other,
											  float scale);

	// ColorProperty
			bool				SetValue(rgb_color color);
			rgb_color			Value() const;

 private:
			rgb_color			fValue;
};


#endif // COLOR_PROPERTY_H


