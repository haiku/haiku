/*
 * Copyright 2003-2010, Haiku, Inc.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Stefano Ceccherini <stefano.ceccherini@gmail.com>
 */

#include <GroupLayout.h>
#include <MenuField.h>
#include <MenuItem.h>
#include <OptionPopUp.h>
#include <PopUpMenu.h>

#include <stdio.h>


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
BOptionPopUp::BOptionPopUp(BRect frame, const char* name, const char* label,
		BMessage* message, uint32 resize, uint32 flags)
	: BOptionControl(frame, name, label, message, resize, flags)
{
	BPopUpMenu* popUp = new BPopUpMenu(label, true, true);
	fMenuField = new BMenuField(Bounds(), "_menu", label, popUp);
	AddChild(fMenuField);
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
BOptionPopUp::BOptionPopUp(BRect frame, const char* name, const char* label, 
		BMessage* message, bool fixed, uint32 resize, uint32 flags)
	: BOptionControl(frame, name, label, message, resize, flags)
{
	BPopUpMenu* popUp = new BPopUpMenu(label, true, true);
	fMenuField = new BMenuField(Bounds(), "_menu", label, popUp, fixed);
	AddChild(fMenuField);
}


BOptionPopUp::BOptionPopUp(const char* name, const char* label,
		BMessage* message, uint32 flags)
	: BOptionControl(name, label, message, flags)
{
	// TODO: Is this really needed ? Without this, the view
	// doesn't get layoutted properly
	SetLayout(new BGroupLayout(B_HORIZONTAL));
	
	BPopUpMenu* popUp = new BPopUpMenu(label, true, true);
	fMenuField = new BMenuField("_menu", label, popUp);
	AddChild(fMenuField);
}
								
								
BOptionPopUp::~BOptionPopUp()
{
}


/*! \brief Returns a pointer to the BMenuField used internally.
	\return A Pointer to the BMenuField which the class uses internally.
*/
BMenuField*
BOptionPopUp::MenuField()
{
	return fMenuField;
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
BOptionPopUp::GetOptionAt(int32 index, const char** outName, int32* outValue)
{
	bool result = false;
	BMenu* menu = fMenuField->Menu();

	if (menu != NULL) {
		BMenuItem* item = menu->ItemAt(index);
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
	BMenu* menu = fMenuField->Menu();
	if (menu != NULL)
		delete menu->RemoveItem(index);
}


/*! \brief Returns the amount of "Options" (entries) contained in the control.
*/
int32
BOptionPopUp::CountOptions() const
{
	BMenu* menu = fMenuField->Menu();	
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
BOptionPopUp::AddOptionAt(const char* name, int32 value, int32 index)
{
	BMenu* menu = fMenuField->Menu();
	if (menu == NULL)
		return B_ERROR;
	
	int32 numItems = menu->CountItems();
	if (index < 0 || index > numItems)
		return B_BAD_VALUE;
	
	BMessage* message = MakeValueMessage(value);
	if (message == NULL)
		return B_NO_MEMORY;
	
	BMenuItem* newItem = new BMenuItem(name, message);
	if (newItem == NULL) {
		delete message;
		return B_NO_MEMORY;
	}
	
	if (!menu->AddItem(newItem, index)) {
		delete newItem;
		return B_NO_MEMORY;
	}

	newItem->SetTarget(this);
	
	// We didnt' have any items before, so select the newly added one
	if (numItems == 0)
		SetValue(value);
	
	return B_OK;
}


/*! \brief Called to take special actions when the child views are attached.
	It's used to set correctly the divider for the BMenuField.
*/
void
BOptionPopUp::AllAttached()
{
	BMenu* menu = fMenuField->Menu();
	if (menu != NULL) {
		float labelWidth = fMenuField->StringWidth(fMenuField->Label());
		fMenuField->SetDivider(labelWidth + kLabelSpace);
		menu->SetTargetForItems(this);
	}
}


void
BOptionPopUp::MessageReceived(BMessage* message)
{
	BOptionControl::MessageReceived(message);
}


/*! \brief Set the label of the control.
	\param text The new label of the control.
*/
void
BOptionPopUp::SetLabel(const char* text)
{
	BControl::SetLabel(text);
	fMenuField->SetLabel(text);
	// We are not sure the menu can keep the whole
	// string as label, so we check against the current label
	float newWidth = fMenuField->StringWidth(fMenuField->Label());
	fMenuField->SetDivider(newWidth + kLabelSpace);
}


/*! \brief Set the control's value.
	\param value The new value of the control.
	Selects the option which has the given value.
*/
void
BOptionPopUp::SetValue(int32 value)
{
	BControl::SetValue(value);
	BMenu* menu = fMenuField->Menu();
	if (menu == NULL)
		return;

	int32 numItems = menu->CountItems();
	for (int32 i = 0; i < numItems; i++) {
		BMenuItem* item = menu->ItemAt(i);
		if (item && item->Message()) {
			int32 itemValue;
			item->Message()->FindInt32("be:value", &itemValue);
			if (itemValue == value) {
				item->SetMarked(true);
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
BOptionPopUp::GetPreferredSize(float* _width, float* _height)
{
	float width, height;
	fMenuField->GetPreferredSize(&width, &height);
		
	if (_height != NULL) {
		font_height fontHeight;
		GetFontHeight(&fontHeight);
		
		*_height = max_c(height, fontHeight.ascent + fontHeight.descent
			+ fontHeight.leading + kHeightModifier);
	}

	if (_width != NULL) {
		width += fMenuField->StringWidth(BControl::Label())
			+ kLabelSpace + kWidthModifier;
		*_width = width;
	}
}


/*! \brief Resizes the control to its preferred size.
*/
void
BOptionPopUp::ResizeToPreferred()
{
	float width, height;
	GetPreferredSize(&width, &height);
	ResizeTo(width, height);
	
	float newWidth = fMenuField->StringWidth(BControl::Label());
	fMenuField->SetDivider(newWidth + kLabelSpace);
}


/*! \brief Gets the currently selected option.
	\param outName A pointer to a string which will held the option's name.
	\param outValue A pointer to an integer which will held the option's value.
	\return The index of the selected option.
*/
int32
BOptionPopUp::SelectedOption(const char** outName, int32* outValue) const
{
	BMenu* menu = fMenuField->Menu();
	if (menu == NULL)
		return B_ERROR;

	BMenuItem* marked = menu->FindMarked();
	if (marked == NULL)
		return -1;

	if (outName != NULL)
		*outName = marked->Label();
	if (outValue != NULL)
		marked->Message()->FindInt32("be:value", outValue);
	
	return menu->IndexOf(marked);
}


// Private Unimplemented
BOptionPopUp::BOptionPopUp()
	:
	BOptionControl(BRect(), "", "", NULL)
{
}


BOptionPopUp::BOptionPopUp(const BOptionPopUp& clone)
	:
	BOptionControl(clone.Frame(), "", "", clone.Message())
{
}


BOptionPopUp &
BOptionPopUp::operator=(const BOptionPopUp& clone)
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
