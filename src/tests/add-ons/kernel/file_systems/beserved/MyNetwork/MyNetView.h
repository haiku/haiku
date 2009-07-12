#ifndef _MYNETVIEW_H_
#define _MYNETVIEW_H_

#include <Window.h>
#include <View.h>
#include <ScrollView.h>
#include <Bitmap.h>
#include <String.h>

#include <stdio.h>

#include "IconListItem.h"

class BStatusBar;


class MyNetView : public BView
{
	public:
		MyNetView(BRect rect);
		~MyNetView();

		void AddHost(char *hostName, uint32 address);
		void Refresh();

		IconListView *list;

	private:
		BBitmap *fileIcon;
		BMenuBar *menuBar;
};


class FileShareView : public BView
{
	public:
		FileShareView(BRect rect, const char *name, uint32 ipAddr);
		~FileShareView();

		void AddShare(char *shareName);
		void AddPrinter(char *printerName);

		IconListView *list;
		char host[B_FILE_NAME_LENGTH];
		uint32 address;

	private:
		BBitmap *fileIcon;
		BBitmap *printerIcon;
};


class HostInfoView : public BView
{
	public:
		HostInfoView(BRect rect, const char *name, uint32 ipAddr);
		~HostInfoView();

		void Draw(BRect rect);

		uint32 address;
		bt_hostinfo hostInfo;

	private:
		hostent *ent;
		BBitmap *icon;
		BStatusBar *status;
};


class MyNetHeaderView : public BView
{
	public:
		MyNetHeaderView(BRect rect);
		~MyNetHeaderView();

		void Draw(BRect rect);

		BButton *openBtn;
		BButton *hostBtn;

	private:
		BBitmap *icon;
};


class MyShareOptionView : public BView
{
	public:
		MyShareOptionView(BRect rect);
		~MyShareOptionView();

		void Draw(BRect rect);

		BButton *openBtn;
		BButton *unmountBtn;
};


class ThreadInfo
{
	public:
		ThreadInfo();
		~ThreadInfo();

		ColumnListView *GetColumnListView()			{ return listView; }
		uint32 GetHostAddress()						{ return address; }

		void SetColumnListView(ColumnListView *clv)	{ listView = clv; }
		void SetHostAddress(uint32 ipAddr)			{ address = ipAddr; }

	private:
		ColumnListView *listView;
		uint32 address;
};

const int32 MSG_HOST_SELECT		= 'HSel';
const int32 MSG_HOST_INVOKE		= 'HInv';
const int32 MSG_SHARE_SELECT	= 'SSel';
const int32 MSG_SHARE_INVOKE	= 'SInv';
const int32 MSG_HOST_INFO		= 'HInf';
const int32 MSG_INET_HOSTS		= 'HInt';
const int32 MSG_HOST_OK			= 'HOky';
const int32 MSG_HOST_CANCEL		= 'HCan';
const int32 MSG_HOST_REFRESH	= 'HRef';
const int32 MSG_SHARE_OPEN		= 'SOpn';
const int32 MSG_SHARE_UNMOUNT	= 'SUmt';
const int32 MSG_NEW_MOUNT		= 'NMnt';

#endif
