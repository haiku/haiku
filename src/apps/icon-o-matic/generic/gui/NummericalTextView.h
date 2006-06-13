/*
 * Copyright 2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#ifndef NUMMERICAL_TEXT_VIEW_H
#define NUMMERICAL_TEXT_VIEW_H

#include "InputTextView.h"

class NummericalTextView : public InputTextView {
 public:
								NummericalTextView(BRect frame,
												   const char* name,
												   BRect textRect,
												   uint32 resizingMode,
												   uint32 flags);
	virtual						~NummericalTextView();

	// BInvoker interface
	virtual	status_t			Invoke(BMessage* message = NULL);

	// InputTextView interface
	virtual	void				RevertChanges();
	virtual	void				ApplyChanges();

	// NummericalTextView
			void				SetFloatMode(bool floatingPoint);

			void				SetValue(int32 value);
			void				SetValue(float value);
			int32				IntValue() const;
			float				FloatValue() const;

protected:
								// BTextView
	virtual	void				Select(int32 start, int32 finish);

	virtual	void				InsertText(const char* inText,
										   int32 inLength,
										   int32 inOffset,
										   const text_run_array* inRuns);
	virtual	void				DeleteText(int32 fromOffset,
										   int32 toOffset);

								// NummericalTextView
			void				_ToggleAllowChar(char c);
			void				_CheckMinusAllowed();
			void				_CheckDotAllowed();

			bool				fFloatMode;

	mutable	int32				fIntValueCache;
	mutable	float				fFloatValueCache;
};

#endif // NUMMERICAL_TEXT_VIEW_H


