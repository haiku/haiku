#ifndef ACCOUNT_H
#define ACCOUNT_H
/* Account - provides an "account" view on the mail chains
**
** Copyright 2001 Dr. Zoidberg Enterprises. All rights reserved.
*/


#include <ListItem.h>

class BView;
class BListView;
class BStringItem;
class BMailChain;

class Account;
class Accounts;


enum item_types
{
	ACCOUNT_ITEM = 0,
	INBOUND_ITEM,
	OUTBOUND_ITEM,
	FILTER_ITEM
};

class AccountItem : public BStringItem
{
	public:
		AccountItem(const char *label,Account *account,int32 type);
		~AccountItem();

		virtual void Update(BView *owner,const BFont *font);
		virtual void DrawItem(BView *owner,BRect rect,bool complete);

		Account	*account;
		int32	type;
};


//-------------------------------------------------------------


enum account_types
{
	INBOUND_TYPE = 0,
	OUTBOUND_TYPE,
	IN_AND_OUTBOUND_TYPE
};

class Account
{
	public:
		Account(BMailChain *inbound = NULL,BMailChain *outbound = NULL);
		~Account();

		void		SetName(const char *name);
		const char	*Name() const;

		void		SetRealName(const char *realName);
		const char	*RealName() const;

		void		SetReturnAddress(const char *returnAddress);
		const char	*ReturnAddress() const;

		void		Selected(int32 type);
		void		Remove(int32 type);

		void		SetType(int32 type);
		int32		Type() const;

		BMailChain	*Inbound() const;
		BMailChain	*Outbound() const;

		void		Save();
		void		Delete(int32 type = IN_AND_OUTBOUND_TYPE);

	private:
		friend Accounts;
		void		AddToListView();
	private:
		void		CreateInbound();
		void		CreateOutbound();
		void		CopyMetaData(BMailChain *targetChain,
						BMailChain *sourceChain);

		BMailChain *fSettings, *fInbound, *fOutbound;
		AccountItem	*fAccountItem, *fInboundItem, *fOutboundItem, *fFilterItem;
};

class Accounts
{
	public:
		static void	Create(BListView *listView,BView *configView);
		static void NewAccount();
		static void Save();
		static void	Delete();

	private:
		static int Compare(const void *,const void *);
};

#endif	/* ACCOUNT_H */
