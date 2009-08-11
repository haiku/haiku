/*
 * Copyright 2009, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _TOOL_TIP_MANAGER_H
#define _TOOL_TIP_MANAGER_H


#include <Locker.h>
#include <Messenger.h>
#include <Point.h>


class BToolTip;


class BToolTipManager {
public:
	static	BToolTipManager*	Manager();

			void				ShowTip(BToolTip* tip, BPoint point);
			void				HideTip();

			void				SetShowDelay(bigtime_t time);
			bigtime_t			ShowDelay() const;
			void				SetHideDelay(bigtime_t time);
			bigtime_t			HideDelay() const;

	static	bool				Lock() { return sLock.Lock(); }
	static	void				Unlock() { sLock.Unlock(); }

private:
								BToolTipManager();
	virtual						~BToolTipManager();

			BMessenger			fWindow;

			bigtime_t			fShowDelay;
			bigtime_t			fHideDelay;

	static	BLocker				sLock;
	static	BToolTipManager*	sDefaultInstance;
};


#endif	// _TOOL_TIP_MANAGER_H
