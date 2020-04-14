/*
 * Copyright 2008, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT license.
 */
#ifndef HYPER_TEXT_VIEW_H
#define HYPER_TEXT_VIEW_H

#include <TextView.h>


// TODO: The current implementation works correctly only for insertions at the
// end of the text. It doesn't keep track of any other insertions or deletions.


class HyperTextView;


class HyperTextAction {
public:
								HyperTextAction();
	virtual						~HyperTextAction();

	virtual	void				MouseOver(HyperTextView* view, BPoint where,
									int32 startOffset, int32 endOffset,
									BMessage* message);
	virtual	void				MouseAway(HyperTextView* view, BPoint where,
									int32 startOffset, int32 endOffset,
									BMessage* message);
	virtual	void				Clicked(HyperTextView* view, BPoint where,
									BMessage* message);
};


class HyperTextView : public BTextView {
public:
								HyperTextView(const char* name,
									uint32 flags = B_WILL_DRAW
										| B_PULSE_NEEDED);
								HyperTextView(BRect frame, const char* name,
									BRect textRect, uint32 resizeMask,
									uint32 flags = B_WILL_DRAW
										| B_PULSE_NEEDED);
	virtual						~HyperTextView();

	virtual	void				MouseDown(BPoint where);
	virtual	void				MouseUp(BPoint where);
	virtual	void				MouseMoved(BPoint where, uint32 transit,
									const BMessage* dragMessage);

			void				AddHyperTextAction(int32 startOffset,
									int32 endOffset, HyperTextAction* action);

			void				InsertHyperText(const char* inText,
									HyperTextAction* action,
									const text_run_array* inRuns = NULL);
			void				InsertHyperText(const char* inText,
									int32 inLength, HyperTextAction* action,
									const text_run_array* inRuns = NULL);
private:
			struct ActionInfo;
			class ActionInfoList;

			HyperTextAction*	_ActionAt(const BPoint& where) const;
			const ActionInfo*	_ActionInfoAt(const BPoint& where) const;

private:
			ActionInfoList*		fActionInfos;
			const ActionInfo*	fLastActionInfo;
};


#endif	// HYPER_TEXT_VIEW_H
