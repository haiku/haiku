/*
 * Copyright 2006 Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus, superstippi@gmx.de
 */
#ifndef EXPRESSION_TEXT_VIEW_H
#define EXPRESSION_TEXT_VIEW_H


#include <List.h>
#include <String.h>

#include "InputTextView.h"


class CalcView;

class ExpressionTextView : public InputTextView {
 public:
								ExpressionTextView(BRect frame,
									CalcView* calcView);
	virtual						~ExpressionTextView();

	// BView
	virtual	void				MakeFocus(bool focused = true);
	virtual	void				KeyDown(const char* bytes, int32 numBytes);

	virtual	void				MouseDown(BPoint where);

	// TextView
	virtual	void				GetDragParameters(BMessage* dragMessage,
									BBitmap** bitmap, BPoint* point,
									BHandler** handler);
	virtual	void 				SetTextRect(BRect rect);

	// InputTextView
	virtual	void				RevertChanges();
	virtual	void				ApplyChanges();

	// ExpressionTextView
			void				AddKeypadLabel(const char* label);

			void				SetExpression(const char* expression);
			void				SetValue(BString value);

			void				BackSpace();
			void				Clear();

			void				AddExpressionToHistory(const char* expression);
			void				PreviousExpression();
			void				NextExpression();

			void				LoadSettings(const BMessage* archive);
			status_t			SaveSettings(BMessage* archive) const;

 private:
			CalcView*			fCalcView;
			BString				fKeypadLabels;

			BList				fPreviousExpressions;
			int32				fHistoryPos;
			BString				fCurrentExpression;
			BString				fCurrentValue;

			bool				fChangesApplied;
};

#endif // EXPRESSION_TEXT_VIEW_H
