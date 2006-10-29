/*
 * Copyright 2006, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 */
#ifndef POWER_STATUS_VIEW_H
#define POWER_STATUS_VIEW_H


#include <View.h>

class BMessageRunner;


class PowerStatusView : public BView {
	public:
		PowerStatusView(BRect frame, int32 resizingMode, bool inDeskbar = false);
		PowerStatusView(BMessage* archive);
		virtual	~PowerStatusView();

		static	PowerStatusView* Instantiate(BMessage* archive);
		virtual	status_t Archive(BMessage* archive, bool deep = true) const;

		virtual	void	AttachedToWindow();
		virtual	void	DetachedFromWindow();

		virtual	void	MessageReceived(BMessage* message);
		virtual	void	MouseDown(BPoint where);
		virtual	void	Draw(BRect updateRect);

	private:
		void			_AboutRequested();
		void			_Quit();
		void			_Init();
		void			_SetLabel(char* buffer, size_t bufferLength);
		void			_Update(bool force = false);
		void			_DrawBattery(BRect rect);

		BMessageRunner*	fMessageRunner;
		bool			fInDeskbar;
		bool			fShowLabel;
		bool			fShowTime;
		bool			fShowStatusIcon;

		int32			fPercent;
		time_t			fTimeLeft;
		bool			fOnline;

#ifndef HAIKU_TARGET_PLATFORM_HAIKU
		int				fDevice;
#endif
};

#endif	// POWER_STATUS_VIEW_H
