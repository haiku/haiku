/*
 * Copyright 2006-2009, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 *		Hugo Santos, hugosantos@gmail.com
 *		Dario Casalinuovo
 */


#include "NetworkStatusView.h"

#include <set>

#include <arpa/inet.h>
#include <net/if.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/sockio.h>
#include <unistd.h>

#include <Alert.h>
#include <Application.h>
#include <Catalog.h>
#include <Bitmap.h>
#include <Deskbar.h>
#include <Dragger.h>
#include <Drivers.h>
#include <IconUtils.h>
#include <Locale.h>
#include <MenuItem.h>
#include <MessageRunner.h>
#include <NetworkDevice.h>
#include <NetworkInterface.h>
#include <NetworkRoster.h>
#include <PopUpMenu.h>
#include <Resources.h>
#include <Roster.h>
#include <String.h>
#include <TextView.h>

#include "NetworkStatus.h"
#include "NetworkStatusIcons.h"
#include "RadioView.h"
#include "WirelessNetworkMenuItem.h"


#undef B_TRANSLATE_CONTEXT
#define B_TRANSLATE_CONTEXT "NetworkStatusView"


static const char *kStatusDescriptions[] = {
	B_TRANSLATE("Unknown"),
	B_TRANSLATE("No link"),
	B_TRANSLATE("No stateful configuration"),
	B_TRANSLATE("Configuring"),
	B_TRANSLATE("Ready")
};

extern "C" _EXPORT BView *instantiate_deskbar_item(void);


const uint32 kMsgShowConfiguration = 'shcf';
const uint32 kMsgOpenNetworkPreferences = 'onwp';
const uint32 kMsgJoinNetwork = 'join';

const uint32 kMinIconWidth = 16;
const uint32 kMinIconHeight = 16;


class SocketOpener {
public:
	SocketOpener()
	{
		fSocket = socket(AF_INET, SOCK_DGRAM, 0);
	}

	~SocketOpener()
	{
		close(fSocket);
	}

	status_t InitCheck()
	{
		return fSocket >= 0 ? B_OK : B_ERROR;
	}

	operator int() const
	{
		return fSocket;
	}

private:
	int	fSocket;
};


//	#pragma mark -


NetworkStatusView::NetworkStatusView(BRect frame, int32 resizingMode,
		bool inDeskbar)
	: BView(frame, kDeskbarItemName, resizingMode,
		B_WILL_DRAW | B_FRAME_EVENTS),
	fInDeskbar(inDeskbar),
	fStatus(kStatusUnknown)
{
	_Init();

	if (!inDeskbar) {
		// we were obviously added to a standard window - let's add a dragger
		frame.OffsetTo(B_ORIGIN);
		frame.top = frame.bottom - 7;
		frame.left = frame.right - 7;
		BDragger* dragger = new BDragger(frame, this,
			B_FOLLOW_RIGHT | B_FOLLOW_BOTTOM);
		AddChild(dragger);
	} else
		_Update();
}


NetworkStatusView::NetworkStatusView(BMessage* archive)
	: BView(archive),
	fInDeskbar(false)
{
	app_info info;
	if (be_app->GetAppInfo(&info) == B_OK
		&& !strcasecmp(info.signature, "application/x-vnd.Be-TSKB"))
		fInDeskbar = true;

	_Init();
}


NetworkStatusView::~NetworkStatusView()
{
}


void
NetworkStatusView::_Init()
{
	for (int i = 0; i < kStatusCount; i++) {
		fBitmaps[i] = NULL;
	}

	_UpdateBitmaps();
}


void
NetworkStatusView::_UpdateBitmaps()
{
	for (int i = 0; i < kStatusCount; i++) {
		delete fBitmaps[i];
		fBitmaps[i] = NULL;
	}

	image_info info;
	if (our_image(info) != B_OK)
		return;

	BFile file(info.name, B_READ_ONLY);
	if (file.InitCheck() < B_OK)
		return;

	BResources resources(&file);
#ifdef HAIKU_TARGET_PLATFORM_HAIKU
	if (resources.InitCheck() < B_OK)
		return;
#endif

	for (int i = 0; i < kStatusCount; i++) {
		const void* data = NULL;
		size_t size;
		data = resources.LoadResource(B_VECTOR_ICON_TYPE,
			kNetworkStatusNoDevice + i, &size);
		if (data != NULL) {
			BBitmap* icon = new BBitmap(Bounds(), B_RGBA32);
			if (icon->InitCheck() == B_OK
				&& BIconUtils::GetVectorIcon((const uint8 *)data,
					size, icon) == B_OK) {
				fBitmaps[i] = icon;
			} else
				delete icon;
		}
	}
}


void
NetworkStatusView::_Quit()
{
	if (fInDeskbar) {
		BDeskbar deskbar;
		deskbar.RemoveItem(kDeskbarItemName);
	} else
		be_app->PostMessage(B_QUIT_REQUESTED);
}


NetworkStatusView *
NetworkStatusView::Instantiate(BMessage* archive)
{
	if (!validate_instantiation(archive, "NetworkStatusView"))
		return NULL;

	return new NetworkStatusView(archive);
}


status_t
NetworkStatusView::Archive(BMessage* archive, bool deep) const
{
	status_t status = BView::Archive(archive, deep);
	if (status == B_OK)
		status = archive->AddString("add_on", kSignature);
	if (status == B_OK)
		status = archive->AddString("class", "NetworkStatusView");

	return status;
}


void
NetworkStatusView::AttachedToWindow()
{
	BView::AttachedToWindow();
	if (Parent())
		SetViewColor(Parent()->ViewColor());
	else
		SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));

	SetLowColor(ViewColor());

	start_watching_network(
		B_WATCH_NETWORK_INTERFACE_CHANGES | B_WATCH_NETWORK_LINK_CHANGES, this);

	_Update();
}


void
NetworkStatusView::DetachedFromWindow()
{
	stop_watching_network(this);
}


void
NetworkStatusView::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case B_NETWORK_MONITOR:
			_Update();
			break;

		case kMsgShowConfiguration:
			_ShowConfiguration(message);
			break;

		case kMsgOpenNetworkPreferences:
			_OpenNetworksPreferences();
			break;

		case kMsgJoinNetwork:
		{
			const char* deviceName;
			const char* name;
			if (message->FindString("device", &deviceName) == B_OK
				&& message->FindString("name", &name) == B_OK) {
				BNetworkDevice device(deviceName);
				status_t status = device.JoinNetwork(name);
				if (status != B_OK) {
					BString text
						= B_TRANSLATE("Could not join wireless network:\n");
					text << strerror(status);
					BAlert* alert = new BAlert(name, text.String(),
						B_TRANSLATE("OK"), NULL, NULL, B_WIDTH_AS_USUAL,
						B_STOP_ALERT);
					alert->Go(NULL);
				}
			}
			break;
		}

		case B_ABOUT_REQUESTED:
			_AboutRequested();
			break;

		case B_QUIT_REQUESTED:
			_Quit();
			break;

		default:
			BView::MessageReceived(message);
	}
}


void
NetworkStatusView::FrameResized(float width, float height)
{
	_UpdateBitmaps();
	Invalidate();
}


void
NetworkStatusView::Draw(BRect updateRect)
{
	if (fBitmaps[fStatus] == NULL)
		return;

	SetDrawingMode(B_OP_ALPHA);
	DrawBitmap(fBitmaps[fStatus]);
	SetDrawingMode(B_OP_COPY);
}


void
NetworkStatusView::_ShowConfiguration(BMessage* message)
{
	static const struct information_entry {
		const char*	label;
		int32		control;
	} kInformationEntries[] = {
		{ B_TRANSLATE("Address"), SIOCGIFADDR },
		{ B_TRANSLATE("Broadcast"), SIOCGIFBRDADDR },
		{ B_TRANSLATE("Netmask"), SIOCGIFNETMASK },
		{ NULL }
	};

	SocketOpener socket;
	if (socket.InitCheck() != B_OK)
		return;

	const char* name;
	if (message->FindString("interface", &name) != B_OK)
		return;

	ifreq request;
	if (!_PrepareRequest(request, name))
		return;

	BString text(B_TRANSLATE("%ifaceName information:\n"));
	text.ReplaceFirst("%ifaceName", name);

	size_t boldLength = text.Length();

	for (int i = 0; kInformationEntries[i].label; i++) {
		if (ioctl(socket, kInformationEntries[i].control, &request,
				sizeof(request)) < 0) {
			continue;
		}

		char address[32];
		sockaddr_in* inetAddress = NULL;
		switch (kInformationEntries[i].control) {
			case SIOCGIFNETMASK:
				inetAddress = (sockaddr_in*)&request.ifr_mask;
				break;
			default:
				inetAddress = (sockaddr_in*)&request.ifr_addr;
				break;
		}

		if (inet_ntop(AF_INET, &inetAddress->sin_addr, address,
				sizeof(address)) == NULL) {
			return;
		}

		text << "\n" << kInformationEntries[i].label << ": " << address;
	}

	BAlert* alert = new BAlert(name, text.String(), B_TRANSLATE("OK"));
	BTextView* view = alert->TextView();
	BFont font;

	view->SetStylable(true);
	view->GetFont(&font);
	font.SetFace(B_BOLD_FACE);
	view->SetFontAndColor(0, boldLength, &font);

	alert->Go(NULL);
}


void
NetworkStatusView::MouseDown(BPoint point)
{
	BPopUpMenu* menu = new BPopUpMenu(B_EMPTY_STRING, false, false);
	menu->SetAsyncAutoDestruct(true);
	menu->SetFont(be_plain_font);

	// Add interfaces

	for (int32 i = 0; i < fInterfaces.CountItems(); i++) {
		BString& name = *fInterfaces.ItemAt(i);

		BString label = name;
		label += ": ";
		label += kStatusDescriptions[
			_DetermineInterfaceStatus(name.String())];

		BMessage* info = new BMessage(kMsgShowConfiguration);
		info->AddString("interface", name.String());
		menu->AddItem(new BMenuItem(label.String(), info));
	}

	if (!fInterfaces.IsEmpty())
		menu->AddSeparatorItem();

	// Add wireless networks, if any

	for (int32 i = 0; i < fInterfaces.CountItems(); i++) {
		BNetworkDevice device(fInterfaces.ItemAt(i)->String());
		if (!device.IsWireless())
			continue;

		std::set<BNetworkAddress> associated;
		BNetworkAddress address;
		uint32 cookie = 0;
		while (device.GetNextAssociatedNetwork(cookie, address) == B_OK)
			associated.insert(address);

		wireless_network network;
		int32 count = 0;
		cookie = 0;
		while (device.GetNextNetwork(cookie, network) == B_OK) {
			BMessage* message = new BMessage(kMsgJoinNetwork);
			message->AddString("device", device.Name());
			message->AddString("name", network.name);

			BMenuItem* item = new WirelessNetworkMenuItem(network.name,
				network.signal_strength,
				(network.flags & B_NETWORK_IS_ENCRYPTED) != 0, message);
			menu->AddItem(item);
			if (associated.find(network.address) != associated.end())
				item->SetMarked(true);

			count++;
		}
		if (count == 0) {
			BMenuItem* item = new BMenuItem(
				B_TRANSLATE("<no wireless networks found>"), NULL);
			item->SetEnabled(false);
			menu->AddItem(item);
		}
		menu->AddSeparatorItem();

		// We only show the networks of the first wireless device we find.
		break;
	}

	menu->AddItem(new BMenuItem(B_TRANSLATE(
		"Open network preferences" B_UTF8_ELLIPSIS),
		new BMessage(kMsgOpenNetworkPreferences)));

	if (fInDeskbar) {
		menu->AddItem(new BMenuItem(B_TRANSLATE("Quit"),
			new BMessage(B_QUIT_REQUESTED)));
	}
	menu->SetTargetForItems(this);

	ConvertToScreen(&point);
	menu->Go(point, true, true, true);
}


void
NetworkStatusView::_AboutRequested()
{
	BString about = B_TRANSLATE(
		"NetworkStatus\n\twritten by %1 and Hugo Santos\n\t%2, Haiku, Inc.\n"
		);
	about.ReplaceFirst("%1", "Axel Dörfler");
		// Append a new developer here
	about.ReplaceFirst("%2", "Copyright 2007-2010");
		// Append a new year here
	BAlert* alert = new BAlert("about", about, B_TRANSLATE("OK"));
	BTextView *view = alert->TextView();
	BFont font;

	view->SetStylable(true);

	view->GetFont(&font);
	font.SetSize(18);
	font.SetFace(B_BOLD_FACE);
	view->SetFontAndColor(0, 13, &font);

	alert->Go();
}


bool
NetworkStatusView::_PrepareRequest(struct ifreq& request, const char* name)
{
	if (strlen(name) > IF_NAMESIZE)
		return false;

	strcpy(request.ifr_name, name);
	return true;
}


int32
NetworkStatusView::_DetermineInterfaceStatus(const char* name)
{
	BNetworkInterface interface(name);
	uint32 flags = interface.Flags();
	int32 status = kStatusNoLink;

	// TODO: no kStatusLinkNoConfig yet

	if (flags & IFF_CONFIGURING)
		status = kStatusConnecting;
	else if ((flags & (IFF_UP | IFF_LINK)) == (IFF_UP | IFF_LINK))
		status = kStatusReady;

	return status;
}


void
NetworkStatusView::_Update(bool force)
{
	int32 oldStatus = fStatus;
	fStatus = kStatusUnknown;
	fInterfaces.MakeEmpty();

	BNetworkRoster& roster = BNetworkRoster::Default();
	BNetworkInterface interface;
	uint32 cookie = 0;

	while (roster.GetNextInterface(&cookie, interface) == B_OK) {
		if ((interface.Flags() & IFF_LOOPBACK) == 0) {
			fInterfaces.AddItem(new BString(interface.Name()));
			int32 status = _DetermineInterfaceStatus(interface.Name());
			if (status > fStatus)
				fStatus = status;
		}
	}

	if (fStatus != oldStatus)
		Invalidate();
}


void
NetworkStatusView::_OpenNetworksPreferences()
{
	status_t status = be_roster->Launch("application/x-vnd.Haiku-Network");
	if (status != B_OK && status != B_ALREADY_RUNNING) {
		BString errorMessage(B_TRANSLATE("Launching the network preflet "
			"failed.\n\nError: "));
		errorMessage << strerror(status);
		BAlert* alert = new BAlert("launch error", errorMessage.String(),
			B_TRANSLATE("OK"));

		// asynchronous alert in order to not block replicant host application
		alert->Go(NULL);
	}
}


//	#pragma mark -


extern "C" _EXPORT BView *
instantiate_deskbar_item(void)
{
	return new NetworkStatusView(BRect(0, 0, 15, 15),
		B_FOLLOW_LEFT | B_FOLLOW_TOP, true);
}

