/*
 * Copyright 2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#ifndef OPTION_PROPERTY_H
#define OPTION_PROPERTY_H

#include <List.h>
#include <String.h>

#include "Property.h"

class OptionProperty : public Property {
 public:
								OptionProperty(uint32 identifier);
								OptionProperty(const OptionProperty& other);
								OptionProperty(BMessage* archive);
	virtual						~OptionProperty();

	// Property interface
	virtual	status_t			Archive(BMessage* archive,
										bool deep = true) const;
	static	BArchivable*		Instantiate(BMessage* archive);

	virtual	Property*			Clone() const;

	virtual	type_code			Type() const;

	virtual	bool				SetValue(const char* value);
	virtual	bool				SetValue(const Property* other);
	virtual	void				GetValue(BString& string);

	// animation
	virtual	bool				MakeAnimatable(bool animatable = true);


	// OptionProperty
			void				AddOption(int32 id, const char* name);

			int32				CurrentOptionID() const;
			bool				SetCurrentOptionID(int32 id);

			bool				GetOption(int32 index, BString* string, int32* id) const;
			bool				GetCurrentOption(BString* string) const;

			bool				SetOptionAtOffset(int32 indexOffset);

 private:

	struct option {
		int32		id;
		BString		name;
	};

			BList				fOptions;
			int32				fCurrentOptionID;
};


#endif // OPTION_PROPERTY_H


