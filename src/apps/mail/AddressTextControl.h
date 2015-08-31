/*
 * Copyright 2015, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef ADDRESS_TEXT_CONTROL_H
#define ADDRESS_TEXT_CONTROL_H


#include <Control.h>


class BButton;
class BPopUpMenu;
class BTextView;


class AddressTextControl : public BControl {
public:
								AddressTextControl(const char* name,
									BMessage* message);
	virtual						~AddressTextControl();

	virtual	void				AttachedToWindow();
	virtual	void				WindowActivated(bool active);
	virtual	void				Draw(BRect updateRect);
	virtual	void				MakeFocus(bool focus = true);
	virtual void				SetEnabled(bool enabled);
	virtual	void				MessageReceived(BMessage* message);

			const BMessage*		ModificationMessage() const;
			void				SetModificationMessage(BMessage* message);

			bool				IsEditable() const;
			void				SetEditable(bool editable);

			void				SetText(const char* text);
			const char*			Text() const;
			int32				TextLength() const;
			void				GetSelection(int32* start, int32* end) const;
			void				Select(int32 start, int32 end);
			void				SelectAll();

			bool				HasFocus();

private:
			void				_AddAddress(const char* text);
			void				_UpdateTextViewColors();

private:
			class TextView;
			class PopUpButton;

			TextView*			fTextView;
			PopUpButton*		fPopUpButton;
			BPopUpMenu*			fRefDropMenu;
			bool				fWindowActive;
			bool				fEditable;
};


#endif // ADDRESS_TEXT_CONTROL_H

