#include "HEventItem.h"
#include "ResourceUtils.h"
#include <Bitmap.h>
#include <View.h>
#include <NodeInfo.h>
#include <Path.h>
#include <MediaFiles.h>
#include <Alert.h>
#include <Beep.h>

/***********************************************************
 * Constructor
 ***********************************************************/
HEventItem::HEventItem(const char* name,const char* path)
	:CLVEasyItem(0,false,false,20)
	,fName(name)
{
	//BBitmap* bitmap = new BBitmap(BRect(0,0,15,15),B_COLOR_8_BIT);
	BBitmap *bitmap = ResourceUtils().GetBitmapResource('BBMP',"Sound Bitmap");
	SetColumnContent(0,bitmap,false);
	SetColumnContent(1,fName.String(),false,false);
	SetColumnContent(2,"",false,false);
	delete bitmap;
	SetPath(path);
}

/***********************************************************
 * Destructor
 ***********************************************************/
HEventItem::~HEventItem()
{
}

/***********************************************************
 * Set path
 ***********************************************************/
void
HEventItem::SetPath(const char* in_path)
{
	fPath = in_path;
	BPath path(in_path);
	BPath parent;
	BMediaFiles mfiles;
	entry_ref ref;
	
	if(path.InitCheck() == B_OK)
	{
		SetColumnContent(2,path.Leaf(),false,false);
		::get_ref_for_path(path.Path(),&ref);
		if(mfiles.SetRefFor(BMediaFiles::B_SOUNDS,Name(),ref) != B_OK)
			(new BAlert("","Error","OK"))->Go();
		path.GetParent(&parent);
		SetColumnContent(3,parent.Path(),false,false);// falta
	}else{
		SetColumnContent(2,"<none>",false,false);
		//if(mfiles.RemoveRefFor(BMediaFiles::B_SOUNDS,Name(),ref) != B_OK)
		//	(new BAlert("","Entry not found","OK"))->Go();
		mfiles.RemoveItem(BMediaFiles::B_SOUNDS,Name());
		::add_system_beep_event(Name());
		SetColumnContent(3,"<none>",false,false);// falta	
	}
}

/***********************************************************
 * Remove
 ***********************************************************/
void
HEventItem::Remove()
{
	BMediaFiles mfiles;
	
	mfiles.RemoveItem(BMediaFiles::B_SOUNDS,Name());
}

/***********************************************************
 *
 ***********************************************************/
const char*
HEventItem::Path() const
{
	return fPath.String();
}
