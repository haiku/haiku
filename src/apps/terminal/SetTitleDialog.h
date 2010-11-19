/*
 * Copyright 2010, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef SET_TITLE_DIALOG_H
#define SET_TITLE_DIALOG_H


#include <String.h>
#include <Window.h>


class BButton;
class BTextControl;


class SetTitleDialog : public BWindow {
public:
			class Listener;

public:
								SetTitleDialog(const char* dialogTitle,
									const char* label, const char* toolTip);
	virtual						~SetTitleDialog();

			void				Go(const BString& title, bool titleUserDefined,
									Listener* listener);
			void				Finish();
									// window must be locked

	virtual void				MessageReceived(BMessage* message);

private:
			Listener*			fListener;
			BTextControl*		fTitleTextControl;
			BButton*			fOKButton;
			BButton*			fCancelButton;
			BButton*			fDefaultButton;
			BString				fOldTitle;
			BString				fTitle;
			bool				fOldTitleUserDefined;
			bool				fTitleUserDefined;
};


class SetTitleDialog::Listener {
public:
	virtual						~Listener();

	// hooks called in the dialog thread with the window locked
	virtual	void				TitleChanged(SetTitleDialog* dialog,
									const BString& title,
									bool titleUserDefined) = 0;
	virtual	void				SetTitleDialogDone(SetTitleDialog* dialog) = 0;
};


#endif	// SET_TITLE_DIALOG_H
