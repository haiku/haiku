/* Account - provides an "account" view on the mail chains
**
** Copyright 2001 Dr. Zoidberg Enterprises. All rights reserved.
*/


#include "Account.h"
#include "ConfigViews.h"
#include "CenterContainer.h"

#include <ListView.h>
#include <ListItem.h>
#include <TextControl.h>
#include <MenuField.h>
#include <PopUpMenu.h>
#include <MenuItem.h>
#include <Box.h>
#include <Alert.h>
#include <List.h>
#include <String.h>
#include <FindDirectory.h>
#include <Entry.h>
#include <Path.h>

#include <MailSettings.h>
#include <stdio.h>

#include <MDRLanguage.h>

static BList gAccounts;
static BListView *gListView;
static BView *gConfigView;

const char *kInboundFilterAddOnPath = "mail_daemon/inbound_filters";
const char *kOutboundFilterAddOnPath = "mail_daemon/outbound_filters";
const char *kSystemFilterAddOnPath = "mail_daemon/system_filters";
const char *kInboundProtocolAddOnPath = "mail_daemon/inbound_protocols";
const char *kOutboundProtocolAddOnPath = "mail_daemon/outbound_protocols";


//---------------------------------------------------------------------------------------
//	#pragma mark -


AccountItem::AccountItem(const char *label,Account *account,int32 type)
	:	BStringItem(label),
		account(account),
		type(type)
{
}


AccountItem::~AccountItem()
{
}


void AccountItem::Update(BView *owner, const BFont *font)
{
	if (type == ACCOUNT_ITEM)
		font = be_bold_font;

	BStringItem::Update(owner,font);
}


void AccountItem::DrawItem(BView *owner, BRect rect, bool complete)
{
	owner->PushState();
	if (type == ACCOUNT_ITEM)
	{
//		BFont font;
//		owner->GetFont(&font);
//		font.SetFace(B_BOLD_FACE);
		owner->SetFont(be_bold_font); //&font);
	}
	BStringItem::DrawItem(owner,rect,complete);
	owner->PopState();
}


//---------------------------------------------------------------------------------------
//	#pragma mark -


Account::Account(BMailChain *inbound,BMailChain *outbound)
	:	fInbound(inbound),
		fOutbound(outbound),

		fAccountItem(NULL),
		fInboundItem(NULL),
		fOutboundItem(NULL),
		fFilterItem(NULL)
{
	fSettings = fInbound ? fInbound : fOutbound;

	BString label;
	if (fSettings)
		label << fSettings->Name();
	else
		label << MDR_DIALECT_CHOICE ("Unnamed","名称未定");
	fAccountItem = new AccountItem(label.String(),this,ACCOUNT_ITEM);

	fInboundItem = new AccountItem(MDR_DIALECT_CHOICE ("   · Incoming","   - 受信"),this,INBOUND_ITEM);
	fOutboundItem = new AccountItem(MDR_DIALECT_CHOICE ("   · Outgoing","   - 送信"),this,OUTBOUND_ITEM);
	fFilterItem = new AccountItem(MDR_DIALECT_CHOICE ("   · E-mail Filters","   - フィルタ"),this,FILTER_ITEM);
}


Account::~Account()
{
	if (gListView)
	{
		gListView->RemoveItem(fAccountItem);
		gListView->RemoveItem(fInboundItem);
		gListView->RemoveItem(fOutboundItem);
		gListView->RemoveItem(fFilterItem);
	}
	delete fAccountItem;  delete fFilterItem;
	delete fInboundItem;  delete fOutboundItem;

	delete fInbound;
	delete fOutbound;
}


void Account::AddToListView()
{
	if (!gListView)
		return;

	gListView->AddItem(fAccountItem);

	if (fInbound)
		gListView->AddItem(fInboundItem);

	if (fOutbound)
		gListView->AddItem(fOutboundItem);

	if (fOutbound || fInbound)
		gListView->AddItem(fFilterItem);
}


void Account::SetName(const char *name)
{
	if (fInbound)
		fInbound->SetName(name);
	if (fOutbound)
		fOutbound->SetName(name);

	if (name && *name)
	{
		fAccountItem->SetText(name);
		gListView->InvalidateItem(gListView->IndexOf(fAccountItem));
	}
}


const char *Account::Name() const
{
	if (fInbound)
		return fInbound->Name();
	if (fOutbound)
		return fOutbound->Name();

	return NULL;
}


void Account::SetRealName(const char *realName)
{
	BMessage *msg;
	if (fInbound && (msg = fInbound->MetaData()) != NULL)
	{
		if (msg->ReplaceString("real_name",realName) < B_OK)
			msg->AddString("real_name",realName);
	}
	if (fOutbound && (msg = fOutbound->MetaData()) != NULL)
	{
		if (msg->ReplaceString("real_name",realName) < B_OK)
			msg->AddString("real_name",realName);
	}
}


const char *Account::RealName() const
{
	if (fInbound && fInbound->MetaData())
		return fInbound->MetaData()->FindString("real_name");
	if (fOutbound && fOutbound->MetaData())
		return fOutbound->MetaData()->FindString("real_name");

	if (fInbound)
		fInbound->MetaData()->PrintToStream();

	return NULL;
}


void Account::SetReturnAddress(const char *returnAddress)
{
	BMessage *msg;
	if (fInbound && (msg = fInbound->MetaData()) != NULL)
	{
		if (msg->ReplaceString("reply_to",returnAddress) < B_OK)
			msg->AddString("reply_to",returnAddress);
	}
	if (fOutbound && (msg = fOutbound->MetaData()) != NULL)
	{
		if (msg->ReplaceString("reply_to",returnAddress) < B_OK)
			msg->AddString("reply_to",returnAddress);
	}
}


const char *Account::ReturnAddress() const
{
	if (fInbound && fInbound->MetaData())
		return fInbound->MetaData()->FindString("reply_to");
	if (fOutbound && fOutbound->MetaData())
		return fOutbound->MetaData()->FindString("reply_to");

	return NULL;
}


void Account::CopyMetaData(BMailChain *targetChain, BMailChain *sourceChain)
{
	BMessage *otherMsg, *thisMsg;
	if (sourceChain && (otherMsg = sourceChain->MetaData()) != NULL
					&& (thisMsg = targetChain->MetaData()) != NULL)
	{
		const char *string;
		if ((string = otherMsg->FindString("real_name")) != NULL)
		{
			if (thisMsg->ReplaceString("real_name",string) < B_OK)
				thisMsg->AddString("real_name",string);
		}
		if ((string = otherMsg->FindString("reply_to")) != NULL)
		{
			if (thisMsg->ReplaceString("reply_to",string) < B_OK)
				thisMsg->AddString("reply_to",string);
		}
		if ((string = sourceChain->Name()) != NULL)
			targetChain->SetName(string);
	}
}


void Account::CreateInbound()
{

	if (!(fInbound = NewMailChain()))
	{
		(new BAlert(
			MDR_DIALECT_CHOICE ("E-mail","メール"),
			MDR_DIALECT_CHOICE ("Could not create inbound chain.","受信チェーンは作成できませんでした。"),
			MDR_DIALECT_CHOICE ("OK","了解")))->Go();
		return;
	}
	fInbound->SetChainDirection(inbound);

	BPath path,addOnPath;
	find_directory(B_USER_ADDONS_DIRECTORY,&addOnPath);

	BMessage msg;
	entry_ref ref;

	// Protocol
	path = addOnPath;
	path.Append(kInboundProtocolAddOnPath);
	path.Append("POP3");
	if (!BEntry(path.Path()).Exists()) {
		find_directory(B_BEOS_ADDONS_DIRECTORY,&path);
		path.Append(kInboundProtocolAddOnPath);
		path.Append("POP3");
	}
	BEntry(path.Path()).GetRef(&ref);
	fInbound->AddFilter(msg,ref);

	// Message Parser	
	path = addOnPath;
	path.Append(kSystemFilterAddOnPath);
	path.Append("Message Parser");
	if (!BEntry(path.Path()).Exists()) {
		find_directory(B_BEOS_ADDONS_DIRECTORY,&path);
		path.Append(kSystemFilterAddOnPath);
		path.Append("Message Parser");
	}
	BEntry(path.Path()).GetRef(&ref);
	fInbound->AddFilter(msg,ref);

	// New Mail Notification
	path = addOnPath;
	path.Append(kSystemFilterAddOnPath);
	path.Append(MDR_DIALECT_CHOICE ("New Mail Notification", "着信通知方法"));
	if (!BEntry(path.Path()).Exists()) {
		find_directory(B_BEOS_ADDONS_DIRECTORY,&path);
		path.Append(kSystemFilterAddOnPath);
		path.Append(MDR_DIALECT_CHOICE ("New Mail Notification", "着信通知方法"));
	}
	BEntry(path.Path()).GetRef(&ref);
	fInbound->AddFilter(msg,ref);

	// Inbox
	path = addOnPath;
	path.Append(kSystemFilterAddOnPath);
	path.Append(MDR_DIALECT_CHOICE ("Inbox", "受信箱"));
	if (!BEntry(path.Path()).Exists()) {
		find_directory(B_BEOS_ADDONS_DIRECTORY,&path);
		path.Append(kSystemFilterAddOnPath);
		path.Append(MDR_DIALECT_CHOICE ("Inbox", "受信箱"));
	}
	BEntry(path.Path()).GetRef(&ref);
	fInbound->AddFilter(msg,ref);

	// set already made account settings
	CopyMetaData(fInbound,fOutbound);
}


void Account::CreateOutbound()
{

	if (!(fOutbound = NewMailChain()))
	{
		(new BAlert(
			MDR_DIALECT_CHOICE ("E-mail","メール"),
			MDR_DIALECT_CHOICE ("Could not create outbound chain.","送信チェーンは作成できませんでした。"),
			MDR_DIALECT_CHOICE ("Ok","了解")))->Go();
		return;
	}
	fOutbound->SetChainDirection(outbound);

	BPath path,addOnPath;
	find_directory(B_USER_ADDONS_DIRECTORY,&addOnPath);

	BMessage msg;
	entry_ref ref;
		
	path = addOnPath;
	path.Append(kSystemFilterAddOnPath);
	path.Append(MDR_DIALECT_CHOICE ("Outbox", "送信箱"));
	if (!BEntry(path.Path()).Exists()) {
		find_directory(B_BEOS_ADDONS_DIRECTORY,&path);
		path.Append(kSystemFilterAddOnPath);
		path.Append(MDR_DIALECT_CHOICE ("Outbox", "送信箱"));
	}
	BEntry(path.Path()).GetRef(&ref);
	fOutbound->AddFilter(msg,ref);

	path = addOnPath;
	path.Append(kOutboundProtocolAddOnPath);
	path.Append("SMTP");
	if (!BEntry(path.Path()).Exists()) {
		find_directory(B_BEOS_ADDONS_DIRECTORY,&path);
		path.Append(kOutboundProtocolAddOnPath);
		path.Append("SMTP");
	}
	BEntry(path.Path()).GetRef(&ref);
	fOutbound->AddFilter(msg,ref);

	// set already made account settings
	CopyMetaData(fOutbound,fInbound);
}


void Account::SetType(int32 type)
{
	if (type < INBOUND_TYPE || type > IN_AND_OUTBOUND_TYPE)
		return;

	int32 index = gListView->IndexOf(fAccountItem) + 1;

	// missing inbound
	if ((type == INBOUND_TYPE || type == IN_AND_OUTBOUND_TYPE) && !Inbound())
	{
		if (!fInbound)
			CreateInbound();

		if (fInbound)
			gListView->AddItem(fInboundItem,index);
	}
	if (Inbound())
		index++;

	// missing outbound
	if ((type == OUTBOUND_TYPE || type == IN_AND_OUTBOUND_TYPE) && !Outbound())
	{
		if (!fOutbound)
			CreateOutbound();

		if (fOutbound)
			gListView->AddItem(fOutboundItem,index);
	}
	if (Outbound())
		index++;

	// missing filter
	if (!gListView->HasItem(fFilterItem))
		gListView->AddItem(fFilterItem,index);

	// remove inbound
	if (type == OUTBOUND_TYPE && Inbound())
		gListView->RemoveItem(fInboundItem);

	// remove outbound
	if (type == INBOUND_TYPE && Outbound())
		gListView->RemoveItem(fOutboundItem);
}


int32 Account::Type() const
{
	return Inbound() ? (Outbound() ? 2 : 0) : (Outbound() ? 1 : -1);
}


void Account::Selected(int32 type)
{
	if (!gConfigView)
		return;
	
	gConfigView->Hide();
	((CenterContainer *)gConfigView)->DeleteChildren();

	switch (type)
	{
		case ACCOUNT_ITEM:
			gConfigView->AddChild(new AccountConfigView(gConfigView->Bounds(),this));
			break;
		case INBOUND_ITEM:
		{
			if (!fInbound)
				break;

			int32 count = fInbound->CountFilters();
			for (int32 i = 0;;i++)
			{
				BMessage *msg = new BMessage();
				entry_ref *ref = new entry_ref;

				// we just want to have the first and the last two filters:
				// Protocol, Parser, Notifier, Folder
				if (i == 2)
				{
					i = count - 2;
					if (i < 2)	// defensive programming...
						i = 3;
				}

				if (fInbound->GetFilter(i,msg,ref) < B_OK)
				{
					delete msg;
					delete ref;
					break;
				}

				// the filter view takes ownership of "msg" and "ref"
				FilterConfigView *view;
				if (i == 0)
					view = new ProtocolsConfigView(fInbound,i,msg,ref);
				else
					view = new FilterConfigView(fInbound,i,msg,ref);

				if (view->InitCheck() >= B_OK)
					gConfigView->AddChild(view);
				else
					delete view;
			}
			break;
		}
		case OUTBOUND_ITEM:
		{
			if (!fOutbound)
				break;

			// we just want to have the first and the last filter here
			int32 count = fOutbound->CountFilters();
			for (int32 i = 0;i < count;i += count-1)
			{
				BMessage *msg = new BMessage();
				entry_ref *ref = new entry_ref;

				if (fOutbound->GetFilter(i,msg,ref) < B_OK)
				{
					delete msg;
					delete ref;
					break;
				}

				// the filter view takes ownership of "msg" and "ref"
				if (FilterConfigView *view = new FilterConfigView(fOutbound,i,msg,ref))
				{
					if (view->InitCheck() >= B_OK)
						gConfigView->AddChild(view);
					else
						delete view;
				}
			}
			break;
		}
		case FILTER_ITEM:
		{
			gConfigView->AddChild(new FiltersConfigView(gConfigView->Bounds(),this));
			break;
		}
	}
	((CenterContainer *)gConfigView)->Layout();
	gConfigView->Show();
}


void Account::Remove(int32 type)
{
	// this should only be called if necessary, but if it's used
	// in the GUI, this will always be the case
	((CenterContainer *)gConfigView)->DeleteChildren();

	switch (type)
	{
		case ACCOUNT_ITEM:
			gListView->RemoveItem(fAccountItem);
			gListView->RemoveItem(fInboundItem);
			gListView->RemoveItem(fOutboundItem);
			gListView->RemoveItem(fFilterItem);
			return;
		case INBOUND_ITEM:
			if (!fInbound || !gListView)
				return;

			gListView->RemoveItem(fInboundItem);
			if (!Outbound())
				gListView->RemoveItem(fFilterItem);
			break;
		case OUTBOUND_ITEM:
			if (!fOutbound || !gListView)
				return;

			gListView->RemoveItem(fOutboundItem);
			if (!Inbound())
				gListView->RemoveItem(fFilterItem);
			break;
	}
}


BMailChain *Account::Inbound() const
{
	return gListView && gListView->HasItem(fInboundItem) ? fInbound : NULL;
}


BMailChain *Account::Outbound() const
{
	return gListView && gListView->HasItem(fOutboundItem) ? fOutbound : NULL;
}


void Account::Save()
{
	if (Inbound())
		fInbound->Save();
	else
		Delete(INBOUND_TYPE);

	if (Outbound())
		fOutbound->Save();
	else
		Delete(OUTBOUND_TYPE);
}


void Account::Delete(int32 type)
{
	if (fInbound && (type == INBOUND_TYPE || type == IN_AND_OUTBOUND_TYPE))
		fInbound->Delete();

	if (fOutbound && (type == OUTBOUND_TYPE || type == IN_AND_OUTBOUND_TYPE))
		fOutbound->Delete();
}


//	#pragma mark -


int Accounts::Compare(const void *_a, const void *_b)
{
	const char *a = (*(Account **)_a)->Name();
	const char *b = (*(Account **)_b)->Name();

	if (!a)
		return b != 0;

	return strcasecmp(a,b);
}


void Accounts::Create(BListView *listView, BView *configView)
{
	gListView = listView;
	gConfigView = configView;

	BList inbound,outbound;

	GetInboundMailChains(&inbound);
	GetOutboundMailChains(&outbound);

	// create inbound accounts and assign matching outbound chains

	for (int32 i = inbound.CountItems();i-- > 0;)
	{
		BMailChain *inChain = (BMailChain *)inbound.ItemAt(i);
		BMailChain *outChain = NULL;
		for (int32 j = outbound.CountItems();j-- > 0;)
		{
			outChain = (BMailChain *)outbound.ItemAt(j);

			if (!strcmp(inChain->Name(),outChain->Name()))
				break;
			outChain = NULL;
		}
		gAccounts.AddItem(new Account(inChain,outChain));
		inbound.RemoveItem(i);
		if (outChain)
			outbound.RemoveItem(outChain);
	}

	// create remaining outbound only accounts

	for (int32 i = outbound.CountItems();i-- > 0;)
	{
		BMailChain *outChain = (BMailChain *)outbound.ItemAt(i);

		gAccounts.AddItem(new Account(NULL,outChain));
		outbound.RemoveItem(i);
	}

	// sort the list alphabetically
	gAccounts.SortItems(Accounts::Compare);
	
	for (int32 i = 0;Account *account = (Account *)gAccounts.ItemAt(i);i++)
		account->AddToListView();
}


void Accounts::NewAccount()
{
	Account *account = new Account();
	gAccounts.AddItem(account);
	account->AddToListView();
	
	if (gListView)
		gListView->Select(gListView->IndexOf(account->fAccountItem));
}


void Accounts::Save()
{
	for (int32 i = gAccounts.CountItems();i-- > 0;)
		((Account *)gAccounts.ItemAt(i))->Save();
}


void Accounts::Delete()
{
	for (int32 i = gAccounts.CountItems();i-- > 0;)
	{
		Account *account = (Account *)gAccounts.RemoveItem(i);
		delete account;
	}
}

