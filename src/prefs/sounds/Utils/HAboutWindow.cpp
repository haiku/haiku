#include "HAboutWindow.h"
#include <Screen.h>
#include <String.h>
#include <Message.h>
#include <Roster.h>
#include <Path.h>
#include <Application.h>
#include <NodeInfo.h>
#include <Resources.h>
#include <Alert.h>
#include <ClassInfo.h>

#include "BitmapView.h"
#include "CTextView.h"
#include "URLView.h"

#define ICON_OFFSET 60

/***********************************************************
 * Constructor
 ***********************************************************/
HAboutWindow::HAboutWindow(const char* app_name,
						const char* built_date,
						const char* comment,
						const char* url,
						const char* email)
		:BWindow(BRect(-1,-1,-1,-1),
			"",
			B_FLOATING_WINDOW_LOOK,
			B_MODAL_APP_WINDOW_FEEL,
			B_NOT_RESIZABLE|B_NOT_ZOOMABLE)
{
	BString window_title = "About ";
	window_title << app_name;
	SetTitle(window_title.String());
	this->AddShortcut('W',0,new BMessage(B_QUIT_REQUESTED));
	CalcFontHeights();
	/********** Get max width **********/
	BString build = "Built date: ";
	build << built_date;

	float max_width = ICON_OFFSET;
	BFont font(be_plain_font);
	font.SetSize(10);

	max_width = font.StringWidth(build.String());
	BFont boldfont(be_bold_font);
	boldfont.SetSize(14);
	if(max_width < boldfont.StringWidth(app_name) )
		max_width = boldfont.StringWidth(app_name);
	
	/**************************************/
	InitGUI();
	LoadIcon();		
	SetComment(comment);
	int32 lines = fComment->CountLines();
	float result = 0;
	for(register int32 i = 0; i < lines ; i++)
	{
		float tmp = fComment->LineWidth(i);
		if(tmp > result )
			result = tmp;
	}	
	if(max_width < result)
		max_width = result;
	if(max_width < font.StringWidth(url))
		max_width = font.StringWidth(url);
	float width = max_width + ICON_OFFSET + 20;
	
	int32 isUrlMail = 0;
	if(url)
		isUrlMail++;
	if(email)
		isUrlMail++;
	
	float height = fPlainHeight* (2+lines + isUrlMail) + fBoldHeight + 35;
	
	ResizeTo(width,height);
	BRect screen_limits = BScreen().Frame();
	MoveTo(screen_limits.left+floor((screen_limits.Width()-width)/2),
		screen_limits.top+floor((screen_limits.Height()-height)/2));
	
	/************** url view ************/
	float start_pos = fComment->Frame().bottom;
	
	BRect url_rect(ICON_OFFSET,start_pos,width,start_pos+fPlainHeight);
	
	HAboutView *bgview = cast_as(FindView("aboutview"),HAboutView);
	
	if(url)
	{
		BString url_string = url;
	
		int32 index = url_string.FindFirst("http:");
		if(index != B_ERROR)
		{
			url_string = &url[index];
		}
		//(new BAlert("",url_string.String(),"OK"))->Go();
		URLView *urlView = new URLView(url_rect,"url",url,url_string.String());
		urlView->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
		urlView->SetFont(&font);
		bgview->AddChild(urlView);
	
		url_rect.OffsetBy(0,fPlainHeight+3);
	}
	/************** mail view ****************/
	if(email)
	{
		BString mail = email;
		
		int32 index = mail.FindLast(" ");
		if(index != B_ERROR)
		{
			mail = &email[index+1];
		}else{
			index = mail.FindFirst(":");
			if(index != B_ERROR)
			{
				mail = &email[index+1];
			}
		}
	
		BString mail_uri = "mailto:";
		mail_uri << mail;
	
		URLView *mailView = new URLView(url_rect,"email",email,mail_uri.String());
		mailView->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
		mailView->SetFont(&font);
		bgview->AddChild(mailView);
	}

	SetAppName(app_name);
	SetBuiltDate(built_date);
	
	LoadVersion();
}

/***********************************************************
 * InitGUI
 ***********************************************************/
void
HAboutWindow::InitGUI()
{
	BRect bitmapRect(0,0,31,31);
	bitmapRect.OffsetBy(20,5);
	
	fAboutView = new HAboutView(Bounds());
	BRect rect(ICON_OFFSET,15,ICON_OFFSET,15+fBoldHeight);
	fAppName = new BStringView(rect,"name","",B_FOLLOW_NONE);
	fAboutView->AddChild(fAppName);
	rect.OffsetBy(0,fBoldHeight);
	rect.bottom = rect.top + fPlainHeight;
	fVersion = new BStringView(rect,"ver","",B_FOLLOW_NONE);
	fAboutView->AddChild(fVersion);
	rect.OffsetBy(0,fPlainHeight);
	fBuiltDate = new BStringView(rect,"build","",B_FOLLOW_NONE);
	fAboutView->AddChild(fBuiltDate);
	rect.OffsetBy(0,fPlainHeight);
	fComment = new CTextView(rect,"comment",B_FOLLOW_NONE,B_WILL_DRAW);
	fAboutView->AddChild(fComment);
	AddChild(fAboutView);
}

/***********************************************************
 * Load icon from application resource
 ***********************************************************/
void
HAboutWindow::LoadIcon()
{
	//Extract title bitmap
	BBitmap *icon = new BBitmap(BRect(0,0,31,31),B_COLOR_8_BIT);
	app_info info;
    BPath path;
    be_app->GetAppInfo(&info); 
	BNodeInfo::GetTrackerIcon(&info.ref,icon,B_LARGE_ICON);
	fAboutView->SetBitmap(icon);
}

/***********************************************************
 * Load versino from application resource.
 ***********************************************************/
void
HAboutWindow::LoadVersion()
{
	//Extract version resource
	BString version = "";
	BResources* app_version_resource = BApplication::AppResources();
	if(app_version_resource)
	{
		size_t resource_size;
		const char* app_version_data = (const char*)app_version_resource->LoadResource('APPV',
			"BEOS:APP_VERSION",&resource_size);
		if(app_version_data && resource_size > 20)
		{
			const char* status[] = {"Development","Alpha","Beta","Gamma",
				"Golden master","Final"};
			app_version_info *info = (app_version_info*)app_version_data;
			uint32 v1 = info->v1;
			uint32 v2 = info->v2;
			uint32 v3 = info->v3;
			version << v1 << "." << v2 << "." << v3;
			if(info->status != 5)
				version << " " <<status[info->status];
			if(info->rel != 0)
				version << " Release " << info->rel;
			SetVersion(version.String());
		}
	}
}

/***********************************************************
 * Set applicaiton name
 ***********************************************************/
void
HAboutWindow::SetAppName(const char* name)
{
	BFont font(be_bold_font);
	font.SetSize(14);
	
	BRect rect(0,0,font.StringWidth(name),fBoldHeight);
	BRect old_rect = fAppName->Bounds();
	fAppName->SetFont(&font);
	fAppName->ResizeBy(rect.Width()-old_rect.Width(),rect.Height()-old_rect.Height());
	fAppName->SetText(name);
}

/***********************************************************
 * Set application built date.
 ***********************************************************/
void
HAboutWindow::SetBuiltDate(const char* date)
{
	BString title="Built date: ";
	title << date;

	BFont font(be_plain_font);
	font.SetSize(10);

	BRect rect(0,0,font.StringWidth(title.String()),fPlainHeight);
	BRect old_rect = fBuiltDate->Bounds();
	fBuiltDate->SetFont(&font);
	fBuiltDate->ResizeBy(rect.Width()-old_rect.Width(),rect.Height()-old_rect.Height());
	fBuiltDate->SetText(title.String());
}

/***********************************************************
 * Set comments
 ***********************************************************/
void
HAboutWindow::SetComment(const char* text)
{
	BFont font(be_plain_font);
	font.SetSize(10);
	
	fComment->SetFont(&font);
	fComment->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	fComment->MakeEditable(false);
	fComment->MakeFocus(false);
	fComment->MakeSelectable(false);
	fComment->SetWordWrap(false);
	fComment->SetText(text);
	int32 lines = fComment->CountLines();	
	BRect rect(0,0,font.StringWidth(text)+4,fPlainHeight*lines+4);
	BRect old_rect = fComment->Bounds();
	fComment->ResizeBy(rect.Width()-old_rect.Width(),rect.Height()-old_rect.Height());
	
}

/***********************************************************
 * Set version info
 ***********************************************************/
void
HAboutWindow::SetVersion(const char* version)
{
	BString title="Version ";
	title << version;
	
	BFont font(be_plain_font);
	font.SetSize(10);

	BRect rect(0,0,font.StringWidth(title.String()),fPlainHeight);
	BRect old_rect = fVersion->Bounds();
	fVersion->SetFont(&font);
	fVersion->ResizeBy(rect.Width()-old_rect.Width(),rect.Height()-old_rect.Height());
	fVersion->SetText(title.String());
}

/***********************************************************
 * Calc font height
 ***********************************************************/
void
HAboutWindow::CalcFontHeights()
{
	BFont plainfont(be_plain_font);
	plainfont.SetSize(10);
	
	font_height FontAttributes;
	plainfont.GetHeight(&FontAttributes);
	fPlainHeight = ceil(FontAttributes.ascent) + ceil(FontAttributes.descent);	
	
	BFont boldfont(be_bold_font);
	boldfont.SetSize(14);
	
	boldfont.GetHeight(&FontAttributes);
	fBoldHeight = ceil(FontAttributes.ascent) + ceil(FontAttributes.descent);
}

/***************************************************************************
 * HAboutView
 ****************************************************************************/
const float kBorderWidth = 32.0f;
const rgb_color kDarkBorderColor = {184, 184, 184, 255};

/***********************************************************
 * Constructor
 ***********************************************************/
HAboutView::HAboutView(BRect rect,BBitmap *icon)
	:BView(rect,"aboutview",B_FOLLOW_ALL,B_WILL_DRAW)
{
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	fIcon = icon;
}

/***********************************************************
 * Destructor
 ***********************************************************/
HAboutView::~HAboutView()
{
	delete fIcon;
}

/***********************************************************
 * Draw
 ***********************************************************/
void
HAboutView::Draw(BRect updateRect)
{
	BRect drawBounds(Bounds());
	drawBounds.right = kBorderWidth;
	SetHighColor(kDarkBorderColor);
	FillRect(drawBounds);
	
	if(fIcon != NULL)
	{
		BRect bitmapRect(0,0,31,31);
		bitmapRect.OffsetBy(20,5);
		drawing_mode mode = DrawingMode();
		SetDrawingMode(B_OP_ALPHA);
		DrawBitmap(fIcon,bitmapRect);	
		SetDrawingMode(mode);
	}
}

/***********************************************************
 * Set view bitmap
 ***********************************************************/
void
HAboutView::SetBitmap(BBitmap *bitmap)
{
	delete fIcon;
	fIcon = bitmap;
}