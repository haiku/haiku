//-----------------------------------------------------------------------------
//	Copyright (c) 2003 Stefano Ceccherini
//
//	Permission is hereby granted, free of charge, to any person obtaining a
//	copy of this software and associated documentation files (the "Software"),
//	to deal in the Software without restriction, including without limitation
//	the rights to use, copy, modify, merge, publish, distribute, sublicense,
//	and/or sell copies of the Software, and to permit persons to whom the
//	Software is furnished to do so, subject to the following conditions:
//
//	The above copyright notice and this permission notice shall be included in
//	all copies or substantial portions of the Software.
//
//	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
//	FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
//	DEALINGS IN THE SOFTWARE.
//
//	File Name:		OptionPopUp.cpp
//	Description:	An option like control.  
//------------------------------------------------------------------------------
#include <MenuField.h>
#include <MenuItem.h>
#include <OptionPopUp.h>
#include <PopUpMenu.h>

#include <cstdio>

// If enabled, behaves like in BeOS R5, in that when you call
// SelectOptionFor() or SetValue(), the selected item isn't marked, and
// so SelectedOption() will return -1. This is broken, IMHO.
#define BEHAVE_LIKE_R5 0

const float kLabelSpace = 8.0;
const float kWidthModifier = 25.0;
const float kHeightModifier = 10.0;
	

/*! \brief Creates and initializes a BOptionPopUp.
	\param frame The frame of the control.
	\param name The name of the control.
	\param label The label which will be displayed by the control.
	\param message The message which the control will send when operated.
	\param resize Resizing flags. They will be passed to the base class.
	\param flags View flags. They will be passed to the base class.
*/	
BOptionPopUp::BOptionPopUp(BRect frame, const char *name, const char *label,
							BMessage *message, uint32 resize, uint32 flags)
	:
	BOptionControl(frame, name, label, message, resize, flags)
{
	BPopUpMenu *popUp = new BPopUpMenu(label, true, true);
	_mField = new BMenuField(Bounds(), "_menu", label, popUp);
	AddChild(_mField);
}


/*! \brief Creates and initializes a BOptionPopUp.
	\param frame The frame of the control.
	\param name The name of the control.
	\param label The label which will be displayed by the control.
	\param message The message which the control will send when operated.
	\param fixed It's passed to the BMenuField constructor. If it's true, 
		the BMenuField size will never change.
	\param resize Resizing flags. They will be passed to the base class.
	\param flags View flags. They will be passed to the base class.
*/
BOptionPopUp::BOptionPopUp(BRect frame, const char *name, const char *label, 
						   BMessage *message, bool fixed, uint32 resize, uint32 flags)
	:
	BOptionControl(frame, name, label, message, resize, flags)
{
	BPopUpMenu *popUp = new BPopUpMenu(label, true, true);
	_mField = new BMenuField(Bounds(), "_menu", label, popUp, fixed);
	AddChild(_mField);
}


/*! \brief Frees the allocated resources.
	It does nothing.
*/
BOptionPopUp::~BOptionPopUp()
{
}


/*! \brief Returns a pointer to the BMenuField used internally.
	\return A Pointer to the BMenuField which the class uses internally.
*/
BMenuField *
BOptionPopUp::MenuField()
{
	return _mField;
}


/*! \brief Gets the option at the given index.
	\param index The option's index.
	\param outName A pointer to a string which will held the option's name,
		as soon as the function returns.
	\param outValue A pointer to an integer which will held the option's value,
		as soon as the funciton returns.
	\return \c true if The wanted option was found,
			\c false otherwise.
*/ 
bool
BOptionPopUp::GetOptionAt(int32 index, const char **outName, int32 *outValue)
{
	bool result = false;
	BMenu *menu = _mField->Menu();
	
	if (menu != NULL) {
		BMenuItem *item = menu->ItemAt(index);
		if (item != NULL) {
			if (outName != NULL)
				*outName = item->Label();
			if (outValue != NULL)
				item->Message()->FindInt32("be:value", outValue);

			result = true;
		}
	}
	return result;
}


/*! \brief Removes the option at the given index.
	\param index The index of the option to remove.
*/
void
BOptionPopUp::RemoveOptionAt(int32 index)
{
	BMenu *menu = _mField->Menu();
	if (menu != NULL) {
		BMenuItem *item = menu->ItemAt(index);
		if (item != NULL) {
			menu->RemoveItem(item);
			delete item;
		}
	}		
}


/*! \brief Returns the amount of "Options" (entries) contained in the control.
*/
int32
BOptionPopUp::CountOptions() const
{
	BMenu *menu = _mField->Menu();	
	return (menu != NULL) ? menu->CountItems() : 0;
}


/*! \brief Adds an option to the control, at the given position.
	\param name The name of the option to add.
	\param value The value of the option.
	\param index The index which the new option will have in the control.
	\return \c B_OK if the option was added succesfully,
		\c B_BAD_VALUE if the given index was invalid.
		\c B_ERROR if something else happened.
*/
status_t
BOptionPopUp::AddOptionAt(const char *name, int32 value, int32 index)
{
	BMenu *menu = _mField->Menu();
	if (menu != NULL) {
		int32 numItems = menu->CountItems();
		if (index < 0 || index > numItems)
			return B_BAD_VALUE;
		
		BMessage *message = MakeValueMessage(value);
		if (message == NULL)
			return B_ERROR;	
		
		BMenuItem *newItem = new BMenuItem(name, message);
		menu->AddItem(newItem, index);
		
		// We didnt' have any items before, so select the newly added one
		if (numItems == 0)
			SetValue(value);
		return B_OK;
	}

	return B_ERROR;
}


/*! \brief Called to take special actions when the child views are attached.
	It's used to set correctly the divider for the BMenuField.
*/
void
BOptionPopUp::AllAttached()
{
	BMenu *menu = _mField->Menu();
	if (menu != NULL) {
		float labelWidth = _mField->StringWidth(_mField->Label());
		_mField->SetDivider(labelWidth + kLabelSpace);
	}
}


void
BOptionPopUp::MessageReceived(BMessage *message)
{
	BOptionControl::MessageReceived(message);
}


/*! \brief Set the label of the control.
	\param text The new label of the control.
*/
void
BOptionPopUp::SetLabel(const char *text)
{
	BControl::SetLabel(text);
	_mField->SetLabel(text);
	// We are not sure the menu can keep the whole
	// string as label, so we ask it what label it's got
	float newWidth = _mField->StringWidth(_mField->Label());
	_mField->SetDivider(newWidth + kLabelSpace);
}


/*! \brief Set the control's value.
	\param value The new value of the control.
	Selects the option which has the given value.
*/
void
BOptionPopUp::SetValue(int32 value)
{
	BControl::SetValue(value);
	BMenu *menu = _mField->Menu();
	int32 numItems = menu->CountItems();
	for (int32 i = 0; i < numItems; i++) {
		BMenuItem *item = menu->ItemAt(i);
		if (item && item->Message()) {
			int32 val;
			item->Message()->FindInt32("be:value", &val);
			if (val == value) {
				item->SetMarked(true);

#if BEHAVE_LIKE_R5
				item->SetMarked(false);
#endif

				break;
			}
		}
	}
}


/*! \brief Enables or disables the control.
	\param state The new control's state.
*/
void
BOptionPopUp::SetEnabled(bool state)
{
	BOptionControl::SetEnabled(state);
}


/*! \brief Gets the preferred size for the control.
	\param width A pointer to a float which will held the control's
		preferred width.
	\param height A pointer to a float which will held the control's
		preferred height.
*/
void
BOptionPopUp::GetPreferredSize(float *width, float *height)
{
	font_height fontHeight;
	_mField->GetFontHeight(&fontHeight);		
	
	if (height != NULL)
		*height = fontHeight.ascent + fontHeight.descent +
					fontHeight.leading + kHeightModifier;
	
	float maxWidth = 0;
	BMenu *menu = _mField->Menu();
	if (menu == NULL)
		return;
		
	int32 numItems = menu->CountItems();	
	for (int32 i = 0; i < numItems; i++) {
		BMenuItem *item = menu->ItemAt(i);
		if (item != NULL) {		
			float stringWidth = menu->StringWidth(item->Label());
			maxWidth = max_c(maxWidth, stringWidth);
		}	
	}
	
	maxWidth += _mField->StringWidth(BControl::Label()) + kLabelSpace + kWidthModifier;
	if (width != NULL)
		*width = maxWidth;
}


/*! \brief Resizes the control to its preferred size.
*/
void
BOptionPopUp::ResizeToPreferred()
{
	// TODO: Some more work is needed either here or in GetPreferredSize(),
	// since the control doesnt' always resize as it should.
	float width, height;
	GetPreferredSize(&width, &height);
	ResizeTo(width, height);
	
	float newWidth = _mField->StringWidth(BControl::Label());
	_mField->SetDivider(newWidth + kLabelSpace);
}


/*! \brief Gets the currently selected option.
	\param outName A pointer to a string which will held the option's name.
	\param outValue A pointer to an integer which will held the option's value.
	\return The index of the selected option.
*/
int32
BOptionPopUp::SelectedOption(const char **outName, int32 *outValue) const
{
	BMenu *menu = _mField->Menu();
	if (menu != NULL) {
		BMenuItem *marked = menu->FindMarked();
		if (marked != NULL) {
			if (outName != NULL)
				*outName = marked->Label();
			if (outValue != NULL)
				*outValue = marked->Message()->FindInt32("be:value", outValue);
			
			return menu->IndexOf(marked);
		}
	}
	
	return B_ERROR;
}


// Private Unimplemented
BOptionPopUp::BOptionPopUp()
	:
	BOptionControl(BRect(), "", "", NULL)
{
}


BOptionPopUp::BOptionPopUp(const BOptionPopUp &clone)
	:
	BOptionControl(clone.Frame(), "", "", clone.Message())
{
}


BOptionPopUp &
BOptionPopUp::operator=(const BOptionPopUp & clone)
{
		return *this;
}


// FBC Stuff
status_t BOptionPopUp::_Reserved_OptionControl_0(void *, ...) { return B_ERROR; }
status_t BOptionPopUp::_Reserved_OptionControl_1(void *, ...) { return B_ERROR; }
status_t BOptionPopUp::_Reserved_OptionControl_2(void *, ...) { return B_ERROR; }
status_t BOptionPopUp::_Reserved_OptionControl_3(void *, ...) { return B_ERROR; }
status_t BOptionPopUp::_Reserved_OptionPopUp_0(void *, ...) { return B_ERROR; }
status_t BOptionPopUp::_Reserved_OptionPopUp_1(void *, ...) { return B_ERROR; }
status_t BOptionPopUp::_Reserved_OptionPopUp_2(void *, ...) { return B_ERROR; }
status_t BOptionPopUp::_Reserved_OptionPopUp_3(void *, ...) { return B_ERROR; }
status_t BOptionPopUp::_Reserved_OptionPopUp_4(void *, ...) { return B_ERROR; }
status_t BOptionPopUp::_Reserved_OptionPopUp_5(void *, ...) { return B_ERROR; }
status_t BOptionPopUp::_Reserved_OptionPopUp_6(void *, ...) { return B_ERROR; }
status_t BOptionPopUp::_Reserved_OptionPopUp_7(void *, ...) { return B_ERROR; }
