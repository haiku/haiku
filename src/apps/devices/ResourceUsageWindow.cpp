// ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~
//
//	Copyright (c) 2003-2004, OpenBeOS
//
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//
//
//  File:        ResourceUsageWindow.cpp
//  Author:      Sikosis, Jérôme Duval
//  Description: Devices Preferences
//  Created :    July 19, 2003
// 
// ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~


// Includes ------------------------------------------------------------------
#include <Box.h>
#include <Catalog.h>
#include <Screen.h>
#include <ScrollView.h>
#include <stdio.h>
#include <strings.h>
#include <TabView.h>

#include "DevicesInfo.h"
#include "DevicesWindows.h"

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "ResourceUsageWindow"

class IRQDMAItem : public BListItem
{
	public:
		IRQDMAItem(int32 number, const char* name);
		~IRQDMAItem();
		virtual void DrawItem(BView *, BRect, bool = false);
	private:
		char* fName;
		int32 fNumber;
};


IRQDMAItem::IRQDMAItem(int32 number, const char* name)
	: BListItem(),
	fNumber(number)
{
	fName = strdup(name);
}

IRQDMAItem::~IRQDMAItem()
{
	delete fName;
}

/***********************************************************
 * DrawItem
 ***********************************************************/
void
IRQDMAItem::DrawItem(BView *owner, BRect itemRect, bool complete)
{
	rgb_color kBlack = { 0,0,0,0 };
	rgb_color kHighlight = { 156,154,156,0 };
	
	if (IsSelected() || complete) {
		rgb_color color;
		if (IsSelected())
			color = kHighlight;
		else
			color = owner->ViewColor();
		
		owner->SetHighColor(color);
		owner->SetLowColor(color);
		owner->FillRect(itemRect);
		owner->SetHighColor(kBlack);
		
	} else {
		owner->SetLowColor(owner->ViewColor());
	}
	
	BFont font = be_plain_font;
	font_height	finfo;
	font.GetHeight(&finfo);
	
	BPoint point = BPoint(itemRect.left + 5, itemRect.bottom - finfo.descent + 1);
	
	owner->SetHighColor(kBlack);
	owner->SetFont(be_plain_font);
	owner->MovePenTo(point);
	if (fNumber > -1) {
		char string[2];
		sprintf(string, "%ld", fNumber);
		owner->DrawString(string);
	}
	point += BPoint(28, 0);
	owner->MovePenTo(point);
	owner->DrawString(fName);
}

class RangeItem : public BListItem
{
	public:
		RangeItem(uint32 lowAddress, uint32 highAddress, const char* name);
		~RangeItem();
		virtual void DrawItem(BView *, BRect, bool = false);
		static int Compare(const void *firstArg, const void *secondArg);
	private:
		char* fName;
		uint32 fLowAddress, fHighAddress;
};


RangeItem::RangeItem(uint32 lowAddress, uint32 highAddress, const char* name)
	: BListItem(),
	fLowAddress(lowAddress), 
	fHighAddress(highAddress)
{
	fName = strdup(name);
}

RangeItem::~RangeItem()
{
	delete fName;
}

/***********************************************************
 * DrawItem
 ***********************************************************/
void
RangeItem::DrawItem(BView *owner, BRect itemRect, bool complete)
{
	rgb_color kBlack = { 0,0,0,0 };
	rgb_color kHighlight = { 156,154,156,0 };
	
	if (IsSelected() || complete) {
		rgb_color color;
		if (IsSelected())
			color = kHighlight;
		else
			color = owner->ViewColor();
		
		owner->SetHighColor(color);
		owner->SetLowColor(color);
		owner->FillRect(itemRect);
		owner->SetHighColor(kBlack);
		
	} else {
		owner->SetLowColor(owner->ViewColor());
	}
	
	BFont font = be_plain_font;
	font_height	finfo;
	font.GetHeight(&finfo);
	
	BPoint point = BPoint(itemRect.left + 17, itemRect.bottom - finfo.descent + 1);
	owner->SetFont(be_fixed_font);
	owner->SetHighColor(kBlack);
	owner->MovePenTo(point);
	
	if (fLowAddress >= 0) {
		char string[255];
		sprintf(string, "0x%04lx - 0x%04lx", fLowAddress, fHighAddress);
		owner->DrawString(string);
	}
	point += BPoint(174, 0);
	owner->SetFont(be_plain_font);
	owner->MovePenTo(point);
	owner->DrawString(fName);
}

int 
RangeItem::Compare(const void *firstArg, const void *secondArg)
{
	const RangeItem *item1 = *static_cast<const RangeItem * const *>(firstArg);
	const RangeItem *item2 = *static_cast<const RangeItem * const *>(secondArg);
	
	if (item1->fLowAddress < item2->fLowAddress) {
		return -1;
	} else if (item1->fLowAddress > item2->fLowAddress) {
		return 1;
	} else 
		return 0;

}


// ----------------------------------------------------------------------------


// ResourceUsageWindow - Constructor
ResourceUsageWindow::ResourceUsageWindow(BRect frame, BList &list) 
	: BWindow (frame, B_TRANSLATE("Resource Usage"), B_TITLED_WINDOW_LOOK,
		B_NORMAL_WINDOW_FEEL , B_NOT_ZOOMABLE|B_NOT_RESIZABLE)
{
	InitWindow(list);
	CenterOnScreen();
	Show();
}
// ----------------------------------------------------------------------------


// ResourceUsageWindow - Destructor
ResourceUsageWindow::~ResourceUsageWindow()
{
	
}
// ----------------------------------------------------------------------------


// ResourceUsageWindow::InitWindow -- Initialization Commands here
void ResourceUsageWindow::InitWindow(BList &list)
{
	BRect rtab = Bounds();
	BRect rlist = Bounds();
	rtab.top += 10;
	rlist.top += 10;
	rlist.left += 12;
	rlist.right -= 15 + B_V_SCROLL_BAR_WIDTH;
	rlist.bottom -= 47;

	// Create the TabView and Tabs
	BTabView *tabView = new BTabView(rtab,"resource_usage_tabview");
	tabView->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	
	rtab = tabView->Bounds();
	rtab.InsetBy(5,5);
	
	// Create the ListViews
	BListView *IRQListView = new BListView(rlist, "IRQListView",
		B_SINGLE_SELECTION_LIST, B_FOLLOW_LEFT | B_FOLLOW_TOP,
		B_WILL_DRAW | B_NAVIGABLE);
	BListView *DMAListView = new BListView(rlist, "DMAListView",
		B_SINGLE_SELECTION_LIST, B_FOLLOW_LEFT | B_FOLLOW_TOP,
		B_WILL_DRAW | B_NAVIGABLE);
	BListView *IORangeListView = new BListView(rlist, "IORangeListView",
		B_SINGLE_SELECTION_LIST, B_FOLLOW_LEFT | B_FOLLOW_TOP,
		B_WILL_DRAW | B_NAVIGABLE);
	BListView *memoryListView = new BListView(rlist, "memoryListView",
		B_SINGLE_SELECTION_LIST, B_FOLLOW_LEFT | B_FOLLOW_TOP,
		B_WILL_DRAW | B_NAVIGABLE);
	
	BScrollView *IRQScrollView = new BScrollView("scroll_list1", IRQListView,
		B_FOLLOW_LEFT|B_FOLLOW_TOP, 0, false, true, B_FANCY_BORDER);
	BScrollView *DMAScrollView = new BScrollView("scroll_list2", DMAListView,
		B_FOLLOW_LEFT|B_FOLLOW_TOP, 0, false, true, B_FANCY_BORDER);
	BScrollView *IORangeScrollView = new BScrollView("scroll_list3",
		IORangeListView, B_FOLLOW_LEFT|B_FOLLOW_TOP, 0, false, true,
		B_FANCY_BORDER);
	BScrollView *memoryScrollView = new BScrollView("scroll_list4",
		memoryListView, B_FOLLOW_LEFT|B_FOLLOW_TOP, 0, false, true,
		B_FANCY_BORDER);
	
	BTab *tab = new BTab();
	tabView->AddTab(IRQScrollView, tab);
	tab->SetLabel(B_TRANSLATE("IRQ"));
	tab = new BTab();
	tabView->AddTab(DMAScrollView, tab);
	tab->SetLabel(B_TRANSLATE("DMA"));
	tab = new BTab();
	tabView->AddTab(IORangeScrollView, tab);
	tab->SetLabel(B_TRANSLATE("IO Range"));
	tab = new BTab();
	tabView->AddTab(memoryScrollView, tab);
	tab->SetLabel(B_TRANSLATE("Memory Range"));
	
	{
		uint32 mask = 1;
		
		for (int i=0;i<16;mask<<=1,i++) {
			bool first = true;
			
			for (int32 j=0; j<list.CountItems(); j++) {
				DevicesInfo *deviceInfo = (DevicesInfo *)list.ItemAt(j);
				struct device_configuration *current = deviceInfo->GetCurrent();
				resource_descriptor r;
				
				int32 num = count_resource_descriptors_of_type(current,
					B_IRQ_RESOURCE);
				
				for (int32 k=0;k<num;k++) {
					get_nth_resource_descriptor_of_type(current, k, B_IRQ_RESOURCE,
							&r, sizeof(resource_descriptor));
					
					if (mask & r.d.m.mask) {
						IRQListView->AddItem(new IRQDMAItem(first ? i : -1,
							deviceInfo->GetCardName()));
						first = false;
					}
				}
			}
			
			if (first) {
				IRQListView->AddItem(new IRQDMAItem(i, ""));
			}
		}
	}
	
	{
		uint32 mask = 1;
		
		for (int i=0;i<8;mask<<=1,i++) {
			bool first = true;
			
			for (int32 j=0; j<list.CountItems(); j++) {
				DevicesInfo *deviceInfo = (DevicesInfo *)list.ItemAt(j);
				struct device_configuration *current = deviceInfo->GetCurrent();
				resource_descriptor r;
				
				int32 num = count_resource_descriptors_of_type(current,
					B_DMA_RESOURCE);
				
				for (int32 k=0;k<num;k++) {
					get_nth_resource_descriptor_of_type(current, k,
						B_DMA_RESOURCE,	&r, sizeof(resource_descriptor));
					
					if (mask & r.d.m.mask) {
						DMAListView->AddItem(new IRQDMAItem(first ? i : -1,
							deviceInfo->GetCardName()));
						first = false;
					}
				}
			}
			
			if (first) {
				DMAListView->AddItem(new IRQDMAItem(i, ""));
			}
		}
	}

	{
		for (int32 j=0; j<list.CountItems(); j++) {
			DevicesInfo *deviceInfo = (DevicesInfo *)list.ItemAt(j);
			struct device_configuration *current = deviceInfo->GetCurrent();
			resource_descriptor r;
			
			int32 num = count_resource_descriptors_of_type(current,
				B_IO_PORT_RESOURCE);
			
			for (int32 k=0;k<num;k++) {
				get_nth_resource_descriptor_of_type(current, k,
					B_IO_PORT_RESOURCE,	&r, sizeof(resource_descriptor));
				
				IORangeListView->AddItem(new RangeItem(r.d.r.minbase, 
					r.d.r.minbase + r.d.r.len - 1, deviceInfo->GetCardName()));
			}
		}
	
		IORangeListView->SortItems(&RangeItem::Compare);
	}
	
	{
		for (int32 j=0; j<list.CountItems(); j++) {
			DevicesInfo *deviceInfo = (DevicesInfo *)list.ItemAt(j);
			struct device_configuration *current = deviceInfo->GetCurrent();
			resource_descriptor r;
			
			int32 num = count_resource_descriptors_of_type(current,
				B_MEMORY_RESOURCE);
			
			for (int32 k=0;k<num;k++) {
				get_nth_resource_descriptor_of_type(current, k, B_MEMORY_RESOURCE,
						&r, sizeof(resource_descriptor));
				
				memoryListView->AddItem(new RangeItem(r.d.r.minbase, 
					r.d.r.minbase + r.d.r.len - 1, deviceInfo->GetCardName()));
			}
		}
	
		memoryListView->SortItems(&RangeItem::Compare);
	}
	
	
	BBox *background = new BBox(Bounds(), "background");
	background->SetBorder(B_NO_BORDER);
	AddChild(background);
	background->AddChild(tabView);
}
// ----------------------------------------------------------------------------


// ResourceUsageWindow::MessageReceived -- receives messages
void 
ResourceUsageWindow::MessageReceived (BMessage *message)
{
	switch(message->what)
	{
		default:
			BWindow::MessageReceived(message);
			break;
	}
}
// ----------------------------------------------------------------------------
