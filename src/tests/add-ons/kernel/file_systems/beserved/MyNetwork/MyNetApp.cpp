// MyNetApp.cpp

#include "Application.h"
#include "Resources.h"
#include "TabView.h"
#include "Alert.h"
#include "Menu.h"
#include "MenuBar.h"
#include "MenuItem.h"
#include "Button.h"
#include "FilePanel.h"
#include "FindDirectory.h"
#include "TextControl.h"
#include "Path.h"
#include "Mime.h"
#include "Roster.h"

#include "assert.h"
#include "unistd.h"
#include "socket.h"
#include "netdb.h"
#include "time.h"

#include "ColumnListView.h"
#include "CLVColumn.h"
#include "CLVEasyItem.h"
#include "NewStrings.h"

#include "betalk.h"
#include "besure_auth.h"
#include "rpc.h"
#include "BlowFish.h"
#include "MyNetView.h"
#include "MyDriveView.h"
#include "IconListItem.h"
#include "btAddOn.h"
#include "LoginPanel.h"
#include "InstallPrinter.h"

#ifndef BONE_VERSION
#include "ksocket_internal.h"
#else
#include "arpa/inet.h"
#endif

#define BT_HOST_PATH		"/boot/home/Connections"
#define IsValid(s)			(strcmp((s), ".") && strcmp((s), ".."))

#define MYNET_ICON_HOST_LARGE		101
#define MYNET_ICON_SHARE_LARGE		102
#define MYNET_ICON_HOST_SMALL		103
#define MYNET_ICON_SHARE_SMALL		104
#define MYNET_ICON_INETHOST_LARGE	105
#define MYNET_ICON_INETHOST_SMALL	106


// ----- SmartColumnListView ----------------------------------------------------------------

class SmartColumnListView : public ColumnListView
{
	public:
		SmartColumnListView(BRect Frame, CLVContainerView** ContainerView,
			const char* Name = NULL, uint32 ResizingMode = B_FOLLOW_LEFT | B_FOLLOW_TOP,
			uint32 flags = B_WILL_DRAW | B_FRAME_EVENTS | B_NAVIGABLE,
			list_view_type Type = B_SINGLE_SELECTION_LIST,
			bool hierarchical = false, bool horizontal = true, bool vertical = true,
			bool scroll_view_corner = true, border_style border = B_NO_BORDER,
			const BFont* LabelFont = be_plain_font) :
			  ColumnListView(Frame, ContainerView, Name, ResizingMode, flags, Type, hierarchical,
				horizontal, vertical, scroll_view_corner, border, LabelFont)
		{
			container = *ContainerView;
		}

		virtual ~SmartColumnListView()
		{
		}

		void MouseDown(BPoint point)
		{
			ColumnListView::MouseDown(point);
			if (CurrentSelection() < 0)
				container->Window()->PostMessage(MSG_LIST_DESELECT);
		}

	private:
		CLVContainerView *container;
};

// ----- MountItem ----------------------------------------------------------------------

class MountItem : public CLVEasyItem
{
	public:
		MountItem(const char *text0, const char *text1, const char *text2) :
		  CLVEasyItem(0, false, false, 20.0)
		{
			// Here we're going to get the mini icon from a specific mime type
			BRect bmpRect(0.0, 0.0, 15.0, 15.0);
			BBitmap *icon = new BBitmap(bmpRect, B_CMAP8);

			BMimeType mime("application/x-vnd.BeServed-fileshare");
			mime.GetIcon(icon, B_MINI_ICON);

			SetColumnContent(0, icon, 2.0);
			SetColumnContent(1, text0);
			SetColumnContent(2, text1);
			SetColumnContent(3, text2);
		}

		~MountItem()
		{
		}
};

// ----- HostInfoPanel -----------------------------------------------------------------

class HostInfoPanel : public BWindow
{
	public:
		HostInfoPanel(BRect frame, const char *name, uint32 ipAddr) :
		  BWindow(frame, name, B_TITLED_WINDOW_LOOK, B_MODAL_APP_WINDOW_FEEL, B_NOT_ZOOMABLE | B_NOT_RESIZABLE)
		{
			BRect r = Bounds();

			strcpy(host, name);
			address = ipAddr;
			AddChild(new HostInfoView(r, name, address));

			Show();
		}

	private:
		char host[B_FILE_NAME_LENGTH + 1];
		uint32 address;
};


class FileShareWindow : public BWindow
{
	public:
		FileShareWindow(BWindow *win, BRect frame, const char *name, uint32 ipAddr) :
		  BWindow(frame, name, B_TITLED_WINDOW, B_NOT_ZOOMABLE)
		{
			parent = win;
			strcpy(host, name);
			address = ipAddr;

			CLVContainerView *containerView;
			BRect r = Bounds();

			MyColumnListView = new SmartColumnListView(r, &containerView, NULL, B_FOLLOW_TOP_BOTTOM | B_FOLLOW_LEFT_RIGHT,
				B_WILL_DRAW | B_FRAME_EVENTS | B_NAVIGABLE, B_SINGLE_SELECTION_LIST,
				false, false, false, false, B_NO_BORDER);

			MyColumnListView->AddColumn(new CLVColumn(NULL, 20.0, CLV_LOCK_AT_BEGINNING | CLV_NOT_MOVABLE |
				CLV_NOT_RESIZABLE | CLV_PUSH_PASS | CLV_MERGE_WITH_RIGHT));
			MyColumnListView->AddColumn(new CLVColumn("Resource", 140.0, CLV_SORT_KEYABLE, 50.0));
			MyColumnListView->AddColumn(new CLVColumn("Type", 90.0, CLV_SORT_KEYABLE));

			MyColumnListView->SetSelectionMessage(new BMessage(MSG_SHARE_SELECT));
			MyColumnListView->SetInvocationMessage(new BMessage(MSG_SHARE_INVOKE));

			AddCLVItems(MyColumnListView);
			AddChild(containerView);

			SetSizeLimits(260, 2000, 110, 2000);
			Show();
		}

		// AddCLVItems()
		//
		void AddCLVItems(ColumnListView* MyColumnListView)
		{
			extern int32 getShares(void *);
			ThreadInfo *info = new ThreadInfo();
			info->SetColumnListView(MyColumnListView);
			info->SetHostAddress(address);
			thread_id shareThread = spawn_thread(getShares, "Get Shares", B_NORMAL_PRIORITY, info);
			resume_thread(shareThread);
		}

		void MessageReceived(BMessage *msg)
		{
			BListView *list;
			int32 curItem;

			switch (msg->what)
			{
				case MSG_SHARE_SELECT:
					break;

				case MSG_SHARE_INVOKE:
					msg->FindPointer("source", (void **) &list);
					curItem = list->CurrentSelection();
					if (curItem != -1)
					{
						CLVEasyItem *item = (CLVEasyItem *) MyColumnListView->ItemAt(curItem);
						if (item)
						{
							const char *share = item->GetColumnContentText(1);
							const char *type = item->GetColumnContentText(2);

							if (strcmp(type, "Shared Files") == 0)
								mountToLocalFolder(host, (char *) share);
							else
								createSharedPrinter(host, (char *) share, 0);
						}
					}
					break;

				default:
					BWindow::MessageReceived(msg);
					break;
			}
		}

	private:
		// mountToLocalFolder()
		//
		void mountToLocalFolder(char *host, char *share)
		{
			char *folder = makeLocalFolder(host, share);
			if (folder)
			{
				if (dev_for_path("/boot/home") == dev_for_path(folder))
					if (!mountFileShare(address, share, folder))
						return;

				updateMountList(host, share, folder);
				showFolder(folder);
				free(folder);
			}
			else
			{
				BAlert *alert = new BAlert("title", "An attempt to create a local mount point to access this shared volume has failed.", "OK");
				alert->Go();
			}
		}

		// makeLocalFolder()
		//
		char *makeLocalFolder(char *host, char *share)
		{
			char *path = (char *) malloc(B_PATH_NAME_LENGTH);
			if (!path)
				return NULL;

			strcpy(path, "/boot/home/Connections");
			if (access(path, 0) != 0)
				if (mkdir(path, S_IRWXU | S_IRWXG | S_IRWXO) != 0)
					return NULL;

			strcat(path, "/");
			strcat(path, host);
			if (access(path, 0) != 0)
				if (mkdir(path, S_IRWXU | S_IRWXG | S_IRWXO) != 0)
					return NULL;

			strcat(path, "/");
			strcat(path, share);
			if (access(path, 0) != 0)
				if (mkdir(path, S_IRWXU | S_IRWXG | S_IRWXO) != 0)
					return NULL;

			return path;
		}

		// getMountFolder()
		//
		void getMountFolder()
		{
			char path[B_PATH_NAME_LENGTH];
			entry_ref entryRef;

			find_directory(B_USER_DIRECTORY, 0, false, path, sizeof(path));
			BEntry entry(path, false);
			entry.GetRef(&entryRef);
			BFilePanel *filePanel = new BFilePanel(B_OPEN_PANEL, &be_app_messenger, &entryRef, B_DIRECTORY_NODE, false);
			filePanel->Show();
			filePanel->Window()->SetTitle("Mount Location");
			filePanel->SetButtonLabel(B_DEFAULT_BUTTON, "Mount");
		}

		bool mountFileShare(uint32 address, char *share, const char *path)
		{
			blf_ctx ctx;
			mount_bt_params params;
			char hostname[256], msg[512], *folder;
			int length;
			port_id port;

			folder = strrchr(path, '/');
			if (folder)
				folder++;
			else
				folder = (char *) path;

			unsigned int serverIP = ntohl(address);

#ifndef BONE_VERSION
			if (!be_roster->IsRunning(KSOCKETD_SIGNATURE))
				if (be_roster->Launch(KSOCKETD_SIGNATURE) < B_NO_ERROR)
				{
					printf("Network communications could not be started.  The shared volume cannot be mounted.\n");
					BAlert *alert = new BAlert("", msg, "OK");
					alert->Go();
					return false;
				}

			for (int32 i = 0; i < 10; i++)
			{
				port = find_port(KSOCKET_DAEMON_NAME);
		
				if (port < B_NO_ERROR)
					snooze(1000000LL);
				else
					break;
			}

			if (port < B_NO_ERROR)
			{
				sprintf(msg, "Network communications are not responding.  The shared volume cannot be mounted.\n");
				BAlert *alert = new BAlert("", msg, "OK");
				alert->Go();
				return false;
			}
#endif

			// Handle user authentication
//			strcpy(user, crypt(user, "u0"));
//			strcpy(password, crypt(password, "p1"));
			if (getAuthentication(serverIP, share))
			{
				BRect frame = Frame();
				frame.top += 75;
				frame.left += 75;
				frame.bottom = frame.top + 140;
				frame.right = frame.left + 250;
				LoginPanel *login = new LoginPanel(frame);
				status_t loginExit;
				wait_for_thread(login->Thread(), &loginExit);
				if (login->IsCancelled())
					return false;

				// Copy the user name.
				strcpy(params.user, login->user);

				// Copy the user name and password supplied in the authentication dialog.
				sprintf(params.password, "%-*s%-*s", B_FILE_NAME_LENGTH, share, MAX_USERNAME_LENGTH, login->user);		//crypt(password, "p1"));
				length = strlen(params.password);
				assert(length == BT_AUTH_TOKEN_LENGTH);

				params.password[length] = 0;
				blf_key(&ctx, (unsigned char *) login->md5passwd, strlen(login->md5passwd));
				blf_enc(&ctx, (unsigned long *) params.password, length / 4);
			}
			else
				params.user[0] = params.password[0] = 0;

			params.serverIP = serverIP;
			params.server = host;
			params._export = share;
			params.folder = folder;
			params.uid = 0;
			params.gid = 0;

			gethostname(hostname, 256);
			params.hostname = hostname;

			int result = mount("beserved_client", path, NULL, 0, &params, sizeof(params));

			if (result < B_NO_ERROR)
			{
				sprintf(msg, "The shared volume could not be mounted (%s).\n", strerror(errno));
				BAlert *alert = new BAlert("", msg, "OK");
				alert->Go();
				return false;
			}

			return true;
		}

		bool getAuthentication(unsigned int serverIP, char *shareName)
		{
			bt_inPacket *inPacket;
			bt_outPacket *outPacket;
			int security;

			security = EHOSTUNREACH;

			outPacket = btRPCPutHeader(BT_CMD_PREMOUNT, 1, strlen(shareName));
			btRPCPutArg(outPacket, B_STRING_TYPE, shareName, strlen(shareName));
			inPacket = btRPCSimpleCall(serverIP, BT_TCPIP_PORT, outPacket);
			if (inPacket)
			{
				security = btRPCGetInt32(inPacket);
				free(inPacket->buffer);
				free(inPacket);
			}

			free(outPacket->buffer);
			free(outPacket);

			return (security == BT_AUTH_BESURE);
		}

		void updateMountList(char *host, char *share, char *path)
		{
			BMessage *relay;
			relay = new BMessage(MSG_NEW_MOUNT);
			relay->AddString("host", host);
			relay->AddString("share", share);
			relay->AddString("path", path);

			parent->PostMessage(relay);
		}

		void showFolder(char *path)
		{
			char command[512];
			sprintf(command, "/boot/beos/system/Tracker \"%s\"", path);
			system(command);
		}

		bool isSharedPrinterInstalled(char *host, char *printer)
		{
			char path[B_PATH_NAME_LENGTH];

			// If a printer by this name already exists, remove the existing configuration
			// in favor of changes being applied.  If not, just create the printer and move
			// on.
			find_directory(B_USER_SETTINGS_DIRECTORY, 0, false, path, sizeof(path));
			strcat(path, "/printers/");
			strcat(path, printer);
			return (access(path, 0) == 0);
		}

		void createSharedPrinter(char *host, char *printer, int type)
		{
			blf_ctx ctx;
			hostent *ent;
			char msg[512], user[MAX_USERNAME_LENGTH + 1], password[BT_AUTH_TOKEN_LENGTH * 2 + 1];
			int length;

			// Abort if the printer type is not valid.
			if (type < 0 || type > 0)
				return;

			BRect frame = Frame();
			frame.top += 75;
			frame.left += 75;
			frame.bottom = frame.top + 135;
			frame.right = frame.left + 350;
			InstallPrinterPanel *prtPanel = new InstallPrinterPanel(frame, host, printer);
			status_t loginExit;
			wait_for_thread(prtPanel->Thread(), &loginExit);
			if (prtPanel->IsCancelled())
				return;

			// Get the IP address of the server.
			ent = gethostbyname(host);
			if (ent == NULL)
			{
				sprintf(msg, "The server %s, which hosts printer %s, cannot be found on the network.", host, printer);
				BAlert *alert = new BAlert("", msg, "OK");
				alert->Go();
				return;
			}

			unsigned int serverIP = ntohl(*((unsigned int *) ent->h_addr));

			if (getAuthentication(serverIP, printer))
			{
				BRect frame = Frame();
				frame.top += 75;
				frame.left += 75;
				frame.bottom = frame.top + 140;
				frame.right = frame.left + 250;
				LoginPanel *login = new LoginPanel(frame);
				status_t loginExit;
				wait_for_thread(login->Thread(), &loginExit);
				if (login->IsCancelled())
					return;

				// Copy the user name.
				strcpy(user, login->user);

				// Copy the user name and password supplied in the authentication dialog.
				sprintf(password, "%-*s%-*s", B_FILE_NAME_LENGTH, printer, MAX_USERNAME_LENGTH, login->user);
				length = strlen(password);
				assert(length == BT_AUTH_TOKEN_LENGTH);

				password[length] = 0;
				blf_key(&ctx, (unsigned char *) login->md5passwd, strlen(login->md5passwd));
				blf_enc(&ctx, (unsigned long *) password, length / 4);
			}
			else
				user[0] = password[0] = 0;

			prtPanel->CreatePrinter(printer, host, type, user, password);
			if (prtPanel->isDefault())
				prtPanel->SetDefaultPrinter(printer);
			if (prtPanel->printTestPage())
				prtPanel->TestPrinter(printer);
		}

		BWindow *parent;
		SmartColumnListView *MyColumnListView;
		char host[B_FILE_NAME_LENGTH];
		uint32 address;
};

// ----- InetHostView -----------------------------------------------------

class InetHostView : public BView
{
	public:
		InetHostView(BRect rect) :
		  BView(rect, "InetHostView", B_FOLLOW_ALL, B_WILL_DRAW)
		{
			rgb_color gray = ui_color(B_PANEL_BACKGROUND_COLOR);
			SetViewColor(gray);

			BRect bmpRect(0.0, 0.0, 31.0, 31.0);
			icon = new BBitmap(bmpRect, B_CMAP8);
			BMimeType mime("application/x-vnd.BeServed-inetserver");
			mime.GetIcon(icon, B_LARGE_ICON);

			BRect r(10, 52, 310, 72);
			editName = new BTextControl(r, "ShareName", "Name or Address:", "", NULL);
			editName->SetDivider(90);
			AddChild(editName);

			r.Set(155, 97, 225, 117);
			BButton *okBtn = new BButton(r, "OkayBtn", "OK", new BMessage(MSG_HOST_OK), B_FOLLOW_RIGHT | B_FOLLOW_BOTTOM);
			okBtn->MakeDefault(true);
			AddChild(okBtn);

			r.Set(235, 97, 310, 117);
			AddChild(new BButton(r, "CancelBtn", "Cancel", new BMessage(MSG_HOST_CANCEL), B_FOLLOW_RIGHT | B_FOLLOW_BOTTOM));

			editName->MakeFocus();
		}

		~InetHostView()
		{
		}

		void Draw(BRect rect)
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
			DrawString("Connect to Internet Host");

			SetFont(be_plain_font);
			SetFontSize(10);
			MovePenTo(55, 28);
			DrawString("Specify the name or address of a computer located");
			MovePenTo(55, 40);
			DrawString("outside your network, such as on the Internet.");

			SetDrawingMode(B_OP_ALPHA); 
			SetHighColor(0, 0, 0, 180);       
			SetBlendingMode(B_CONSTANT_ALPHA, B_ALPHA_COMPOSITE);
			DrawBitmap(icon, iconRect);
		}

		BTextControl *editName;

	private:
		BBitmap *icon;
};


// ----- InetHostPanel ----------------------------------------------------------------------

class InetHostPanel : public BWindow
{
	public:
		InetHostPanel(BRect frame, BWindow *win) :
		  BWindow(frame, "Internet Host", B_TITLED_WINDOW_LOOK, B_NORMAL_WINDOW_FEEL, B_NOT_ZOOMABLE | B_NOT_RESIZABLE)
		{
			parent = win;

			BRect r = Bounds();
			hostView = new InetHostView(r);
			AddChild(hostView);

			Show();
		}

		// MessageReceived()
		//
		void MessageReceived(BMessage *msg)
		{
			BMessage *relay;

			switch (msg->what)
			{
				case MSG_HOST_OK:
					relay = new BMessage(MSG_HOST_OK);
					relay->AddString("host", hostView->editName->Text());
					parent->PostMessage(relay);
					
				case MSG_HOST_CANCEL:
					BWindow::Quit();
					break;

				default:
					BWindow::MessageReceived(msg);
					break;
			}
		}

	private:
		BWindow *parent;
		InetHostView *hostView;
};

// ----- MyNetWindow --------------------------------------------------------------------------

class MyNetWindow : public BWindow
{
	public:
		MyNetWindow(BRect frame) :
		  BWindow(frame, "My Network", B_TITLED_WINDOW, B_NOT_ZOOMABLE)
		{
			BRect r;

			r = Bounds();
			r.bottom = 85;
			headerView = new MyNetHeaderView(r);
			AddChild(headerView);

			CLVContainerView *containerView;
			r = Bounds();
			r.top = r.bottom - 140;
			r.bottom -= 40;

			MyColumnListView = new SmartColumnListView(r, &containerView, NULL, B_FOLLOW_BOTTOM | B_FOLLOW_LEFT_RIGHT,
				B_WILL_DRAW | B_FRAME_EVENTS | B_NAVIGABLE, B_SINGLE_SELECTION_LIST,
				false, false, false, false, B_NO_BORDER);

			MyColumnListView->AddColumn(new CLVColumn(NULL, 20.0, CLV_LOCK_AT_BEGINNING | CLV_NOT_MOVABLE |
				CLV_NOT_RESIZABLE | CLV_PUSH_PASS | CLV_MERGE_WITH_RIGHT));
			MyColumnListView->AddColumn(new CLVColumn("Computer", 120.0, CLV_SORT_KEYABLE, 50.0));
			MyColumnListView->AddColumn(new CLVColumn("Share Name", 85.0, CLV_SORT_KEYABLE));
			MyColumnListView->AddColumn(new CLVColumn("Mounted At", 250.0, CLV_SORT_KEYABLE));

			MyColumnListView->SetSelectionMessage(new BMessage(MSG_SHARE_SELECT));
			MyColumnListView->SetInvocationMessage(new BMessage(MSG_SHARE_OPEN));

			AddCLVItems(MyColumnListView);
			AddChild(containerView);

			r = Bounds();
			r.top = 85;
			r.bottom -= 140;
			netView = new MyNetView(r);
			AddChild(netView);

			r = Bounds();
			r.top = r.bottom - 40;
			shareView = new MyShareOptionView(r);
			AddChild(shareView);

			SetSizeLimits(400, 2000, 270, 2000);

			Show();
		}

		// AddCLVItems()
		//
		void AddCLVItems(ColumnListView* MyColumnListView)
		{
			DIR *hosts, *shares;
			struct dirent *hostInfo, *shareInfo;
			struct stat st;
			struct tm *mountTime;
			char path[B_PATH_NAME_LENGTH];

			hosts = opendir(BT_HOST_PATH);
			if (hosts)
			{
				while ((hostInfo = readdir(hosts)) != NULL)
					if (IsValid(hostInfo->d_name))
					{
						sprintf(path, "%s/%s", BT_HOST_PATH, hostInfo->d_name);
						shares = opendir(path);
						if (shares)
						{
							while ((shareInfo = readdir(shares)) != NULL)
								if (IsValid(shareInfo->d_name))
								{
									sprintf(path, "%s/%s/%s", BT_HOST_PATH, hostInfo->d_name, shareInfo->d_name);
									if (dev_for_path("/boot/home") != dev_for_path(path))
									{
//										stat(path, &st);
//										mountTime = localtime(&st.st_ctime);
//										strftime(path, sizeof(path), "%a, %b %d %Y at %I:%M %p", mountTime);
										MyColumnListView->AddItem(new MountItem(hostInfo->d_name, shareInfo->d_name, path));
									}
									else
										rmdir(path);
								}

							closedir(shares);
						}
					}

				closedir(hosts);
			}
		}

		void MessageReceived(BMessage *msg)
		{
			BRect frame;
			int32 curItem;
			status_t exitStatus;

			switch (msg->what)
			{
				case MSG_HOST_INFO:
					curItem = netView->list->CurrentSelection();
					if (curItem != -1)
					{
						IconListItem *item = (IconListItem *) netView->list->ItemAt(curItem);
						if (item)
						{
							frame = Frame();
							frame.left += 100;
							frame.top += 100;
							frame.right = frame.left + 260;
							frame.bottom = frame.top + 310;
							HostInfoPanel *panel = new HostInfoPanel(frame, item->GetItemText(), item->GetItemData());
						}
					}
					break;

				case MSG_HOST_SELECT:
					headerView->openBtn->SetEnabled(true);
					headerView->hostBtn->SetEnabled(true);
					break;

				case MSG_LIST_DESELECT:
					if (MyColumnListView->CountItems() > 0 && MyColumnListView->CurrentSelection() == -1)
					{
						shareView->openBtn->SetEnabled(false);
						shareView->unmountBtn->SetEnabled(false);
					}
					else
					{
						headerView->openBtn->SetEnabled(false);
						headerView->hostBtn->SetEnabled(false);
					}
					break;

				case MSG_SHARE_SELECT:
					shareView->openBtn->SetEnabled(true);
					shareView->unmountBtn->SetEnabled(true);
					break;

				case MSG_HOST_INVOKE:
					curItem = netView->list->CurrentSelection();
					if (curItem != -1)
					{
						IconListItem *item = (IconListItem *) netView->list->ItemAt(curItem);
						if (item)
						{
							frame = Frame();
							frame.left += 100;
							frame.top += 100;
							frame.right = frame.left + 260;
							frame.bottom = frame.top + 300;
							FileShareWindow *win = new FileShareWindow(this, frame, item->GetItemText(), item->GetItemData());
						}
					}
					break;

				case MSG_HOST_REFRESH:
					netView->Refresh();
					break;

				case MSG_INET_HOSTS:
					{
						frame = Frame();
						frame.left += 150;
						frame.top += 50;
						frame.right = frame.left + 320;
						frame.bottom = frame.top + 128;
						InetHostPanel *win = new InetHostPanel(frame, this);
						wait_for_thread(win->Thread(), &exitStatus);
					}
					break;

				case MSG_HOST_OK:
					{
						const char *name;
						msg->FindString("host", &name);

						hostent *ent = gethostbyname(name);
						if (ent)
						{
							char buffer[256];
							frame = Frame();
							frame.left += 100;
							frame.top += 100;
							frame.right = frame.left + 260;
							frame.bottom = frame.top + 300;
							sprintf(buffer, "%d.%d.%d.%d", (uint8) ent->h_addr_list[0][0],
								(uint8) ent->h_addr_list[0][1], (uint8) ent->h_addr_list[0][2],
								(uint8) ent->h_addr_list[0][3]);
							FileShareWindow *win = new FileShareWindow(this, frame, name, (uint32) inet_addr(buffer));
						}
						else
						{
							BAlert *alert = new BAlert("Error", "The specified name or address cannot be contacted.", "OK", NULL, NULL, B_WIDTH_AS_USUAL, B_WARNING_ALERT);
							alert->Go();
						}
					}
					break;

				case MSG_NEW_MOUNT:
					{
						const char *host, *share, *path;
						msg->FindString("host", &host);
						msg->FindString("share", &share);
						msg->FindString("path", &path);
						MyColumnListView->LockLooper();
						MyColumnListView->AddItem(new MountItem(host, share, path));
						MyColumnListView->UnlockLooper();
					}
					break;

				case MSG_SHARE_OPEN:
					curItem = MyColumnListView->CurrentSelection();
					if (curItem != -1)
					{
						char command[512];
						MountItem *item = (MountItem *) MyColumnListView->ItemAt(curItem);
						const char *path = item->GetColumnContentText(3);
						sprintf(command, "/boot/beos/system/Tracker \"%s\"", path);
						system(command);
					}
					break;

				case MSG_SHARE_UNMOUNT:
					curItem = MyColumnListView->CurrentSelection();
					if (curItem != -1)
					{
						MountItem *item = (MountItem *) MyColumnListView->ItemAt(curItem);
						const char *path = item->GetColumnContentText(3);
						int error = unmount(path);
						if (error == 0)
						{
							MyColumnListView->LockLooper();
							MyColumnListView->RemoveItems(curItem, 1);
							MyColumnListView->UnlockLooper();

							shareView->openBtn->SetEnabled(false);
							shareView->unmountBtn->SetEnabled(false);
						}
					}
					break;

				default:
					BWindow::MessageReceived(msg);
					break;
			}
		}

		bool QuitRequested()
		{
			be_app->PostMessage(B_QUIT_REQUESTED);
			return true;
		}

	private:
		BMenuBar *menuBar;
		MyNetHeaderView *headerView;
		MyNetView *netView;
		MyShareOptionView *shareView;
		ColumnListView *MyColumnListView;
};


class MyNetApp : public BApplication
{
	public:
		MyNetApp() : BApplication("application/x-vnd.Teldar-MyNetwork")
		{
			checkMimeTypes();
			BRect frame(100, 150, 500, 600);
			MyNetWindow *win = new MyNetWindow(frame);
		}

		void RefsReceived(BMessage *msg)
		{
			entry_ref entryRef;
			BPath path;
			BEntry entry;

			switch (msg->what)
			{
				case B_REFS_RECEIVED:
					msg->FindRef("refs", &entryRef);
					entry.SetTo(&entryRef, true);
					entry.GetPath(&path);
//					mountFileShare(lastHost, lastShare, path.Path());
					break;

				default:
					BApplication::RefsReceived(msg);
					break;
			}
		}

	private:
		void buildMimeDB()
		{
			BMimeType mime;
			BMessage msg;
			char *data;
			ssize_t bytes;
			FILE *fp = fopen("/boot/home/mime.txt", "w");
			mime.GetInstalledTypes(&msg);
			for (int i = 0; msg.FindString("types", i, (const char **) &data) == B_OK; i++)
			{
				BMimeType t(data);
				BMessage m;
				char *ext;
				if (t.GetFileExtensions(&m) == B_OK)
				{
					for (int j = 0; m.FindString("extensions", j, (const char **) &ext) == B_OK; j++)
						fprintf(fp, "%-6s%s\n", ext, data);
				}
			}
			fclose(fp);
		}
	
		void checkMimeTypes()
		{
			BMimeType mime;
			mime.SetTo("application/x-vnd.BeServed-fileserver");
			mime.Delete();
			if (!mime.IsInstalled())
			{
				mime.Install();
				mime.SetShortDescription("Network File Server");
				mime.SetLongDescription("A network server running BeServed");
				setMimeIcon(&mime, MYNET_ICON_HOST_LARGE, B_LARGE_ICON);
				setMimeIcon(&mime, MYNET_ICON_HOST_SMALL, B_MINI_ICON);
			}

			mime.SetTo("application/x-vnd.BeServed-inetserver");
			mime.Delete();
			if (!mime.IsInstalled())
			{
				mime.Install();
				mime.SetShortDescription("Public File Server");
				mime.SetLongDescription("A remote network server running BeServed");
				setMimeIcon(&mime, MYNET_ICON_INETHOST_LARGE, B_LARGE_ICON);
				setMimeIcon(&mime, MYNET_ICON_INETHOST_SMALL, B_MINI_ICON);
			}

			mime.SetTo("application/x-vnd.BeServed-fileshare");
			mime.Delete();
			if (!mime.IsInstalled())
			{
				mime.Install();
				mime.SetShortDescription("Shared Volume");
				mime.SetLongDescription("A BeServed network shared volume");
				setMimeIcon(&mime, MYNET_ICON_SHARE_LARGE, B_LARGE_ICON);
				setMimeIcon(&mime, MYNET_ICON_SHARE_SMALL, B_MINI_ICON);
			}
		}

		bool setMimeIcon(BMimeType *mime, int32 resourceID, icon_size which)
		{
			BMessage archive;
			size_t size;
			char *bits = (char *) be_app->AppResources()->LoadResource(type_code('BBMP'), resourceID, &size);
			if (bits)
				if (archive.Unflatten(bits) == B_OK)
				{
					BBitmap *icon = new BBitmap(&archive);
					mime->SetIcon(icon, which);
					return true;
				}

			return false;
		}
};


int main()
{
	MyNetApp *app = new MyNetApp();
	app->Run();

	return 0;
}

/*
			r = Bounds();
			r.bottom = 18.0;
			menuBar = new BMenuBar(r, "menu_bar");
			BMenu *compMenu = new BMenu("Computer");
			compMenu->AddItem(new BMenuItem("Open", new BMessage(MENU_MSG_COPEN), 'O', B_COMMAND_KEY));
			compMenu->AddItem(new BMenuItem("Find...", new BMessage(MENU_MSG_CFIND), 'F', B_COMMAND_KEY));
			compMenu->AddItem(new BMenuItem("Quit", new BMessage(MENU_MSG_QUIT), 'Q', B_COMMAND_KEY));
			menuBar->AddItem(compMenu);

			AddChild(menuBar);
*/
