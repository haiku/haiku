/*
 * Copyright 2009 - 2010 Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Ankur Sethi (get.me.ankur@gmail.com)
 *		Clemens Zeidler (haiku@clemens-zeidler.de)
 */
#ifndef _SEARCH_WINDOW_H
#define _SEARCH_WINDOW_H

#include <Button.h>
#include <ListView.h>
#include <ScrollView.h>
#include <TextControl.h>
#include <Window.h>


class SearchWindow : public BWindow {
public:
								SearchWindow(BRect frame);
	
			void				MessageReceived(BMessage *message);

private:
			void				_Search();
			void				_LaunchFile(BMessage *message);

			// Window controls.
			BButton*			fSearchButton;
			BTextControl*		fSearchField;
			BListView*			fSearchResults;
			BScrollView*		fScrollView;
} ;

#endif /* _SEARCH_WINDOW_H_ */
