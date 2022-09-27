/*
 * Copyright 2020, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include "InputIcons.h"

#include <Application.h>
#include <ControlLook.h>
#include <File.h>
#include <IconUtils.h>
#include <Resources.h>
#include <Roster.h>

#include "IconHandles.h"


const BRect InputIcons::sBounds;


InputIcons::InputIcons()
	:
	mouseIcon(NULL, false),
	touchpadIcon(NULL, false),
	keyboardIcon(NULL, false)
{
	if (!sBounds.IsValid()) {
		*const_cast<BRect*>(&sBounds) = BRect(BPoint(0, 0),
			be_control_look->ComposeIconSize(B_MINI_ICON));
	}

	app_info info;
	be_app->GetAppInfo(&info);
	BFile executableFile(&info.ref, B_READ_ONLY);
	BResources resources(&executableFile);
	resources.PreloadResourceType(B_VECTOR_ICON_TYPE);

	_LoadBitmap(&resources);
}


void
InputIcons::_LoadBitmap(BResources* resources)
{
	const uint8* mouse;
	const uint8* touchpad;
	const uint8* keyboard;

	size_t size;

	mouse = (const uint8*)resources->LoadResource(
		B_VECTOR_ICON_TYPE, "mouse_icon", &size);
	if (mouse) {
		mouseIcon = new BBitmap(sBounds, 0, B_RGBA32);
		BIconUtils::GetVectorIcon(mouse, size, &mouseIcon);
	}

	touchpad = (const uint8*)resources->LoadResource(
		B_VECTOR_ICON_TYPE, "touchpad_icon", &size);
	if (touchpad) {
		touchpadIcon = new BBitmap(sBounds, 0, B_RGBA32);
		BIconUtils::GetVectorIcon(touchpad, size, &touchpadIcon);
	}

	keyboard = (const uint8*)resources->LoadResource(
		B_VECTOR_ICON_TYPE, "keyboard_icon", &size);
	if (keyboard) {
		keyboardIcon = new BBitmap(sBounds, 0, B_RGBA32);
		BIconUtils::GetVectorIcon(keyboard, size, &keyboardIcon);
	}
}


BRect
InputIcons::IconRectAt(const BPoint& topLeft)
{
	return BRect(sBounds).OffsetToSelf(topLeft);
}
