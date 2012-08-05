/*
 * Copyright 2004-2009, Haiku. All rights reserved.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Jérôme Duval
 */


#include "MethodReplicant.h"

#include <new>
#include <string.h>

#include <Alert.h>
#include <AppDefs.h>
#include <Debug.h>
#include <Dragger.h>
#include <Bitmap.h>
#include <MenuItem.h>
#include <Message.h>
#include <Messenger.h>
#include <PopUpMenu.h>

#include "remote_icon.h"

#include "InputServerTypes.h"

#ifdef DEBUG
#	define CALLED() PRINT(("CALLED %s \n", __PRETTY_FUNCTION__));
#else
#	define CALLED()
#endif


MethodReplicant::MethodReplicant(const char* signature)
	:
	BView(BRect(0, 0, 15, 15), REPLICANT_CTL_NAME, B_FOLLOW_ALL, B_WILL_DRAW),
	fMenu("", false, false)
{
	// Background Bitmap
	fSegments = new BBitmap(BRect(0, 0, kRemoteWidth - 1, kRemoteHeight - 1),
		kRemoteColorSpace);
	fSegments->SetBits(kRemoteBits, kRemoteWidth * kRemoteHeight, 0,
		kRemoteColorSpace);
	// Background Color

	// add dragger
	BRect rect(Bounds());
	rect.left = rect.right - 7.0;
	rect.top = rect.bottom - 7.0;
	BDragger* dragger = new BDragger(rect, this,
		B_FOLLOW_RIGHT | B_FOLLOW_BOTTOM);
	dragger->SetViewColor(B_TRANSPARENT_32_BIT);
	AddChild(dragger);

	ASSERT(signature != NULL);
	fSignature = strdup(signature);

	fMenu.SetFont(be_plain_font);
	fMenu.SetRadioMode(true);
}


MethodReplicant::MethodReplicant(BMessage* message)
	:
	BView(message),
	fMenu("", false, false)
{
	// Background Bitmap
	fSegments = new BBitmap(BRect(0, 0, kRemoteWidth - 1, kRemoteHeight - 1),
		kRemoteColorSpace);
	fSegments->SetBits(kRemoteBits, kRemoteWidth * kRemoteHeight, 0,
		kRemoteColorSpace);

	const char* signature = NULL;
	message->FindString("add_on", &signature);
	ASSERT(signature != NULL);
	fSignature = strdup(signature);

	fMenu.SetFont(be_plain_font);
	fMenu.SetRadioMode(true);
}


MethodReplicant::~MethodReplicant()
{
	delete fSegments;
	free(fSignature);
}


// archiving overrides
MethodReplicant*
MethodReplicant::Instantiate(BMessage* data)
{
	CALLED();
	if (!validate_instantiation(data, REPLICANT_CTL_NAME))
		return NULL;
	return new(std::nothrow) MethodReplicant(data);
}


status_t
MethodReplicant::Archive(BMessage* data, bool deep) const
{
	BView::Archive(data, deep);

	data->AddString("add_on", fSignature);
	return B_NO_ERROR;
}


void
MethodReplicant::AttachedToWindow()
{
	CALLED();

	SetViewColor(Parent()->ViewColor());

	BMessenger messenger(this);
	BMessage msg(IS_METHOD_REGISTER);
	msg.AddMessenger("address", messenger);

	BMessenger inputMessenger(fSignature);
	if (inputMessenger.SendMessage(&msg) != B_OK)
		printf("error when contacting input_server\n");
}


void
MethodReplicant::MessageReceived(BMessage* message)
{
	PRINT(("%s what:%c%c%c%c\n", __PRETTY_FUNCTION__, message->what >> 24,
		message->what >> 16, message->what >> 8, message->what));
	PRINT_OBJECT(*message);

	switch (message->what) {
		case B_ABOUT_REQUESTED:
		{
			BAlert* alert = new BAlert("About Method Replicant",
				"Method Replicant (Replicant)\n"
				"  Brought to you by Jérôme DUVAL.\n\n"
				"Haiku, 2004-2009", "OK");
			alert->SetFlags(alert->Flags() | B_CLOSE_ON_ESCAPE);
			alert->Go();
			break;
		}
		case IS_UPDATE_NAME:
			UpdateMethodName(message);
			break;
		case IS_UPDATE_ICON:
			UpdateMethodIcon(message);
			break;
		case IS_UPDATE_MENU:
			UpdateMethodMenu(message);
			break;
		case IS_UPDATE_METHOD:
			UpdateMethod(message);
			break;
		case IS_ADD_METHOD:
			AddMethod(message);
			break;
		case IS_REMOVE_METHOD:
			RemoveMethod(message);
			break;

		default:
			BView::MessageReceived(message);
			break;
	}
}


void
MethodReplicant::Draw(BRect rect)
{
	BView::Draw(rect);

	SetDrawingMode(B_OP_OVER);
	DrawBitmap(fSegments);
}


void
MethodReplicant::MouseDown(BPoint point)
{
	CALLED();
	uint32 mouseButtons;
	BPoint where;
	GetMouse(&where, &mouseButtons, true);

	where = ConvertToScreen(point);

	fMenu.SetTargetForItems(this);
	BMenuItem* item = fMenu.Go(where, true, true,
		BRect(where - BPoint(4, 4), where + BPoint(4, 4)));

	if (dynamic_cast<MethodMenuItem*>(item) != NULL) {
		BMessage msg(IS_SET_METHOD);
		msg.AddInt32("cookie", ((MethodMenuItem*)item)->Cookie());
		BMessenger messenger(fSignature);
		messenger.SendMessage(&msg);
	}
}


void
MethodReplicant::MouseUp(BPoint point)
{
	/* don't Quit() ! thanks for FFM users */
}


void
MethodReplicant::UpdateMethod(BMessage* message)
{
	CALLED();
	int32 cookie;
	if (message->FindInt32("cookie", &cookie) != B_OK) {
		fprintf(stderr, "can't find cookie in message\n");
		return;
	}

	MethodMenuItem* item = FindItemByCookie(cookie);
	if (item == NULL) {
		fprintf(stderr, "can't find item with cookie %lx\n", cookie);
		return;
	}
	item->SetMarked(true);

	fSegments->SetBits(item->Icon(), kRemoteWidth * kRemoteHeight, 0,
		kRemoteColorSpace);
	Invalidate();
}


void
MethodReplicant::UpdateMethodIcon(BMessage* message)
{
	CALLED();
	int32 cookie;
	if (message->FindInt32("cookie", &cookie) != B_OK) {
		fprintf(stderr, "can't find cookie in message\n");
		return;
	}

	const uchar* data;
	ssize_t numBytes;
	if (message->FindData("icon", B_ANY_TYPE, (const void**)&data, &numBytes)
			!= B_OK) {
		fprintf(stderr, "can't find icon in message\n");
		return;
	}

	MethodMenuItem* item = FindItemByCookie(cookie);
	if (item == NULL) {
		fprintf(stderr, "can't find item with cookie 0x%lx\n", cookie);
		return;
	}

	item->SetIcon(data);
}


void
MethodReplicant::UpdateMethodMenu(BMessage* message)
{
	CALLED();
	int32 cookie;
	if (message->FindInt32("cookie", &cookie) != B_OK) {
		fprintf(stderr, "can't find cookie in message\n");
		return;
	}

	BMessage msg;
	if (message->FindMessage("menu", &msg) != B_OK) {
		fprintf(stderr, "can't find menu in message\n");
		return;
	}
	PRINT_OBJECT(msg);

	BMessenger messenger;
	if (message->FindMessenger("target", &messenger) != B_OK) {
		fprintf(stderr, "can't find target in message\n");
		return;
	}

	BMenu* menu = (BMenu*)BMenu::Instantiate(&msg);
	if (menu == NULL) {
		PRINT(("can't instantiate menu\n"));
	} else
		menu->SetTargetForItems(messenger);

	MethodMenuItem* item = FindItemByCookie(cookie);
	if (item == NULL) {
		fprintf(stderr, "can't find item with cookie 0x%lx\n", cookie);
		return;
	}
	int32 index = fMenu.IndexOf(item);

	MethodMenuItem* item2 = NULL;
	if (menu) {
		item2 = new MethodMenuItem(cookie, item->Label(), item->Icon(),
			menu, messenger);
	} else
		item2 = new MethodMenuItem(cookie, item->Label(), item->Icon());

	item = (MethodMenuItem*)fMenu.RemoveItem(index);
	fMenu.AddItem(item2, index);
	item2->SetMarked(item->IsMarked());
	delete item;
}


void
MethodReplicant::UpdateMethodName(BMessage* message)
{
	CALLED();
	int32 cookie;
	if (message->FindInt32("cookie", &cookie) != B_OK) {
		fprintf(stderr, "can't find cookie in message\n");
		return;
	}

	const char* name;
	if (message->FindString("name", &name) != B_OK) {
		fprintf(stderr, "can't find name in message\n");
		return;
	}

	MethodMenuItem* item = FindItemByCookie(cookie);
	if (item == NULL) {
		fprintf(stderr, "can't find item with cookie 0x%lx\n", cookie);
		return;
	}

	item->SetName(name);
}


MethodMenuItem*
MethodReplicant::FindItemByCookie(int32 cookie)
{
	for (int32 i = 0; i < fMenu.CountItems(); i++) {
		MethodMenuItem* item = (MethodMenuItem*)fMenu.ItemAt(i);
		PRINT(("cookie : 0x%lx\n", item->Cookie()));
		if (item->Cookie() == cookie)
			return item;
	}

	return NULL;
}


void
MethodReplicant::AddMethod(BMessage* message)
{
	CALLED();
	int32 cookie;
	if (message->FindInt32("cookie", &cookie) != B_OK) {
		fprintf(stderr, "can't find cookie in message\n");
		return;
	}

	const char* name;
	if (message->FindString("name", &name) != B_OK) {
		fprintf(stderr, "can't find name in message\n");
		return;
	}

	const uchar* icon;
	ssize_t numBytes;
	if (message->FindData("icon", B_ANY_TYPE, (const void**)&icon, &numBytes)
			!= B_OK) {
		fprintf(stderr, "can't find icon in message\n");
		return;
	}

	MethodMenuItem* item = FindItemByCookie(cookie);
	if (item != NULL) {
		fprintf(stderr, "item with cookie %lx already exists\n", cookie);
		return;
	}

	item = new MethodMenuItem(cookie, name, icon);
	fMenu.AddItem(item);
	item->SetTarget(this);

	if (fMenu.CountItems() == 1)
		item->SetMarked(true);
}


void
MethodReplicant::RemoveMethod(BMessage* message)
{
	CALLED();
	int32 cookie;
	if (message->FindInt32("cookie", &cookie) != B_OK) {
		fprintf(stderr, "can't find cookie in message\n");
		return;
	}

	MethodMenuItem* item = FindItemByCookie(cookie);
	if (item == NULL) {
		fprintf(stderr, "can't find item with cookie %lx\n", cookie);
		return;
	}
	fMenu.RemoveItem(item);
	delete item;
}
