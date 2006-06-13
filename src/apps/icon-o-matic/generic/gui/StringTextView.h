/*
 * Copyright 2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#ifndef STRING_TEXT_VIEW_H
#define STRING_TEXT_VIEW_H

#include "InputTextView.h"

#include <String.h>

class StringTextView : public InputTextView {
 public:
								StringTextView(BRect frame,
											   const char* name,
											   BRect textRect,
											   uint32 resizingMode,
											   uint32 flags);
	virtual						~StringTextView();

	// BInvoker interface
	virtual	status_t			Invoke(BMessage* message = NULL);

	// InputTextView interface
	virtual	void				RevertChanges();
	virtual	void				ApplyChanges();

	// StringTextView
			void				SetValue(const char* string);
			const char*			Value() const;

protected:
	mutable	BString				fStringCache;
};

#endif // STRING_TEXT_VIEW_H


