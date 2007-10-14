/*
 * Copyright 2004-2007, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Andrew McCall <mccall@@digitalparadise.co.uk>
 *		Mike Berg <mike@berg-net.us>
 *		Julun <host.haiku@gmx.de>
 */
#ifndef SETTINGS_VIEW_H
#define SETTINGS_VIEW_H


#include <View.h>


class TDateEdit;
class TTimeEdit;
class BCalendarView;
class BRadioButton;
class TAnalogClock;


class TSettingsView : public BView {
	public:
						TSettingsView(BRect frame);
		virtual 		~TSettingsView();

		virtual void 	AttachedToWindow();
		virtual void 	Draw(BRect updaterect);
		virtual void 	MessageReceived(BMessage *message);
		virtual void	GetPreferredSize(float *width, float *height);

	private:
		void 			_InitView();
		void 			_ReadRTCSettings();
		void			_WriteRTCSettings();
		void			_UpdateGmtSettings();
		void 			_UpdateDateTime(BMessage *message);

	private:
		BRadioButton 	*fLocalTime;
		BRadioButton 	*fGmtTime;
		TDateEdit 		*fDateEdit;
		TTimeEdit 		*fTimeEdit;
		BCalendarView 	*fCalendarView;
		TAnalogClock 	*fClock;

		bool 			fIsLocalTime;
		bool			fInitialized;
};

#endif	// SETTINGS_VIEW_H

