/*
 * Copyright 2005, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 */
#ifndef SCREEN_MANAGER_H
#define SCREEN_MANAGER_H


#include <Looper.h>
#include <ObjectList.h>

class BMessage;

class DrawingEngine;
class HWInterface;
class Screen;


typedef BObjectList<Screen> ScreenList;


class ScreenOwner {
	public:
		virtual void	ScreenRemoved(Screen* screen) = 0;
		virtual void	ScreenAdded(Screen* screen) = 0;

		virtual bool	ReleaseScreen(Screen* screen) = 0;
};


class ScreenManager : public BLooper {
	public:
		ScreenManager();
		virtual ~ScreenManager();

		Screen*			ScreenAt(int32 index) const;
		int32			CountScreens() const;

		status_t		AcquireScreens(ScreenOwner* owner, int32* wishList,
							int32 wishCount, bool force, ScreenList& list);
		void			ReleaseScreens(ScreenList& list);

		virtual void	MessageReceived(BMessage* message);

	private:
		void			_ScanDrivers();
		void			_AddHWInterface(HWInterface* interface);

		struct screen_item {
			Screen*			screen;
			ScreenOwner*	owner;
		};

		BObjectList<screen_item>	fScreenList;
};

extern ScreenManager *gScreenManager;

#endif	/* SCREEN_MANAGER_H */
