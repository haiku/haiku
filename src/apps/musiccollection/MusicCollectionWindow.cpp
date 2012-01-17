/*
 * Copyright 2011, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Clemens Zeidler <haiku@clemens-zeidler.de>
 */

#include "MusicCollectionWindow.h"

#include <Application.h>
#include <ControlLook.h>
#include <ScrollView.h>
#include <VolumeRoster.h>

#include <NaturalCompare.h>

#include "ALMLayout.h"
#include "ALMLayoutBuilder.h"


static int
StringItemComp(const BListItem* first, const BListItem* second)
{
	BStringItem* firstItem = (BStringItem*)first;
	BStringItem* secondItem = (BStringItem*)second;
	return BPrivate::NaturalCompare(firstItem->Text(), secondItem->Text());
}


template <class ListItem = FileListItem>
class ListViewListener : public EntryViewInterface {
public:
	ListViewListener(BOutlineListView* list, BStringView* countView)
		:
		fListView(list),
		fCountView(countView),
		fItemCount(0)
	{

	}


	void
	SetQueryString(const char* string)
	{
		fQueryString = string;
	}


	void
	EntryCreated(WatchedFile* file)
	{
		//ListItem* item1 = new ListItem(file->entry.name, file);
		//fListView->AddItem(item1);

		fItemCount++;
		BString count("Count: ");
		count << fItemCount;
		fCountView->SetText(count);

		const ssize_t bufferSize = 256;
		char buffer[bufferSize];
		BNode node(&file->entry);

		ssize_t readBytes;
		readBytes = node.ReadAttr("Audio:Artist", B_STRING_TYPE, 0, buffer,
			bufferSize);
		if (readBytes < 0)
			readBytes = 0;
		if (readBytes >= bufferSize)
			readBytes = bufferSize - 1;
		buffer[readBytes] = '\0';

		BString artist = (strcmp(buffer, "") == 0) ? "Unknown" : buffer;
		ListItem* artistItem = _AddSuperItem(artist, fArtistList, NULL);

		readBytes = node.ReadAttr("Audio:Album", B_STRING_TYPE, 0, buffer,
			bufferSize);
		if (readBytes < 0)
			readBytes = 0;
		buffer[readBytes] = '\0';
		BString album = (strcmp(buffer, "") == 0) ? "Unknown" : buffer;
		ListItem* albumItem = _AddSuperItem(album, fAlbumList, artistItem);

		readBytes = node.ReadAttr("Media:Title", B_STRING_TYPE, 0, buffer,
			bufferSize);
		if (readBytes < 0)
			readBytes = 0;
		buffer[readBytes] = '\0';
		BString title= (strcmp(buffer, "") == 0) ? file->entry.name
			: buffer;

		ListItem* item = new ListItem(title, file);
		file->cookie = item;
		fListView->AddUnder(item, albumItem);
		fListView->SortItemsUnder(albumItem, true, StringItemComp);

		if (fQueryString == "")
			return;
		if (title.IFindFirst(fQueryString) >= 0) {
			fListView->Expand(artistItem);
			fListView->Expand(albumItem);
		} else if (album.IFindFirst(fQueryString) >= 0) {
			fListView->Expand(artistItem);
		}
	};


	void
	EntryRemoved(WatchedFile* file)
	{
		ListItem* item = (ListItem*)file->cookie;
		ListItem* album = (ListItem*)fListView->Superitem(item);
		fListView->RemoveItem(item);
		if (album != NULL && fListView->CountItemsUnder(album, true) == 0) {
			ListItem* artist = (ListItem*)fListView->Superitem(album);
			fListView->RemoveItem(album);
			if (artist != NULL && fListView->CountItemsUnder(artist, true) == 0)
				fListView->RemoveItem(artist);
		}
	};

	void
	EntryMoved(WatchedFile* file)
	{
		AttrChanged(file);
	};


	void
	AttrChanged(WatchedFile* file)
	{
		EntryRemoved(file);
		EntryCreated(file);
	}


	void
	EntriesCleared()
	{
		for (int32 i = 0; i < fListView->FullListCountItems(); i++)
			delete fListView->FullListItemAt(i);
		fListView->MakeEmpty();

		fArtistList.MakeEmpty();
		fAlbumList.MakeEmpty();

		printf("prev count %i\n", (int)fItemCount);
		fItemCount = 0;
		fCountView->SetText("Count: 0");
	}


private:
	ListItem*
	_AddSuperItem(const char* name, BObjectList<ListItem>& list,
		ListItem* under)
	{
		ListItem* item = _FindStringItem(list, name, under);
		if (item != NULL)
			return item;

		item = new ListItem(name);
		fListView->AddUnder(item, under);
		fListView->SortItemsUnder(under, true, StringItemComp);
		list.AddItem(item);

		fListView->Collapse(item);

		return item;
	}

	ListItem*
	_FindStringItem(BObjectList<ListItem>& list, const char* text,
		ListItem* parent)
	{
		for (int32 i = 0; i < list.CountItems(); i++) {
			ListItem* item = list.ItemAt(i);
			ListItem* superItem = (ListItem*)fListView->Superitem(item);
			if (parent != NULL && parent != superItem)
				continue;
			if (strcmp(item->Text(), text) == 0)
				return item;
		}
		return NULL;
	}

			BOutlineListView*	fListView;
			BStringView*		fCountView;

			BObjectList<ListItem>	fArtistList;
			BObjectList<ListItem> 	fAlbumList;

			BString					fQueryString;
			int32					fItemCount;
};


const uint32 kMsgQueryInput = '&qin';
const uint32 kMsgItemInvoked = '&iin';


MusicCollectionWindow::MusicCollectionWindow(BRect frame, const char* title)
	:
	BWindow(frame, title, B_DOCUMENT_WINDOW, B_AVOID_FRONT)
{
	fQueryField = new BTextControl("Search: ", "", NULL);
	fQueryField->SetExplicitAlignment(BAlignment(B_ALIGN_HORIZONTAL_CENTER,
		B_ALIGN_USE_FULL_HEIGHT));
	fQueryField->SetModificationMessage(new BMessage(kMsgQueryInput));

	fCountView = new BStringView("Count View", "Count:");

	fFileListView = new MusicFileListView("File List View");
	fFileListView->SetInvocationMessage(new BMessage(kMsgItemInvoked));
	BScrollView* scrollView = new BScrollView("list scroll", fFileListView, 0,
		true, true, B_PLAIN_BORDER);

	BALMLayout* layout = new BALMLayout(B_USE_ITEM_SPACING, B_USE_ITEM_SPACING);
	BALM::BALMLayoutBuilder(this, layout)
		.SetInsets(B_USE_WINDOW_INSETS)
		.Add(fQueryField, layout->Left(), layout->Top())
		.StartingAt(fQueryField)
			.AddToRight(fCountView, layout->Right())
			.AddBelow(scrollView, layout->Bottom(), layout->Left(),
				layout->Right());

	Area* area = layout->AreaFor(scrollView);
	area->SetLeftInset(0);
	area->SetRightInset(0);
	area->SetBottomInset(0);

	BSize min = layout->MinSize();
	BSize max = layout->MaxSize();
	SetSizeLimits(min.Width(), max.Width(), min.Height(), max.Height());

	fEntryViewInterface = new ListViewListener<FileListItem>(fFileListView,
		fCountView);
	fQueryHandler = new QueryHandler(fEntryViewInterface);
	AddHandler(fQueryHandler);
	fQueryReader = new QueryReader(fQueryHandler);
	fQueryHandler->SetReadThread(fQueryReader);

	// start initial query
	PostMessage(kMsgQueryInput);
}


MusicCollectionWindow::~MusicCollectionWindow()
{
	delete fQueryReader;
	delete fQueryHandler;
	delete fEntryViewInterface;
}


bool
MusicCollectionWindow::QuitRequested()
{
	be_app->PostMessage(B_QUIT_REQUESTED);
	return true;
}


void
MusicCollectionWindow::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case kMsgQueryInput:
			_StartNewQuery();
			break;

		case kMsgItemInvoked:
			fFileListView->Launch(message);
			break;

		default:
			BWindow::MessageReceived(message);
	}
}


void
CaseInsensitiveString(BString &instring,  BString &outstring)
{
	outstring = "";
	int i = 0;
	while (instring[i])
	{
		if (instring[i] >= 65 && instring[i] <= 90) // capital letters   
		{
			int ch = instring[i] + 32;
			outstring += "[";
			outstring += ch;
			outstring += instring[i];
			outstring += "]";
		} else if (instring[i] >= 97 && instring[i] <= 122)
		{
			int ch = instring[i]-32;
			outstring += "[";
			outstring += instring[i];
			outstring += ch;
			outstring += "]";
		} else
			outstring += instring[i];
		i++;
	}
}


void
MusicCollectionWindow::_StartNewQuery()
{
	fQueryReader->Reset();
	fQueryHandler->Reset();

	BString orgString = fQueryField->Text();
	((ListViewListener<FileListItem>*)fEntryViewInterface)->SetQueryString(
		orgString);

	BVolume volume;
	//BVolumeRoster().GetBootVolume(&volume);
	BVolumeRoster roster;
	while (roster.GetNextVolume(&volume) == B_OK) {
		if (!volume.KnowsQuery())
			continue;
		BQuery* query = _CreateQuery(orgString);
		query->SetVolume(&volume);
		fQueryReader->AddQuery(query);
	}

	fQueryReader->Run();
}


BQuery*
MusicCollectionWindow::_CreateQuery(BString& orgString)
{
	BQuery* query = new BQuery;

	BString queryString;
	CaseInsensitiveString(orgString, queryString);

	query->PushAttr("Media:Title");
	query->PushString(queryString);
	query->PushOp(B_CONTAINS);
		
	query->PushAttr("Audio:Album");
	query->PushString(queryString);
	query->PushOp(B_CONTAINS);
	query->PushOp(B_OR);
	
	query->PushAttr("Audio:Artist");
	query->PushString(queryString);
	query->PushOp(B_CONTAINS);
	query->PushOp(B_OR);

	if (queryString == "") {
		query->PushAttr("BEOS:TYPE");
		query->PushString("audio/");
		query->PushOp(B_BEGINS_WITH);
		query->PushOp(B_OR);
	}

	query->PushAttr("BEOS:TYPE");
	query->PushString("audio/");
	query->PushOp(B_BEGINS_WITH);
	
	query->PushAttr("name");
	query->PushString(queryString);
	query->PushOp(B_CONTAINS);
	query->PushOp(B_AND);
	query->PushOp(B_OR);

	return query;
}
