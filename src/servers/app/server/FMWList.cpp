
#include <stdio.h>
#include <List.h>

#include "FMWList.h"
#include "WinBorder.h"
#include "ServerWindow.h"

FMWList::FMWList(){
}

FMWList::~FMWList(){
//printf("~FMWList()\n");
//	if (fList.CountItems() > 0)
//		printf("SERVER: WARNING: FMWList NOT empty in destructor!\n");
}
	
void FMWList::AddItem(void* item){
	int32		feelItem = ((WinBorder*)item)->Window()->Feel();;
	int32		feelTemp = 0;
	int32		location = 0;

	for(int32 i=0; i<fList.CountItems(); i++){
		location	= i + 1;
		feelTemp	= ((WinBorder*)fList.ItemAt(i))->Window()->Feel();
			// in short: if 'item' is a floating window
		if(	(feelItem == B_FLOATING_SUBSET_WINDOW_FEEL ||
			 feelItem == B_FLOATING_APP_WINDOW_FEEL ||
			 feelItem == B_FLOATING_ALL_WINDOW_FEEL)
			&& // and 'temp' a modal one
			(feelTemp == B_MODAL_SUBSET_WINDOW_FEEL ||
			 feelTemp == B_MODAL_APP_WINDOW_FEEL ||
			 feelTemp == B_MODAL_ALL_WINDOW_FEEL)
			)
			// means we found the place for our window('wb')
		{ location--; break; }
	}
	fList.AddItem(item, location);
}

bool FMWList::HasItem(void* item) const{
	return fList.HasItem(item);
}

bool FMWList::RemoveItem(void* item){
	return fList.RemoveItem(item);
}

void* FMWList::ItemAt(int32 i) const{
	return fList.ItemAt(i);
}

int32 FMWList::CountItems() const{
	return fList.CountItems();
}

void* FMWList::LastItem() const{
	return fList.LastItem();
}

void* FMWList::FirstItem() const{
	return fList.FirstItem();
}

int32 FMWList::IndexOf(void *item){
	return fList.IndexOf(item);
}

void FMWList::AddFMWList(FMWList *list){
	int32		i = 0;
	for(i=0; i<fList.CountItems(); i++){
		int32	feel = ((WinBorder*)fList.ItemAt(i))->Window()->Feel();
		if(feel == B_MODAL_SUBSET_WINDOW_FEEL ||
			feel == B_MODAL_APP_WINDOW_FEEL ||
			feel == B_MODAL_ALL_WINDOW_FEEL)
		{
			break;
		}
	}
	
	int32		j = 0;
	for(j=0; j<list->CountItems(); j++){
		void	*item = list->ItemAt(j);
		int32	feel = ((WinBorder*)item)->Window()->Feel();
		if(feel == B_MODAL_SUBSET_WINDOW_FEEL ||
			feel == B_MODAL_APP_WINDOW_FEEL ||
			feel == B_MODAL_ALL_WINDOW_FEEL)
		{
			fList.AddItem(item, fList.CountItems());
		}
		else{
			fList.AddItem(item, i);
			i++;
		}
	}
}

void FMWList::PrintToStream() const{
	printf("Floating and modal windows list:\n");
	WinBorder	*wb = NULL;
	for (int32 i=0; i<fList.CountItems(); i++){
		wb		= (WinBorder*)fList.ItemAt(i);
		printf("\t%s", wb->GetName());
		if (wb->Window()->Feel() == B_FLOATING_SUBSET_WINDOW_FEEL)
			printf("\t%s\n", "B_FLOATING_SUBSET_WINDOW_FEEL");
		if (wb->Window()->Feel() == B_FLOATING_APP_WINDOW_FEEL)
			printf("\t%s\n", "B_FLOATING_APP_WINDOW_FEEL");
		if (wb->Window()->Feel() == B_FLOATING_ALL_WINDOW_FEEL)
			printf("\t%s\n", "B_FLOATING_ALL_WINDOW_FEEL");
		if (wb->Window()->Feel() == B_MODAL_SUBSET_WINDOW_FEEL)
			printf("\t%s\n", "B_MODAL_SUBSET_WINDOW_FEEL");
		if (wb->Window()->Feel() == B_MODAL_APP_WINDOW_FEEL)
			printf("\t%s\n", "B_MODAL_APP_WINDOW_FEEL");
		if (wb->Window()->Feel() == B_MODAL_ALL_WINDOW_FEEL)
			printf("\t%s\n", "B_MODAL_ALL_WINDOW_FEEL");

			// this should NOT happen
		if (wb->Window()->Feel() == B_NORMAL_WINDOW_FEEL)
			printf("\t%s\n", "B_NORMAL_WINDOW_FEEL");
	}
}
