/*
Open Tracker License

Terms and Conditions

Copyright (c) 1991-2001, Be Incorporated. All rights reserved.

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:

The above copyright notice and this permission notice applies to all licensees
and shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF TITLE, MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
BE INCORPORATED BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF, OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of Be Incorporated shall not be
used in advertising or otherwise to promote the sale, use or other dealings in
this Software without prior written authorization from Be Incorporated.

BeMail(TM), Tracker(TM), Be(R), BeOS(R), and BeIA(TM) are trademarks or registered trademarks
of Be Incorporated in the United States and other countries. Other brand product
names are registered trademarks or trademarks of their respective holders.
All rights reserved.
*/

//
// ComboBox.h
//
//  A view that is the combination of a text control and a pop-up list
//
//

#ifndef _COMBOBOX_H
#define _COMBOBOX_H

#include <Control.h>
#include <View.h>

class BButton;
class BComboBox;
class BList;
class BTextControl;
class BTextView;
class BWindow;
struct text_run_array;

typedef const char* (*text_input_filter_hook)(const char* inText, int32& length,
	const text_run_array*& runs);

/*
// Abstract class provides an interface for BComboBox to access possible choices.
// Choices are used for auto-completion and for showing the pop-up list
class BChoiceList {
public:
	// Returns the choice at index or NULL if the index is invalid
	virtual const char *ChoiceAt(int32 index) = 0;

	// Looks for a match at or after startIndex which contains a choice
	// that starts with prefix.  If a match is found, B_OK is returned,
	// matchIndex is set to the list index that should be selected, and completionText
	// is set to point at the text that should be appended to the text input.
	// If no match is found, a negative value is returned.
	virtual status_t GetMatch(const char *prefix, int32 startIndex,
		int32 *matchIndex, const char **completionText) = 0;

	// Returns the number of choices
	virtual int32 CountChoices() = 0;
};
*/

class StringObjectList;

// Implementation of BChoiceList.  Keeps copies of each choice added, and frees
// the memory when the choices are removed. 
class BDefaultChoiceList // : public BChoiceList
{
public:
	BDefaultChoiceList(BComboBox *owner = NULL);
	virtual ~BDefaultChoiceList();

	virtual const char *ChoiceAt(int32 index);
	virtual status_t GetMatch(const char *prefix, int32 startIndex,
		int32 *matchIndex, const char **completionText);
	virtual int32 CountChoices();

	status_t AddChoice(const char *toAdd);
	status_t AddChoiceAt(const char *toAdd, int32 index);
	status_t RemoveChoice(const char *toRemove);
	status_t RemoveChoiceAt(int32 index);

	void SetOwner(BComboBox *owner);
	BComboBox *Owner();
	
private:
	StringObjectList *fList;
	BComboBox *fOwner;
};
typedef BDefaultChoiceList BChoiceList;


class BComboBox : public BControl {
public:
	BComboBox(BRect frame, const char *name, const char *label,
		BMessage *message, uint32 resizeMask = B_FOLLOW_LEFT | B_FOLLOW_TOP,
		uint32 flags = B_WILL_DRAW | B_FRAME_EVENTS | B_NAVIGABLE); 
//	BComboBox(BMessage *data);
	virtual ~BComboBox();

	// BArchivable methods
//	static BArchivable *Instantiate(BMessage *data);
//	virtual status_t Archive(BMessage *data, bool deep = true) const;

	// SetChoiceList causes the BComboBox to delete the old BChoiceList object,
	// take ownership of the new choice list, and then invalidate the pop-up list
	void SetChoiceList(BChoiceList *list);

	// ChoiceList returns a pointer to the current choice list
	BChoiceList *ChoiceList();

	// ChoiceListUpdated should be called whenever an item in the choice list changes
	// so that BComboBox can perform the proper updating.
	virtual void ChoiceListUpdated();

	// Select changes the list selection to the specified index, and if the
	// changeTextSelection flag is true, changes the text in the TextView to
	// the value at index and selects all the text in the TextView.
	virtual void Select(int32 index, bool changeTextSelection = false);

	virtual void Deselect();

	// Returns the index of the current selection, or a negative value
	int32 CurrentSelection();
	
	// SetAutoComplete enables or disables auto-completion
	virtual void SetAutoComplete(bool on);
	bool GetAutoComplete();

	// The following methods are mostly identical to their BTextControl counterparts
	virtual	void SetValue(int32 value);
	virtual void SetEnabled(bool enabled);
	virtual	void SetLabel(const char *text);
	virtual void SetText(const char *text);
	const char *Text() const;
	BTextView *TextView();		
	virtual	void SetDivider(float dividing_line);
	float Divider() const;
	virtual void SetAlignment(alignment label, alignment text);
	void GetAlignment(alignment *label, alignment *text) const;

	virtual void SetModificationMessage(BMessage *message);
    BMessage *ModificationMessage() const;

	void SetFilter(text_input_filter_hook hook);

	virtual	void GetPreferredSize(float *width, float *height);
	virtual void ResizeToPreferred();
	virtual	void FrameMoved(BPoint new_position);
	virtual	void FrameResized(float new_width, float new_height);
	virtual	void WindowActivated(bool active);
	virtual void MakeFocus(bool state);

	virtual void Draw(BRect update);
	virtual void MessageReceived(BMessage *msg);
	virtual void MouseDown(BPoint where);
	virtual void MouseUp(BPoint where);
	virtual void MouseMoved(BPoint where, uint32 transit,
		const BMessage *dragMessage);
//	virtual void AllAttached();
	virtual void AttachedToWindow();
	virtual void DetachedFromWindow();
	virtual void SetFlags(uint32 flags);
//	virtual void SetFont(const BFont *font, uint32 properties = B_FONT_ALL);
	
	virtual	status_t Invoke(BMessage *msg = NULL);

//	virtual BHandler *ResolveSpecifier(BMessage *msg, int32 index,
//		BMessage *specifier, int32 form, const char *property);
//	virtual status_t GetSupportedSuites(BMessage *data);
//
//
//	virtual status_t Perform(perform_code d, void *arg);

private:
	class ComboBoxWindow;
	class ChoiceListView;
	class TextInput;
	class MovedMessageFilter;
	
protected:
	ComboBoxWindow *CreatePopupWindow();
	void CommitValue();
	void TryAutoComplete();
	void ShowPopupWindow();	
	void HidePopupWindow();
	
	BRect fButtonRect;
	int32 fSelected;
	int32 fCompletionIndex;
	float fDivider;
	TextInput *fText;
	ComboBoxWindow *fPopupWindow;		
	BMessage *fModificationMessage;
	BChoiceList *fChoiceList;
	alignment fLabelAlign;
	bool fAutoComplete;
	bool fButtonDepressed;
	bool fDepressedWhenClicked;
	bool fTrackingButtonDown;
		
/*----- Private or reserved -----------------------------------------*/	
private:
	BRect fFrameCache;
	MovedMessageFilter *fWinMovedFilter;
	int32 fTextEnd;
	bool fSkipSetFlags;
		
	friend class ChoiceListView;
	friend class ComboBoxWindow;
	friend class TextInput;
};

#endif // #ifndef _COMBOBOX_H

