/*
 * Copyright 2019-2023, Adrien Destugues, pulkomandy@pulkomandy.tk.
 * Distributed under the terms of the MIT License.
 */
#ifndef ALERTWITHCHECKBOX_H
#define ALERTWITHCHECKBOX_H


#include <Bitmap.h>
#include <Window.h>


class BCheckBox;


class AlertWithCheckbox : public BWindow {
public:
					AlertWithCheckbox(const char* title, const char* message, const char* checkBox,
						const char* button1, const char* button2, const char* button3);
					~AlertWithCheckbox();

	void			MessageReceived(BMessage* message);
	int32			Go(bool& dontAskAgain);

private:
	static	BRect	IconSize();

private:
	BBitmap			fBitmap;
	BCheckBox*		fDontAskAgain;
	sem_id			fSemaphore;
	int32			fAction;
};


#endif /* !ALERTWITHCHECKBOX_H */
