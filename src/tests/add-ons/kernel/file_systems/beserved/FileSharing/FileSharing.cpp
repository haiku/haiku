// File Sharing Preferences Application
//
// FileSharing.cpp
//

// TODO:
// 1.  Make share properties panel modal but still work with file panel

#include "Application.h"
#include "Window.h"
#include "Region.h"
#include "Alert.h"
#include "CheckBox.h"
#include "Button.h"
#include "TextControl.h"
#include "Menu.h"
#include "PopUpMenu.h"
#include "MenuItem.h"
#include "MenuField.h"
#include "FilePanel.h"
#include "FindDirectory.h"
#include "Path.h"
#include "Mime.h"
#include "Roster.h"
#include "Resources.h"

#include "string.h"
#include "stdio.h"
#include "ctype.h"
#include "signal.h"
#include "unistd.h"

#include "betalk.h"
#include "rpc.h"
//#include "besure_auth.h"

#include "ColumnListView.h"
#include "CLVColumn.h"
#include "CLVEasyItem.h"
#include "NewStrings.h"

#define BT_MAX_FILE_SHARES	128

#define MSG_SHARE_INVOKE	'SInv'
#define MSG_PATH_BROWSE		'PBrw'
#define MSG_SHARE_OK		'SOky'
#define MSG_SHARE_CANCEL	'SCan'
#define MSG_SHARE_REFRESH	'SRef'
#define MSG_NEW_SHARE		'SNew'
#define MSG_EDIT_SHARE		'SEdt'
#define MSG_REMOVE_SHARE	'SRem'
#define MSG_SHARE_DEPLOY	'SDep'
#define MSG_SHARE_SELECT	'SSel'
#define MSG_SHARE_PERMS		'SPrm'
#define MSG_EDIT_SECURITY	'ESec'

#define MSG_PICKUSER_SELECT	'MSel'
#define MSG_PICKUSER_ADD	'PAdd'
#define MSG_PICKUSER_REMOVE	'PRem'

#define MSG_DOMUSER_OK		'DOky'
#define MSG_DOMUSER_CANCEL	'DCan'
#define MSG_DOMUSER_SELECT	'DSel'

#define ICON_USER_LARGE		111
#define ICON_USER_SMALL		112
#define ICON_GROUP_LARGE	115
#define ICON_GROUP_SMALL	116
#define ICON_DOMAIN_LARGE	109
#define ICON_DOMAIN_SMALL	110
#define ICON_SWITCH_LARGE	119
#define ICON_SWITCH_SMALL	120
#define ICON_LIGHT_LARGE	121
#define ICON_PADLOCK_LARGE	122
#define ICON_KEY_LARGE		123

#define WARN_REMOVE_SHARE	"Removing this file share will prevent other users on your network from accessing certain files on this computer.  Are you sure you want to remove this file share?"

#ifndef iswhite
#define iswhite(c)			((c == ' ' || c == '\t'))
#endif

typedef void op_refresh(int share);

typedef struct userRights
{
	char *user;
	int rights;
	bool isGroup;
	struct userRights *next;
} bt_user_rights;

typedef struct fileShare
{
	char path[B_PATH_NAME_LENGTH];
	char name[B_FILE_NAME_LENGTH];

	bool used;
	bool readOnly;
	bool followLinks;

	// What rights does each user have?
	bt_user_rights *rights;
	int security;

	int index;
	struct fileShare *next;
} bt_fileShare_t;

void printString(FILE *fp, const char *str);

bt_fileShare_t fileShares[BT_MAX_FILE_SHARES];
char tokBuffer[B_PATH_NAME_LENGTH], *tokPtr;
char authServerName[B_FILE_NAME_LENGTH], authKey[MAX_KEY_LENGTH + 1];
unsigned int authServerIP = 0;

char *keywords[] =
{
	"share",
	"as",
	"set",
	"read",
	"write",
	"read-write",
	"promiscuous",
	"on",
	"to",
	"authenticate",
	"with",
	"group",
	NULL
};


// ----- SmartColumnListView ----------------------------------------------------------------

const int32 MSG_LIST_DESELECT = 'CLV0';

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

// ----- FileShareItem ----------------------------------------------------------------------

class FileShareItem : public CLVEasyItem
{
	public:
		FileShareItem(const char *text0, const char *text1, const char *text2) :
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

		~FileShareItem()
		{
		}
};


// ----- MilestoneView -----------------------------------------------------

class MilestoneView : public BView
{
	public:
		MilestoneView(BRect rect, const char *title) :
		  BView(rect, "MilestoneView", B_FOLLOW_ALL, B_WILL_DRAW)
		{
			strcpy(heading, title);

			rgb_color gray = ui_color(B_PANEL_BACKGROUND_COLOR);
			SetViewColor(gray);

			BRect bmpRect(0.0, 0.0, 31.0, 31.0);
			icon = new BBitmap(bmpRect, B_CMAP8);
			BMimeType mime("application/x-vnd.Teldar-FileSharing");
			mime.GetIcon(icon, B_LARGE_ICON);

			BRect r;
			r.Set(120, 48, 190, 68);
			okBtn = new BButton(r, "OKBtn", "OK", new BMessage(MSG_SHARE_OK), B_FOLLOW_RIGHT | B_FOLLOW_BOTTOM);
			okBtn->MakeDefault(true);
			okBtn->SetEnabled(false);
			AddChild(okBtn);
		}

		~MilestoneView()
		{
		}

		void Draw(BRect rect)
		{
			BRect r = Bounds();
			BRect iconRect(13.0, 5.0, 45.0, 37.0);
			rgb_color black = { 0, 0, 0, 255 };
			rgb_color gray = ui_color(B_PANEL_BACKGROUND_COLOR);
			rgb_color light = ui_color(B_MENU_BACKGROUND_COLOR);

			SetViewColor(gray);
			SetLowColor(gray);
			FillRect(r, B_SOLID_LOW);

			SetHighColor(black);
			SetFont(be_bold_font);
			SetFontSize(11);
			MovePenTo(55, 20);
			DrawString(heading);

			SetFont(be_plain_font);
			MovePenTo(55, 35);
			DrawString(msg);

			SetDrawingMode(B_OP_ALPHA); 
			SetHighColor(0, 0, 0, 180);       
			SetBlendingMode(B_CONSTANT_ALPHA, B_ALPHA_COMPOSITE);
			DrawBitmap(icon, iconRect);
		}

		void setText(const char *text, bool done)
		{
			strcpy(msg, text);
			LockLooper();
			Invalidate(Bounds());
			okBtn->SetEnabled(done);
			UnlockLooper();
			snooze(200000);
		}

	private:
		BButton *okBtn;
		BBitmap *icon;
		char heading[50];
		char msg[256];
};

// ----- MilestonePanel ----------------------------------------------------------------------

class MilestonePanel : public BWindow
{
	public:
		MilestonePanel(BRect frame, const char *title) :
		  BWindow(frame, "", B_MODAL_WINDOW_LOOK, B_MODAL_APP_WINDOW_FEEL, B_NOT_ZOOMABLE | B_NOT_RESIZABLE)
		{
			BRect r = Bounds();
			milestoneView = new MilestoneView(r, title);
			AddChild(milestoneView);

			Show();
		}

		// MessageReceived()
		//
		void MessageReceived(BMessage *msg)
		{
			switch (msg->what)
			{
				case MSG_SHARE_OK:
					BWindow::Quit();
					break;

				default:
					BWindow::MessageReceived(msg);
					break;
			}
		}

		// setText()
		//
		void setText(const char *text, bool done)
		{
			milestoneView->setText(text, done);
		}

	private:
		MilestoneView *milestoneView;
};

// ----- PickUserItem ------------------------------------------------------------

class PickUserItem : public CLVEasyItem
{
	public:
		PickUserItem(bool group, const char *text0, const char *text1) :
		  CLVEasyItem(0, false, false, 20.0)
		{
			BBitmap *icon;
			BMessage archive;
			size_t size;

			char *bits = (char *) be_app->AppResources()->LoadResource(type_code('BBMP'), group ? ICON_GROUP_SMALL : ICON_USER_SMALL, &size);
			if (bits)
				if (archive.Unflatten(bits) == B_OK)
				{
					icon = new BBitmap(&archive);

					SetColumnContent(0, icon, 2.0);
					SetColumnContent(1, text0);
					SetColumnContent(2, text1);

					isGroupItem = group;
				}
		}

		~PickUserItem()
		{
		}

		bool isUser()
		{
			return !isGroupItem;
		}

		bool isGroup()
		{
			return isGroupItem;
		}

		static int CompareItems(const CLVListItem *item1, const CLVListItem *item2, int32 KeyColumn)
		{
			const char *str1 = ((PickUserItem *) item1)->GetColumnContentText(1);
			const char *str2 = ((PickUserItem *) item2)->GetColumnContentText(1);
			return strcasecmp(str1, str2);
		}

	private:
		bool isGroupItem;
};

// ----- SecurityView -----------------------------------------------------

class SecurityView : public BView
{
	public:
		SecurityView(BRect rect) :
		  BView(rect, "DomainUserView", B_FOLLOW_ALL, B_WILL_DRAW)
		{
			BMessage archive;
			size_t size;
			char *bits;

			rgb_color gray = ui_color(B_PANEL_BACKGROUND_COLOR);
			SetViewColor(gray);

			bits = (char *) be_app->AppResources()->LoadResource(type_code('BBMP'), ICON_PADLOCK_LARGE, &size);
			if (bits)
				if (archive.Unflatten(bits) == B_OK)
					lockIcon = new BBitmap(&archive);

			BRect r;
			r.Set(10, 60, 360, 80);

			mnuMethod = new BPopUpMenu("");
			mnuMethod->AddItem(new BMenuItem("No Authentication Required", NULL));
			mnuMethod->AddItem(new BMenuItem("BeSure Authentication Server", NULL));
//			mnuMethod->AddItem(new BMenuItem("Directory Server (LDAP)", NULL));

			mnuAccess = new BMenuField(r, "AuthMethod", "Authentication Method:", mnuMethod);
			mnuAccess->SetDivider(120);
			if (authServerIP)
				item = mnuMethod->FindItem("BeSure Authentication Server");
			else
				item = mnuMethod->FindItem("No Authentication Required");

			if (item)
				item->SetMarked(true);
			AddChild(mnuAccess);

			r.Set(10, 85, 360, 105);
			editServer = new BTextControl(r, "ServerName", "Server Name:", authServerName, NULL);
			editServer->SetDivider(120);
			AddChild(editServer);

			r.Set(210, 115, 280, 135);
			okBtn = new BButton(r, "OkayBtn", "OK", new BMessage(MSG_DOMUSER_OK), B_FOLLOW_RIGHT | B_FOLLOW_BOTTOM);
			okBtn->MakeDefault(true);
			AddChild(okBtn);

			r.Set(290, 115, 360, 135);
			AddChild(new BButton(r, "CancelBtn", "Cancel", new BMessage(MSG_DOMUSER_CANCEL), B_FOLLOW_RIGHT | B_FOLLOW_BOTTOM));
		}

		~SecurityView()
		{
		}

		void Draw(BRect rect)
		{
			BRect r = Bounds();
			BRect iconRect(13.0, 10.0, 45.0, 42.0);
			rgb_color black = { 0, 0, 0, 255 };
			rgb_color gray = ui_color(B_PANEL_BACKGROUND_COLOR);

			SetViewColor(gray);
			SetLowColor(gray);
			FillRect(r, B_SOLID_LOW);

			SetHighColor(black);
			SetFont(be_plain_font);
			SetFontSize(10);
			MovePenTo(55, 17);
			DrawString("When users connect to this computer to access shared files,");
			MovePenTo(55, 29);
			DrawString("the settings here will be used to determine how those users");
			MovePenTo(55, 41);
			DrawString("are authenticated.");

			SetDrawingMode(B_OP_ALPHA); 
			SetHighColor(0, 0, 0, 180);       
			SetBlendingMode(B_CONSTANT_ALPHA, B_ALPHA_COMPOSITE);
			DrawBitmap(lockIcon, iconRect);
		}

		void UpdateData()
		{
			struct hostent *ent;

			// If the selected item is to use no authentication, blank out the server name
			// and IP address.  We'll assume this is the case until we can validate the data
			// provided.
			authServerName[0] = 0;
			authServerIP = 0;

			// Now get the selected authentication method.  If it's BeSure, then store the
			// server name and obtain its IP address.
			BMenuItem *item = mnuMethod->FindMarked();
			if (item)
				if (strcmp(item->Label(), "BeSure Authentication Server") == 0)
				{
					strcpy(authServerName, editServer->Text());
		
					ent = gethostbyname(authServerName);
					if (ent != NULL)
						authServerIP = ntohl(*((unsigned int *) ent->h_addr));
					else
						authServerIP = 0;
				}
		}

	private:
		BButton *okBtn;
		BPopUpMenu *mnuMethod;
		BMenuField *mnuAccess;
		BMenuItem *item;
		BTextControl *editServer;
		BBitmap *lockIcon;
};


// ----- SecurityPanel ----------------------------------------------------------------------

class SecurityPanel : public BWindow
{
	public:
		SecurityPanel(BRect frame) :
		  BWindow(frame, "Security", B_TITLED_WINDOW_LOOK, B_NORMAL_WINDOW_FEEL, B_NOT_ZOOMABLE | B_NOT_RESIZABLE)
		{
			BRect r = Bounds();
			infoView = new SecurityView(r);
			AddChild(infoView);

			Show();
		}

		// MessageReceived()
		//
		void MessageReceived(BMessage *msg)
		{
			switch (msg->what)
			{
				case MSG_DOMUSER_OK:
					infoView->UpdateData();
					BWindow::Quit();
					break;

				case MSG_DOMUSER_CANCEL:
					BWindow::Quit();
					break;

				default:
					BWindow::MessageReceived(msg);
					break;
			}
		}

	private:
		SecurityView *infoView;
};

// ----- DomainUserView -----------------------------------------------------

class DomainUserView : public BView
{
	public:
		DomainUserView(BRect rect) :
		  BView(rect, "DomainUserView", B_FOLLOW_ALL, B_WILL_DRAW)
		{
			BMessage archive;
			size_t size;
			char *bits;

			rgb_color gray = ui_color(B_PANEL_BACKGROUND_COLOR);
			SetViewColor(gray);

			bits = (char *) be_app->AppResources()->LoadResource(type_code('BBMP'), ICON_LIGHT_LARGE, &size);
			if (bits)
				if (archive.Unflatten(bits) == B_OK)
					lightIcon = new BBitmap(&archive);

			BRect r;
			r.Set(10, 240, 200, 260);

			mnuRights = new BPopUpMenu("");
			mnuRights->AddItem(new BMenuItem("Read-only", NULL));
			mnuRights->AddItem(new BMenuItem("Read-write", NULL));

			mnuAccess = new BMenuField(r, "AccessRights", "Access Rights:", mnuRights);
			mnuAccess->SetDivider(75);
			item = mnuRights->FindItem("Read-only");
			if (item)
				item->SetMarked(true);
			AddChild(mnuAccess);

			r.Set(210, 320, 280, 340);
			okBtn = new BButton(r, "OkayBtn", "OK", new BMessage(MSG_DOMUSER_OK), B_FOLLOW_RIGHT | B_FOLLOW_BOTTOM);
			okBtn->MakeDefault(true);
			AddChild(okBtn);

			r.Set(290, 320, 360, 340);
			AddChild(new BButton(r, "CancelBtn", "Cancel", new BMessage(MSG_DOMUSER_CANCEL), B_FOLLOW_RIGHT | B_FOLLOW_BOTTOM));

			// Now add the membership list.
			CLVContainerView *listContView;
			r.Set(13, 48, 360 - B_V_SCROLL_BAR_WIDTH, 230);
			listView = new SmartColumnListView(r, &listContView, NULL, B_FOLLOW_ALL_SIDES,
				B_WILL_DRAW | B_FRAME_EVENTS | B_NAVIGABLE, B_SINGLE_SELECTION_LIST,
				false, false, true, false, B_FANCY_BORDER);

			listView->AddColumn(new CLVColumn(NULL, 20.0, CLV_LOCK_AT_BEGINNING | CLV_NOT_MOVABLE |
				CLV_NOT_RESIZABLE | CLV_PUSH_PASS | CLV_MERGE_WITH_RIGHT));
			listView->AddColumn(new CLVColumn("User or Group", 140.0, CLV_SORT_KEYABLE, 50.0));
			listView->AddColumn(new CLVColumn("Full Name", 160.0, CLV_SORT_KEYABLE));

			AddUserList(listView);
			AddGroupList(listView);
			listView->SetSortFunction(PickUserItem::CompareItems);
			listView->SortItems();

			AddChild(listContView);
		}

		~DomainUserView()
		{
		}

		void Draw(BRect rect)
		{
			BRect r = Bounds();
			rgb_color black = { 0, 0, 0, 255 };
			rgb_color gray = ui_color(B_PANEL_BACKGROUND_COLOR);

			SetViewColor(gray);
			SetLowColor(gray);
			FillRect(r, B_SOLID_LOW);

			SetHighColor(black);
			SetFont(be_plain_font);
			SetFontSize(10);
			MovePenTo(10, 17);
			DrawString("Select a user or group from the list below, select the desired access rights,");
			MovePenTo(10, 29);
			DrawString("then click the OK button.  Click the Cancel button to abort the selection.");

			MovePenTo(55, 288);
			DrawString("When granting write access to a shared volume, you are by");
			MovePenTo(55, 300);
			DrawString("definition also granting the ability to rename and delete files.");

			SetDrawingMode(B_OP_ALPHA); 
			SetHighColor(0, 0, 0, 180);       
			SetBlendingMode(B_CONSTANT_ALPHA, B_ALPHA_COMPOSITE);
			r.Set(13, 275, 45, 307);
			DrawBitmap(lightIcon, r);
		}

		void AddUserList(ColumnListView* listView)
		{
			DIR *dir = NULL;
			char user[MAX_USERNAME_LENGTH], fullName[MAX_DESC_LENGTH];
			bt_outPacket *outPacket = btRPCPutHeader(BT_CMD_READUSERS, 1, 4);
			btRPCPutArg(outPacket, B_INT32_TYPE, &dir, sizeof(int32));
			bt_inPacket *inPacket = btRPCSimpleCall(authServerIP, BT_BESURE_PORT, outPacket);
			if (inPacket)
			{
				int error, count = 0;

				do
				{
					error = btRPCGetInt32(inPacket);
					if (error == B_OK)
					{
						memset(user, 0, sizeof(user));
						memset(fullName, 0, sizeof(fullName));
						btRPCGetString(inPacket, user, sizeof(user));
						btRPCGetString(inPacket, fullName, sizeof(fullName));
						listView->AddItem(new PickUserItem(false, user, fullName));
					}
					else break;
				} while (++count < 80);

				free(inPacket->buffer);
				free(inPacket);
			}
		}

		void AddGroupList(ColumnListView* listView)
		{
			DIR *dir = NULL;
			char group[MAX_USERNAME_LENGTH];
			bt_outPacket *outPacket = btRPCPutHeader(BT_CMD_READGROUPS, 1, 4);
			btRPCPutArg(outPacket, B_INT32_TYPE, &dir, sizeof(int32));
			bt_inPacket *inPacket = btRPCSimpleCall(authServerIP, BT_BESURE_PORT, outPacket);
			if (inPacket)
			{
				int error, count = 0;

				do
				{
					error = btRPCGetInt32(inPacket);
					if (error == B_OK)
					{
						memset(group, 0, sizeof(group));
						btRPCGetString(inPacket, group, sizeof(group));
						listView->AddItem(new PickUserItem(true, group, ""));
					}
					else break;
				} while (++count < 80);

				free(inPacket->buffer);
				free(inPacket);
			}
		}

		char *GetSelectedUser()
		{
			if (listView)
			{
				int index = listView->CurrentSelection();
				if (index >= 0)
				{
					PickUserItem *item = (PickUserItem *) listView->ItemAt(index);
					return ((char *) item->GetColumnContentText(1));
				}
			}

			return NULL;
		}

		int GetRights()
		{
			BMenuItem *item = mnuRights->FindMarked();
			if (item)
				if (strcmp(item->Label(), "Read-only") == 0)
					return BT_RIGHTS_READ;
				else if (strcmp(item->Label(), "Read-write") == 0)
					return (BT_RIGHTS_READ | BT_RIGHTS_WRITE);

			return 0;
		}

		bool isGroup()
		{
			if (listView)
			{
				int index = listView->CurrentSelection();
				if (index >= 0)
				{
					PickUserItem *item = (PickUserItem *) listView->ItemAt(index);
					return item->isGroup();
				}
			}

			return false;
		}

	private:
		SmartColumnListView *listView;
		BButton *okBtn;
		BPopUpMenu *mnuRights;
		BMenuField *mnuAccess;
		BMenuItem *item;
		BBitmap *lightIcon;
};


// ----- DomainUserPanel ----------------------------------------------------------------------

class DomainUserPanel : public BWindow
{
	public:
		DomainUserPanel(BRect frame, bt_fileShare_t *sharePtr) :
		  BWindow(frame, "Domain Users", B_TITLED_WINDOW_LOOK, B_NORMAL_WINDOW_FEEL, B_NOT_ZOOMABLE | B_NOT_RESIZABLE)
		{
			share = sharePtr;

			BRect r = Bounds();
			infoView = new DomainUserView(r);
			AddChild(infoView);

			Show();
		}

		// MessageReceived()
		//
		void MessageReceived(BMessage *msg)
		{
			switch (msg->what)
			{
				case MSG_DOMUSER_OK:
					SaveUserRights(infoView->GetSelectedUser(), infoView->GetRights(), infoView->isGroup());
					BWindow::Quit();
					break;

				case MSG_DOMUSER_CANCEL:
					BWindow::Quit();
					break;

				default:
					BWindow::MessageReceived(msg);
					break;
			}
		}

		void SaveUserRights(char *user, int rights, bool isGroup)
		{
			bt_user_rights *ur;

			if (user == NULL || rights == 0)
				return;

			for (ur = share->rights; ur; ur = ur->next)
				if (strcmp(ur->user, user) == 0)
				{
					ur->rights = rights;
					return;
				}

			ur = (bt_user_rights *) malloc(sizeof(bt_user_rights));
			if (ur)
			{
				ur->user = (char *) malloc(strlen(user) + 1);
				if (ur->user)
				{
					strcpy(ur->user, user);
					ur->rights = rights;
					ur->isGroup = isGroup;
					ur->next = share->rights;
					share->rights = ur;
				}
				else
					free(ur);
			}
		}

	private:
		DomainUserView *infoView;
		bt_fileShare_t *share;
};

// ----- ShareInfoView -----------------------------------------------------

class ShareInfoView : public BView
{
	public:
		ShareInfoView(BRect rect, bt_fileShare_t *sharePtr) :
		  BView(rect, "HostInfoView", B_FOLLOW_ALL, B_WILL_DRAW)
		{
			BMessage archive;
			size_t size;
			char *bits;

			share = sharePtr;
			optionIcon = domainIcon = NULL;

			rgb_color gray = ui_color(B_PANEL_BACKGROUND_COLOR);
			SetViewColor(gray);

			BRect bmpRect(0.0, 0.0, 31.0, 31.0);
			icon = new BBitmap(bmpRect, B_CMAP8);
			BMimeType mime("application/x-vnd.BeServed-fileshare");
			mime.GetIcon(icon, B_LARGE_ICON);

			bits = (char *) be_app->AppResources()->LoadResource(type_code('BBMP'), ICON_SWITCH_LARGE, &size);
			if (bits)
				if (archive.Unflatten(bits) == B_OK)
					optionIcon = new BBitmap(&archive);

			bits = (char *) be_app->AppResources()->LoadResource(type_code('BBMP'), ICON_DOMAIN_LARGE, &size);
			if (bits)
				if (archive.Unflatten(bits) == B_OK)
					domainIcon = new BBitmap(&archive);

			BRect r(10, 52, 200, 72);
			editName = new BTextControl(r, "ShareName", "Name:", share->name, NULL);
			editName->SetDivider(40);
			AddChild(editName);

			r.top = 77;
			r.bottom = r.top + 20;
			r.right = 300;
			editPath = new BTextControl(r, "SharePath", "Path:", share->path, NULL);
			editPath->SetDivider(40);
			AddChild(editPath);

			r.top = 150;
			r.bottom = r.top + 15;
			chkReadWrite = new BCheckBox(r, "ReadWrite", "Users can make changes to files and folders", NULL);
			chkReadWrite->SetValue(share->readOnly ? B_CONTROL_OFF : B_CONTROL_ON);
			AddChild(chkReadWrite);

			r.top = 170;
			r.bottom = r.top + 15;
			chkFollowLinks = new BCheckBox(r, "FollowLinks", "Follow links leading outside the path", NULL);
			chkFollowLinks->SetValue(share->followLinks ? B_CONTROL_ON : B_CONTROL_OFF);
			chkFollowLinks->SetEnabled(false);
			AddChild(chkFollowLinks);

			r.Set(310, 75, 390, 95);
			AddChild(new BButton(r, "BrowseBtn", "Browse...", new BMessage(MSG_PATH_BROWSE)));

			r.Set(325, 235, 400, 255);
			btnAdd = new BButton(r, "AddBtn", "Add", new BMessage(MSG_PICKUSER_ADD), B_FOLLOW_RIGHT | B_FOLLOW_BOTTOM);
			btnAdd->SetEnabled(authServerIP != 0);
			AddChild(btnAdd);

			r.Set(325, 265, 400, 285);
			btnRemove = new BButton(r, "RemoveBtn", "Remove", new BMessage(MSG_PICKUSER_REMOVE), B_FOLLOW_RIGHT | B_FOLLOW_BOTTOM);
			btnRemove->SetEnabled(false);
			AddChild(btnRemove);

			// Now add the membership list.
			CLVContainerView *listContView;
			r.Set(13, 235, 315 - B_V_SCROLL_BAR_WIDTH, 340);
			listView = new SmartColumnListView(r, &listContView, NULL, B_FOLLOW_ALL_SIDES,
				B_WILL_DRAW | B_FRAME_EVENTS | B_NAVIGABLE, B_SINGLE_SELECTION_LIST,
				false, false, true, false, B_FANCY_BORDER);

			listView->AddColumn(new CLVColumn(NULL, 20.0, CLV_LOCK_AT_BEGINNING | CLV_NOT_MOVABLE |
				CLV_NOT_RESIZABLE | CLV_PUSH_PASS | CLV_MERGE_WITH_RIGHT));
			listView->AddColumn(new CLVColumn("User or Group", 150.0, CLV_SORT_KEYABLE, 50.0));
			listView->AddColumn(new CLVColumn("Access", 80.0, CLV_SORT_KEYABLE));

			listView->SetSelectionMessage(new BMessage(MSG_PICKUSER_SELECT));
			listView->SetInvocationMessage(new BMessage(MSG_LIST_DESELECT));

			AddUserList(listView);
			listView->SetSortFunction(PickUserItem::CompareItems);
			listView->SortItems();
			AddChild(listContView);

			r.Set(245, 355, 315, 375);
			BButton *okBtn = new BButton(r, "OkayBtn", "OK", new BMessage(MSG_SHARE_OK), B_FOLLOW_RIGHT | B_FOLLOW_BOTTOM);
			okBtn->MakeDefault(true);
			AddChild(okBtn);

			r.Set(325, 355, 400, 375);
			AddChild(new BButton(r, "CancelBtn", "Cancel", new BMessage(MSG_SHARE_CANCEL), B_FOLLOW_RIGHT | B_FOLLOW_BOTTOM));
		}

		~ShareInfoView()
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
			DrawString(share->name);
			MovePenTo(55, 130);
			DrawString("General Options");
			MovePenTo(55, 212);
			DrawString("Domain User Access");

			SetFont(be_plain_font);
			SetFontSize(10);
			MovePenTo(55, 28);
			DrawString("Use this properties window to define the parameters under which you will");
			MovePenTo(55, 40);
			DrawString("allow users to share files on this computer.");

			SetDrawingMode(B_OP_ALPHA); 
			SetHighColor(0, 0, 0, 180);       
			SetBlendingMode(B_CONSTANT_ALPHA, B_ALPHA_COMPOSITE);
			DrawBitmap(icon, iconRect);

			r.Set(13, 113, 45, 145);
			DrawBitmap(optionIcon, r);
			r.Set(13, 195, 45, 227);
			DrawBitmap(domainIcon, r);

			DrawDivider(10, 107, 390);
			DrawDivider(10, 193, 390);
		}

		void DrawDivider(int left, int top, int width)
		{
			rgb_color darkGray = { 189, 186, 189, 255 };
			rgb_color lightGray = { 239, 239, 239, 255 };
			BPoint startPt(left, top);
			BPoint endPt(left + width, top);
			SetHighColor(darkGray);
			StrokeLine(startPt, endPt);
			startPt.y += 1;
			endPt.y += 1;
			SetHighColor(lightGray);
			StrokeLine(startPt, endPt);
		}

		void ItemSelected()
		{
			if (authServerIP)
				btnRemove->SetEnabled(true);
		}

		void ItemDeselected()
		{
			btnRemove->SetEnabled(false);
		}

		void AddUserList(ColumnListView* listView)
		{
			bt_user_rights *ur;
			if (share->security != BT_AUTH_NONE)
				for (ur = share->rights; ur; ur = ur->next)
				{
					char access[50];
					access[0] = 0;
					if (ur->rights & BT_RIGHTS_READ)
						strcat(access, "Read ");
					if (ur->rights & BT_RIGHTS_WRITE)
						strcat(access, "Write");

					listView->AddItem(new PickUserItem(ur->isGroup, ur->user, access));
				}
		}

		void ResetUserList()
		{
			FileShareItem *item;

			listView->LockLooper();

			do
			{
				item = (FileShareItem *) listView->RemoveItem(int32(0));
				if (item)
					delete item;
			} while(item);

			AddUserList(listView);
			listView->UnlockLooper();
		}

		// setPath()
		//
		void setPath(const char *path)
		{
			if (editPath)
			{
				editPath->LockLooper();
				editPath->SetText(path);
				editPath->UnlockLooper();
			}
		}

		// RemoveSelectedRights()
		//
		void RemoveSelectedRights()
		{
			int curItem;

			listView->LockLooper();

			curItem = listView->CurrentSelection();
			if (curItem >= 0)
			{
				FileShareItem *item = (FileShareItem *) listView->ItemAt(curItem);
				if (item)
				{
					bt_user_rights *ur, *lastUR;
					const char *user = item->GetColumnContentText(1);
					for (ur = share->rights, lastUR = NULL; ur; ur = ur->next)
					{
						if (strcmp(ur->user, user) == 0)
						{
							if (lastUR)
								lastUR->next = ur->next;
							else
								share->rights = ur->next;

							free(ur->user);
							free(ur);
							break;
						}

						lastUR = ur;
					}
				}

				listView->RemoveItem(curItem);
			}

			listView->UnlockLooper();
		}

		// updateShare()
		//
		bool updateShare()
		{
			BAlert *alert;
			char buf[256];
			unsigned int length;

			length = sizeof(share->name);
			if (strlen(editName->Text()) > length)
			{
				sprintf(buf, "The name of this share is too long.  Please provide a name that is at most %d characters.", length);
				alert = new BAlert("Oops!", buf, "OK");
				return false;
			}

			length = sizeof(share->path);
			if (strlen(editPath->Text()) > length)
			{
				sprintf(buf, "The path name is too long.  The share path can be at most %d characters.", length);
				alert = new BAlert("Oops!", buf, "OK");
				return false;
			}

			strcpy(share->name, editName->Text());
			strcpy(share->path, editPath->Text());
			share->readOnly = chkReadWrite->Value() == B_CONTROL_OFF;
			share->followLinks = chkFollowLinks->Value() == B_CONTROL_ON;
			return true;
		}

	private:
		SmartColumnListView *listView;
		BBitmap *icon, *optionIcon, *domainIcon;
		BTextControl *editName;
		BTextControl *editPath;
		BCheckBox *chkReadWrite;
		BCheckBox *chkFollowLinks;
		BButton *btnAdd, *btnRemove;
		bt_fileShare_t *share;
};


// ----- ShareInfoPanel ----------------------------------------------------------------------

class ShareInfoPanel : public BWindow
{
	public:
		ShareInfoPanel(BRect frame, bt_fileShare_t *sharePtr, BWindow *parent) :
		  BWindow(frame, sharePtr->name, B_TITLED_WINDOW_LOOK, B_NORMAL_WINDOW_FEEL, B_NOT_ZOOMABLE | B_NOT_RESIZABLE)
		{
			share = sharePtr;
			shareWin = parent;

			BRect r = Bounds();
			infoView = new ShareInfoView(r, share);
			AddChild(infoView);

			Show();
		}

		// MessageReceived()
		//
		void MessageReceived(BMessage *msg)
		{
			BFilePanel *filePanel;
			entry_ref entryRef;
			BRect frame;
			DomainUserPanel *picker;
			status_t status;

			BEntry entry(share->path, false);
			entry.GetRef(&entryRef);

			switch (msg->what)
			{
				case MSG_PATH_BROWSE:
					filePanel = new BFilePanel(B_OPEN_PANEL, &be_app_messenger, &entryRef, B_DIRECTORY_NODE, false);
					filePanel->Show();
					filePanel->Window()->SetTitle("Shared Folder");
					filePanel->SetButtonLabel(B_DEFAULT_BUTTON, "Select");
					break;

				case MSG_PICKUSER_ADD:
					frame = Frame();
					frame.left += 100;
					frame.top += 30;
					frame.right = frame.left + 370;
					frame.bottom = frame.top + 350;
					picker = new DomainUserPanel(frame, share);
					wait_for_thread(picker->Thread(), &status);
					infoView->ResetUserList();
					break;

				case MSG_PICKUSER_REMOVE:
					infoView->RemoveSelectedRights();
					break;

				case MSG_PICKUSER_SELECT:
					infoView->ItemSelected();
					break;

				case MSG_LIST_DESELECT: //(int32) 'CLV0':
					infoView->ItemDeselected();
					break;

				case MSG_SHARE_OK:
					if (!infoView->updateShare())
						break;
					else
					{
						BMessage *msg = new BMessage(MSG_SHARE_REFRESH);
						msg->AddPointer("share", share);
						shareWin->PostMessage(msg);
					}

				case MSG_SHARE_CANCEL:
					BWindow::Quit();
					break;

				default:
					BWindow::MessageReceived(msg);
					break;
			}
		}

		// setPath()
		//
		void setPath(const char *path)
		{
			infoView->setPath(path);
		}

	private:
		ShareInfoView *infoView;
		BWindow *shareWin;
		bt_fileShare_t *share;
};

// ----- FileSharingHeaderView ---------------------------------------------------------------

class FileSharingHeaderView : public BView
{
	public:
		FileSharingHeaderView(BRect rect) :
		  BView(rect, "FileSharingHeaderView", B_FOLLOW_TOP | B_FOLLOW_LEFT_RIGHT, B_WILL_DRAW)
		{
			rgb_color gray = ui_color(B_PANEL_BACKGROUND_COLOR);
			SetViewColor(gray);

			BRect bmpRect(0.0, 0.0, 31.0, 31.0);
			icon = new BBitmap(bmpRect, B_CMAP8);
			BMimeType mime("application/x-vnd.Teldar-FileSharing");
			mime.GetIcon(icon, B_LARGE_ICON);

			BRect r;
			r.Set(15, 55, 70, 75);
			AddChild(new BButton(r, "NewBtn", "New", new BMessage(MSG_NEW_SHARE)));

			r.Set(80, 55, 135, 75);
			editBtn = new BButton(r, "EditBtn", "Edit", new BMessage(MSG_EDIT_SHARE));
			editBtn->SetEnabled(false);
			AddChild(editBtn);

			r.Set(145, 55, 210, 75);
			removeBtn = new BButton(r, "RemoveBtn", "Remove", new BMessage(MSG_REMOVE_SHARE));
			removeBtn->SetEnabled(false);
			AddChild(removeBtn);

			r.Set(337, 55, 412, 75);
			AddChild(new BButton(r, "SecurityBtn", "Security", new BMessage(MSG_EDIT_SECURITY)));
		}

		~FileSharingHeaderView()
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
			DrawString("File Sharing Preferences");

			SetFont(be_plain_font);
			SetFontSize(10);
			MovePenTo(55, 28);
			DrawString("Normally, remote computers on your network do not have access to files on");
			MovePenTo(55, 40);
			DrawString("this computer.  You grant them access by explicitly sharing specific folders.");

			SetDrawingMode(B_OP_ALPHA); 
			SetHighColor(0, 0, 0, 180);       
			SetBlendingMode(B_CONSTANT_ALPHA, B_ALPHA_COMPOSITE);
			DrawBitmap(icon, iconRect);
		}

		BButton *editBtn;
		BButton *removeBtn;

	private:
		BBitmap *icon;
};

// ----- FileSharingFooterView ---------------------------------------------------------------

class FileSharingFooterView : public BView
{
	public:
		FileSharingFooterView(BRect rect) :
		  BView(rect, "FileSharingHeaderView", B_FOLLOW_BOTTOM | B_FOLLOW_LEFT_RIGHT, B_WILL_DRAW)
		{
			rgb_color gray = ui_color(B_PANEL_BACKGROUND_COLOR);
			SetViewColor(gray);

			BRect r;
			r.Set(177, 52, 247, 72);
			BButton *okBtn = new BButton(r, "SaveBtn", "Save", new BMessage(MSG_SHARE_OK), B_FOLLOW_RIGHT | B_FOLLOW_BOTTOM);
			okBtn->MakeDefault(true);
			AddChild(okBtn);

			r.Set(257, 52, 327, 72);
			deployBtn = new BButton(r, "DeployBtn", "Deploy", new BMessage(MSG_SHARE_DEPLOY), B_FOLLOW_RIGHT | B_FOLLOW_BOTTOM);
			deployBtn->SetEnabled(false);
			AddChild(deployBtn);

			r.Set(337, 52, 412, 72);
			AddChild(new BButton(r, "CancelBtn", "Close", new BMessage(MSG_SHARE_CANCEL), B_FOLLOW_RIGHT | B_FOLLOW_BOTTOM));
		}

		~FileSharingFooterView()
		{
		}

		void Draw(BRect rect)
		{
			BRect r = Bounds();
			rgb_color black = { 0, 0, 0, 255 };
			rgb_color gray = ui_color(B_PANEL_BACKGROUND_COLOR);

			SetViewColor(gray);
			SetLowColor(gray);
			FillRect(r, B_SOLID_LOW);

			SetHighColor(black);
			SetFont(be_plain_font);
			SetFontSize(10);
			MovePenTo(15, 15);
			DrawString("Save your changes to file sharing preferences by clicking the 'Save' button.  This");
			MovePenTo(15, 27);
			DrawString("will not have an effect on users until you click the 'Deploy' button or manually");
			MovePenTo(15, 39);
			DrawString("restart the BeServed file serving application.");
		}

		void SetDeployable(bool deployable)
		{
			deployBtn->SetEnabled(deployable);
		}

	private:
		BButton *deployBtn;
};

// ----- FileSharingWindow -------------------------------------------------------------------

class FileSharingWindow : public BWindow
{
	public:
		FileSharingWindow() :
		  BWindow(BRect(50, 50, 475, 460), "File Sharing", B_TITLED_WINDOW, 0)
		{
			info = NULL;

			BRect r = Bounds();

			r = Bounds();
			r.bottom = 85;
			headerView = new FileSharingHeaderView(r);
			AddChild(headerView);

			CLVContainerView *containerView;
			r = Bounds();
			r.top = 85;
			r.bottom = 325;

			MyColumnListView = new SmartColumnListView(r, &containerView, NULL, B_FOLLOW_ALL_SIDES,
				B_WILL_DRAW | B_FRAME_EVENTS | B_NAVIGABLE, B_SINGLE_SELECTION_LIST,
				false, false, false, false, B_NO_BORDER);

			MyColumnListView->AddColumn(new CLVColumn(NULL, 20.0, CLV_LOCK_AT_BEGINNING | CLV_NOT_MOVABLE |
				CLV_NOT_RESIZABLE | CLV_PUSH_PASS | CLV_MERGE_WITH_RIGHT));
			MyColumnListView->AddColumn(new CLVColumn("Share Name", 130.0, CLV_SORT_KEYABLE, 50.0));
			MyColumnListView->AddColumn(new CLVColumn("Access", 75.0, CLV_SORT_KEYABLE));
			MyColumnListView->AddColumn(new CLVColumn("Path", 250.0, CLV_SORT_KEYABLE));

			MyColumnListView->SetSelectionMessage(new BMessage(MSG_SHARE_SELECT));
			MyColumnListView->SetInvocationMessage(new BMessage(MSG_SHARE_INVOKE));

			initShares();
			AddCLVItems(MyColumnListView);
			AddChild(containerView);

			r = Bounds();
			r.top = 325;
			footerView = new FileSharingFooterView(r);
			AddChild(footerView);

			SetSizeLimits(425, 2000, 250, 2000);

			Show();
		}

		// ~FileSharingWindow()
		//
		~FileSharingWindow()
		{
			be_app->PostMessage(B_QUIT_REQUESTED);
		}

		// AddCLVItems()
		//
		void AddCLVItems(ColumnListView* MyColumnListView)
		{
			struct stat st;
			int i;

			for (i = 0; i < BT_MAX_FILE_SHARES; i++)
				if (fileShares[i].used)
					MyColumnListView->AddItem(new FileShareItem(fileShares[i].name,
						fileShares[i].readOnly ? "Read-only" : "Read-Write",
						stat(fileShares[i].path, &st) == 0 ? fileShares[i].path : "--- Invalid Path ---"));
		}

		// MessageReceived()
		//
		void MessageReceived(BMessage *msg)
		{
			BRect frame;
			bt_fileShare_t *share;
			struct stat st;
			int32 curItem;

			switch (msg->what)
			{
				case MSG_EDIT_SHARE:
				case MSG_SHARE_INVOKE:
					curItem = MyColumnListView->CurrentSelection();
					if (curItem >= 0)
					{
						FileShareItem *item = (FileShareItem *) MyColumnListView->ItemAt(curItem);
						if (item)
						{
							int index = getShareId((char *) item->GetColumnContentText(1));
							if (index >= 0)
							{
								fileShares[index].index = curItem;
								frame = Frame();
								frame.left += 100;
								frame.top += 100;
								frame.right = frame.left + 410;
								frame.bottom = frame.top + 390;
								info = new ShareInfoPanel(frame, &fileShares[index], this);
							}
						}
					}
					break;

				case MSG_SHARE_SELECT:
					headerView->editBtn->SetEnabled(true);
					headerView->removeBtn->SetEnabled(true);
					break;

				case MSG_LIST_DESELECT:
					headerView->editBtn->SetEnabled(false);
					headerView->removeBtn->SetEnabled(false);
					break;

				case MSG_NEW_SHARE:
					curItem = MyColumnListView->CountItems();
					strcpy(fileShares[curItem].name, "Untitled");
					strcpy(fileShares[curItem].path, "/boot/home");
					fileShares[curItem].readOnly = true;
					fileShares[curItem].followLinks = false;
					fileShares[curItem].index = curItem;

					frame = Frame();
					frame.left += 100;
					frame.top += 100;
					frame.right = frame.left + 410;
					frame.bottom = frame.top + 390;
					info = new ShareInfoPanel(frame, &fileShares[curItem], this);
					break;

				case MSG_REMOVE_SHARE:
					curItem = MyColumnListView->CurrentSelection();
					if (curItem >= 0)
					{
						FileShareItem *item = (FileShareItem *) MyColumnListView->ItemAt(curItem);
						if (item)
						{
							curItem = getShareId((char *) item->GetColumnContentText(1));
							if (curItem >= 0)
							{
								BAlert *alert = new BAlert(fileShares[curItem].name, WARN_REMOVE_SHARE, "Yes", "No");
								alert->SetShortcut(1, B_ESCAPE);
								if (alert->Go() == 0)
								{
									fileShares[curItem].used = false;
									MyColumnListView->LockLooper();
									MyColumnListView->RemoveItems(0, MyColumnListView->CountItems());
									AddCLVItems(MyColumnListView);
									MyColumnListView->UnlockLooper();
								}
							}
						}
					}
					break;

				case MSG_SHARE_REFRESH:
					msg->FindPointer("share", (void **) &share);
					if (share)
						if (share->index >= MyColumnListView->CountItems())
						{
							share->used = true;
							MyColumnListView->LockLooper();
							MyColumnListView->AddItem(new FileShareItem(share->name,
								share->readOnly ? "Read-only" : "Read-Write",
								stat(share->path, &st) == 0 ? share->path : "--- Invalid Path ---"));
							MyColumnListView->UnlockLooper();
						}
						else
							refreshShare(share);

					break;

				case MSG_EDIT_SECURITY:
					frame = Frame();
					frame.left += 100;
					frame.top += 30;
					frame.right = frame.left + 370;
					frame.bottom = frame.top + 145;
					new SecurityPanel(frame);
					break;

				case MSG_SHARE_OK:
					writeShares();
					footerView->SetDeployable(true);
					break;

				case MSG_SHARE_DEPLOY:
					footerView->SetDeployable(false);
					deployServer();
					break;
					
				case MSG_SHARE_CANCEL:
					Quit();
					break;

				default:
					BWindow::MessageReceived(msg);
					break;
			}
		}

		// Quit()
		//
		void Quit()
		{
			FileShareItem *item;

			do
			{
				item = (FileShareItem *) MyColumnListView->RemoveItem(int32(0));
				if (item)
					delete item;
			} while(item);

			BWindow::Quit();
		}

		// refreshShare()
		//
		void refreshShare(bt_fileShare_t *share)
		{
			struct stat st;
			int curItem = share->index;
			FileShareItem *item = (FileShareItem *) MyColumnListView->ItemAt(curItem);
			if (item)
			{
				int index = getShareId((char *) item->GetColumnContentText(1));
				if (index >= 0)
				{
					MyColumnListView->LockLooper();
	
					item->SetColumnContent(1, fileShares[index].name);
					item->SetColumnContent(2, fileShares[index].readOnly ? "Read-only" : "Read-Write");
					item->SetColumnContent(3, stat(fileShares[index].path, &st) == 0
						? fileShares[index].path
						: "--- Invalid Path ---");
	
					MyColumnListView->InvalidateItem(index);
					MyColumnListView->UnlockLooper();
				}	
			}
		}

		// setPath()
		//
		void setPath(const char *path)
		{
			if (info)
				info->setPath(path);
		}

	private:
		// initShares()
		//
		void initShares()
		{
			FILE *fp;
			char path[B_PATH_NAME_LENGTH], buffer[512];
			int i, length;

			for (i = 0; i < BT_MAX_FILE_SHARES; i++)
			{
				fileShares[i].name[0] = 0;
				fileShares[i].path[0] = 0;
				fileShares[i].used = false;
				fileShares[i].readOnly = true;
				fileShares[i].followLinks = false;
				fileShares[i].index = i;
				fileShares[i].next = NULL;
			}

			find_directory(B_COMMON_SETTINGS_DIRECTORY, 0, false, path, sizeof(path));
			strcat(path, "/BeServed-Settings");

			fp = fopen(path, "r");
			if (fp)
			{
				while (fgets(buffer, sizeof(buffer) - 1, fp))
				{
					length = strlen(buffer);
					if (length <= 1 || buffer[0] == '#')
						continue;

					if (buffer[length - 1] == '\n')
						buffer[--length] = 0;

					if (strncmp(buffer, "share ", 6) == 0)
						getFileShare(buffer);
					else if (strncmp(buffer, "set ", 4) == 0)
						getShareProperty(buffer);
					else if (strncmp(buffer, "grant ", 6) == 0)
						getGrant(buffer);
					else if (strncmp(buffer, "authenticate ", 13) == 0)
						getAuthenticate(buffer);
				}

				fclose(fp);
			}
		}

		// getFileShare()
		//
		void getFileShare(const char *buffer)
		{
			char path[B_PATH_NAME_LENGTH], share[MAX_NAME_LENGTH + 1], *folder;
			int i, tok;

			// Skip over SHARE command.
			tokPtr = (char *) buffer + (6 * sizeof(char));

			tok = getToken();
			if (tok != BT_TOKEN_STRING)
				return;

			strcpy(path, tokBuffer);
			tok = getToken();
			if (tok != BT_TOKEN_AS)
				return;

			tok = getToken();
			if (tok != BT_TOKEN_STRING)
				return;

			strcpy(share, tokBuffer);

			// Now verify that the share name specified has not already been
			// used to share another path.
			folder = getSharePath(share);
			if (folder)
				return;

			for (i = 0; i < BT_MAX_FILE_SHARES; i++)
				if (!fileShares[i].used)
				{
					strcpy(fileShares[i].name, share);
					strcpy(fileShares[i].path, path);
					fileShares[i].used = true;
					return;
				}
		}

		void getShareProperty(const char *buffer)
		{
			char share[B_FILE_NAME_LENGTH + 1];
			int tok, shareId;

			// Skip over SET command.
			tokPtr = (char *) buffer + (4 * sizeof(char));

			tok = getToken();
			if (tok != BT_TOKEN_STRING)
				return;

			strcpy(share, tokBuffer);

			// Get the index of the share referred to.  If the named share cannot be
			// found, then abort.
			shareId = getShareId(share);
			if (shareId < 0)
				return;

			tok = getToken();
			if (tok == BT_TOKEN_READWRITE)
				fileShares[shareId].readOnly = false;
		}

		void getGrant(const char *buffer)
		{
			char share[MAX_NAME_LENGTH + 1];
			int tok, rights;
			bool isGroup = false;

			// Skip over GRANT command.
			tokPtr = (char *) buffer + (6 * sizeof(char));
			rights = 0;

			do
			{
				tok = getToken();
				if (tok == BT_TOKEN_READ)
				{
					rights |= BT_RIGHTS_READ;
					tok = getToken();
				}
				else if (tok == BT_TOKEN_WRITE)
				{
					rights |= BT_RIGHTS_WRITE;
					tok = getToken();
				}
			} while (tok == BT_TOKEN_COMMA);

			if (tok != BT_TOKEN_ON)
				return;

			tok = getToken();
			if (tok != BT_TOKEN_STRING)
				return;

			strcpy(share, tokBuffer);
			tok = getToken();
			if (tok != BT_TOKEN_TO)
				return;

			tok = getToken();
			if (tok == BT_TOKEN_GROUP)
			{
				isGroup = true;
				tok = getToken();
			}

			if (tok != BT_TOKEN_STRING)
				return;

			addUserRights(share, tokBuffer, rights, isGroup);
		}

		void getAuthenticate(const char *buffer)
		{
			struct hostent *ent;
			int i, tok;

			// Skip over AUTHENTICATE command.
			tokPtr = (char *) buffer + (13 * sizeof(char));

			tok = getToken();
			if (tok != BT_TOKEN_WITH)
				return;

			tok = getToken();
			if (tok != BT_TOKEN_STRING)
				return;

			// Look up address for given host.
			strcpy(authServerName, tokBuffer);
			ent = gethostbyname(tokBuffer);
			if (ent != NULL)
				authServerIP = ntohl(*((unsigned int *) ent->h_addr));
			else
				authServerIP = 0;

			// Make all file shares use BeSure authentication.
			for (i = 0; i < BT_MAX_FILE_SHARES; i++)
				fileShares[i].security = BT_AUTH_BESURE;
		}

		void addUserRights(char *share, char *user, int rights, bool isGroup)
		{
			bt_user_rights *ur;
			int shareId;

			shareId = getShareId(share);
			if (shareId < 0)
				return;

			ur = (bt_user_rights *) malloc(sizeof(bt_user_rights));
			if (ur)
			{
				ur->user = (char *) malloc(strlen(user) + 1);
				if (ur->user)
				{
					strcpy(ur->user, user);
					ur->rights = rights;
					ur->isGroup = isGroup;
					ur->next = fileShares[shareId].rights;
					fileShares[shareId].rights = ur;
				}
				else
					free(ur);
			}
		}

		char *getSharePath(char *shareName)
		{
			int i;

			for (i = 0; i < BT_MAX_FILE_SHARES; i++)
				if (fileShares[i].used)
					if (strcasecmp(fileShares[i].name, shareName) == 0)
						return fileShares[i].path;

			return NULL;
		}

		int getToken()
		{
			bool quoted = false;

			tokBuffer[0] = 0;
			while (*tokPtr && iswhite(*tokPtr))
				tokPtr++;

			if (*tokPtr == ',')
			{
				*tokPtr++;
				return BT_TOKEN_COMMA;
			}
			else if (*tokPtr == '\"')
			{
				quoted = true;
				tokPtr++;
			}

			if (isalnum(*tokPtr) || *tokPtr == '/')
			{
				int i = 0;
				while (isalnum(*tokPtr) || isValid(*tokPtr) || (quoted && *tokPtr == ' '))
					if (i < B_PATH_NAME_LENGTH)
						tokBuffer[i++] = *tokPtr++;
					else
						tokPtr++;

				tokBuffer[i] = 0;

				if (!quoted)
					for (i = 0; keywords[i]; i++)
						if (strcasecmp(tokBuffer, keywords[i]) == 0)
							return ++i;

				if (quoted)
					if (*tokPtr != '\"')
						return BT_TOKEN_ERROR;
					else
						tokPtr++;

				return BT_TOKEN_STRING;
			}

			return BT_TOKEN_ERROR;
		}

		int getShareId(char *shareName)
		{
			int i;

			for (i = 0; i < BT_MAX_FILE_SHARES; i++)
				if (fileShares[i].used)
					if (strcasecmp(fileShares[i].name, shareName) == 0)
						return i;

			return -1;
		}

		// writeShares()
		//
		void writeShares()
		{
			FILE *fp;
			bt_user_rights *ur;
			char path[B_PATH_NAME_LENGTH + 1];
			int i;

			find_directory(B_COMMON_SETTINGS_DIRECTORY, 0, false, path, sizeof(path));
			strcat(path, "/BeServed-Settings");

			fp = fopen(path, "w");
			if (fp)
			{
				if (authServerIP)
					fprintf(fp, "authenticate with %s\n", authServerName);

				for (i = 0; i < BT_MAX_FILE_SHARES; i++)
					if (fileShares[i].used)
					{
						fprintf(fp, "share ");
						printString(fp, fileShares[i].path);
						fprintf(fp, " as ");
						printString(fp, fileShares[i].name);
						fputc('\n', fp);

						if (!fileShares[i].readOnly)
						{
							fprintf(fp, "set ");
							printString(fp, fileShares[i].name);
							fprintf(fp, " read-write\n");
						}

						if (fileShares[i].followLinks)
						{
							fprintf(fp, "set ");
							printString(fp, fileShares[i].name);
							fprintf(fp, " promiscuous\n");
						}

						for (ur = fileShares[i].rights; ur; ur = ur->next)
						{
							int rights = ur->rights;
							fprintf(fp, "grant ");

							do
							{
								if (rights & BT_RIGHTS_READ)
								{
									fprintf(fp, "read");
									rights &= ~BT_RIGHTS_READ;
								}
								else if (rights & BT_RIGHTS_WRITE)
								{
									fprintf(fp, "write");
									rights &= ~BT_RIGHTS_WRITE;
								}

								fprintf(fp, "%c", rights > 0 ? ',' : ' ');
							} while (rights > 0);

							fprintf(fp, "on ");
							printString(fp, fileShares[i].name);
							fprintf(fp, " to ");
							if (ur->isGroup)
								fprintf(fp, "group ");
							fprintf(fp, "%s\n", ur->user);
						}
					}

				fclose(fp);
			}
		}

		// deployServer()
		//
		void deployServer()
		{
			thread_id serverId;

			BRect frame;
			frame = Frame();
			frame.left += (frame.right - frame.left) / 2 - 100;
			frame.top += (frame.bottom - frame.top) / 2 - 40;
			frame.right = frame.left + 200;
			frame.bottom = frame.top + 80;
			MilestonePanel *statusWin = new MilestonePanel(frame, "Deploying...");
			statusWin->setText("Connecting to the server", false);

			// Find and restart the server.
			serverId = find_thread("beserved_server");
			if (serverId != B_NAME_NOT_FOUND)
			{
				statusWin->setText("Updating configuration", false);
				kill(serverId, SIGHUP);
				snooze(500000);
			}

			statusWin->setText("File sharing deployed", true);
		}

		FileSharingHeaderView *headerView;
		FileSharingFooterView *footerView;
		ColumnListView *MyColumnListView;
		ShareInfoPanel *info;
};


// ----- Application -------------------------------------------------------------------------

class FileSharingApp : public BApplication
{
	public:
		FileSharingApp() :
		  BApplication("application/x-vnd.Teldar-FileSharing")
		{
			win = new FileSharingWindow;
		}

		~FileSharingApp()
		{
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
					win->setPath(path.Path());
					break;

				default:
					BApplication::RefsReceived(msg);
					break;
			}
		}

	private:
		void checkMimeTypes()
		{
			BMimeType mime;
			mime.SetTo("application/x-vnd.Teldar-FileSharing");
			mime.Delete();
			if (!mime.IsInstalled())
			{
				mime.Install();
				mime.SetShortDescription("Network File Server");
				mime.SetLongDescription("A network server running BeServed");
//				setMimeIcon(&mime, MYNET_ICON_HOST_LARGE, B_LARGE_ICON);
//				setMimeIcon(&mime, MYNET_ICON_HOST_SMALL, B_MINI_ICON);
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

		FileSharingWindow *win;
};

// main()
//
int main()
{
	new FileSharingApp;
	be_app->Run();
	delete be_app;
}

// printString()
//
void printString(FILE *fp, const char *str)
{
	if (strchr(str, ' '))
		fprintf(fp, "\"%s\"", str);
	else
		fprintf(fp, "%s", str);
}
