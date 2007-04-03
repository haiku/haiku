/*
 * Copyright 2007, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Hugo Santos, hugosantos@gmail.com
 */


#include "StatusReplicant.h"

#include "NetServer.h"
#include "NetworkStatusIcons.h"

#include <Alert.h>
#include <Application.h>
#include <Bitmap.h>
#include <IconUtils.h>
#include <MenuItem.h>
#include <Message.h>
#include <Messenger.h>
#include <Resources.h>
#include <TextView.h>

#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/sockio.h>


const char *kStatusReplicant = "NetStatusView";
static const uint32 kShowConfiguration = 'shcf';

static const uint8 kStatusColors[kStatusCount][3] = {
	{ 0,	0,		0 },	// Unknown
	{ 255,	0,		0 },	// NoLink
	{ 0,	0,		255 },	// LinkNoConfig
	{ 0,	255,	0 },	// Connected
	{ 255,	255,	0 },	// Preparing
};

struct information_entry {
	const char *label;
	int ioc;
};

static const information_entry kInformationEntries[] = {
	{ "Address", SIOCGIFADDR },
	{ "Broadcast", SIOCGIFBRDADDR },
	{ "Netmask", SIOCGIFNETMASK },
	{ NULL }
};

static const char *kStatusDescriptions[] = {
	"Unknown",
	"No Link",
	"No stateful configuration",
	"Ready",
	"Configuring",
};


StatusReplicant::StatusReplicant()
	: BView(BRect(0, 0, 15, 15), kStatusReplicant, B_FOLLOW_ALL, 0),
	fPopUp("", false, false)
{
	_Init();
}


StatusReplicant::StatusReplicant(BMessage *message)
	: BView(message),
	fPopUp("", false, false)
{
	_Init();
}


StatusReplicant::~StatusReplicant()
{
	for (int32 i = 0; i < kStatusCount; i++) {
		delete fBitmaps[i];
	}
}


StatusReplicant *
StatusReplicant::Instantiate(BMessage *data)
{
	if (validate_instantiation(data, kStatusReplicant))
		return new StatusReplicant(data);

	return NULL;
}


status_t
StatusReplicant::Archive(BMessage *data, bool deep) const
{
	BView::Archive(data, deep);
	data->AddString("add_on", kSignature);
	data->AddString("class", kStatusReplicant);
	return B_OK;
}


void
StatusReplicant::AttachedToWindow()
{
	BMessage message(kRegisterStatusReplicant);
	message.AddMessenger("messenger", BMessenger(this));

	if (BMessenger(kSignature).SendMessage(&message) != B_OK) {
		// TODO: Remove myself from Deskbar
	}

	_ChangeStatus(kStatusUnknown);
	_PrepareMenu(InterfaceStatusList());
}


void
StatusReplicant::MessageReceived(BMessage *message)
{
	switch (message->what) {
		case kStatusUpdate:
			_UpdateFromMessage(message);
			break;
		case kShowConfiguration:
			_ShowConfiguration(message);
			break;

		default:
			BView::MessageReceived(message);
			break;
	}
}


void
StatusReplicant::MouseDown(BPoint point)
{
	uint32 buttons;
	BPoint where;

	GetMouse(&where, &buttons, true);
	where = ConvertToScreen(point);

	fPopUp.SetTargetForItems(this);
	fPopUp.Go(where, true, true);
}


void
StatusReplicant::_Init()
{
	// Load status bitmaps from resources

	BResources* resources = BApplication::AppResources();

	for (int i = 0; i < kStatusCount; i++) {
		fBitmaps[i] = NULL;

		if (resources != NULL) {
			const void* data = NULL;
			size_t size;
			data = resources->LoadResource(B_VECTOR_ICON_TYPE,
				kNetworkStatusNoDevice + i, &size);
			if (data != NULL) {
				BBitmap* icon = new BBitmap(Bounds(), B_RGB32);
				if (icon->InitCheck() == B_OK
					&& BIconUtils::GetVectorIcon((const uint8 *)data,
						size, icon) == B_OK) {
					fBitmaps[i] = icon;
				} else
					delete icon;

				free((void*)data);
			}
		}
	}
}


void
StatusReplicant::_UpdateFromMessage(BMessage *message)
{
	int32 newStatus;
	if (message->FindInt32("net:status", &newStatus) != B_OK
		|| newStatus < 0 || newStatus >= kStatusCount)
		return;

	InterfaceStatusList status;
	int32 interfaceStatus;

	for (uint32 index = 0; message->FindInt32("interface:status", index,
			&interfaceStatus) == B_OK; index++) {
		if (interfaceStatus < 0 || interfaceStatus >= kStatusCount)
			break;

		const char *name;
		if (message->FindString("interface:name", index, &name) != B_OK)
			break;

		status.push_back(InterfaceStatus(name, interfaceStatus));
	}

	_ChangeStatus(newStatus);
	_PrepareMenu(status);
}


void
StatusReplicant::_ShowConfiguration(BMessage *message)
{
	const char *device;
	if (message->FindString("interface:name", &device) != B_OK)
		return;

	int32 status;
	if (message->FindInt32("interface:status", &status) != B_OK)
		return;

	if (status != kStatusConnected && status != kStatusLinkNoConfig)
		return;

	if (strlen(device) > IF_NAMESIZE)
		return;

	int sock = socket(AF_INET, SOCK_DGRAM, 0);
	if (sock < 0)
		return;

	ifreq request;
	memset(&request, 0, sizeof(request));
	strcpy(request.ifr_name, device);

	std::string text = device;
	text += " information:\n";
	size_t boldLength = text.size();

	for (int i = 0; kInformationEntries[i].label; i++) {
		if (ioctl(sock, kInformationEntries[i].ioc, &request,
				  sizeof(request)) < 0) {
			close(sock);
			return;
		}

		char address[32];
		if (inet_ntop(AF_INET, &((sockaddr_in *)&request.ifr_addr)->sin_addr,
					  address, sizeof(address)) == NULL) {
			close(sock);
			return;
		}

		text += "\n";
		text += kInformationEntries[i].label;
		text += ": ";
		text += address;
	}

	close(sock);

	BAlert *info = new BAlert(device, text.c_str(), "Ok");
	BTextView *view = info->TextView();
	BFont font;

	view->SetStylable(true);
	view->GetFont(&font);
	font.SetSize(12);
	font.SetFace(B_BOLD_FACE);
	view->SetFontAndColor(0, boldLength, &font);

	info->Go(NULL);
}


void
StatusReplicant::_PrepareMenu(const InterfaceStatusList &status)
{
	fPopUp.RemoveItems(0, fPopUp.CountItems(), true);

	if (status.empty()) {
		fPopUp.AddItem(new BMenuItem("No interfaces available.", NULL));
		fPopUp.ItemAt(0)->SetEnabled(false);
	} else {
		for (InterfaceStatusList::const_iterator i = status.begin();
				i != status.end(); ++i) {
			std::string label = i->first;
			label += ": ";
			label += kStatusDescriptions[i->second];

			BMessage *message = new BMessage(kShowConfiguration);
			message->AddString("interface:name", i->first.c_str());
			message->AddInt32("interface:status", i->second);

			BMenuItem *item = new BMenuItem(label.c_str(), message);
			if (i->second != kStatusConnected &&
				i->second != kStatusLinkNoConfig)
				item->SetEnabled(false);
			fPopUp.AddItem(item);
		}
	}
}


void
StatusReplicant::_ChangeStatus(int newStatus)
{
	if (fBitmaps[newStatus]) {
		SetViewColor(Parent()->ViewColor());
		SetViewBitmap(fBitmaps[newStatus]);
	} else {
		ClearViewBitmap();
		SetViewColor(kStatusColors[newStatus][0],
					 kStatusColors[newStatus][1],
					 kStatusColors[newStatus][2]);
	}

	Invalidate();
}
