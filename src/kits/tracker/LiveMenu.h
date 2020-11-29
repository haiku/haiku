/*
 * Copyright 2020-2024 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		John Scipione, jscipione@gmail.com
 */
#ifndef _LIVE_MENU_H
#define _LIVE_MENU_H


#include <Menu.h>
#include <PopUpMenu.h>

#include "ContainerWindow.h"
#include "Shortcuts.h"


namespace BPrivate {


// mixin class for sharing virtual methods
struct TLiveMixin {
							TLiveMixin(const BContainerWindow* window);

	virtual	void			UpdateFileMenu(BMenu* menu);
	virtual	void			UpdateWindowMenu(BMenu* menu);

private:
	const BContainerWindow*	fWindow;
};


class TLiveMenu : public BMenu {
public:
							TLiveMenu(const char* label);
	virtual 				~TLiveMenu();

	virtual	void			MessageReceived(BMessage* message);

protected:
	virtual	void			Update();
};


class TLivePopUpMenu : public BPopUpMenu {
public:
							TLivePopUpMenu(const char* label,
								bool radioMode = true,
								bool labelFromMarked = true,
								menu_layout layout = B_ITEMS_IN_COLUMN);
	virtual					~TLivePopUpMenu();

	virtual	void			MessageReceived(BMessage* message);

protected:
	virtual	void			Update();

private:
	const BContainerWindow*	fWindow;
};


class TLiveArrangeByMenu: public TLiveMenu, public TLiveMixin {
public:
							TLiveArrangeByMenu(const char* label, const BContainerWindow* window);
	virtual					~TLiveArrangeByMenu();

protected:
	virtual	void			Update();
};


class TLiveFileMenu : public TLiveMenu, public TLiveMixin {
public:
							TLiveFileMenu(const char* label, const BContainerWindow* window);
	virtual					~TLiveFileMenu();

protected:
	virtual	void			Update();
};

class TLivePosePopUpMenu : public TLivePopUpMenu, public TLiveMixin {
public:
							TLivePosePopUpMenu(const char* label, const BContainerWindow* window,
								bool radioMode = true, bool labelFromMarked = true,
								menu_layout layout = B_ITEMS_IN_COLUMN);
	virtual					~TLivePosePopUpMenu();

protected:
	virtual	void			Update();
};


class TLiveWindowMenu : public TLiveMenu, public TLiveMixin {
public:
							TLiveWindowMenu(const char* label, const BContainerWindow* window);
	virtual					~TLiveWindowMenu();

protected:
	virtual	void			Update();
};

class TLiveWindowPopUpMenu : public TLivePopUpMenu, public TLiveMixin {
public:
							TLiveWindowPopUpMenu(const char* label, const BContainerWindow* window,
								bool radioMode = true, bool labelFromMarked = true,
								menu_layout layout = B_ITEMS_IN_COLUMN);
	virtual					~TLiveWindowPopUpMenu();

protected:
	virtual	void			Update();
};


} // namespace BPrivate

using namespace BPrivate;


#endif // _LIVE_MENU_H
