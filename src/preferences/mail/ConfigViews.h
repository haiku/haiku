#ifndef CONFIG_VIEWS_H
#define CONFIG_VIEWS_H
/* ConfigViews - config views for the account, protocols, and filters
**
** Copyright 2001 Dr. Zoidberg Enterprises. All rights reserved.
*/


#include <interface/Box.h>
#include <kernel/image.h>

class BTextControl;
class BListView;
class BMenuField;
class BButton;

class BMailChain;

class Account;


class AccountConfigView : public BBox
{
	public:
		AccountConfigView(BRect rect,Account *account);

		virtual void DetachedFromWindow();
		virtual void AttachedToWindow();
		virtual void MessageReceived(BMessage *msg);

		void UpdateViews();

	private:
		BTextControl	*fNameControl, *fRealNameControl, *fReturnAddressControl;
		BMenuField		*fTypeField;
		Account			*fAccount;
};


//--------------------------------------------------------------------------------

class ProtocolsConfigView;
class FiltersConfigView;

class FilterConfigView : public BBox
{
	public:
		FilterConfigView(BMailChain *chain,int32 index,BMessage *msg,entry_ref *ref);
		~FilterConfigView();

		status_t InitCheck();

		virtual void DetachedFromWindow();
		virtual void AttachedToWindow();

	protected:
		friend class FiltersConfigView;

		void		Load(BMessage *msg,entry_ref *ref);
		void		Remove(bool deleteMessage = true);

		BView		*fConfigView;
		
		BMailChain *fChain;
		int32		fIndex;
		BMessage	*fMessage;
		entry_ref	*fEntryRef;
		image_id	fImage;
};


//--------------------------------------------------------------------------------


class ProtocolsConfigView : public FilterConfigView
{
	public:
		ProtocolsConfigView(BMailChain *chain, int32 index, BMessage *msg, entry_ref *ref);

		void AttachedToWindow();
		void MessageReceived(BMessage *msg);

	private:
		BMenuField	*fProtocolsMenuField;
};


//--------------------------------------------------------------------------------


class FiltersConfigView : public BBox
{
	public:
		FiltersConfigView(BRect rect,Account *account);
		~FiltersConfigView();

		virtual void	AttachedToWindow();
//		virtual void	DetachedFromWindow();
		virtual void	MessageReceived(BMessage *msg);

	private:
		void			SelectFilter(int32 index);
		void			SetTo(BMailChain *chain);

		Account				*fAccount;
		BMailChain			*fChain;
		int32				fFirst, fLast;

		BMenuField			*fChainsField;
		BListView			*fListView;
		BMenuField			*fAddField;
		BButton				*fRemoveButton;
		FilterConfigView	*fFilterView;
};

#endif	/* CONFIG_VIEWS_H */
