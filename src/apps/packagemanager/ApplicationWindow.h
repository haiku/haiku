/*
 * Copyright (C) 2010 Stephan Aßmus <superstippi@gmx.de>
 * Copyright (C) 2010 Adrien Destugues <pulkomandy@pulkomandy.ath.cx>
 *
 * Distributed under the terms of the MIT Licence.
 */
#ifndef APPLICATION_WINDOW_H
#define APPLICATION_WINDOW_H


#include <String.h>
#include <Window.h>

class BButton;
class BFile;
class BGroupLayout;
class BScrollView;
class BWebApplication;
class SettingsMessage;


class ApplicationWindow : public BWindow {
public:
								ApplicationWindow(BRect frame, bool visible);
	virtual						~ApplicationWindow();

	virtual	void				DispatchMessage(BMessage* message,
									BHandler* target);
	virtual	void				MessageReceived(BMessage* message);
	virtual	bool				QuitRequested();

			void				SetMinimizeOnClose(bool minimize);

			void				AddCategory(const char* name,
									const char* icon,
									const char* description);
			void				AddApplication(const BMessage* info);

private:
			void				_ValidateButtonStatus();

private:
			BScrollView*		fApplicationsScrollView;
			BGroupLayout*		fApplicationViewsLayout;
			BButton*			fDiscardButton;
			BButton*			fApplyChangesButton;
			BString				fApplicationPath;
			bool				fMinimizeOnClose;
};

#endif // APPLICATION_WINDOW_H
