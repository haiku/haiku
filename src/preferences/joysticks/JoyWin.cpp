/*
 * Copyright 2007-2009 Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Oliver Ruiz Dorantes	oliver.ruiz.dorantes_at_gmail.com
 *		Ryan Leavengood			leavengood@gmail.com
 *		Fredrik Modéen 			fredrik@modeen.se
 */

#include "JoyWin.h"
#include "MessageWin.h"
#include "CalibWin.h"
#include "Global.h"
#include "PortItem.h"
//#include "FileReadWrite.h"

#include <stdio.h>
#include <stdlib.h>

#include <Box.h>
#include <Button.h>
#include <CheckBox.h>
#include <ListView.h>
#include <ListItem.h>
#include <ScrollView.h>
#include <String.h>
#include <StringView.h>
#include <Application.h>
#include <View.h>
#include <Path.h>
#include <Entry.h>
#include <Directory.h>
#include <Alert.h>
#include <File.h>
#include <SymLink.h>
#include <Messenger.h>
#include <Directory.h>
#include <Joystick.h>
#include <FindDirectory.h>
#include <Joystick.h>

#define JOYSTICKPATH "/dev/joystick/"
#define JOYSTICKFILEPATH "/boot/system/data/joysticks/"
#define JOYSTICKFILESETTINGS "/boot/home/config/settings/joysticks/"
#define SELECTGAMEPORTFIRST "Select a game port first"

static int
ShowMessage(char* string)
{
	BAlert *alert = new BAlert("Message", string, "OK");
	alert->SetFlags(alert->Flags() | B_CLOSE_ON_ESCAPE);
	return alert->Go();
}

JoyWin::JoyWin(BRect frame, const char *title)
	: BWindow(frame, title, B_DOCUMENT_WINDOW_LOOK, B_NORMAL_WINDOW_FEEL,
		B_NOT_ZOOMABLE), fSystemUsedSelect(false),
		fJoystick(new BJoystick)
{
	fProbeButton = new BButton(BRect(15.00, 260.00, 115.00, 285.00),
		"ProbeButton", "Probe", new BMessage(PROBE));

	fCalibrateButton = new BButton(BRect(270.00, 260.00, 370.00, 285.00),
		"CalibrateButton", "Calibrate", new BMessage(CALIBRATE));

	fGamePortL = new BListView(BRect(15.00, 30.00, 145.00, 250.00),
		"gamePort");
	fGamePortL->SetSelectionMessage(new BMessage(PORT_SELECTED));
	fGamePortL->SetInvocationMessage(new BMessage(PORT_INVOKE));

	fConControllerL = new BListView(BRect(175.00,30.00,370.00,250.00),
		"conController");
	fConControllerL->SetSelectionMessage(new BMessage(JOY_SELECTED));
	fConControllerL->SetInvocationMessage(new BMessage(JOY_INVOKE));

	fGamePortS = new BStringView(BRect(15, 5, 160, 25), "gpString",
		"Game port");
	fGamePortS->SetFont(be_bold_font);
	fConControllerS = new BStringView(BRect(170, 5, 330, 25), "ccString",
		"Connected controller");

	fConControllerS->SetFont(be_bold_font);

	fCheckbox = new BCheckBox(BRect(131.00, 260.00, 227.00, 280.00),
		"Disabled", "Disabled", new BMessage(DISABLEPORT));
	BBox *box = new BBox( Bounds(),"box", B_FOLLOW_ALL,
		B_WILL_DRAW | B_FRAME_EVENTS | B_FULL_UPDATE_ON_RESIZE,
		B_PLAIN_BORDER);

	box->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));

	// Add listViews with their scrolls
	box->AddChild(new BScrollView("PortScroll", fGamePortL,
		B_FOLLOW_LEFT | B_FOLLOW_TOP_BOTTOM, B_WILL_DRAW, false, true));

	box->AddChild(new BScrollView("ConScroll", fConControllerL, B_FOLLOW_ALL,
		B_WILL_DRAW, false, true));

	// Adding object
	box->AddChild(fCheckbox);
	box->AddChild(fGamePortS);
	box->AddChild(fConControllerS);
	box->AddChild(fProbeButton);
	box->AddChild(fCalibrateButton);
	AddChild(box);

	SetSizeLimits(400, 600, Bounds().Height(), Bounds().Height());

	/* Add all the devices */
	int32 nr = fJoystick->CountDevices();
	for (int32 i = 0; i < nr;i++) {
		//BString str(path.Path());
		char buf[B_OS_NAME_LENGTH];
		fJoystick->GetDeviceName(i, buf, B_OS_NAME_LENGTH);
		fGamePortL->AddItem(new PortItem(buf));
	}
	fGamePortL->Select(0);

	/* Add the joysticks specifications */
	_AddToList(fConControllerL, JOY_SELECTED, JOYSTICKFILEPATH);

	_GetSettings();
}


JoyWin::~JoyWin()
{
	//delete fFileTempProbeJoystick;
	be_app_messenger.SendMessage(B_QUIT_REQUESTED);
}


void
JoyWin::MessageReceived(BMessage *message)
{
//	message->PrintToStream();
	switch(message->what)
	{
		case DISABLEPORT:
		break;
		{
			PortItem *item = _GetSelectedItem(fGamePortL);
			if (item != NULL) {
				//ToDo: item->SetEnabled(true);
				//don't work as you can't select a item that are disabled
				if(fCheckbox->Value()) {
					item->SetEnabled(false);
					_SelectDeselectJoystick(fConControllerL, false);
				} else {
					item->SetEnabled(true);
					_SelectDeselectJoystick(fConControllerL, true);
					_PerformProbe(item->Text());
				}
			} //else
				//printf("We have a null value\n");
		break;
		}

		case PORT_SELECTED:
		{
			PortItem *item = _GetSelectedItem(fGamePortL);
			if (item != NULL) {
				fSystemUsedSelect = true;
				if (item->IsEnabled()) {
					//printf("SetEnabled = false\n");
					fCheckbox->SetValue(false);
					_SelectDeselectJoystick(fConControllerL, true);
				} else {
					//printf("SetEnabled = true\n");
					fCheckbox->SetValue(true);
					_SelectDeselectJoystick(fConControllerL, false);
				}

				if (_CheckJoystickExist(item->Text()) == B_ERROR) {
					if (_ShowCantFindFileMessage(item->Text()) == B_OK) {
						_PerformProbe(item->Text());
					}
				} else {
					BString str(_FindFilePathForSymLink(JOYSTICKFILESETTINGS,
						item));
					if (str != NULL) {
						BString str(_FixPathToName(str.String()));
						int32 id = _FindStringItemInList(fConControllerL,
									new PortItem(str.String()));
						if (id > -1) {
							fConControllerL->Select(id);
							item->SetJoystickName(BString(str.String()));
						}
					}
				}
			} else {
				fConControllerL->DeselectAll();
				ShowMessage((char*)SELECTGAMEPORTFIRST);
			}
		break;
		}

		case PROBE:
		case PORT_INVOKE:
		{
			PortItem *item = _GetSelectedItem(fGamePortL);
			if (item != NULL) {
				//printf("invoke.. inte null\n");
				_PerformProbe(item->Text());
			} else
				ShowMessage((char*)SELECTGAMEPORTFIRST);
		break;
		}

		case JOY_SELECTED:
		{
			if (!fSystemUsedSelect) {
				PortItem *controllerName = _GetSelectedItem(fConControllerL);
				PortItem *portname = _GetSelectedItem(fGamePortL);
				if (portname != NULL && controllerName != NULL) {
					portname->SetJoystickName(BString(controllerName->Text()));

					BString str = portname->GetOldJoystickName();
					if (str != NULL) {
						BString strOldFile(JOYSTICKFILESETTINGS);
						strOldFile.Append(portname->Text());
						BEntry entry(strOldFile.String());
						entry.Remove();
					}
					BString strLinkPlace(JOYSTICKFILESETTINGS);
					strLinkPlace.Append(portname->Text());

					BString strLinkTo(JOYSTICKFILEPATH);
					strLinkTo.Append(controllerName->Text());

					BDirectory *dir = new BDirectory();
					dir->CreateSymLink(strLinkPlace.String(),
						strLinkTo.String(), NULL);
				} else
					ShowMessage((char*)SELECTGAMEPORTFIRST);
			}

			fSystemUsedSelect = false;
		break;
		}

		case CALIBRATE:
		case JOY_INVOKE:
		{
			PortItem *controllerName = _GetSelectedItem(fConControllerL);
			PortItem *portname = _GetSelectedItem(fGamePortL);
			if (portname != NULL) {
				if (controllerName == NULL)
					_ShowNoDeviceConnectedMessage("known", portname->Text());
				else {
					_ShowNoDeviceConnectedMessage(controllerName->Text(), portname->Text());
					/*
					ToDo:
					Check for a device, and show calibrate window if so
					*/
				}
			} else
				ShowMessage((char*)SELECTGAMEPORTFIRST);
		break;
		}
		default:
			BWindow::MessageReceived(message);
			break;
	}
}


bool
JoyWin::QuitRequested()
{
	_ApplyChanges();
	return BWindow::QuitRequested();
}


//---------------------- Private ---------------------------------//
status_t
JoyWin::_AddToList(BListView *list, uint32 command, const char* rootPath,
	BEntry *rootEntry)
{
	BDirectory root;

	if ( rootEntry != NULL )
		root.SetTo( rootEntry );
	else if ( rootPath != NULL )
		root.SetTo( rootPath );
	else
		return B_ERROR;

	BEntry entry;
	while ((root.GetNextEntry(&entry)) > B_ERROR ) {
		if (entry.IsDirectory()) {
			_AddToList(list, command, rootPath, &entry);
		} else {
			BPath path;
			entry.GetPath(&path);
			BString str(path.Path());
			str.RemoveFirst(rootPath);
			list->AddItem(new PortItem(str.String()));
		}
	}
	return B_OK;
}


status_t
JoyWin::_Calibrate()
{
	CalibWin* calibw;
	BRect rect(100, 100, 500, 400);
	calibw = new CalibWin(rect, "Calibrate", B_DOCUMENT_WINDOW_LOOK,
		B_NORMAL_WINDOW_FEEL, B_NOT_RESIZABLE | B_NOT_ZOOMABLE);
	calibw->Show();
	return B_OK;
}


status_t
JoyWin::_PerformProbe(const char* path)
{
	status_t err = B_ERROR;
	err = _ShowProbeMesage(path);
	if (err != B_OK) {
		return err;
	}

	MessageWin* mesgw = new MessageWin(Frame(),"Probing", B_MODAL_WINDOW_LOOK,
		B_MODAL_APP_WINDOW_FEEL, B_NOT_RESIZABLE | B_NOT_ZOOMABLE);

	mesgw->Show();
	int32 number = fConControllerL->CountItems();
	PortItem *item;
	for (int i = 0; i < number; i++) {
		// Do a search in JOYSTICKFILEPATH with item->Text() find the string
		// that starts with "gadget" (tex gadget = "GamePad Pro") remove
		// spacing and the "=" ty to open this one, if failed move to next and
		// try to open.. list those that suxesfully work
		fConControllerL->Select(i);
		int32 selected = fConControllerL->CurrentSelection();
		item = dynamic_cast<PortItem*>(fConControllerL->ItemAt(selected));
		BString str("Looking for: ");
		str << item->Text() << " in port " << path;
		mesgw->SetText(str.String());
		_FindSettingString(item->Text(), JOYSTICKFILEPATH);
		//Need a check to find joysticks (don't know how right now so show a
		// don't find message)
	}
	mesgw->Hide();

	//Need a if !found then show this message. else list joysticks.
	_ShowNoCompatibleJoystickMessage();
	return B_OK;
}


status_t
JoyWin::_ApplyChanges()
{
	BString str = _BuildDisabledJoysticksFile();
	//ToDo; Save the string as the file "disabled_joysticks" under settings
	//   (/boot/home/config/settings/disabled_joysticks)
	return B_OK;
}


status_t
JoyWin::_GetSettings()
{
	// ToDo; Read the file "disabled_joysticks" and make the port with the
	// same name disabled (/boot/home/config/settings/disabled_joysticks)
	return B_OK;
}


status_t
JoyWin::_CheckJoystickExist(const char* path)
{
	BString str(JOYSTICKFILESETTINGS);
	str << path;

	BFile file;
	status_t status = file.SetTo(str.String(), B_READ_ONLY | B_FAIL_IF_EXISTS);

	if (status == B_FILE_EXISTS || status == B_OK)
		return B_OK;
	else
		return B_ERROR;
}


status_t
JoyWin::_ShowProbeMesage(const char* device)
{
	BString str("An attempt will be made to probe the port '");
	str << device << "' to try to figure out what kind of joystick (if any) ";
	str << "are attached. There is a small chance this process might cause ";
	str << "your machine to lock up and require a reboot. Make sure you have ";
	str << "saved changes in all open applications before you start probing.";

	BAlert *alert = new BAlert("test1", str.String(), "Probe", "Cancel");
	alert->SetShortcut(1, B_ESCAPE);
	int32 bindex = alert->Go();

	if (bindex == 0)
		return B_OK;
	else
		return B_ERROR;
}


//Used when a files/joysticks are no were to be found
status_t
JoyWin::_ShowCantFindFileMessage(const char* port)
{
	BString str("The file '");
	str <<  _FixPathToName(port).String() << "' used by '" << port;
	str << "' cannot be found.\n Do you want to ";
	str << "try auto-detecting a joystick for this port?";

	BAlert *alert = new BAlert("test1", str.String(), "Stop", "Probe");
	alert->SetShortcut(1, B_ENTER);
	int32 bindex = alert->Go();

	if (bindex == 1)
		return B_OK;
	else
		return B_ERROR;
}


void
JoyWin::_ShowNoCompatibleJoystickMessage()
{
	BString str("There were no compatible joysticks detected on this game");
	str << " port. Try another port, or ask the manufacturer of your joystick";
	str << " for a driver designed for Haiku or BeOS.";

	BAlert *alert = new BAlert("test1", str.String(), "OK");
	alert->SetFlags(alert->Flags() | B_CLOSE_ON_ESCAPE);
	alert->Go();
}

void
JoyWin::_ShowNoDeviceConnectedMessage(const char* joy, const char* port)
{
	BString str("There does not appear to be a ");
	str << joy << " device connected to the port '" << port << "'.";

	BAlert *alert = new BAlert("test1", str.String(), "Stop");
	alert->SetFlags(alert->Flags() | B_CLOSE_ON_ESCAPE);
	alert->Go();
}


// Use this function to get a string of disabled ports
BString
JoyWin::_BuildDisabledJoysticksFile()
{
	BString temp("# This is a list of disabled joystick devices.");
	temp << "# Do not include the /dev/joystick/ part of the device name.";

	int32 number = fGamePortL->CountItems();
	PortItem *item;
	for (int i = 0; i < number; i++) {
		item = dynamic_cast<PortItem*>(fGamePortL->ItemAt(i));
		if (!item->IsEnabled())
			temp << "disable = \"" <<  item->Text() << "\"";
	}
	return temp;
}


PortItem*
JoyWin::_GetSelectedItem(BListView* list)
{
	int32 id = list->CurrentSelection();
	if (id > -1) {
		//PortItem *item = dynamic_cast<PortItem*>(list->ItemAt(id));
		return dynamic_cast<PortItem*>(list->ItemAt(id));
	} else {
		return NULL;
	}
}


BString
JoyWin::_FixPathToName(const char* port)
{
	BString temp(port);
	temp = temp.Remove(0, temp.FindLast('/') + 1) ;
	return temp;
}


void
JoyWin::_SelectDeselectJoystick(BListView* list, bool enable)
{
	list->DeselectAll();
	int32 number = fGamePortL->CountItems();
	PortItem *item;
	for (int i = 0; i < number; i++) {
		item = dynamic_cast<PortItem*>(list->ItemAt(i));
		item->SetEnabled(enable);
	}
}


int32
JoyWin::_FindStringItemInList(BListView *view, PortItem *item)
{
	PortItem *strItem = NULL;
	int32 number = view->CountItems();
	for (int32 i = 0; i < number; i++) {
		strItem = dynamic_cast<PortItem*>(view->ItemAt(i));
		if (!strcmp(strItem->Text(), item->Text())) {
			return i;
		}
	}
	delete strItem;
	return -1;
}


BString
JoyWin::_FindFilePathForSymLink(const char* symLinkPath, PortItem *item)
{
	BPath path(symLinkPath);
	path.Append(item->Text());
	BEntry entry(path.Path());
	if (entry.IsSymLink()) {
		BSymLink symLink(&entry);
		BDirectory parent;
		entry.GetParent(&parent);
		symLink.MakeLinkedPath(&parent, &path);
		BString str(path.Path());
		return str;
	}
	return NULL;
}


status_t
JoyWin::_FindStick(const char* name)
{
	BJoystick *stick = new BJoystick();
	return stick->Open(name);
}


const char*
JoyWin::_FindSettingString(const char* name, const char* strPath)
{
	//Make a BJoystick try open it
	BString str;

	BPath path(strPath);
	path.Append(name);
	fFileTempProbeJoystick = new BFile(path.Path(), B_READ_ONLY);

	//status_t err = find_directory(B_COMMON_ETC_DIRECTORY, &path);
//	if (err == B_OK) {
		//BString str(path.Path());
		//str << "/joysticks/" << name;
		//printf("path'%s'\n", str.String());
		//err = file->SetTo(strPath, B_READ_ONLY);
		status_t err = fFileTempProbeJoystick->InitCheck();
		if (err == B_OK) {
			//FileReadWrite frw(fFileTempProbeJoystick);
			//printf("Get going\n");
			//printf("Opening file = %s\n", path.Path());
			//while (frw.Next(str)) {
				//printf("In While loop\n");
			//	printf("getline %s \n", str.String());
				//Test to open joystick with x number
			//}
			/*while (_GetLine(str)) {
				//printf("In While loop\n");
				printf("getline %s \n", str.String());
				//Test to open joystick with x number
			}*/
			return "";
		} else
			printf("BFile.SetTo error: %s, Path = %s\n", strerror(err), str.String());
//	} else
//		printf("find_directory error: %s\n", strerror(err));

//	delete file;
	return "";
}

/*
//Function to get a line from a file
bool
JoyWin::_GetLine(BString& string)
{
	// Fill up the buffer with the first chunk of code
	if (fPositionInBuffer == 0)
		fAmtRead = fFileTempProbeJoystick->Read(&fBuffer, sizeof(fBuffer));
	while (fAmtRead > 0) {
		while (fPositionInBuffer < fAmtRead) {
			// Return true if we hit a newline or the end of the file
			if (fBuffer[fPositionInBuffer] == '\n') {
				fPositionInBuffer++;
				//Convert begin
				int32 state = 0;
				int32 bufferLen = string.Length();
				int32 destBufferLen = bufferLen;
				char destination[destBufferLen];
//				if (fSourceEncoding)
//					convert_to_utf8(fSourceEncoding, string.String(), &bufferLen, destination, &destBufferLen, &state);
				string = destination;
				return true;
			}
			string += fBuffer[fPositionInBuffer];
			fPositionInBuffer++;
		}

		// Once the buffer runs out, grab some more and start again
		fAmtRead = fFileTempProbeJoystick->Read(&fBuffer, sizeof(fBuffer));
		fPositionInBuffer = 0;
	}
	return false;
}
*/
