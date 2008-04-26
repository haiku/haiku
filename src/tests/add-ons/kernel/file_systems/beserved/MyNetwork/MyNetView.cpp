#include "Mime.h"
#include "TypeConstants.h"
#include "Application.h"
#include "InterfaceDefs.h"
#include "TranslationUtils.h"
#include "Alert.h"
#include "Button.h"
#include "ListView.h"
#include "StatusBar.h"
#include "Errors.h"
#include "OS.h"

// ColumnListView includes
#include "ColumnListView.h"
#include "CLVColumn.h"
#include "CLVEasyItem.h"
#include "NewStrings.h"

// POSIX includes
#include "errno.h"
#include "malloc.h"
#include "socket.h"
#include "signal.h"
#include "netdb.h"

#include "betalk.h"
#include "sysdepdefs.h"
#include "MyNetView.h"

#ifndef INVALID_SOCKET
#define INVALID_SOCKET		(int)(~0)
#endif

#define TEXT_WARN_NETUNREACH	"A network error has prevented the discovery of other computers on your network.  Consequently, the list of those computers cannot be displayed."

int32 getHosts(void *data);
int32 getShares(void *data);
int32 getHostInfo(void *data);
void recvAlarm(int signal);
void networkError(int error);


// ----- ThreadInfo --------------------------------------------------------------------

ThreadInfo::ThreadInfo()
{
	address = 0;
	listView = NULL;
}

ThreadInfo::~ThreadInfo()
{
}

// ----- MyNetHeaderView ---------------------------------------------------------------

MyNetHeaderView::MyNetHeaderView(BRect rect)
	: BView(rect, "MyNetHeaderView", B_FOLLOW_TOP | B_FOLLOW_LEFT_RIGHT, B_WILL_DRAW)
{
	rgb_color gray = ui_color(B_PANEL_BACKGROUND_COLOR);
	SetViewColor(gray);

	BRect bmpRect(0.0, 0.0, 31.0, 31.0);
	icon = new BBitmap(bmpRect, B_CMAP8);
	BMimeType mime("application/x-vnd.Teldar-MyNetwork");
	mime.GetIcon(icon, B_LARGE_ICON);

	BRect r;
	r.Set(15, 55, 70, 75);
	openBtn = new BButton(r, "OpenBtn", "Open", new BMessage(MSG_HOST_INVOKE));
	openBtn->SetEnabled(false);
	AddChild(openBtn);

	r.Set(77, 55, 215, 75);
	hostBtn = new BButton(r, "HostBtn", "About this Computer...", new BMessage(MSG_HOST_INFO));
	hostBtn->SetEnabled(false);
	AddChild(hostBtn);

	r.Set(222, 55, 288, 75);
	AddChild(new BButton(r, "RefreshBtn", "Refresh", new BMessage(MSG_HOST_REFRESH)));

	r.Set(295, 55, 390, 75);
	AddChild(new BButton(r, "InternetBtn", "Internet Host...", new BMessage(MSG_INET_HOSTS)));
}

MyNetHeaderView::~MyNetHeaderView()
{
}

void MyNetHeaderView::Draw(BRect rect)
{
	BRect r = Bounds();
	BRect iconRect(13.0, 5.0, 45.0, 37.0);
	rgb_color black = { 0, 0, 0, 255 };
	rgb_color gray = ui_color(B_PANEL_BACKGROUND_COLOR);

	SetViewColor(gray);
	SetLowColor(gray);
	FillRect(r, B_SOLID_LOW);

	SetHighColor(black);
	SetFont(be_bold_font);
	SetFontSize(11);
	MovePenTo(55, 15);
	DrawString("My Network Browser");

	SetFont(be_plain_font);
	SetFontSize(10);
	MovePenTo(55, 28);
	DrawString("This utility allows you to connect to remote computers and access");
	MovePenTo(55, 40);
	DrawString("resources, such as files and printers, that others have chosen to share.");

	SetDrawingMode(B_OP_ALPHA); 
	SetHighColor(0, 0, 0, 180);       
	SetBlendingMode(B_CONSTANT_ALPHA, B_ALPHA_COMPOSITE);
	DrawBitmap(icon, iconRect);
}

// ----- MyShareOptionView ---------------------------------------------------------------

MyShareOptionView::MyShareOptionView(BRect rect)
	: BView(rect, "MyNetHeaderView", B_FOLLOW_BOTTOM | B_FOLLOW_LEFT_RIGHT, B_WILL_DRAW)
{
	rgb_color gray = ui_color(B_PANEL_BACKGROUND_COLOR);
	SetViewColor(gray);

	BRect r;
	r.Set(15, 5, 70, 25);
	openBtn = new BButton(r, "OpenBtn", "Open", new BMessage(MSG_SHARE_OPEN));
	openBtn->SetEnabled(false);
	AddChild(openBtn);

	r.Set(77, 5, 160, 25);
	unmountBtn = new BButton(r, "UnmountBtn", "Disconnect", new BMessage(MSG_SHARE_UNMOUNT));
	unmountBtn->SetEnabled(false);
	AddChild(unmountBtn);
}

MyShareOptionView::~MyShareOptionView()
{
}

void MyShareOptionView::Draw(BRect rect)
{
	BRect r = Bounds();
	rgb_color gray = ui_color(B_PANEL_BACKGROUND_COLOR);

	SetViewColor(gray);
	SetLowColor(gray);
	FillRect(r, B_SOLID_LOW);
}

// ----- MyNetView ---------------------------------------------------------------

MyNetView::MyNetView(BRect rect)
	: BView(rect, "MyNetView", B_FOLLOW_TOP_BOTTOM | B_FOLLOW_LEFT_RIGHT, B_WILL_DRAW)
{
	list = new IconListView(Bounds(), "Other Computers");
	list->SetSelectionMessage(new BMessage(MSG_HOST_SELECT));
	list->SetInvocationMessage(new BMessage(MSG_HOST_INVOKE));

	// Here we're going to get the mini icon from a specific mime type
	BRect bmpRect(0.0, 0.0, 15.0, 15.0);
	fileIcon = new BBitmap(bmpRect, B_CMAP8);
	BMimeType mime("application/x-vnd.BeServed-fileserver");
	mime.GetIcon(fileIcon, B_MINI_ICON);

	thread_id hostThread = spawn_thread(getHosts, "Get Hosts", B_NORMAL_PRIORITY, this);
	resume_thread(hostThread);

	AddChild(new BScrollView("scroll", list, B_FOLLOW_ALL, 0, false, true));
}

MyNetView::~MyNetView()
{
	void *item;
	int32 i;

	for (i = 0; item = list->ItemAt(i); i++)
		delete item;

	delete list;
	delete fileIcon;
}

void MyNetView::AddHost(char *hostName, uint32 address)
{
	char buffer[256];

	strcpy(buffer, hostName);
	IconListItem *item = new IconListItem(fileIcon, buffer, address, 0, false); 

	list->LockLooper();
	list->AddItem(item);
	list->UnlockLooper();
}

void MyNetView::Refresh()
{
	list->LockLooper();
	list->RemoveItems(0, list->CountItems());
	list->UnlockLooper();

	thread_id hostThread = spawn_thread(getHosts, "Get Hosts", B_NORMAL_PRIORITY, this);
	resume_thread(hostThread);
}

// ----- ResourceItem ------------------------------------------------------------------

class ResourceItem : public CLVEasyItem
{
	public:
		ResourceItem::ResourceItem(const char *text0, int type)
		: CLVEasyItem(0, false, false, 20.0)
		{
			// Here we're going to get the mini icon from a specific mime type
			BRect bmpRect(0.0, 0.0, 15.0, 15.0);
			BBitmap *icon = new BBitmap(bmpRect, B_CMAP8);

			BMimeType mime(type == BT_SHARED_FOLDER  ? "application/x-vnd.BeServed-fileshare"
				: "application/x-vnd.Be.printer");
			mime.GetIcon(icon, B_MINI_ICON);

			SetColumnContent(0, icon, 2.0);
			SetColumnContent(1, text0);
			SetColumnContent(2, type == BT_SHARED_FOLDER ? "Shared Files" : "Shared Printer");
		}

		ResourceItem::~ResourceItem()
		{
		}
};

// ----- FileShareView -----------------------------------------------------

FileShareView::FileShareView(BRect rect, const char *name, uint32 ipAddr)
	: BView(rect, "FileShareView", B_FOLLOW_ALL, B_WILL_DRAW)
{
	strcpy(host, name);
	address = ipAddr;

	list = new IconListView(Bounds(), "File Shares");
	list->SetSelectionMessage(new BMessage(MSG_SHARE_SELECT));
	list->SetInvocationMessage(new BMessage(MSG_SHARE_INVOKE));

	// Here we're going to get the mini icon from a specific mime type
	BRect bmpRect(0.0, 0.0, 15.0, 15.0);
	fileIcon = new BBitmap(bmpRect, B_CMAP8);
	BMimeType mime("application/x-vnd.BeServed-fileshare");
	mime.GetIcon(fileIcon, B_MINI_ICON);
	printerIcon = new BBitmap(bmpRect, B_CMAP8);
	mime.SetTo("application/x-vnd.Be.printer");
	mime.GetIcon(printerIcon, B_MINI_ICON);

	thread_id shareThread = spawn_thread(getShares, "Get Shares", B_NORMAL_PRIORITY, this);
	resume_thread(shareThread);

	AddChild(new BScrollView("scroll", list, B_FOLLOW_ALL, 0, false, true));
}

FileShareView::~FileShareView()
{
	void *item;
	int32 i;

	for (i = 0; item = list->ItemAt(i); i++)
		delete item;

	delete list;
	delete printerIcon;
	delete fileIcon;
}

void FileShareView::AddShare(char *shareName)
{
	char buffer[256];

	strcpy(buffer, shareName);
	IconListItem *item = new IconListItem(fileIcon, buffer, 0, 0, false); 

	list->LockLooper();
	list->AddItem(item);
	list->UnlockLooper();
}

void FileShareView::AddPrinter(char *printerName)
{
	char buffer[256];

	strcpy(buffer, printerName);
	IconListItem *item = new IconListItem(printerIcon, buffer, 0, 0, false); 

	list->LockLooper();
	list->AddItem(item);
	list->UnlockLooper();
}

// ----- HostInfoView -----------------------------------------------------

HostInfoView::HostInfoView(BRect rect, const char *name, uint32 ipAddr)
	: BView(rect, "HostInfoView", B_FOLLOW_ALL, B_WILL_DRAW)
{
	address = ipAddr;

	hostInfo.cpus = 0;
	thread_id infoThread = spawn_thread(getHostInfo, "Get Host Info", B_NORMAL_PRIORITY, this);
	resume_thread(infoThread);

	rgb_color gray = ui_color(B_PANEL_BACKGROUND_COLOR);
	SetViewColor(gray);

	BRect bmpRect(0.0, 0.0, 31.0, 31.0);
	icon = new BBitmap(bmpRect, B_CMAP8);

	BMimeType mime("application/x-vnd.BeServed-fileserver");
	mime.GetIcon(icon, B_LARGE_ICON);

	BRect r(40, 55, 230, 120);
	BListView *aliasList = new BListView(r, "Aliases", B_SINGLE_SELECTION_LIST);
	AddChild(new BScrollView("ScrollAliases", aliasList, B_FOLLOW_LEFT | B_FOLLOW_TOP,
		0, false, true));

	r.top = 145;
	r.bottom = 210;
	BListView *addressList = new BListView(r, "Addresses", B_SINGLE_SELECTION_LIST);
	AddChild(new BScrollView("ScrollAddresses", addressList, B_FOLLOW_LEFT | B_FOLLOW_TOP,
		0, false, true));

	r.top = 275;
	r.bottom = 305;
	r.left = 38;
	r.right = 245;
	status = new BStatusBar(r, "Connections");
	status->SetBarHeight(13.0);
	AddChild(status);

	ent = gethostbyaddr((char *) &ipAddr, sizeof(ipAddr), AF_INET);
	if (ent)
	{
		char buf[50];
		int i;
		for (i = 0; ent->h_aliases[i] != 0; i++)
			aliasList->AddItem(new BStringItem(ent->h_aliases[i]));

		for (i = 0; ent->h_addr_list[i] != 0; i++)
		{
			sprintf(buf, "%d.%d.%d.%d", (uint8) ent->h_addr_list[i][0], (uint8) ent->h_addr_list[i][1],
				(uint8) ent->h_addr_list[i][2], (uint8) ent->h_addr_list[i][3]);
			addressList->AddItem(new BStringItem(buf));
		}
	}
}

HostInfoView::~HostInfoView()
{
}

void HostInfoView::Draw(BRect rect)
{
	BRect r = Bounds();
	BRect iconRect(13.0, 5.0, 45.0, 37.0);
	rgb_color black = { 0, 0, 0, 255 };
	rgb_color green = { 0, 255, 0, 255 };
	rgb_color red = { 255, 0, 0, 255 };
	rgb_color gray = ui_color(B_PANEL_BACKGROUND_COLOR);
	rgb_color shadow = { 190, 190, 190, 255 };
	char buf[50];

	SetViewColor(gray);
	SetLowColor(shadow);
	r.right = 35;
	FillRect(r, B_SOLID_LOW);
	r = Bounds();
	r.left = 35;
	SetLowColor(gray);
	FillRect(r, B_SOLID_LOW);

	if (ent)
	{
		SetHighColor(black);
		SetFont(be_bold_font);
		SetFontSize(14);
		MovePenTo(50, 25);
		DrawString(ent->h_name);
	}

	SetFont(be_plain_font);
	SetFontSize(10);
	MovePenTo(40, 50);
	DrawString("Aliases:");

	MovePenTo(40, 140);
	DrawString("Known Addresses:");

	SetFont(be_plain_font);
	SetFontSize(10);
	MovePenTo(40, 230);
	DrawString("System:");
	MovePenTo(100, 230);
	if (hostInfo.cpus > 0)
		DrawString(hostInfo.system);

	MovePenTo(40,243);
	DrawString("Network:");
	MovePenTo(100, 243);
	if (hostInfo.cpus > 0)
		DrawString(hostInfo.beServed);

	MovePenTo(40, 256);
	DrawString("Platform:");
	MovePenTo(100, 256);
	if (hostInfo.cpus > 0)
		DrawString(hostInfo.platform);

	MovePenTo(40, 269);
	DrawString("Processors:");
	MovePenTo(100, 269);
	if (hostInfo.cpus > 0)
	{
		sprintf(buf, "%ld", hostInfo.cpus);
		DrawString(buf);
	}

	if (hostInfo.cpus > 0)
	{
		double pct = hostInfo.connections * 100.0 / hostInfo.maxConnections;
		sprintf(buf, "%ld of %ld total", hostInfo.connections, hostInfo.maxConnections);
		status->SetMaxValue(hostInfo.maxConnections);
		status->Reset();
		status->Update(hostInfo.connections, "Connections", buf);
		status->SetBarColor(pct < 90.0 ? green : red);
	}

	SetDrawingMode(B_OP_ALPHA); 
	SetHighColor(0, 0, 0, 180);       
	SetBlendingMode(B_CONSTANT_ALPHA, B_ALPHA_COMPOSITE);
	DrawBitmap(icon, iconRect);
}

// ----- Thread Code -----------------------------------------------------

int32 getHosts(void *data)
{
	bt_request request;
	struct sockaddr_in ourAddr, toAddr, fromAddr;
	int sock, addrLen, bytes;
	char buffer[256];
#ifdef SO_BROADCAST
	int on = 1;
#endif

	MyNetView *view = (MyNetView *) data;

	memset(&toAddr, 0, sizeof(toAddr));
	toAddr.sin_family = AF_INET;
	toAddr.sin_port = htons(BT_QUERYHOST_PORT);
	toAddr.sin_addr.s_addr = htonl(INADDR_BROADCAST);

	sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (sock == INVALID_SOCKET)
		return 0;

	memset(&ourAddr, 0, sizeof(ourAddr));
	ourAddr.sin_family = AF_INET;
	ourAddr.sin_port = htons(BT_QUERYHOST_PORT);
	ourAddr.sin_addr.s_addr = htonl(INADDR_ANY);

	if (bind(sock, (struct sockaddr *) &ourAddr, sizeof(ourAddr)))
		if (errno != EADDRINUSE)
		{
			closesocket(sock);
			return 0;
		}

	// Normally, a setsockopt() call is necessary to turn broadcast mode on
	// explicitly, although some versions of Unix don't care.  BeOS doesn't
	// currently even define SO_BROADCAST, unless you have BONE installed.
#ifdef SO_BROADCAST
	setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &on, sizeof(on));
#endif
	signal(SIGALRM, recvAlarm);

	strcpy(request.signature, BT_RPC_SIGNATURE);
	request.command = BT_REQ_HOST_PROBE;
	bytes = sendto(sock, (char *) &request, sizeof(request), 0, (struct sockaddr *) &toAddr, sizeof(toAddr));
	if (bytes == -1)
	{
		networkError(errno);
		closesocket(sock);
		return 0;
	}

	alarm(5);

	while (1)
	{
		addrLen = sizeof(fromAddr);
		bytes = recvfrom(sock, buffer, sizeof(buffer) - 1, 0, (struct sockaddr *) &fromAddr, &addrLen);
		if (bytes < 0)
		{
			if (errno == EINTR)
				break;
		}
		else if (bytes > 0)
			if (strncmp(buffer, BT_RPC_SIGNATURE, strlen(BT_RPC_SIGNATURE)) != 0)
			{
				struct sockaddr_in *sin = (struct sockaddr_in *) &fromAddr;
				hostent *ent = gethostbyaddr((char *) &sin->sin_addr, sizeof(sin->sin_addr), AF_INET);
				if (ent)
					view->AddHost(ent->h_name, *((uint32 *) &sin->sin_addr));
				else
				{
					uint8 *p = (uint8 *) &sin->sin_addr;
					sprintf(buffer, "%d.%d.%d.%d", p[0], p[1], p[2], p[3]);
					view->AddHost(buffer, *((uint32 *) &sin->sin_addr));
				}
			}
	}

	alarm(0);
	signal(SIGALRM, SIG_DFL);
	closesocket(sock);
	return 1;
}

int32 getShares(void *data)
{
	bt_request request;
	bt_resource resource;
	struct sockaddr_in ourAddr, toAddr, fromAddr;
	char buffer[8192];
	int sock, addrLen, bytes, bufPos = 0;

	ThreadInfo *info = (ThreadInfo *) data;
	unsigned int serverIP = ntohl(info->GetHostAddress());

	memset(&toAddr, 0, sizeof(toAddr));
	toAddr.sin_family = AF_INET;
	toAddr.sin_port = htons(BT_QUERYHOST_PORT);
	toAddr.sin_addr.s_addr = htonl(serverIP);

	sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (sock == INVALID_SOCKET)
		return 0;

	memset(&ourAddr, 0, sizeof(ourAddr));
	ourAddr.sin_family = AF_INET;
	ourAddr.sin_port = htons(BT_QUERYHOST_PORT);
	ourAddr.sin_addr.s_addr = htonl(INADDR_ANY);

	if (bind(sock, (struct sockaddr *) &ourAddr, sizeof(ourAddr)))
		if (errno != EADDRINUSE)
		{
			closesocket(sock);
			return 0;
		}

	strcpy(request.signature, BT_RPC_SIGNATURE);
	request.command = BT_REQ_SHARE_PROBE;
	sendto(sock, (char *) &request, sizeof(request), 0, (struct sockaddr *) &toAddr, sizeof(toAddr));

	signal(SIGALRM, recvAlarm);
	alarm(4);

	addrLen = sizeof(fromAddr);
	bytes = recvfrom(sock, buffer, sizeof(buffer) - 1, 0, (struct sockaddr *) &fromAddr, &addrLen);
	if (bytes > 0)
		if (strncmp(buffer, BT_RPC_SIGNATURE, strlen(BT_RPC_SIGNATURE)) != 0)
		{
			info->GetColumnListView()->LockLooper();

			while (bufPos < bytes)
			{
				memcpy(&resource, buffer + bufPos, sizeof(bt_resource));
				resource.type = B_LENDIAN_TO_HOST_INT32(resource.type);
				resource.subType = B_LENDIAN_TO_HOST_INT32(resource.subType);
				if (resource.type == BT_SHARED_NULL)
					break;

				bufPos += sizeof(bt_resource);
				info->GetColumnListView()->AddItem(new ResourceItem(resource.name, resource.type));
			}

			info->GetColumnListView()->UnlockLooper();
		}

	alarm(0);
	signal(SIGALRM, SIG_DFL);
	closesocket(sock);
	return 1;
}

int32 getHostInfo(void *data)
{
	bt_request request;
	struct sockaddr_in ourAddr, toAddr, fromAddr;
	char buffer[1024];
	int sock, addrLen, bytes;
	bool retry = true;

	HostInfoView *view = (HostInfoView *) data;
	unsigned int serverIP = ntohl(view->address);

	memset(&toAddr, 0, sizeof(toAddr));
	toAddr.sin_family = AF_INET;
	toAddr.sin_port = htons(BT_QUERYHOST_PORT);
	toAddr.sin_addr.s_addr = htonl(serverIP);

	sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (sock == INVALID_SOCKET)
		return 0;

	memset(&ourAddr, 0, sizeof(ourAddr));
	ourAddr.sin_family = AF_INET;
	ourAddr.sin_port = htons(BT_QUERYHOST_PORT);
	ourAddr.sin_addr.s_addr = htonl(INADDR_ANY);

	if (bind(sock, (struct sockaddr *) &ourAddr, sizeof(ourAddr)))
		if (errno != EADDRINUSE)
		{
			closesocket(sock);
			return 0;
		}

sendReq:
	strcpy(request.signature, BT_RPC_SIGNATURE);
	request.command = BT_REQ_HOST_INFO;
	sendto(sock, (char *) &request, sizeof(request), 0, (struct sockaddr *) &toAddr, sizeof(toAddr));

	signal(SIGALRM, recvAlarm);
	alarm(3);

	addrLen = sizeof(fromAddr);
	bytes = recvfrom(sock, buffer, sizeof(buffer) - 1, 0, (struct sockaddr *) &fromAddr, &addrLen);
	if (bytes > 0)
	{
		memcpy(&view->hostInfo, buffer, sizeof(bt_hostinfo));

		view->hostInfo.cpus = B_LENDIAN_TO_HOST_INT32(view->hostInfo.cpus);
		view->hostInfo.connections = B_LENDIAN_TO_HOST_INT32(view->hostInfo.connections);
		view->hostInfo.maxConnections = B_LENDIAN_TO_HOST_INT32(view->hostInfo.maxConnections);
	}
	else if (retry)
	{
		retry = false;
		goto sendReq;
	}

	alarm(0);
	signal(SIGALRM, SIG_DFL);
	closesocket(sock);
	return 1;
}

void recvAlarm(int signal)
{
	return;
}

void networkError(int error)
{
	char errMsg[512];
	sprintf(errMsg, "%s\n\nDetails:  %s", TEXT_WARN_NETUNREACH, strerror(error));
	BAlert *alert = new BAlert("Error", errMsg, "OK", NULL, NULL, B_WIDTH_AS_USUAL, B_OFFSET_SPACING, B_WARNING_ALERT);
	alert->Go();
}
