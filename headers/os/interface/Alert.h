/*
 * Copyright 2001-2006, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef	_ALERT_H
#define	_ALERT_H


#include <Window.h>


class BBitmap;
class BButton;
class BInvoker;
class BTextView;

// enum for flavors of alert
enum alert_type {
	B_EMPTY_ALERT = 0,
	B_INFO_ALERT,
	B_IDEA_ALERT,
	B_WARNING_ALERT,
	B_STOP_ALERT
};

enum button_spacing {
	B_EVEN_SPACING = 0,
	B_OFFSET_SPACING
};


class BAlert : public BWindow {
	public:
							BAlert(const char* title, const char* text,
								const char* button1, const char* button2 = NULL,
								const char* button3 = NULL,
								button_width width = B_WIDTH_AS_USUAL,
								alert_type type = B_INFO_ALERT);
							BAlert(const char *title, const char *text,
								const char *button1, const char *button2,
								const char *button3, button_width width,
								button_spacing spacing,
								alert_type type = B_INFO_ALERT);
		virtual				~BAlert();

		// Archiving
							BAlert(BMessage *data);
		static	BArchivable	*Instantiate(BMessage *data);
		virtual	status_t	Archive(BMessage *data, bool deep = true) const;

		// BAlert guts
				void		SetShortcut(int32 button_index, char key);
				char		Shortcut(int32 button_index) const;

				int32		Go();
				status_t	Go(BInvoker *invoker);

		virtual	void		MessageReceived(BMessage *an_event);
		virtual	void		FrameResized(float new_width, float new_height);
				BButton*	ButtonAt(int32 index) const;
				BTextView*	TextView() const;

		virtual BHandler*	ResolveSpecifier(BMessage* message, int32 index,
								BMessage* specifier, int32 form,
								const char* property);
		virtual	status_t	GetSupportedSuites(BMessage *data);

		virtual void		DispatchMessage(BMessage* message, BHandler* handler);
		virtual	void		Quit();
		virtual	bool		QuitRequested();

		static	BPoint		AlertPosition(float width, float height);

		virtual status_t	Perform(perform_code d, void *arg);

	private:
		friend class _BAlertFilter_;

		virtual	void		_ReservedAlert1();
		virtual	void		_ReservedAlert2();
		virtual	void		_ReservedAlert3();

				void		_InitObject(const char* text, const char* button1,
								const char* button2 = NULL,
								const char* button3 = NULL,
								button_width width = B_WIDTH_AS_USUAL,
								button_spacing spacing = B_EVEN_SPACING,
								alert_type type = B_INFO_ALERT);
				BBitmap*	_InitIcon();
				BButton*	_CreateButton(int32 which, const char* label);

		sem_id				fAlertSem;
		int32				fAlertValue;
		BButton*			fButtons[3];
		BTextView*			fTextView;
		char				fKeys[3];
		alert_type			fMsgType;
		button_width		fButtonWidth;
		BInvoker*			fInvoker;
		uint32				_reserved[4];
};

#endif	// _ALERT_H
