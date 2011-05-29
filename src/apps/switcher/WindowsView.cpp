/*
 * Copyright 2011, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include "WindowsView.h"

#include <stdlib.h>

#include <LayoutBuilder.h>
#include <ObjectList.h>
#include <StringView.h>

#include <MessengerPrivate.h>
#include <WindowInfo.h>
#include <WindowPrivate.h>

#include "GroupListView.h"
#include "LaunchButton.h"
#include "Switcher.h"


static const uint32 kMsgActivateWindow = 'AcWn';


class WindowModel : public GroupListModel {
public:
	WindowModel(team_id team)
		:
		fWindows(true),
		fWorkspaces(0),
		fWorkspaceCount(0)
	{
		// TODO: more than one team via signature!
		int32 count;
		int32* tokens = get_token_list(team, &count);

		for (int32 i = 0; i < count; i++) {
			client_window_info* info = get_window_info(tokens[i]);
			if (!_WindowShouldBeListed(info)) {
				free(info);
				continue;
			}

			fWorkspaces |= info->workspaces;
			fWindows.AddItem(info);
		}
		free(tokens);

		for (uint32 i = 0; i < 32; i++) {
			if ((fWorkspaces & (1UL << i)) != 0)
				fWorkspaceCount++;
		}

		fWindows.SortItems(&_CompareWindowInfo);
	}

	virtual ~WindowModel()
	{
	}

	void BringToFront(int32 index)
	{
		client_window_info* info = fWindows.ItemAt(index);
		if (info == NULL)
			return;

		do_window_action(info->server_token, B_BRING_TO_FRONT, BRect(), false);
	}

	void Close(int32 index)
	{
		client_window_info* info = fWindows.ItemAt(index);
		if (info == NULL)
			return;

		BMessenger window;
		BMessenger::Private(window).SetTo(info->team, info->client_port,
			info->client_token);
		window.SendMessage(B_QUIT_REQUESTED);
	}

	virtual	int32 CountItems()
	{
		return fWindows.CountItems();
	}

	virtual void* ItemAt(int32 index)
	{
		return fWindows.ItemAt(index);
	}

	virtual int32 CountGroups()
	{
		return fWorkspaceCount;
	}

	virtual void* GroupAt(int32 index)
	{
		return (void*)(_NthSetBit(index, fWorkspaces) + 1);
	}

	virtual void* GroupForItemAt(int32 index)
	{
		client_window_info* info = fWindows.ItemAt(index);
		return (void*)(_NthSetBit(0, info->workspaces) + 1);
	}

private:
	bool _WindowShouldBeListed(client_window_info* info)
	{
		return info != NULL
			&& (info->feel == B_NORMAL_WINDOW_FEEL
				|| info->feel == kWindowScreenFeel)
			&& (info->show_hide_level <= 0 || info->is_mini);
	}

	int32 _NthSetBit(int32 index, uint32 mask)
	{
		for (uint32 i = 0; i < 32; i++) {
			if ((mask & (1UL << i)) != 0) {
				if (index-- == 0)
					return i;
			}
		}
		return 0;
	}

	static int _CompareWindowInfo(const client_window_info* a,
		const client_window_info* b)
	{
		return strcasecmp(a->name, b->name);
	}

private:
	BObjectList<client_window_info>	fWindows;
	uint32							fWorkspaces;
	int32							fWorkspaceCount;
};


class StringItemRenderer : public ListItemRenderer {
public:
	StringItemRenderer()
	{
	}

	virtual ~StringItemRenderer()
	{
	}

	void SetText(BView* owner, const BString& text)
	{
		fText = text;
		owner->TruncateString(&fText, B_TRUNCATE_MIDDLE, 200);
		SetWidth((int32)ceilf(owner->StringWidth(fText.String())));

		font_height fontHeight;
		owner->GetFontHeight(&fontHeight);

		SetBaselineOffset(
			2 + (int32)ceilf(fontHeight.ascent + fontHeight.leading / 2));

		SetHeight((int32)ceilf(fontHeight.ascent)
			+ (int32)ceilf(fontHeight.descent)
			+ (int32)ceilf(fontHeight.leading) + 4);
	}

	virtual void SetWidth(int32 width)
	{
		fWidth = width;
	}

	virtual void SetHeight(int32 height)
	{
		fHeight = height;
	}

	virtual void SetBaselineOffset(int32 offset)
	{
		fBaselineOffset = offset;
	}

	const BString& Text() const
	{
		return fText;
	}

	virtual BSize MinSize()
	{
		return BSize(fWidth, fHeight);
	}

	virtual BSize MaxSize()
	{
		return BSize(B_SIZE_UNLIMITED, fHeight);
	}

	virtual BSize PreferredSize()
	{
		return BSize(fWidth, fHeight);
	}

	virtual void Draw(BView* owner, BRect frame, int32 index, bool selected)
	{
		owner->SetLowColor(owner->ViewColor());
		owner->MovePenTo(frame.left, frame.top + fBaselineOffset);
		owner->DrawString(fText);
	}

private:
	BString	fText;
	int32	fWidth;
	int32	fHeight;
	int32	fBaselineOffset;
};


class WorkspaceRenderer : public StringItemRenderer {
public:
	virtual void SetTo(BView* owner, void* item)
	{
		fWorkspace = (uint32)item;

		if ((uint32)current_workspace() == fWorkspace - 1)
			SetText(owner, "Current workspace");
		else {
			BString text("Workspace ");
			text << fWorkspace;
			SetText(owner, text);
		}
	}

	virtual void Draw(BView* owner, BRect frame, int32 index, bool selected)
	{
		owner->SetHighColor(tint_color(owner->ViewColor(), B_DARKEN_2_TINT));
		StringItemRenderer::Draw(owner, frame, index, false);
	}

private:
	uint32	fWorkspace;
};


class WindowRenderer : public StringItemRenderer {
public:
	virtual void SetTo(BView* owner, void* item)
	{
		fInfo = (client_window_info*)item;
		SetText(owner, fInfo->name);
	}

	virtual void SetWidth(int32 width)
	{
		StringItemRenderer::SetWidth(width + 20);
	}

	virtual void Draw(BView* owner, BRect frame, int32 index, bool selected)
	{
		owner->SetHighColor(0, 0, 0);
		frame.left += 20;
		StringItemRenderer::Draw(owner, frame, index, selected);
	}

private:
	client_window_info*	fInfo;
};


// #pragma mark -


WindowsView::WindowsView(team_id team, uint32 location)
	:
	BGridView("windows")
{
	app_info info;
	be_roster->GetRunningAppInfo(team, &info);

	LaunchButton* launchButton = new LaunchButton(info.signature, NULL, NULL,
		this);
	launchButton->SetTo(&info.ref);

	BStringView* nameView = new BStringView("name", info.ref.name);
	BFont font(be_plain_font);
	font.SetSize(font.Size() * 2);
	font.SetFace(B_BOLD_FACE);
	nameView->SetFont(&font);

	fListView = new GroupListView("list", new WindowModel(team),
		_Orientation(location));
	fListView->SetItemRenderer(new WindowRenderer());
	fListView->SetGroupRenderer(new WorkspaceRenderer());

	GridLayout()->SetInsets(B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING,
		B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING);

	if (_Orientation(location) == B_HORIZONTAL) {
		BLayoutBuilder::Grid<>(this)
			.Add(launchButton, 0, 0)
			.Add(nameView, 1, 0)
			.Add(fListView, 2, 0);
	} else {
		BLayoutBuilder::Grid<>(this)
			.Add(launchButton, 0, 0)
			.Add(nameView, 1, 0)
			.Add(fListView, 0, 1, 2);
	}
}


WindowsView::~WindowsView()
{
}


void
WindowsView::AttachedToWindow()
{
	fListView->SetSelectionMessage(new BMessage(kMsgActivateWindow), this);
}


void
WindowsView::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case kMsgActivateWindow:
		{
			int32 index;
			if (message->FindInt32("index", &index) == B_OK
				&& message->HasPointer("item")) {
				WindowModel* model = (WindowModel*)fListView->Model();

				if (message->FindInt32("buttons") == B_SECONDARY_MOUSE_BUTTON)
					model->Close(index);
				else
					model->BringToFront(index);
			}
			break;
		}

		default:
			BGridView::MessageReceived(message);
			break;
	}
}


orientation
WindowsView::_Orientation(uint32 location)
{
	return (location & (kTopEdge | kBottomEdge)) != 0
		? B_HORIZONTAL : B_VERTICAL;
}
