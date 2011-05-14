/*
 * Copyright 2011, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Clemens Zeidler <haiku@clemens-zeidler.de>
 */
#ifndef MUSIC_COLLECTION_WINDOW_H
#define MUSIC_COLLECTION_WINDOW_H


#include <OutlineListView.h>
#include <StringView.h>
#include <TextControl.h>
#include <Window.h>

#include "MusicFileListView.h"


class MusicCollectionWindow : public BWindow {
public:
								MusicCollectionWindow(BRect rect,
									const char* name);
	virtual						~MusicCollectionWindow();

	virtual	bool				QuitRequested();
	virtual void				MessageReceived(BMessage* message);

private:
			void				_StartNewQuery();
			BQuery*				_CreateQuery(BString& queryString);

			BTextControl*		fQueryField;
			BStringView*		fCountView;
			MusicFileListView*	fFileListView;

			EntryViewInterface*	fEntryViewInterface;
			QueryHandler*		fQueryHandler;
			QueryReader*		fQueryReader;
};

#endif	// MUSIC_COLLECTION_WINDOW_H
