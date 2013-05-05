/*
 * Copyright 2011 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 1999 M.Kawamura
 */


#include "CannaLooper.h"

#include <string.h>

#include <Debug.h>

#include <Alert.h>
#include <FindDirectory.h>
#include <Input.h>
#include <Menu.h>
#include <MenuItem.h>
#include <Messenger.h>
#include <Path.h>
#include <Screen.h>

#include "CannaCommon.h"
#include "CannaMethod.h"
#include "KouhoWindow.h"
#include "PaletteWindow.h"


CannaLooper::CannaLooper(CannaMethod* method)
	: BLooper("Canna Looper"),
	fOwner(method),
	fCanna(NULL),
	fKouhoWindow(NULL),
	fPaletteWindow(NULL)
{
	fMenu = new BMenu(B_EMPTY_STRING);
	fMenu->SetFont(be_plain_font);
	fMenu->AddItem(new BMenuItem("About CannaIM" B_UTF8_ELLIPSIS,
		new BMessage( B_ABOUT_REQUESTED)));
	fMenu->AddSeparatorItem();
	fMenu->AddItem(new BMenuItem("Convert arrow keys",
		new BMessage(ARROW_KEYS_FLIPPED)));
	fMenu->AddItem(new BMenuItem("Reload Init file",
		new BMessage(RELOAD_INIT_FILE)));

	if (gSettings.convert_arrowkey) {
		BMenuItem* item = fMenu->FindItem(ARROW_KEYS_FLIPPED);
		item->SetMarked(true);
	}

	Run();
}


status_t
CannaLooper::Init()
{
	char basePath[B_PATH_NAME_LENGTH + 1];
	status_t err = ReadSettings(basePath);
	if (err != B_NO_ERROR)
		return err;

	fCanna = new CannaInterface(basePath);

	return fCanna->InitCheck();
}


void
CannaLooper::Quit()
{
	// delete palette here
	SERIAL_PRINT(("CannaLooper: destructor called.\n"));
	delete fCanna;

	if (fKouhoWindow != NULL) {
		SERIAL_PRINT(("CannaLooper: Sending QUIT to kouho window...\n"));

		fKouhoWindow->Lock();
		fKouhoWindow->Quit();
	}

	if (fPaletteWindow) {
		SERIAL_PRINT(("CannaLooper: Sending QUIT to palette...\n"));

		fPaletteWindow->Lock();
		fPaletteWindow->Quit();
	}

	fOwner->SetMenu(NULL, BMessenger());
	delete fMenu;
	BLooper::Quit();
}


status_t
CannaLooper::ReadSettings(char* basePath)
{
	BPath path;
	status_t status = find_directory(B_SYSTEM_DATA_DIRECTORY, &path);
	if (status != B_OK)
		return status;

	path.Append("Canna");

	strlcpy(basePath, path.Path(), B_PATH_NAME_LENGTH);
	strlcat(basePath, "/", B_PATH_NAME_LENGTH);

	font_family family;
	font_style style;
	strcpy(family, "VL PGothic");
	strcpy(style, "regular");

	fKouhoFont.SetFamilyAndStyle(family, style);
	fKouhoFont.SetSize(12);

	return B_OK;
}


void
CannaLooper::EnqueueMessage(BMessage* msg)
{
	fOwner->EnqueueMessage(msg);
}


void
CannaLooper::MessageReceived(BMessage* msg)
{
	SERIAL_PRINT(("CannaLooper: Entering MessageReceived() what=%.4s\n",
		(char*)&msg->what));

	switch (msg->what) {
		case B_KEY_DOWN:
		case NUM_SELECTED_FROM_KOUHO_WIN:
			_HandleKeyDown(msg);
			break;

		case B_INPUT_METHOD_EVENT:
			uint32 opcode, result;
			msg->FindInt32("be:opcode", (int32*)&opcode);

			switch (opcode) {
				case B_INPUT_METHOD_LOCATION_REQUEST:
					_HandleLocationReply(msg);
					break;

				case B_INPUT_METHOD_STOPPED:
					SERIAL_PRINT(("CannaLooper: B_INPUT_METHOD_STOPPED "
						"received\n"));
					_ForceKakutei();
					break;
			}
			break;

		case CANNA_METHOD_ACTIVATED:
			SERIAL_PRINT(("CannaLooper: CANNA_METHOD_ACTIVATED received\n"));
			_HandleMethodActivated(msg->HasBool("active"));
			break;

		case MODE_CHANGED_FROM_PALETTE:
			_ForceKakutei();
			int32 mode;
			msg->FindInt32("mode", &mode);
			result = fCanna->ChangeMode(mode);
			_ProcessResult(result);
			break;

		case B_ABOUT_REQUESTED:
		{
			SERIAL_PRINT(("CannaLooper: B_ABOUT_REQUESTED received\n"));

			BAlert* panel = new BAlert( "", "Canna Input Method\n"
				"  by Masao Kawamura 1999\n\n"
				"Canna\n"
				"  Copyright 1992 NEC Corporation, Tokyo, Japan\n"
				"  Special thanks to T.Murai for porting\n",
				"OK");
			panel->SetFlags(panel->Flags() | B_CLOSE_ON_ESCAPE);
			panel->Go();
			break;
		}

		case RELOAD_INIT_FILE:
			_ForceKakutei();
			fCanna->Reset();
			break;

		case ARROW_KEYS_FLIPPED:
		{
			BMenuItem* item = fMenu->FindItem(ARROW_KEYS_FLIPPED);
			gSettings.convert_arrowkey = !gSettings.convert_arrowkey;
			item->SetMarked(gSettings.convert_arrowkey);
			fCanna->SetConvertArrowKey(gSettings.convert_arrowkey);
			break;
		}

		default:
			BLooper::MessageReceived(msg);
			break;
	}
}


void
CannaLooper::_HandleKeyDown(BMessage* msg)
{
	uint32 modifier;
	int32 key;
	msg->FindInt32("modifiers", (int32*)&modifier);
	msg->FindInt32("key", &key);

	if ((modifier & B_COMMAND_KEY) != 0) {
		EnqueueMessage(DetachCurrentMessage());
		return;
	}

	char character;
	msg->FindInt8("byte", (int8*)&character);

	// The if clause below is to avoid processing key input which char code
	// is 0x80 or more.
	// if mikakutei string exists, dispose current message.
	// Otherwise, send it to application as usual B_KEY_DOWN message.
	if ((character & 0x80) != 0) {
		if (fCanna->MikakuteiLength() != 0)
			delete DetachCurrentMessage();
		else
			EnqueueMessage(DetachCurrentMessage());

		return;
	}

	SERIAL_PRINT(("CannaLooper: HandleKeyDown() calling "
		"CannaInterface::KeyIn()...\n"));

	uint32 result = fCanna->KeyIn(character, modifier, key);

	SERIAL_PRINT(("CannaLooper: HandleKeyDown() received result = %d from "
		"CannaInterface.\n", result));

	_ProcessResult(result);
}


void
CannaLooper::_ProcessResult(uint32 result)
{
	SERIAL_PRINT(("CannaLooper: _ProcessResult() processing result = %d\n",
		result));

	if ((result & GUIDELINE_APPEARED) != 0) {
		SERIAL_PRINT(("CannaLooper: _ProcessResult() processing "
			"GUIDELINE_APPEARED\n"));

		if (fCanna->MikakuteiLength() != 0) {
			// usual guideline i.e. kouho

			BMessage* msg = new BMessage(B_INPUT_METHOD_EVENT);
			msg->AddInt32("be:opcode", B_INPUT_METHOD_LOCATION_REQUEST);
			fOwner->EnqueueMessage(msg);

			SERIAL_PRINT(("CannaLooper: B_INPUT_METHOD_LOCATION_REQUEST has "
				"been sent\n"));
		} else {
			// guideline exists, but no mikakutei string - means extend mode
			// and such.
			SERIAL_PRINT(("  GUIDELINE_APPEARED: calling "
				"GenerateKouho()...\n"));

			fKouhoWindow->PostMessage(fCanna->GenerateKouhoString());
			SERIAL_PRINT(("  GUIDELINE_APPEARED: posting KouhoMsg to "
				"KouhoWindow %x...\n", fKouhoWindow));

			fKouhoWindow->PostMessage(KOUHO_WINDOW_SHOW_ALONE);
		}
	}

	if ((result & GUIDELINE_DISAPPEARED) != 0) {
		SERIAL_PRINT(("CannaLooper: _ProcessResult() processing "
			"GUIDELINE_DISAPPEARED\n"));
		fKouhoWindow->PostMessage(KOUHO_WINDOW_HIDE);
	}

	if ((result & MODE_CHANGED) != 0) {
		SERIAL_PRINT(("CannaLooper: _ProcessResult() processing "
			"MODE_CHANGED\n"));

		BMessage message(PALETTE_WINDOW_BUTTON_UPDATE);
		message.AddInt32("mode", fCanna->GetMode());
		fPaletteWindow->PostMessage(&message);

		SERIAL_PRINT(("CannaLooper: PALETTE_BUTTON_UPDATE has been sent. "
			"mode = %d\n", fCanna->GetMode()));
	}

	if ((result & GUIDELINE_CHANGED) != 0) {
		SERIAL_PRINT(("CannaLooper: _ProcessResult() processing "
			"GUIDELINE_CHANGED\n"));
		fKouhoWindow->PostMessage(fCanna->GenerateKouhoString());
	}

	if ((result & THROUGH_INPUT) != 0) {
		SERIAL_PRINT(("CannaLooper: _ProcessResult() processing "
			"THROUGH_INPUT\n"));
		EnqueueMessage(DetachCurrentMessage());
	}

	if ((result & NEW_INPUT_STARTED) != 0) {
		SERIAL_PRINT(("CannaLooper: _ProcessResult() processing "
			"NEW_INPUT_STARTED\n"));
		SendInputStarted();
	}

	if ((result & KAKUTEI_EXISTS) != 0) {
		SERIAL_PRINT(("CannaLooper: _ProcessResult() processing "
			"KAKUTEI_EXISTS\n"));

		BMessage* msg = new BMessage(B_INPUT_METHOD_EVENT);
		msg->AddInt32("be:opcode", B_INPUT_METHOD_CHANGED);

		msg->AddString("be:string", fCanna->GetKakuteiStr());
		msg->AddInt32("be:clause_start", 0);
		msg->AddInt32("be:clause_end", fCanna->KakuteiLength());
		msg->AddInt32("be:selection", fCanna->KakuteiLength());
		msg->AddInt32("be:selection", fCanna->KakuteiLength());
		msg->AddBool("be:confirmed", true);
		fOwner->EnqueueMessage(msg);

		SERIAL_PRINT(("CannaLooper: B_INPUT_METHOD_CHANGED (confired) has "
			"been sent\n"));

		// if both kakutei and mikakutei exist, do not send B_INPUT_STOPPED
		if (!(result & MIKAKUTEI_EXISTS))
			SendInputStopped();
	}

	if ((result & MIKAKUTEI_EXISTS) != 0) {
		SERIAL_PRINT(("CannaLooper: _ProcessResult() processing "
			"MIKAKUTEI_EXISTS\n" ));

		int32 start, finish;
		BMessage* msg = new BMessage(B_INPUT_METHOD_EVENT);
		msg->AddInt32("be:opcode", B_INPUT_METHOD_CHANGED);
		msg->AddString("be:string", fCanna->GetMikakuteiStr());

		if (fCanna->HasRev()) {
			fCanna->GetRevPosition( &start, &finish);
			msg->AddInt32("be:clause_start", 0);
			msg->AddInt32("be:clause_end", start);
			msg->AddInt32("be:clause_start", start);
			msg->AddInt32("be:clause_end", finish);
			msg->AddInt32("be:clause_start", finish);
			msg->AddInt32("be:clause_end", fCanna->MikakuteiLength());
		} else {
			start = finish = fCanna->MikakuteiLength();
			msg->AddInt32("be:clause_start", 0);
			msg->AddInt32("be:clause_end", fCanna->MikakuteiLength());
		}

		msg->AddInt32("be:selection", start);
		msg->AddInt32("be:selection", finish);
		//msg->AddBool("be:confirmed", false);
		fOwner->EnqueueMessage(msg);

		SERIAL_PRINT(("CannaLooper: B_INPUT_METHOD_CHANGED (non-confirmed) "
			"has been sent\n"));
	}

	if ((result & MIKAKUTEI_BECOME_EMPTY) != 0) {
		SERIAL_PRINT(("CannaLooper: _ProcessResult() processing "
			"MIKAKUTEI_BECOME_EMPTY\n" ));
		BMessage* msg = new BMessage(B_INPUT_METHOD_EVENT);
		msg->AddInt32("be:opcode", B_INPUT_METHOD_CHANGED);

		msg->AddString("be:string", B_EMPTY_STRING);
		msg->AddInt32("be:clause_start", 0);
		msg->AddInt32("be:clause_end", 0);
		msg->AddInt32("be:selection", 0);
		msg->AddInt32("be:selection", 0);
		msg->AddBool( "be:confirmed", true);
		fOwner->EnqueueMessage(msg);

		SERIAL_PRINT(( "CannaLooper: B_INPUT_METHOD_CHANGED (NULL, confired) "
			"has been sent\n"));

		SendInputStopped();
	}
}


void
CannaLooper::_HandleLocationReply(BMessage* msg)
{
	BPoint where = B_ORIGIN;
	float height = 0.0;
	int32 start;

	SERIAL_PRINT(("CannaLooper: B_INPUT_METHOD_LOCATION_REQUEST received\n"));

	start = fCanna->GetRevStartPositionInChar();

#ifdef DEBUG
	type_code type;
	int32 count;
	msg->GetInfo("be:location_reply", &type, &count);
	SERIAL_PRINT(("CannaLooper: LOCATION_REPLY has %d locations.\n", count));
	SERIAL_PRINT(("CannaLooper: RevStartPosition is %d.\n", start));
#endif

	msg->FindPoint("be:location_reply", start, &where);
	msg->FindFloat("be:height_reply", start, &height);
	BMessage m(KOUHO_WINDOW_SHOWAT);
	m.AddPoint("position", where);
	m.AddFloat("height", height);

	fKouhoWindow->PostMessage(fCanna->GenerateKouhoString());
	fKouhoWindow->PostMessage(&m);
}


void
CannaLooper::_HandleMethodActivated(bool active)
{
	if (active) {
		if (!fPaletteWindow) {
			// first time input method activated
			float x = gSettings.palette_loc.x;
			float y = gSettings.palette_loc.y;
			BRect frame(x, y, x + 114, y + 44);
			fPaletteWindow = new PaletteWindow(frame, this);
			fPaletteWindow->Show();
			fKouhoWindow = new KouhoWindow(&fKouhoFont, this);
			fKouhoWindow->Run();
		}

		fPaletteWindow->PostMessage(PALETTE_WINDOW_SHOW);
		fOwner->SetMenu(fMenu, this);
	} else {
		_ForceKakutei();
		fCanna->ChangeMode(CANNA_MODE_HenkanMode);
		BMessage m(PALETTE_WINDOW_BUTTON_UPDATE);
		m.AddInt32("mode", CANNA_MODE_HenkanMode);
		fPaletteWindow->PostMessage(&m);
		fPaletteWindow->PostMessage(PALETTE_WINDOW_HIDE);
		fKouhoWindow->PostMessage(KOUHO_WINDOW_HIDE);
		fOwner->SetMenu(NULL, this);
	}
}


void
CannaLooper::_ForceKakutei()
{
	SERIAL_PRINT(( "CannaLooper: _ForceKakutei() called\n" ));

	uint32 result = fCanna->Kakutei();

	SERIAL_PRINT(("CannaLooper: returned from Kakutei(). result = %d\n",
		result));

	if (result != NOTHING_TO_KAKUTEI)
		_ProcessResult(result);
	else
		SendInputStopped();
}


void
CannaLooper::SendInputStopped()
{
	BMessage* msg = new BMessage(B_INPUT_METHOD_EVENT);
	msg->AddInt32("be:opcode", B_INPUT_METHOD_STOPPED);
	EnqueueMessage(msg);

	SERIAL_PRINT(("CannaLooper: B_INPUT_METHOD_STOPPED has been sent\n"));
}


void
CannaLooper::SendInputStarted()
{
	BMessage* msg = new BMessage(B_INPUT_METHOD_EVENT);
	msg->AddInt32("be:opcode", B_INPUT_METHOD_STARTED);
	msg->AddMessenger("be:reply_to", BMessenger(NULL, this));
	EnqueueMessage(msg);

	SERIAL_PRINT(("CannaLooper: B_INPUT_METHOD_STARTED has been sent\n"));
}

