/* ConfigViews - config views for the account, protocols, and filters
**
** Copyright 2001 Dr. Zoidberg Enterprises. All rights reserved.
*/


#include "ConfigViews.h"
#include "Account.h"
#include "CenterContainer.h"

#include <TextControl.h>
#include <ListView.h>
#include <ScrollView.h>
#include <PopUpMenu.h>
#include <MenuField.h>
#include <MenuItem.h>
#include <Button.h>
#include <Bitmap.h>
#include <Looper.h>
#include <Path.h>
#include <Alert.h>
#include <Entry.h>
#include <FindDirectory.h>
#include <Directory.h>

#include <string.h>

#include <MailSettings.h>

#include <MDRLanguage.h>

// AccountConfigView
const uint32 kMsgAccountTypeChanged = 'atch';
const uint32 kMsgAccountNameChanged = 'anmc';

// ProtocolsConfigView
const uint32 kMsgProtocolChanged = 'prch';

// FiltersConfigView
const uint32 kMsgItemDragged = 'itdr';
const uint32 kMsgFilterMoved = 'flmv';
const uint32 kMsgChainSelected = 'chsl';
const uint32 kMsgAddFilter = 'addf';
const uint32 kMsgRemoveFilter = 'rmfi';
const uint32 kMsgFilterSelected = 'fsel';


AccountConfigView::AccountConfigView(BRect rect,Account *account)
	:	BBox(rect),
		fAccount(account)
{
	SetLabel(MDR_DIALECT_CHOICE ("Account Configuration","アカウント設定"));
	BMailChain *settings = account->Inbound() ? account->Inbound() : account->Outbound();

	rect = Bounds().InsetByCopy(8,8);
	rect.top += 10;
	CenterContainer *view = new CenterContainer(rect,false);
	view->SetSpacing(5);

	// determine font height
	font_height fontHeight;
	view->GetFontHeight(&fontHeight);
	int32 height = (int32)(fontHeight.ascent + fontHeight.descent + fontHeight.leading) + 5;

	rect = view->Bounds();
	rect.bottom = height + 5;

	float labelWidth = view->StringWidth(MDR_DIALECT_CHOICE ("Account Name:","アカウント名：")) + 6;

	view->AddChild(fNameControl = new BTextControl(rect,NULL,MDR_DIALECT_CHOICE ("Account Name:","アカウント名："),NULL,new BMessage(kMsgAccountNameChanged)));
	fNameControl->SetDivider(labelWidth);
	view->AddChild(fRealNameControl = new BTextControl(rect,NULL,MDR_DIALECT_CHOICE ("Real Name:","名前　　　　："),NULL,NULL));
	fRealNameControl->SetDivider(labelWidth);
	view->AddChild(fReturnAddressControl = new BTextControl(rect,NULL,MDR_DIALECT_CHOICE ("Return Address:","返信アドレス："),NULL,NULL));
	fReturnAddressControl->SetDivider(labelWidth);
//			control->TextView()->HideTyping(true);

	BPopUpMenu *chainsPopUp = new BPopUpMenu(B_EMPTY_STRING);
	const char *chainModes[] = {
		MDR_DIALECT_CHOICE ("Inbound Only","受信のみ"),
		MDR_DIALECT_CHOICE ("Outbound Only","送信のみ"),
		MDR_DIALECT_CHOICE ("Inbound & Outbound","送受信")};
	BMenuItem *item;
	for (int32 i = 0;i < 3;i++)
		chainsPopUp->AddItem(item = new BMenuItem(chainModes[i],new BMessage(kMsgAccountTypeChanged)));

	fTypeField = new BMenuField(rect,NULL,MDR_DIALECT_CHOICE ("Account Type:","用途　　　　："),chainsPopUp);
	fTypeField->SetDivider(labelWidth + 3);
	view->AddChild(fTypeField);

	float w,h;
	view->GetPreferredSize(&w,&h);
	ResizeTo(w + 15,h + 22);
	view->ResizeTo(w,h);

	AddChild(view);
}


void AccountConfigView::DetachedFromWindow()
{
	fAccount->SetName(fNameControl->Text());
	fAccount->SetRealName(fRealNameControl->Text());
	fAccount->SetReturnAddress(fReturnAddressControl->Text());
}


void AccountConfigView::AttachedToWindow()
{
	UpdateViews();
	fNameControl->SetTarget(this);
	fTypeField->Menu()->SetTargetForItems(this);
}


void AccountConfigView::MessageReceived(BMessage *msg)
{
	switch (msg->what)
	{
		case kMsgAccountTypeChanged:
		{
			int32 index;
			if (msg->FindInt32("index",&index) < B_OK)
				break;

			if (fAccount->Type() < 0)
			{
				fNameControl->SetEnabled(true);
				fRealNameControl->SetEnabled(true);
				fReturnAddressControl->SetEnabled(true);
			}
			fAccount->SetType(index);
			UpdateViews();
			break;
		}
		case kMsgAccountNameChanged:
			fAccount->SetName(fNameControl->Text());
			break;

		default:
			BView::MessageReceived(msg);
	}
}


void AccountConfigView::UpdateViews()
{
	if (!fAccount->Inbound() && !fAccount->Outbound())
	{
		if (BMenuItem *item = fTypeField->Menu()->FindMarked())
			item->SetMarked(false);
		fTypeField->Menu()->Superitem()->SetLabel(MDR_DIALECT_CHOICE ("<select account type>","<用途を選択してください>"));
		
		fNameControl->SetEnabled(false);
		fRealNameControl->SetEnabled(false);
		fReturnAddressControl->SetEnabled(false);
		return;
	}
	fNameControl->SetText(fAccount->Name());
	fRealNameControl->SetText(fAccount->RealName());
	fReturnAddressControl->SetText(fAccount->ReturnAddress());

	if (BMenuItem *item = fTypeField->Menu()->ItemAt(fAccount->Type()))
		item->SetMarked(true);
}


//---------------------------------------------------------------------------------------
//	#pragma mark -

#include <stdio.h>
FilterConfigView::FilterConfigView(BMailChain *chain,int32 index,BMessage *msg,entry_ref *ref)
	:	BBox(BRect(0,0,100,100)),
		fConfigView(NULL),
		fChain(chain),
		fIndex(index),
		fMessage(msg),
		fEntryRef(ref)
{
	Load(msg,ref);
	BPath addon(ref);
	SetLabel(addon.Leaf());
}


FilterConfigView::~FilterConfigView()
{
	Remove();
}


void FilterConfigView::Load(BMessage *msg,entry_ref *ref)
{
	ResizeTo(264,30);

	BView *(* instantiate_config)(BMessage *,BMessage *);
	BPath addon(ref);
	fImage = load_add_on(addon.Path());
	if (fImage < B_OK)
		return;

	if (get_image_symbol(fImage,"instantiate_config_panel",B_SYMBOL_TYPE_TEXT,(void **)&instantiate_config) < B_OK)
	{
		unload_add_on(fImage);
		fImage = B_MISSING_SYMBOL;
		return;
	}

	fConfigView = (*instantiate_config)(msg,fChain->MetaData());

	float w = fConfigView->Bounds().Width();
	float h = fConfigView->Bounds().Height();
	fConfigView->MoveTo(3,13);
	ResizeTo(w + 6,h + 16);
	AddChild(fConfigView);
}


void FilterConfigView::Remove(bool deleteMessage)
{
	// remove config view here, because they may not be available
	// anymore, if the add-on is unloaded
	if (fConfigView && RemoveChild(fConfigView))
	{
		delete fConfigView;
		fConfigView = NULL;
	}
	unload_add_on(fImage);

	if (deleteMessage)
	{
		delete fMessage;
		fMessage = NULL;
	}
	delete fEntryRef;
	fEntryRef = NULL;
}


status_t FilterConfigView::InitCheck()
{
	return fImage;
}


void FilterConfigView::DetachedFromWindow()
{
	if (fConfigView == NULL)
		return;

	if (fConfigView->Archive(fMessage) >= B_OK)
		fChain->SetFilter(fIndex,*fMessage,*fEntryRef);
}


void FilterConfigView::AttachedToWindow()
{
}


//---------------------------------------------------------------------------------------
//	#pragma mark -


ProtocolsConfigView::ProtocolsConfigView(BMailChain *chain,int32 index,BMessage *msg,entry_ref *ref)
	:	FilterConfigView(chain,index,msg,ref)
{
	BPopUpMenu *menu = new BPopUpMenu("<choose protocol>");

	for (int i = 0; i < 2; i++) {
		BPath path;
		status_t status = find_directory((i == 0) ? B_USER_ADDONS_DIRECTORY : B_BEOS_ADDONS_DIRECTORY,&path);
		if (status != B_OK)
		{
			fImage = status;
			return;
		}
	
		path.Append("mail_daemon");
		
		if (chain->ChainDirection() == inbound)
			path.Append("inbound_protocols");
		else
			path.Append("outbound_protocols");
		
		BDirectory dir(path.Path());
		entry_ref protocolRef;
		while (dir.GetNextRef(&protocolRef) == B_OK)
		{
			char name[B_FILE_NAME_LENGTH];
			BEntry entry(&protocolRef);
			entry.GetName(name);
	
			BMenuItem *item;
			BMessage *msg;
			menu->AddItem(item = new BMenuItem(name,msg = new BMessage(kMsgProtocolChanged)));
			msg->AddRef("protocol",&protocolRef);
	
			if (*ref == protocolRef)
				item->SetMarked(true);
		}
	}

	fProtocolsMenuField = new BMenuField(BRect(0,0,200,40),NULL,NULL,menu);
	fProtocolsMenuField->ResizeToPreferred();
	SetLabel(fProtocolsMenuField);

	if (fConfigView)
	{
		fConfigView->MoveTo(3,21);
		ResizeBy(0,8);
	}
	else
		fImage = B_OK;
}


void ProtocolsConfigView::AttachedToWindow()
{
	FilterConfigView::AttachedToWindow();
	fProtocolsMenuField->Menu()->SetTargetForItems(this);
}


void ProtocolsConfigView::MessageReceived(BMessage *msg)
{
	switch (msg->what)
	{
		case kMsgProtocolChanged:
		{
			entry_ref ref;
			if (msg->FindRef("protocol",&ref) < B_OK)
				break;
			
			DetachedFromWindow();
			Remove(false);

			fEntryRef = new entry_ref(ref);
			Load(fMessage,fEntryRef);
			fChain->SetFilter(fIndex,*fMessage,*fEntryRef);

			// resize view
			if (LockLooperWithTimeout(1000000L) == B_OK)
			{
				if (fConfigView)
				{
					fConfigView->MoveTo(3,21);
					ResizeBy(0,8);
				}
				UnlockLooper();
				
				if (CenterContainer *container = dynamic_cast<CenterContainer *>(Parent()))
					container->Layout();
			}
			break;
		}
		default:
			BView::MessageReceived(msg);
			break;
	}
}


//---------------------------------------------------------------------------------------
//	#pragma mark -

#include <stdio.h>
class DragListView : public BListView
{
	public:
		DragListView(BRect frame,const char *name,list_view_type type = B_SINGLE_SELECTION_LIST,
					 uint32 resizingMode = B_FOLLOW_LEFT | B_FOLLOW_TOP,BMessage *itemMovedMsg = NULL)
			: BListView(frame,name,type,resizingMode),
			fDragging(false),
			fItemMovedMessage(itemMovedMsg)
		{
		}
		
		virtual bool InitiateDrag(BPoint point,int32 index,bool wasSelected)
		{
			BRect frame(ItemFrame(index));
			BBitmap *bitmap = new BBitmap(frame.OffsetToCopy(B_ORIGIN),B_RGBA32,true);
			BView *view = new BView(bitmap->Bounds(),NULL,0,0);
			bitmap->AddChild(view);

			if (view->LockLooper())
			{
				BListItem *item = ItemAt(index);
				bool selected = item->IsSelected();

				view->SetLowColor(225,225,225,128);
				view->FillRect(view->Bounds());

				if (selected)
					item->Deselect();
				ItemAt(index)->DrawItem(view,view->Bounds(),true);
				if (selected)
					item->Select();

				view->UnlockLooper();
			}
			fLastDragTarget = -1;
			fDragIndex = index;
			fDragging = true;

			BMessage drag(kMsgItemDragged);
			drag.AddInt32("index",index);
			DragMessage(&drag,bitmap,B_OP_ALPHA,point - frame.LeftTop(),this);

			return true;
		}

		void DrawDragTargetIndicator(int32 target)
		{
			PushState();
			SetDrawingMode(B_OP_INVERT);

			bool last = false;
			if (target >= CountItems())
				target = CountItems() - 1, last = true;

			BRect frame = ItemFrame(target);
			if (last)
				frame.OffsetBy(0,frame.Height());
			frame.bottom = frame.top + 1;

			FillRect(frame);

			PopState();
		}

		virtual void MouseMoved(BPoint point,uint32 transit,const BMessage *msg)
		{
			BListView::MouseMoved(point,transit,msg);

			if ((transit != B_ENTERED_VIEW && transit != B_INSIDE_VIEW) || !fDragging)
				return;

			int32 target = IndexOf(point);
			if (target == -1)
				target = CountItems();

			// correct the target insertion index
			if (target == fDragIndex || target == fDragIndex + 1)
				target = -1;

			if (target == fLastDragTarget)
				return;

			// remove old target indicator
			if (fLastDragTarget != -1)
				DrawDragTargetIndicator(fLastDragTarget);
			
			// draw new one
			fLastDragTarget = target;
			if (target != -1)
				DrawDragTargetIndicator(target);
		}

		virtual void MouseUp(BPoint point)
		{
			if (fDragging)
			{
				fDragging = false;
				if (fLastDragTarget != -1)
					DrawDragTargetIndicator(fLastDragTarget);
			}
			BListView::MouseUp(point);
		}

		virtual void MessageReceived(BMessage *msg)
		{
			switch(msg->what)
			{
				case kMsgItemDragged:
				{
					int32 source = msg->FindInt32("index");
					BPoint point = msg->FindPoint("_drop_point_");
					ConvertFromScreen(&point);
					int32 to = IndexOf(point);
					if (to > fDragIndex)
						to--;
					if (to == -1)
						to = CountItems() - 1;

					if (source != to)
					{
						MoveItem(source,to);
						
						if (fItemMovedMessage != NULL)
						{
							BMessage msg(fItemMovedMessage->what);
							msg.AddInt32("from",source);
							msg.AddInt32("to",to);
							Messenger().SendMessage(&msg);
						}
					}
					break;
				}
			}
			BListView::MessageReceived(msg);
		}
	
	private:
		bool		fDragging;
		int32		fLastDragTarget,fDragIndex;
		BMessage	*fItemMovedMessage;
};


void GetPrettyDescriptiveName(BPath &path, char *name, BMessage *msg = NULL)
{
	strcpy(name, path.Leaf());

	image_id image = load_add_on(path.Path());
	if (image < B_OK)
		return;

	if (msg)
	{
		status_t (* descriptive_name)(BMessage *,char *);
		if (get_image_symbol(image,"descriptive_name",B_SYMBOL_TYPE_TEXT,(void **)&descriptive_name) == B_OK)
			(*descriptive_name)(msg,name);
	}
	unload_add_on(image);
}


//	#pragma mark -


FiltersConfigView::FiltersConfigView(BRect rect,Account *account)
	:	BBox(rect),
		fAccount(account),
		fFilterView(NULL)
{
	BPopUpMenu *menu = new BPopUpMenu(B_EMPTY_STRING);

	BMenuItem *item;
	BMessage *msg;
	if (fChain = fAccount->Inbound())
	{
		menu->AddItem(item = new BMenuItem(MDR_DIALECT_CHOICE ("Incoming E-mail Filters","受信フィルタ"),msg = new BMessage(kMsgChainSelected)));
		msg->AddPointer("chain",fChain);
		item->SetMarked(true);
	}
	if (BMailChain *chain = fAccount->Outbound())
	{
		menu->AddItem(item = new BMenuItem(MDR_DIALECT_CHOICE ("Outgoing E-mail Filters","送信フィルタ"),msg = new BMessage(kMsgChainSelected)));
		msg->AddPointer("chain",chain);
		if (fChain == NULL)
		{
			item->SetMarked(true);
			fChain = chain;
		}
	}

	fChainsField = new BMenuField(BRect(0,0,200,40),NULL,NULL,menu);
	fChainsField->ResizeToPreferred();
	SetLabel(fChainsField);

	// determine font height
	font_height fontHeight;
	fChainsField->GetFontHeight(&fontHeight);
	int32 height = (int32)(fontHeight.ascent + fontHeight.descent + fontHeight.leading) + 5;

	rect = Bounds().InsetByCopy(10,10);
	rect.top += 18;
	rect.right -= B_V_SCROLL_BAR_WIDTH;
	rect.bottom = rect.top + 4 * height + 2;
	fListView = new DragListView(rect,NULL,B_SINGLE_SELECTION_LIST,B_FOLLOW_ALL,new BMessage(kMsgFilterMoved));
	AddChild(new BScrollView(NULL,fListView,B_FOLLOW_ALL,0,false,true));
	rect.right += B_V_SCROLL_BAR_WIDTH;

//	fListView->Select(gSettings.formats.IndexOf(format));
	fListView->SetSelectionMessage(new BMessage(kMsgFilterSelected));

	rect.top = rect.bottom + 8;  rect.bottom = rect.top + height;
	BRect sizeRect = rect;	sizeRect.right = sizeRect.left + 30 + fChainsField->StringWidth(MDR_DIALECT_CHOICE ("Add Filter","フィルタの追加"));

	menu = new BPopUpMenu(MDR_DIALECT_CHOICE ("Add Filter","フィルタの追加"));
	menu->SetRadioMode(false);

	fAddField = new BMenuField(rect,NULL,NULL,menu);
	fAddField->ResizeToPreferred();
	AddChild(fAddField);

	sizeRect.left = sizeRect.right + 5;	sizeRect.right = sizeRect.left + 30 + fChainsField->StringWidth(MDR_DIALECT_CHOICE ("Remove","削除"));
	sizeRect.top--;
	AddChild(fRemoveButton = new BButton(sizeRect,NULL,MDR_DIALECT_CHOICE ("Remove","削除"),new BMessage(kMsgRemoveFilter),B_FOLLOW_BOTTOM));

	ResizeTo(Bounds().Width(),sizeRect.bottom + 10);
	SetTo(fChain);
}


FiltersConfigView::~FiltersConfigView()
{
}


void FiltersConfigView::SelectFilter(int32 index)
{
	if (Parent())
		Parent()->Hide();

	// remove old config view
	if (fFilterView)
	{
		Parent()->RemoveChild(fFilterView);
		
		// update the name in the list
		BStringItem *item = (BStringItem *)fListView->ItemAt(fFilterView->fIndex - fFirst);

		char name[B_FILE_NAME_LENGTH];
		BPath path(fFilterView->fEntryRef);
		GetPrettyDescriptiveName(path, name, fFilterView->fMessage);
		item->SetText(name);

		delete fFilterView;
		fFilterView = NULL;
	}

	if (index >= 0)
	{
		// add new config view
		BMessage *msg = new BMessage();
		entry_ref *ref = new entry_ref();
		if (fChain->GetFilter(index + fFirst,msg,ref) >= B_OK && Parent())
		{
			fFilterView = new FilterConfigView(fChain,index + fFirst,msg,ref);
			if (fFilterView->InitCheck() >= B_OK)
				Parent()->AddChild(fFilterView);
			else
			{
				delete fFilterView;
				fFilterView = NULL;
			}
		}
		else
		{
			delete msg;
			delete ref;
		}
	}

	// re-layout the view containing the config view
	if (CenterContainer *container = dynamic_cast<CenterContainer *>(Parent()))
		container->Layout();

	if (Parent())
		Parent()->Show();
}


void FiltersConfigView::SetTo(BMailChain *chain)
{
	// remove the filter config view
	SelectFilter(-1);

	for (int32 i = fListView->CountItems();i-- > 0;)
	{
		BStringItem *item = (BStringItem *)fListView->RemoveItem(i);
		delete item;
	}

	if (chain->ChainDirection() == inbound)
	{
		fFirst = 2;		// skip protocol (e.g. POP3), and Parser
		fLast = 2;		// skip Notifier, and Folder
	}
	else
	{
		fFirst = 1;		// skip Producer
		fLast = 1;		// skip protocol (e.g. SMTP)
	}
	int32 last = chain->CountFilters() - fLast;
	for (int32 i = fFirst;i < last;i++)
	{
		BMessage msg;
		entry_ref ref;
		if (chain->GetFilter(i,&msg,&ref) == B_OK)
		{
			char name[B_FILE_NAME_LENGTH];
			BPath addon(&ref);
			GetPrettyDescriptiveName(addon, name, &msg);

			fListView->AddItem(new BStringItem(name));
		}
	}
	fChain = chain;

	/*** search inbound/outbound filters ***/

	// remove old filter items
	BMenu *menu = fAddField->Menu();
	for (int32 i = menu->CountItems();i-- > 0;)
	{
		BMenuItem *item = menu->RemoveItem(i);
		delete item;
	}
	
	for (int i = 0; i < 2; i++) {
		BPath path;
		status_t status = find_directory((i == 0) ? B_USER_ADDONS_DIRECTORY : B_BEOS_ADDONS_DIRECTORY,&path);
		if (status != B_OK)
			return;
	
		path.Append("mail_daemon");
		
		if (fChain->ChainDirection() == inbound)
			path.Append("inbound_filters");
		else
			path.Append("outbound_filters");
			
		BDirectory dir(path.Path());
		entry_ref ref;
		while (dir.GetNextRef(&ref) == B_OK)
		{
			char name[B_FILE_NAME_LENGTH];
			BPath path(&ref);
			GetPrettyDescriptiveName(path, name);
	
			BMenuItem *item;
			BMessage *msg;
			menu->AddItem(item = new BMenuItem(name,msg = new BMessage(kMsgAddFilter)));
			msg->AddRef("filter",&ref);
		}
	}
	menu->SetTargetForItems(this);
}


void FiltersConfigView::AttachedToWindow()
{
	fChainsField->Menu()->SetTargetForItems(this);
	fListView->SetTarget(this);
	fAddField->Menu()->SetTargetForItems(this);
	fRemoveButton->SetTarget(this);
}


void FiltersConfigView::MessageReceived(BMessage *msg)
{
	switch (msg->what)
	{
		case kMsgChainSelected:
		{
			BMailChain *chain;
			if (msg->FindPointer("chain",(void **)&chain) < B_OK)
				break;

			SetTo(chain);
			break;
		}
		case kMsgAddFilter:
		{
			entry_ref ref;
			if (msg->FindRef("filter",&ref) < B_OK)
				break;

			BMessage msg;
			if (fChain->AddFilter(fChain->CountFilters() - fLast, msg, ref) >= B_OK)
			{
				char name[B_FILE_NAME_LENGTH];
				BPath path(&ref);
				GetPrettyDescriptiveName(path, name, &msg);
				fListView->AddItem(new BStringItem(name));
			}
			break;
		}
		case kMsgRemoveFilter:
		{
			int32 index = fListView->CurrentSelection();
			if (index < 0)
				break;

			SelectFilter(-1);
			if (BStringItem *item = (BStringItem *)fListView->RemoveItem(index))
			{
				fChain->RemoveFilter(index + fFirst);
				delete item;
			}
			break;
		}
		case kMsgFilterSelected:
		{
			int32 index;
			if (msg->FindInt32("index",&index) < B_OK)
				break;

			SelectFilter(index);
			break;
		}
		case kMsgFilterMoved:
		{
			int32 from = msg->FindInt32("from");
			int32 to = msg->FindInt32("to");
			if (from == to)
				break;

			from += fFirst;
			to += fFirst;

			entry_ref ref;
			BMessage settings;
			if (fChain->GetFilter(from,&settings,&ref) == B_OK)
			{
				// disable filter view saving
				if (fFilterView && fFilterView->fIndex == from)
					fFilterView->fIndex = -1;

				fChain->RemoveFilter(from);
				
				if (fChain->AddFilter(to,settings,ref) < B_OK)
				{
					(new BAlert("E-mail",MDR_DIALECT_CHOICE (
					"Could not move filter, filter deleted.",
					"フィルタが削除された為、移動できません"),"Ok"))->Go();

					// the filter view belongs to the moved filter
					if (fFilterView && fFilterView->fIndex == -1)
						SelectFilter(-1);

					fListView->RemoveItem(msg->FindInt32("to"));
				}
				else if (fFilterView)
				{
					int32 index = fFilterView->fIndex;
					if (index == -1)
						// the view belongs to the moved filter
						fFilterView->fIndex = to;
					else if (index > from && index < to)
						// the view belongs to another filter (between the
						// 'from' & 'to' positions) - all others can keep
						// their index value
						fFilterView->fIndex--;
				}
			}
			break;
		}
		default:
			BView::MessageReceived(msg);
			break;
	}
}

