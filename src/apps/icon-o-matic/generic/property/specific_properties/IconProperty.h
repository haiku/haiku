/*
 * Copyright 2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#ifndef ICON_PROPERTY_H
#define ICON_PROPERTY_H

#include <GraphicsDefs.h>

#include "Property.h"

class IconProperty : public Property {
 public:
								IconProperty(uint32 identifier,
											 const uchar* icon,
											 uint32 width, uint32 height,
											 color_space format,
											 BMessage* message = NULL);
								IconProperty(const IconProperty& other);
								IconProperty(BMessage* archive);
	virtual						~IconProperty();

	// BArchivable interface
	virtual	status_t			Archive(BMessage* archive,
										bool deep = true) const;
	static	BArchivable*		Instantiate(BMessage* archive);

	// Property interface
	virtual	Property*			Clone() const;

	virtual	type_code			Type() const
									{ return B_RGB_COLOR_TYPE; }

	virtual	bool				SetValue(const char* value);
	virtual	bool				SetValue(const Property* other);
	virtual	void				GetValue(BString& string);

	virtual	bool				InterpolateTo(const Property* other,
											  float scale);
	// IconProperty
			BMessage*			Message() const
									{ return fMessage; }

			void				SetMessage(const BMessage* message);

			const uchar* 		Icon() const { return fIcon; }
			uint32 				Width() const { return fWidth; }
			uint32 				Height() const { return fHeight; }
			color_space 		Format() const { return fFormat; }
 private:
			BMessage*			fMessage;

			const uchar* 		fIcon;
			uint32				fWidth;
			uint32				fHeight;
			color_space			fFormat;
};


#endif // ICON_PROPERTY_H


