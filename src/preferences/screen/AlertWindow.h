/*
 * Copyright 2001-2005, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Rafael Romo
 *		Stefano Ceccherini (burton666@libero.it)
 *		Axel Dörfler, axeld@pinc-software.de
 */
#ifndef ALERT_WINDOW_H
#define ALERT_WINDOW_H


#include <Window.h>
#include <Messenger.h>


class BMessageRunner;
class BButton;
class AlertView;


class AlertWindow : public BWindow {
	public:
		AlertWindow(BMessenger target);

		virtual void MessageReceived(BMessage *message);

	private:
		BMessenger	fTarget;
		AlertView*	fAlertView;
};

#endif	/* ALERT_WINDOW_H */
