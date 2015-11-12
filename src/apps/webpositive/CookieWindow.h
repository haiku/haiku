/*
 * Copyright 2015 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Adrien Destugues
 */
#ifndef COOKIE_WINDOW_H
#define COOKIE_WINDOW_H


#include <Window.h>


class BColumnListView;
class BNetworkCookieJar;
class BOutlineListView;
class BStringItem;
class BStringView;
class BString;


class CookieWindow : public BWindow {
public:
								CookieWindow(BRect frame,
									BNetworkCookieJar& jar);
	virtual	void				MessageReceived(BMessage* message);
	virtual void				Show();
	virtual	bool				QuitRequested();

private:
			void				_BuildDomainList();
			BStringItem*		_AddDomain(BString domain, bool fake);
			void				_ShowCookiesForDomain(BString domain);
			void				_DeleteCookies();

private:
	BOutlineListView*			fDomains;
	BColumnListView*			fCookies;
	BStringView*				fHeaderView;

	BNetworkCookieJar&			fCookieJar;
};


#endif // COOKIE_WINDOW_H

