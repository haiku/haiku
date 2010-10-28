/*
 * Copyright 2009 - 2010 Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Ankur Sethi (get.me.ankur@gmail.com)
 *		Clemens Zeidler (haiku@clemens-zeidler.de)
 */

#include "SearchWindow.h"

#include <Alert.h>
#include <Application.h>
#include <GroupLayout.h>
#include <GroupLayoutBuilder.h>
#include <Roster.h>

#include "BeaconSearcher.h"


const uint32 kSearchMessage = '&sea';
const uint32 kLaunchMessage = '&lnc';


SearchWindow::SearchWindow(BRect frame)
	:
	BWindow(frame, "Fulltext Search", B_TITLED_WINDOW,
		B_QUIT_ON_WINDOW_CLOSE | B_AUTO_UPDATE_SIZE_LIMITS)
{
	fSearchButton = new BButton("Search", new BMessage(kSearchMessage));
	fSearchField = new BTextControl("", "", new BMessage(kSearchMessage));
	
	fSearchResults = new BListView();
	fSearchResults->SetInvocationMessage(new BMessage(kLaunchMessage));

	fScrollView = new BScrollView("SearchResults", fSearchResults, 0,
		true, true);

	SetLayout(new BGroupLayout(B_VERTICAL));

	AddChild(BGroupLayoutBuilder(B_VERTICAL, 10)
		.Add(BGroupLayoutBuilder(B_HORIZONTAL, 10)
			.Add(fSearchField)
			.Add(fSearchButton)
			.SetInsets(5, 5, 5, 5)
		)
	.Add(fScrollView)
	.SetInsets(5, 5, 5, 5)
	);
}


void
SearchWindow::MessageReceived(BMessage *message)
{
	switch(message->what) {
		case kSearchMessage:
			_Search();
			break;

		case kLaunchMessage:
			_LaunchFile(message);
			break;

		default:
			BWindow::MessageReceived(message);
	}
}


void
SearchWindow::_Search()
{
	fSearchResults->MakeEmpty();
	BeaconSearcher searcher;
	searcher.Search(fSearchField->Text());
	wchar_t *wPath;
	char *path;
	while((wPath = searcher.GetNextHit()) != NULL) {
		path = new char[wcslen(wPath)*sizeof(wchar_t)];
		wcstombs(path, wPath, wcslen(wPath)*sizeof(wchar_t));
		fSearchResults->AddItem(new BStringItem(path));
	}
}


void
SearchWindow::_LaunchFile(BMessage *message)
{
	BListView *searchResults;
	int32 index;
	
	message->FindPointer("source", (void**)&searchResults);
	message->FindInt32("index", &index);
	BStringItem *result = (BStringItem*)searchResults->ItemAt(index);
	
	entry_ref ref ;
	BEntry entry(result->Text());
	entry.GetRef(&ref);
	be_roster->Launch(&ref);
}
