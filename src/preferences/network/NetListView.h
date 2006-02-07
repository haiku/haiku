#ifndef NETLISTVIEW_H
#define NETLISTVIEW_H

#include <ListView.h>
#include <ListItem.h>
#include <String.h>
#include "ConfigData.h"

class NetListView : public BListView 
{
public:
	NetListView(BRect frame,const char *name);
	
	virtual void SelectionChanged(void);
};


class NetListItem : public BStringItem
{
public:
	NetListItem(const char *text);
	
	InterfaceData fData;
	
	BString fIPADDRESS;
	BString fNETMASK;
	BString fPRETTYNAME;
	int 	fENABLED;
	int		fDHCP;
};

#endif
