#include "RectUtils.h"
#include <Screen.h>
#include <Roster.h>
#include <Application.h>
#include <fs_attr.h>

/*
* Constructor
*/
BRect
RectUtils::CenterRect(float width,float height)
{
	BRect frame = BScreen().Frame();
	BRect rect;

	rect.left = frame.Width()/2.0 - width/2.0;
	rect.right = rect.left + width;
	rect.top = frame.Height()/2.0 - height/2.0;
	rect.bottom = rect.top + height;
	return rect;
}

/*
* Load rect data from the application file attribute.
*/
bool
RectUtils::LoadRectFromApp(const char* name,BRect *rect)
{
	app_info info; 
    be_app->GetAppInfo(&info); 
   	BEntry entry(&info.ref); 
   	bool rc = false;
   	
	BFile appfile(&entry,B_READ_ONLY);
	attr_info ainfo;
	status_t err = appfile.GetAttrInfo(name,&ainfo);
	if(err == B_OK)
	{
		appfile.ReadAttr(name,B_RECT_TYPE,0,rect,sizeof(BRect));
		rc = true;
	} else {
		rc = false;
	}
	return rc;
}

/*
* Save rect data to the application file attribute.
*/
void
RectUtils::SaveRectToApp(const char* name,BRect rect)
{
	app_info info; 
    be_app->GetAppInfo(&info); 
   	BEntry entry(&info.ref); 
   	
	BFile appfile(&entry,B_WRITE_ONLY);
	appfile.WriteAttr(name,B_RECT_TYPE,0,&rect,sizeof(BRect));
}
