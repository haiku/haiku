/*

	ChartWindow.cpp

	by Pierre Raynaud-Richard.

	Copyright 1998 Be Incorporated, All Rights Reserved.

*/

#include "ChartWindow.h"

#include <AppFileInfo.h>
#include <Application.h>
#include <Bitmap.h>
#include <Box.h>
#include <Button.h>
#include <ByteOrder.h>
#include <Catalog.h>
#include <CheckBox.h>
#include <Directory.h>
#include <Entry.h>
#include <File.h>
#include <FindDirectory.h>
#include <Menu.h>
#include <MenuBar.h>
#include <MenuField.h>
#include <MenuItem.h>
#include <Path.h>
#include <PlaySound.h>
#include <PopUpMenu.h>
#include <RadioButton.h>
#include <Screen.h>
#include <Slider.h>
#include <TextControl.h>

#include <math.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "ChartWindow"

/* pseudo-random generator parameters (not very good ones,
   but good enough for what we do here). */
enum {
	CRC_START		= 0x29dec231,
	CRC_KEY			= 0x1789feb3
};

#define	MAX_FONT_SIZE	12.0f

/* various offset, width, height and position used to align and
   set the various UI elements. */
enum {
	TOP_LEFT_LIMIT	= 26,
	H_BORDER		= 5,
	V_BORDER		= 2,
	ANIM_LABEL		= 52,
	ANIM_POPUP		= 42,
	DISP_LABEL		= 40,
	DISP_POPUP		= 42,
	BUTTON_WIDTH	= 50,
	BUTTON_OFFSET	= -100,
	SPACE_LABEL		= 40,
	SPACE_POPUP		= 53,
	INSTANT_LOAD	= 205,
	LEFT_WIDTH		= 96,
	LEFT_OFFSET		= 2,
	STATUS_BOX		= 98,
	STATUS_LABEL	= 13,
	STATUS_EDIT		= 25,
	STATUS_OFFSET	= 2,
	BOX_H_OFFSET	= 4,
	BOX_V_OFFSET	= 14,
	FULL_SCREEN		= 16,
	AUTO_DEMO		= 22,
	SECOND_THREAD	= 16,
	COLORS_BOX		= 146,
	COLORS_LABEL	= 16,
	COLORS_OFFSET	= 2,
	COLOR_CELL		= 8,
	SPECIAL_BOX		= 92,
	SPECIAL_LABEL	= 16,
	SPECIAL_OFFSET	= 2,
	STAR_DENSITY_H	= 160,
	STAR_DENSITY_V	= 34,
	REFRESH_RATE_H	= 320,
	REFRESH_RATE_V	= 34
};

/* min, max and standard setting of the star count, also
   called starfield density. */
enum {
	STAR_DENSITY_MIN		= 400,
	STAR_DENSITY_MAX		= 20000,
	STAR_DENSITY_DEFAULT	= 2000
};

/* min, max and standard setting of the refresh rate. */
#define	REFRESH_RATE_MIN		0.6
#define	REFRESH_RATE_MAX		600.0
#define	REFRESH_RATE_DEFAULT	60.0

/* private enums used to identify the 3 types of
  programmable picture buttons. */
enum {
	COLOR_BUTTON_PICT	= 0,
	DENSITY_BUTTON_PICT	= 1,
	REFRESH_BUTTON_PICT	= 2
};

/* min, max zoom (also default offscreen size), and max dimensions
   of the content area of the window. */
enum {
	WINDOW_H_MIN		= 220,
	WINDOW_V_MIN		= 146,
	WINDOW_H_STD		= 800,
	WINDOW_V_STD		= 600,
	WINDOW_H_MAX		= 1920,
	WINDOW_V_MAX		= 1440,

/* increment step used to dynamically resize the offscreen buffer */
	WINDOW_H_STEP		= 224,
	WINDOW_V_STEP		= 168
};

/* time delay between refresh of the stat counters */
enum {
	STAT_DELAY			= 1000000
};

/* ratio between the rear clipping depth and the front clipping
   depth. */
#define		Z_CUT_RATIO		20.0

/* prefered aspect ratio between horizontal and vertical
   dimensions of the animation frame. */
#define		DH_REF			0.8
#define		DV_REF			0.6

/* no comments (almost :-) */
#define		abs(x) 	(((x)>0)?(x):-(x))

/* default background color for the UI. */
rgb_color	background_color = { 216, 216, 216, 255 };

/* the 7 colors for stars. */
static rgb_color	color_list[7] = {
	{ 255, 160, 160, 255 },	/* red    */
	{ 160, 255, 160, 255 },	/* green  */
	{ 160, 160, 255, 255 },	/* blue   */
	{ 255, 255, 160, 255 },	/* yellow */
	{ 255, 208, 160, 255 },	/* orange */
	{ 255, 160, 255, 255 },	/* pink   */
	{ 255, 255, 255, 255 }	/* white  */
};

/* the 8 levels of lighting, in 1/65536. */
static int32 light_gradient[8] = {
	0x2000,
	0x5000,
	0x7800,
	0x9800,
	0xb800,
	0xd000,
	0xe800,
	0x10000
};


//	#pragma mark helper classes


/* multiply a vector by a constant */
TPoint
TPoint::operator* (const float k) const
{
	TPoint v;

	v.x = x*k;
	v.y = y*k;
	v.z = z*k;
	return v;
}

/* substract 2 vectors */
TPoint
TPoint::operator- (const TPoint& v2) const
{
	TPoint v;

	v.x = x-v2.x;
	v.y = y-v2.y;
	v.z = z-v2.z;
	return v;
}

/* add 2 vectors */
TPoint TPoint::operator+ (const TPoint& v2) const {
	TPoint v;

	v.x = x+v2.x;
	v.y = y+v2.y;
	v.z = z+v2.z;
	return v;
}

/* vectorial product of 2 vectors */
TPoint
TPoint::operator^ (const TPoint& v2) const
{
	TPoint v;

	v.x = y*v2.z - z*v2.y;
	v.y = z*v2.x - x*v2.z;
	v.z = x*v2.y - y*v2.x;
	return v;
}

/* length of a vector */
float
TPoint::Length() const
{
	return sqrt(x*x + y*y + z*z);
}

/* product of a vector by a matrix */
TPoint
TMatrix::operator* (const TPoint& v) const
{
	TPoint res;

	res.x = m[0][0]*v.x + m[1][0]*v.y + m[2][0]*v.z;
	res.y = m[0][1]*v.x + m[1][1]*v.y + m[2][1]*v.z;
	res.z = m[0][2]*v.x + m[1][2]*v.y + m[2][2]*v.z;
	return res;
}

/* extract the Nth vector/column of a matrix. */
TPoint
TMatrix::Axis(int32 index)
{
	TPoint v;

	v.x = m[index][0];
	v.y = m[index][1];
	v.z = m[index][2];
	return v;
}

/* as we use rotation matrix, the invert of the matrix
   is equal to the transpose */
TMatrix
TMatrix::Transpose() const
{
	TMatrix inv;

	inv.m[0][0] = m[0][0];
	inv.m[0][1] = m[1][0];
	inv.m[0][2] = m[2][0];
	inv.m[1][0] = m[0][1];
	inv.m[1][1] = m[1][1];
	inv.m[1][2] = m[2][1];
	inv.m[2][0] = m[0][2];
	inv.m[2][1] = m[1][2];
	inv.m[2][2] = m[2][2];
	return inv;
}

/* set a spherical rotation matrix */
void
TMatrix::Set(const float alpha, const float theta, const float phi)
{
	float cD,sD,cI,sI,cA,sA;

	/* trigonometry */
	cD = cos(alpha);
	sD = sin(alpha);
	cI = cos(theta);
	sI = sin(theta);
	cA = cos(phi);
	sA = sin(phi);

	/* rotation matrix */
	m[0][0] = cD*cA+sD*sI*sA;
	m[1][0] = -sA*cI;
	m[2][0] = sD*cA-cD*sI*sA;
	m[0][1] = cD*sA-sD*sI*cA;
	m[1][1] = cI*cA;
	m[2][1] = sD*sA+cD*cA*sI;
	m[0][2] = -sD*cI;
	m[1][2] = -sI;
	m[2][2] = cD*cI;
}


//	#pragma mark -


/* this function will play a wav sound file, with the specified
   following name, in the application folder. This is activated
   when you press the button "Auto demo". */
void
LaunchSound()
{
/*
	BEntry			soundFile;
	app_info 		info;
	status_t		err;
	entry_ref		snd_ref;
	BDirectory		appFolder;
	sound_handle	sndhandle;

	err = be_app->GetAppInfo(&info);
	BEntry appEntry(&info.ref);
	if (err != B_NO_ERROR)
		return;
	err = appEntry.GetParent(&appFolder);
	if (err != B_NO_ERROR)
		return;
	appFolder.FindEntry("demo.wav", &soundFile);
	err = soundFile.GetRef(&snd_ref);
	sndhandle = play_sound(&snd_ref, true, true, true);
*/
}

/* return the version_info of a file, described by its name
   and its generic folder (in find_directory syntax). */
status_t
get_file_version_info(directory_which dir,
	char *filename, version_info *info)
{
	BPath 			path;
	BFile			file;
	status_t		res;
	BAppFileInfo	appinfo;

	/* find the directory */
	if ((res = find_directory(dir, &path)) != B_NO_ERROR)
		return res;

	/* find the file */
	path.Append(filename);
	file.SetTo(path.Path(), O_RDONLY);
	if ((res = file.InitCheck()) != B_NO_ERROR)
		return res;

	/* get the version_info */
	if ((res = appinfo.SetTo(&file)) != B_NO_ERROR)
		return res;
	return appinfo.GetVersionInfo(info, B_APP_VERSION_KIND);
}


//	#pragma mark -


ChartWindow::ChartWindow(BRect frame, const char *name)
	: BDirectWindow(frame, name, B_TITLED_WINDOW, 0)
{
	float h, v, h2, v2;
	int32 colors[3];
	BRect r;
	BMenu *menu;
	BButton *button;
	BCheckBox *check_box, *full_screen;
	BMenuItem *item;
	BMenuField *popup;
	BStringView *string;
	BRadioButton *radio;

	// we're not font-sensitive, so we make sure we don't look too ugly
	BFont font;
	if (font.Size() > MAX_FONT_SIZE)
		font.SetSize(MAX_FONT_SIZE);
	BFont boldFont(be_bold_font);
	if (boldFont.Size() > MAX_FONT_SIZE)
		boldFont.SetSize(MAX_FONT_SIZE);

	/* offset the content area frame in window relative coordinate */
	frame.OffsetTo(0.0, 0.0);

	/* init the pattern anti-aliasing tables. */
	InitPatterns();

	/* set window size limits */
	SetSizeLimits(WINDOW_H_MIN, WINDOW_H_MAX, WINDOW_V_MIN, WINDOW_V_MAX);
	SetZoomLimits(WINDOW_H_STD, WINDOW_V_STD);

	/* initial bitmap buffer */
	fOffscreen = NULL;
	fMaxWidth = WINDOW_H_STD - LEFT_WIDTH;
	fMaxHeight = WINDOW_V_STD - TOP_LEFT_LIMIT;

	/* initialise the default setting state */
	for (int32 i = 0; i < 7; i++)
		fCurrentSettings.colors[i] = false;
	fCurrentSettings.colors[1] = true;
	fCurrentSettings.colors[2] = true;
	fCurrentSettings.colors[3] = true;
	fCurrentSettings.fullscreen_mode = WINDOW_MODE;
	fCurrentSettings.special = SPECIAL_NONE;
	fCurrentSettings.display = DISPLAY_OFF;
	fCurrentSettings.animation = ANIMATION_OFF;
	fCurrentSettings.back_color.red = 0;
	fCurrentSettings.back_color.green = 0;
	fCurrentSettings.back_color.blue = 0;
	fCurrentSettings.back_color.alpha = 255;
	fCurrentSettings.star_density = STAR_DENSITY_DEFAULT;
	fCurrentSettings.refresh_rate = REFRESH_RATE_DEFAULT;
	BScreen	screen(this);
	fCurrentSettings.depth	= screen.ColorSpace();
	fCurrentSettings.width = (int32)frame.right+1-LEFT_WIDTH;
	fCurrentSettings.height = (int32)frame.bottom+1-TOP_LEFT_LIMIT;
	fPreviousFullscreenMode = WINDOW_MODE;
	fNextSettings.Set(&fCurrentSettings);

	/* initialise various global parameters */
	fInstantLoadLevel = 0;
	fSecondThreadThreshold = 0.5;
	fLastDynamicDelay = 0.0;
	fCrcAlea = CRC_START;

	/* initialise the starfield and the special structs */
	fStars.list = (star*)malloc(sizeof(star)*STAR_DENSITY_MAX);
	fSpecials.list = (star*)malloc(sizeof(star)*SPECIAL_COUNT_MAX);
	fSpecialList = (special*)malloc(sizeof(special)*SPECIAL_COUNT_MAX);
	InitStars(SPACE_CHAOS);
	fStars.count = fCurrentSettings.star_density;
	fStars.erase_count = 0;
	InitSpecials(SPECIAL_NONE);
	fSpecials.erase_count = 0;
	colors[0] = 1;
	colors[1] = 2;
	colors[2] = 3;
	SetStarColors(colors, 3);

	/* set camera default position and rotation */
	fCameraAlpha = 0.2;
	fCameraTheta = 0.0;
	fCameraPhi = 0.0;
	fCamera.Set(fCameraAlpha, fCameraTheta, fCameraPhi);
	fCameraInvert = fCamera.Transpose();
	fOrigin.x = 0.5;
	fOrigin.y = 0.5;
	fOrigin.z = 0.1;

	/* initialise camera animation */
	fTrackingTarget = -1;
	fSpeed = 0.0115;
	fTargetSpeed = fSpeed;

	/* initialise the view coordinate system */
	InitGeometry();
	SetGeometry(fCurrentSettings.width, fCurrentSettings.height);
	SetCubeOffset();

	/* init the direct buffer in a valid state */
	fDirectBuffer.buffer_width = fCurrentSettings.width;
	fDirectBuffer.buffer_height = fCurrentSettings.height;
	fDirectBuffer.clip_list_count = 1;
	fDirectBuffer.clip_bounds.top = 0;
	fDirectBuffer.clip_bounds.left = 0;
	fDirectBuffer.clip_bounds.right = -1;
	fDirectBuffer.clip_bounds.bottom = -1;
	fDirectBuffer.clip_list[0].top = 0;
	fDirectBuffer.clip_list[0].left = 0;
	fDirectBuffer.clip_list[0].right = -1;
	fDirectBuffer.clip_list[0].bottom = -1;
	fDirectConnected = false;

	/* build the UI content of the window */

	/* top line background */
	r.Set(0.0, 0.0, frame.right, TOP_LEFT_LIMIT - 1);
	fTopView = new BView(r, "top view", B_FOLLOW_LEFT_RIGHT, B_WILL_DRAW);
	fTopView->SetViewColor(background_color);
	AddChild(fTopView);

	h = 2;
	v = V_BORDER;

		/* instant load vue-meter */
		r.Set(h, v, h+INSTANT_LOAD-1, v + (TOP_LEFT_LIMIT - 1 - 2*V_BORDER));
		fInstantLoad = new InstantView(r);
		fTopView->AddChild(fInstantLoad);
		fInstantLoad->SetViewColor(0, 0, 0);

	h += INSTANT_LOAD+H_BORDER;

		/* camera animation popup */
		menu = new BPopUpMenu(B_TRANSLATE("Off"));
		item = new BMenuItem(B_TRANSLATE("Off"), new BMessage(ANIM_OFF_MSG));
		item->SetTarget(this);
		menu->AddItem(item);
		item = new BMenuItem(B_TRANSLATE("Slow rotation"),
			new BMessage(ANIM_SLOW_ROT_MSG));
		item->SetTarget(this);
		menu->AddItem(item);
		item = new BMenuItem(B_TRANSLATE("Slow motion"),
			new BMessage(ANIM_SLOW_MOVE_MSG));
		item->SetTarget(this);
		menu->AddItem(item);
		item = new BMenuItem(B_TRANSLATE("Fast motion"),
			new BMessage(ANIM_FAST_MOVE_MSG));
		item->SetTarget(this);
		menu->AddItem(item);
		item = new BMenuItem(B_TRANSLATE("Free motion"),
			new BMessage(ANIM_FREE_MOVE_MSG));
		item->SetTarget(this);
		menu->AddItem(item);

		r.Set(h, v, h+ANIM_LABEL+ANIM_POPUP-1, v +
			(TOP_LEFT_LIMIT - 1 - 2*V_BORDER));
		popup = new BMenuField(r, "", B_TRANSLATE("Animation:"), menu);
		popup->SetFont(&font);
		popup->MenuBar()->SetFont(&font);
		popup->Menu()->SetFont(&font);
		popup->ResizeToPreferred();
		popup->SetDivider(popup->StringWidth(popup->Label()) + 4.0f);
		fTopView->AddChild(popup);

	h += ANIM_LABEL + ANIM_POPUP +
		 popup->StringWidth(B_TRANSLATE("Slow rotation"));

		/* display mode popup */
		menu = new BPopUpMenu(B_TRANSLATE("Off"));
		item = new BMenuItem(B_TRANSLATE("Off"), new BMessage(DISP_OFF_MSG));
		item->SetTarget(this);
		menu->AddItem(item);
		item = new BMenuItem(B_TRANSLATE("LineArray"),
			new BMessage(DISP_LINE_MSG));
		item->SetTarget(this);
		item->SetEnabled(false);
		menu->AddItem(item);
		item = new BMenuItem(B_TRANSLATE("DrawBitmap"),
			new BMessage(DISP_BITMAP_MSG));
		item->SetTarget(this);
		menu->AddItem(item);
		item = new BMenuItem(B_TRANSLATE("DirectWindow"),
			new BMessage(DISP_DIRECT_MSG));
		item->SetTarget(this);
		item->SetEnabled(BDirectWindow::SupportsWindowMode());
		menu->AddItem(item);

		r.Set(h, v, h+DISP_LABEL+DISP_POPUP-1, v +
			(TOP_LEFT_LIMIT - 1 - 2*V_BORDER));
		popup = new BMenuField(r, "", B_TRANSLATE("Display:"), menu);
		popup->SetFont(&font);
		popup->MenuBar()->SetFont(&font);
		popup->Menu()->SetFont(&font);
		popup->ResizeToPreferred();
		popup->SetDivider(popup->StringWidth(popup->Label()) + 4.0f);
		fTopView->AddChild(popup);

	h += DISP_LABEL + DISP_POPUP +
		popup->StringWidth(B_TRANSLATE("DirectWindow")) + H_BORDER;

		/* create the offwindow (invisible) button on the left side.
		   this will be used to record the content of the Picture
		   button. */
		r.Set(0, 0, BUTTON_WIDTH-1, TOP_LEFT_LIMIT - 1 - 2*V_BORDER);
		fOffwindowButton = new BButton(r, "", "", NULL);
		fOffwindowButton->Hide();
		AddChild(fOffwindowButton);
		fOffwindowButton->ResizeTo(r.Width(), r.Height());

		/* refresh rate picture button */
		r.Set(h, v, h+BUTTON_WIDTH-1, v + (TOP_LEFT_LIMIT - 1 - 2*V_BORDER));
		fRefreshButton = new BPictureButton(r, "",
										  ButtonPicture(false, REFRESH_BUTTON_PICT),
										  ButtonPicture(true, REFRESH_BUTTON_PICT),
										  new BMessage(OPEN_REFRESH_MSG));
		fRefreshButton->SetViewColor(B_TRANSPARENT_32_BIT);
		fRefreshButton->ResizeToPreferred();
		fTopView->AddChild(fRefreshButton);

	h += BUTTON_WIDTH+2*H_BORDER;

		/* background color button */
		r.Set(h, v, h+BUTTON_WIDTH-1, v + (TOP_LEFT_LIMIT - 1 - 2*V_BORDER));
		fColorButton = new BPictureButton(r, "",
										  ButtonPicture(false, COLOR_BUTTON_PICT),
										  ButtonPicture(true, COLOR_BUTTON_PICT),
										  new BMessage(OPEN_COLOR_MSG));
		fColorButton->SetViewColor(B_TRANSPARENT_32_BIT);
		fColorButton->ResizeToPreferred();
		fTopView->AddChild(fColorButton);

	h += BUTTON_WIDTH+2*H_BORDER;

		/* star density button */
		r.Set(h, v, h+BUTTON_WIDTH-1, v + (TOP_LEFT_LIMIT - 1 - 2*V_BORDER));
		fDensityButton = new BPictureButton(r, "",
											ButtonPicture(false, DENSITY_BUTTON_PICT),
											ButtonPicture(true, DENSITY_BUTTON_PICT),
											new BMessage(OPEN_DENSITY_MSG));
		fDensityButton->SetViewColor(B_TRANSPARENT_32_BIT);
		fDensityButton->ResizeToPreferred();
		fTopView->AddChild(fDensityButton);

	h += BUTTON_WIDTH+H_BORDER;

		/* starfield type popup */
		menu = new BPopUpMenu(B_TRANSLATE("Chaos"));
		item = new BMenuItem(B_TRANSLATE("Chaos"),
			new BMessage(SPACE_CHAOS_MSG));
		item->SetTarget(this);
		menu->AddItem(item);
		item = new BMenuItem(B_TRANSLATE("Amas"), new BMessage(SPACE_AMAS_MSG));
		item->SetTarget(this);
		menu->AddItem(item);
		item = new BMenuItem(B_TRANSLATE("Spiral"),
			new BMessage(SPACE_SPIRAL_MSG));
		item->SetTarget(this);
		menu->AddItem(item);

		r.Set(h, v, h+SPACE_LABEL+SPACE_POPUP-1, v +
			(TOP_LEFT_LIMIT - 1 - 2*V_BORDER));
		popup = new BMenuField(r, "", B_TRANSLATE("Space:"), menu);
		popup->SetFont(&font);
		popup->MenuBar()->SetFont(&font);
		popup->Menu()->SetFont(&font);
		popup->ResizeToPreferred();
		popup->SetDivider(popup->StringWidth(popup->Label()) + 4.0f);
		fTopView->AddChild(popup);

	h += SPACE_LABEL+SPACE_POPUP+2*H_BORDER;

	/* left column gray background */
	r.Set(0.0, TOP_LEFT_LIMIT, LEFT_WIDTH - 1, frame.bottom);
	fLeftView = new BView(r, "top view", B_FOLLOW_LEFT | B_FOLLOW_TOP_BOTTOM, B_WILL_DRAW);
	fLeftView->SetViewColor(background_color);
	AddChild(fLeftView);

	h2 = LEFT_OFFSET;
	v2 = LEFT_OFFSET;
	h = h2;
	v = v2;

		/* status box */
		r.Set(h, v, h+LEFT_WIDTH-2*LEFT_OFFSET-2, v+STATUS_BOX-1);
		fStatusBox = new BBox(r);
		fStatusBox->SetFont(&boldFont);
		fStatusBox->SetLabel(B_TRANSLATE("Status"));
		fLeftView->AddChild(fStatusBox);
		float boxWidth, boxHeight;
		fStatusBox->GetPreferredSize(&boxWidth, &boxHeight);
		boxWidth += r.left;

		h = BOX_H_OFFSET;
		v = BOX_V_OFFSET;

			/* frames per second title string */
			r.Set(h, v, h+LEFT_WIDTH-2*LEFT_OFFSET-2*BOX_H_OFFSET-1,
				v+STATUS_LABEL-1);
			string = new BStringView(r, "", B_TRANSLATE("Frames/s"));
			string->SetFont(&font);
			string->SetAlignment(B_ALIGN_CENTER);
			fStatusBox->AddChild(string);

		v += STATUS_LABEL+STATUS_OFFSET;

			/* frames per second display string */
			r.Set(h-1, v, h+LEFT_WIDTH-2*LEFT_OFFSET-2*BOX_H_OFFSET,
				v+STATUS_EDIT-1);
			fFramesView = new BStringView(r, "", "0.0");
			fFramesView->SetAlignment(B_ALIGN_RIGHT);
			fFramesView->SetFont(be_bold_font);
			fFramesView->SetFontSize(24.0);
			fFramesView->SetViewColor(B_TRANSPARENT_32_BIT);
			fStatusBox->AddChild(fFramesView);

		v += STATUS_EDIT+STATUS_OFFSET;

			/* CPU load pourcentage title string */
			r.Set(h, v, h+LEFT_WIDTH-2*LEFT_OFFSET-2*BOX_H_OFFSET-1,
				v+STATUS_LABEL-1);
			string = new BStringView(r, "", B_TRANSLATE("CPU load"));
			string->SetAlignment(B_ALIGN_CENTER);
			string->SetFont(&font);
			fStatusBox->AddChild(string);

		v += STATUS_LABEL+STATUS_OFFSET;

			/* CPU load pourcentage display string */
			r.Set(h-1, v, h+LEFT_WIDTH-2*LEFT_OFFSET-2*BOX_H_OFFSET,
				v+STATUS_EDIT-1);
			fCpuLoadView = new BStringView(r, "", "0.0");
			fCpuLoadView->SetAlignment(B_ALIGN_RIGHT);
			fCpuLoadView->SetFont(be_bold_font);
			fCpuLoadView->SetFontSize(24.0);
			fCpuLoadView->SetViewColor(B_TRANSPARENT_32_BIT);
			fStatusBox->AddChild(fCpuLoadView);

	v2 += STATUS_BOX+LEFT_OFFSET*2;
	h = h2;
	v = v2;

		/* Fullscreen mode check box */
		r.Set(h, v, h+LEFT_WIDTH-2*LEFT_OFFSET-1, v+FULL_SCREEN-1);
		full_screen = new BCheckBox(r, "", B_TRANSLATE("Full screen"),
			new BMessage(FULL_SCREEN_MSG));
		full_screen->SetTarget(this);
		full_screen->SetFont(&font);
		full_screen->ResizeToPreferred();

		float width, height;
		full_screen->GetPreferredSize(&width, &height);
		boxWidth = max_c(width + r.left, boxWidth);
		fLeftView->AddChild(full_screen);

	v2 += FULL_SCREEN+LEFT_OFFSET*2;
	h = h2;
	v = v2;

		/* Automatic demonstration activation button */
		r.Set(h, v, h+LEFT_WIDTH-2*LEFT_OFFSET-1, v+AUTO_DEMO-1);
		button = new BButton(r, "", B_TRANSLATE("Auto demo"),
			new BMessage(AUTO_DEMO_MSG));
		button->SetTarget(this);
		button->ResizeToPreferred();
		button->GetPreferredSize(&width, &height);
		boxWidth = max_c(width + r.left, boxWidth);
		fLeftView->AddChild(button);

	v2 += AUTO_DEMO+LEFT_OFFSET*2;
	h = h2;
	v = v2;

		/* Enabling second thread check box */
		r.Set(h, v, h+LEFT_WIDTH-2*LEFT_OFFSET-1, v+SECOND_THREAD-1);
		check_box = new BCheckBox(r, "", B_TRANSLATE("2 threads"),
			new BMessage(SECOND_THREAD_MSG));
		check_box->SetTarget(this);
		check_box->SetFont(&font);
		check_box->ResizeToPreferred();
		fLeftView->AddChild(check_box);

	v2 += SECOND_THREAD+LEFT_OFFSET*2 + 2;
	h = h2;
	v = v2;

		/* Star color selection box */
		r.Set(h, v, h+LEFT_WIDTH-2*LEFT_OFFSET-2, v+COLORS_BOX-1);
		fColorsBox = new BBox(r);
		fColorsBox->SetLabel(B_TRANSLATE("Colors"));
		fColorsBox->SetFont(&boldFont);
		fLeftView->AddChild(fColorsBox);

		h = BOX_H_OFFSET;
		v = BOX_V_OFFSET;

			/* star color red check box */
			r.Set(h, v, h+LEFT_WIDTH-2*LEFT_OFFSET-2*BOX_H_OFFSET-1,
				v+COLORS_LABEL-1);
			check_box = new BCheckBox(r, "", B_TRANSLATE("Red"),
				new BMessage(COLORS_RED_MSG));
			check_box->SetFont(&font);
			check_box->ResizeToPreferred();
			fColorsBox->AddChild(check_box);

		v += COLORS_LABEL+COLORS_OFFSET;

			/* star color green check box */
			r.Set(h, v, h+LEFT_WIDTH-2*LEFT_OFFSET-2*BOX_H_OFFSET-1,
				v+COLORS_LABEL-1);
			check_box = new BCheckBox(r, "", B_TRANSLATE("Green"),
				new BMessage(COLORS_GREEN_MSG));
			check_box->SetValue(1);
			check_box->SetFont(&font);
			check_box->ResizeToPreferred();
			fColorsBox->AddChild(check_box);

		v += COLORS_LABEL+COLORS_OFFSET;

			/* star color blue check box */
			r.Set(h, v, h+LEFT_WIDTH-2*LEFT_OFFSET-2*BOX_H_OFFSET-1,
				v+COLORS_LABEL-1);
			check_box = new BCheckBox(r, "", B_TRANSLATE("Blue"),
				new BMessage(COLORS_BLUE_MSG));
			check_box->SetValue(1);
			check_box->SetFont(&font);
			check_box->ResizeToPreferred();
			fColorsBox->AddChild(check_box);

		v += COLORS_LABEL+COLORS_OFFSET;

			/* star color yellow check box */
			r.Set(h, v, h+LEFT_WIDTH-2*LEFT_OFFSET-2*BOX_H_OFFSET-1,
				v+COLORS_LABEL-1);
			check_box = new BCheckBox(r, "", B_TRANSLATE("Yellow"),
				new BMessage(COLORS_YELLOW_MSG));
			check_box->SetValue(1);
			check_box->SetFont(&font);
			check_box->ResizeToPreferred();
			fColorsBox->AddChild(check_box);

		v += COLORS_LABEL+COLORS_OFFSET;

			/* star color orange check box */
			r.Set(h, v, h+LEFT_WIDTH-2*LEFT_OFFSET-2*BOX_H_OFFSET-1,
				v+COLORS_LABEL-1);
			check_box = new BCheckBox(r, "", B_TRANSLATE("Orange"),
				new BMessage(COLORS_ORANGE_MSG));
			check_box->SetFont(&font);
			check_box->ResizeToPreferred();
			fColorsBox->AddChild(check_box);

		v += COLORS_LABEL+COLORS_OFFSET;

			/* star color pink check box */
			r.Set(h, v, h+LEFT_WIDTH-2*LEFT_OFFSET-2*BOX_H_OFFSET-1,
				v+COLORS_LABEL-1);
			check_box = new BCheckBox(r, "", B_TRANSLATE("Pink"),
				new BMessage(COLORS_PINK_MSG));
			check_box->SetFont(&font);
			check_box->ResizeToPreferred();
			fColorsBox->AddChild(check_box);

		v += COLORS_LABEL+COLORS_OFFSET;

			/* star color white check box */
			r.Set(h, v, h+LEFT_WIDTH-2*LEFT_OFFSET-2*BOX_H_OFFSET-1,
				v+COLORS_LABEL-1);
			check_box = new BCheckBox(r, "", B_TRANSLATE("White"),
				new BMessage(COLORS_WHITE_MSG));
			check_box->SetFont(&font);
			check_box->ResizeToPreferred();
			fColorsBox->AddChild(check_box);

	v2 += COLORS_BOX+LEFT_OFFSET*2;
	h = h2;
	v = v2;

		/* Special type selection box */
		r.Set(h, v, h+LEFT_WIDTH-2*LEFT_OFFSET-2, v+SPECIAL_BOX-1);
		fSpecialBox = new BBox(r);
		fSpecialBox->SetFont(&boldFont);
		fSpecialBox->SetLabel(B_TRANSLATE("Special"));
		fLeftView->AddChild(fSpecialBox);

		h = BOX_H_OFFSET;
		v = BOX_V_OFFSET;

			/* no special radio button */
			r.Set(h, v, h+LEFT_WIDTH-2*LEFT_OFFSET-2*BOX_H_OFFSET-1,
				v+COLORS_LABEL-1);
			radio = new BRadioButton(r, "", B_TRANSLATE("None"),
				new BMessage(SPECIAL_NONE_MSG));
			radio->SetValue(1);
			radio->SetFont(&font);
			radio->ResizeToPreferred();
			fSpecialBox->AddChild(radio);

		v += COLORS_LABEL+COLORS_OFFSET;

			/* comet special animation radio button */
			r.Set(h, v, h+LEFT_WIDTH-2*LEFT_OFFSET-2*BOX_H_OFFSET-1,
				v+COLORS_LABEL-1);
			radio = new BRadioButton(r, "", B_TRANSLATE("Comet"),
				new BMessage(SPECIAL_COMET_MSG));
			radio->SetFont(&font);
			radio->ResizeToPreferred();
			fSpecialBox->AddChild(radio);

		v += COLORS_LABEL+COLORS_OFFSET;

			/* novas special animation radio button */
			r.Set(h, v, h+LEFT_WIDTH-2*LEFT_OFFSET-2*BOX_H_OFFSET-1,
				v+COLORS_LABEL-1);
			radio = new BRadioButton(r, "", B_TRANSLATE("Novas"),
				new BMessage(SPECIAL_NOVAS_MSG));
			radio->SetFont(&font);
			radio->ResizeToPreferred();
			fSpecialBox->AddChild(radio);

		v += COLORS_LABEL+COLORS_OFFSET;

			/* space batle special animation radio button (not implemented) */
			r.Set(h, v, h+LEFT_WIDTH-2*LEFT_OFFSET-2*BOX_H_OFFSET-1,
				v+COLORS_LABEL-1);
			radio = new BRadioButton(r, "", B_TRANSLATE("Battle"),
				new BMessage(SPECIAL_BATTLE_MSG));
			radio->SetEnabled(false);
			radio->SetFont(&font);
			radio->ResizeToPreferred();
			fSpecialBox->AddChild(radio);

	// Note: direct window mode uses LEFT_WIDTH to calculate
	// the left border of the animation view, so we use it here too.
	//fLeftView->ResizeTo(max_c(boxWidth + 2, fLeftView->Bounds().Width()), fLeftView->Bounds().Height());
	/* animation area */
	r.Set(LEFT_WIDTH, TOP_LEFT_LIMIT, frame.right, frame.bottom);
	fChartView = new ChartView(r);
	fChartView->SetViewColor(0, 0, 0);
	AddChild(fChartView);

	/* allocate the semaphores */
	fDrawingLock = create_sem(1, "chart locker");
	fSecondThreadLock = create_sem(0, "chart second locker");
	fSecondThreadRelease = create_sem(0, "chart second release");

	/* spawn the asynchronous animation threads */
	fKillThread = false;
	fAnimationThread = spawn_thread(ChartWindow::Animation, "chart animation",
								B_NORMAL_PRIORITY,
								(void*)this);

	fSecondAnimationThread = spawn_thread(ChartWindow::Animation2, "chart animation2",
								B_NORMAL_PRIORITY,
								(void*)this);
	resume_thread(fSecondAnimationThread);
	resume_thread(fAnimationThread);
}


ChartWindow::~ChartWindow()
{
	status_t result;

	/* setting this flag force both animation threads to quit */
	fKillThread = true;

	/* wait for the two animation threads to quit */
	wait_for_thread(fAnimationThread, &result);
	wait_for_thread(fSecondAnimationThread, &result);

	/* free the offscreen bitmap if any */
	delete fOffscreen;

	/* release the semaphores used for synchronisation */
	delete_sem(fDrawingLock);
	delete_sem(fSecondThreadLock);
	delete_sem(fSecondThreadRelease);

	/* free the buffers used to store the starlists */
	free(fStars.list);
	free(fSpecials.list);
	free(fSpecialList);
}


//	#pragma mark Standard window members


bool
ChartWindow::QuitRequested()
{
	be_app->PostMessage(B_QUIT_REQUESTED);
	return BWindow::QuitRequested();
}


void
ChartWindow::MessageReceived(BMessage *message)
{
	int32			index, color;
	BHandler		*handler;
	BCheckBox		*check_box;
	BSlider			*slider;

	message->FindPointer("source", (void**)&handler);
	switch(message->what) {
		/* This is a key part of the architecture. MessageReceived is
		   called whenever the user interact with a UI element to change
		   a setting. The window is locked at this point, so changing
		   the setting of the engine at that point would be dangerous.
		   We could easily goofed and create a bad dependencies between
		   the Window locking mechanism and DirectConnected, that
		   would generate a deadlock and force the app_server to kill
		   the application. Bad business. To avoid that, we keep two
		   different engine setting. One that is currently used by the
		   animation engine, the other one that will retain all the
		   changes generated by the user until the engine is ready to
		   use them. So message received will write into that setting
		   state and the engine will read it from time to time. Both
		   access can be done asynchronously as all intermediate state
		   generated by the MessageReceived write are valid (we don't
		   need to make those transactions atomic). */
		case ANIM_OFF_MSG :
		case ANIM_SLOW_ROT_MSG :
		case ANIM_SLOW_MOVE_MSG :
		case ANIM_FAST_MOVE_MSG :
		case ANIM_FREE_MOVE_MSG :
			fNextSettings.animation = ANIMATION_OFF + (message->what - ANIM_OFF_MSG);
			break;
		case DISP_OFF_MSG :
		case DISP_BITMAP_MSG :
		case DISP_DIRECT_MSG :
			fNextSettings.display = DISPLAY_OFF + (message->what - DISP_OFF_MSG);
			break;
		case SPACE_CHAOS_MSG :
		case SPACE_AMAS_MSG :
		case SPACE_SPIRAL_MSG :
			fNextSettings.space_model = SPACE_CHAOS + (message->what - SPACE_CHAOS_MSG);
			break;
		case FULL_SCREEN_MSG :
			check_box = dynamic_cast<BCheckBox*>(handler);
			if (check_box == NULL)
				break;

			if (check_box->Value())
				fNextSettings.fullscreen_mode = FULLSCREEN_MODE;
			else
				fNextSettings.fullscreen_mode = WINDOW_MODE;
			break;
		case AUTO_DEMO_MSG :
			fNextSettings.fullscreen_mode = FULLDEMO_MODE;
			fNextSettings.animation = ANIMATION_FREE_MOVE;
			fNextSettings.special = SPECIAL_COMET;
			LaunchSound();
			break;
		case BACK_DEMO_MSG :
			fNextSettings.fullscreen_mode = fPreviousFullscreenMode;
			break;
		case SECOND_THREAD_MSG :
			check_box = dynamic_cast<BCheckBox*>(handler);
			if (check_box == NULL)
				break;

			fNextSettings.second_thread =  (check_box->Value()?true:false);
			break;
		case COLORS_RED_MSG :
		case COLORS_GREEN_MSG :
		case COLORS_BLUE_MSG :
		case COLORS_YELLOW_MSG :
		case COLORS_ORANGE_MSG :
		case COLORS_PINK_MSG :
		case COLORS_WHITE_MSG :
			index = message->what - COLORS_RED_MSG;
			check_box = dynamic_cast<BCheckBox*>(handler);
			if (check_box == NULL)
				break;

			fNextSettings.colors[index] = (check_box->Value()?true:false);
			break;
		case SPECIAL_NONE_MSG :
		case SPECIAL_COMET_MSG :
		case SPECIAL_NOVAS_MSG :
		case SPECIAL_BATTLE_MSG :
			fNextSettings.special = SPECIAL_NONE + (message->what - SPECIAL_NONE_MSG);
			break;
		case COLOR_PALETTE_MSG :
			message->FindInt32("be:value", &color);
			fNextSettings.back_color.red = (color >> 24);
			fNextSettings.back_color.green = (color >> 16);
			fNextSettings.back_color.blue = (color >> 8);
			fNextSettings.back_color.alpha = color;
			break;
		case STAR_DENSITY_MSG :
			slider = dynamic_cast<BSlider*>(handler);
			if (slider == NULL)
				break;

			fNextSettings.star_density = slider->Value();
			break;
		case REFRESH_RATE_MSG :
			slider = dynamic_cast<BSlider*>(handler);
			if (slider == NULL)
				break;

			fNextSettings.refresh_rate = exp(slider->Value()*0.001*(log(REFRESH_RATE_MAX/REFRESH_RATE_MIN)))*
									REFRESH_RATE_MIN;
			break;
		/* open the three floating window used to do live setting of
		   some advanced parameters. Those windows will return live
		   feedback that will be executed by some of the previous
		   messages. */
		case OPEN_COLOR_MSG :
			OpenColorPalette(BPoint(200.0, 200.0));
			break;
		case OPEN_DENSITY_MSG :
			OpenStarDensity(BPoint(280.0, 280.0));
			break;
		case OPEN_REFRESH_MSG :
			OpenRefresh(BPoint(240.0, 340.0));
			break;
		/* let other messages pass through... */
		default :
			BDirectWindow::MessageReceived(message);
			break;
	}
}


void
ChartWindow::ScreenChanged(BRect screen_size, color_space depth)
{
	/* this is the same principle than the one described for
	   MessageReceived, to inform the engine that the depth of
	   the screen changed (needed only for offscreen bitmap.
	   In DirectWindow, you get a direct notification). */
	fNextSettings.depth = BScreen(this).ColorSpace();
}


void
ChartWindow::FrameResized(float new_width, float new_height)
{
	/* this is the same principle than the one described for
	   MessageReceived, to inform the engine that the window
	   size changed (needed only for offscreen bitmap. In
	   DirectWindow, you get a direct notification). */
	fNextSettings.width = (int32)Frame().Width()+1-LEFT_WIDTH;
	fNextSettings.height = (int32)Frame().Height()+1-TOP_LEFT_LIMIT;
}


//	#pragma mark User Interface related stuff...


/* loop through the window list of the application, looking for
   a window with a specified name. */
BWindow	*
ChartWindow::GetAppWindow(const char *name)
{
	int32		index;
	BWindow		*window;

	for (index = 0;; index++) {
		window = be_app->WindowAt(index);
		if (window == NULL)
			break;
		if (window->LockWithTimeout(200000) == B_OK) {
			if (strcmp(window->Name(), name) == 0) {
				window->Unlock();
				break;
			}
			window->Unlock();
		}
	}
	return window;
}

/* this function return a picture (in active or inactive state) of
   a standard BButton with some specific content draw in the middle.
   button_type indicate what special content should be used. */
BPicture *
ChartWindow::ButtonPicture(bool active, int32 button_type)
{
	char		word[6];
	int32		value;
	BRect		r;
	BPoint		delta;
	BPicture	*pict;


	/* create and open the picture */
	pict = new BPicture();
	r = fOffwindowButton->Bounds();
	fOffwindowButton->SetValue(active);
	fOffwindowButton->BeginPicture(pict);
	/* draw the standard BButton in whatever state is required. */
	fOffwindowButton->Draw(r);
	if (button_type == COLOR_BUTTON_PICT) {
		/* this button just contains a rectangle of the current background
		   color, with a one pixel black border. */
		r.InsetBy(6.0, 4.0);
		fOffwindowButton->SetHighColor(0, 0, 0);
		fOffwindowButton->StrokeRect(r);
		r.InsetBy(1.0, 1.0);
		fOffwindowButton->SetHighColor(fCurrentSettings.back_color);
		fOffwindowButton->FillRect(r);
	}
	else if (button_type == DENSITY_BUTTON_PICT) {
		/* this button just contains a big string (using a bigger font size
		   than what a standard BButton would allow) with the current value
		   of the star density pourcentage. */
		value = (fCurrentSettings.star_density*100 + STAR_DENSITY_MAX/2) / STAR_DENSITY_MAX;
		sprintf(word, "%3ld", value);
	draw_string:
		fOffwindowButton->SetFont(be_bold_font);
		fOffwindowButton->SetFontSize(14.0);
		delta.x = BUTTON_WIDTH/2-(fOffwindowButton->StringWidth(word) * 0.5);
		delta.y = (TOP_LEFT_LIMIT-2*V_BORDER)/2 + 6.0;
		fOffwindowButton->DrawString(word, delta);
	}
	else {
		/* this button just contains a big string (using a bigger font size
		   than what a standard BButton would allow) with the current value
		   of the target refresh rate in frames per second. */
		sprintf(word, "%3.1f", fCurrentSettings.refresh_rate + 0.05);
		goto draw_string;
	}
	/* close and return the picture */
	return fOffwindowButton->EndPicture();
}

/* Create a floating window including a slightly modified version of
   BColorControl, ChartColorControl, that will return live feedback
   as the same time the user will change the color setting of the
   background. */
void
ChartWindow::OpenColorPalette(BPoint here)
{
	BRect frame;
	BPoint point;

	BWindow *window = GetAppWindow(B_TRANSLATE("Space color"));
	if (window == NULL) {
		frame.Set(here.x, here.y, here.x + 199.0, here.y + 99.0);
		window = new BWindow(frame, B_TRANSLATE("Space color"),
							 B_FLOATING_WINDOW_LOOK,
							 B_FLOATING_APP_WINDOW_FEEL,
							 B_NOT_ZOOMABLE | B_WILL_ACCEPT_FIRST_CLICK | B_NOT_RESIZABLE);
		point.Set(0, 0);
		BColorControl *colorControl = new ChartColorControl(point,
			new BMessage(COLOR_PALETTE_MSG));
		colorControl->SetViewColor(background_color);
		colorControl->SetTarget(NULL, this);
		colorControl->SetValue(fCurrentSettings.back_color);
		colorControl->ResizeToPreferred();
		window->ResizeTo(colorControl->Bounds().Width(), colorControl->Bounds().Height());
		window->AddChild(colorControl);
		window->Show();
	}
	window->Activate();
}

/* Create a floating window including a BSlider, that will return
   live feedback when the user will change the star density of the
   starfield */
void
ChartWindow::OpenStarDensity(BPoint here)
{
	BWindow	*window = GetAppWindow(B_TRANSLATE("Star density"));
	if (window == NULL) {
		BRect frame(here.x, here.y, here.x + STAR_DENSITY_H-1,
			here.y + STAR_DENSITY_V-1);
		window = new BWindow(frame, B_TRANSLATE("Star density"),
							 B_FLOATING_WINDOW_LOOK,
							 B_FLOATING_APP_WINDOW_FEEL,
							 B_NOT_RESIZABLE | B_NOT_ZOOMABLE | B_WILL_ACCEPT_FIRST_CLICK);
		frame.OffsetTo(0.0, 0.0);
		BSlider	*slider = new BSlider(frame, "", NULL, new BMessage(STAR_DENSITY_MSG),
							 STAR_DENSITY_MIN, STAR_DENSITY_MAX);
		slider->SetViewColor(background_color);
		slider->SetTarget(NULL, this);
		slider->SetValue(fCurrentSettings.star_density);
		slider->SetModificationMessage(new BMessage(STAR_DENSITY_MSG));
		slider->SetLimitLabels(B_TRANSLATE(" 5% (low)"),
			B_TRANSLATE("(high) 100% "));
		slider->ResizeToPreferred();
		window->ResizeTo(slider->Bounds().Width(), slider->Bounds().Height());
		window->AddChild(slider);
		window->Show();
	}
	window->Activate();
}

/* Create a floating window including a BSlider, that will return
   live feedback when the user will change the target refresh rate
   of the animation */
void
ChartWindow::OpenRefresh(BPoint here)
{
	BWindow *window = GetAppWindow(B_TRANSLATE("Refresh rate"));
	if (window == NULL) {
		BRect frame(here.x, here.y, here.x + REFRESH_RATE_H-1,
			here.y + REFRESH_RATE_V-1);
		window = new BWindow(frame, B_TRANSLATE("Refresh rate"),
							 B_FLOATING_WINDOW_LOOK,
							 B_FLOATING_APP_WINDOW_FEEL,
							 B_NOT_RESIZABLE | B_NOT_ZOOMABLE | B_WILL_ACCEPT_FIRST_CLICK);
		frame.OffsetTo(0.0, 0.0);
		BSlider *slider = new BSlider(frame, "", NULL, new BMessage(REFRESH_RATE_MSG), 0, 1000);
		slider->SetViewColor(background_color);
		slider->SetTarget(NULL, this);
		slider->SetValue((int32)(1000 * log(fCurrentSettings.refresh_rate / REFRESH_RATE_MIN) /
						log(REFRESH_RATE_MAX/REFRESH_RATE_MIN)));
		slider->SetModificationMessage(new BMessage(REFRESH_RATE_MSG));
		slider->SetLimitLabels(B_TRANSLATE(" 0.6 f/s  (logarythmic scale)"),
			B_TRANSLATE("600.0 f/s"));
		slider->ResizeToPreferred();
		window->ResizeTo(slider->Bounds().Width(), slider->Bounds().Height());
		window->AddChild(slider);
		window->Show();
	}
	window->Activate();
}

/* This update the state of the frames per second vue-meter in a lazy way. */
void
ChartWindow::DrawInstantLoad(float frame_per_second)
{
	int32 i;
	bigtime_t	timeout;

	int32 level = (int32)((frame_per_second + 6.0) * (1.0/12.0));
	if (level > 50)
		level = 50;

	/* if the load level is inchanged, nothing more to do... */
	if (level == fInstantLoadLevel)
		return;

	/* We need to lock the window to be able to draw that. But as some
	   BControl are still synchronous, if the user is still tracking them,
	   the window can stay block for a long time. It's not such a big deal
	   when using the offscreen buffer as we won't be able to draw it in
	   any case. But in DirectWindow mode, we're not limited by that so
	   it would be stupid to block the engine loop here. That's why in
	   that case, we will try to lock the window with a timeout of 0us. */
	if (fCurrentSettings.display == DISPLAY_BITMAP)
		timeout = 100000;
	else
		timeout = 0;
	if (LockWithTimeout(timeout) != B_OK)
		return;

	/* the new level is higher than the previous. We need to draw more
	   colored bars. */
	if (level > fInstantLoadLevel) {
		for (i = fInstantLoadLevel; i < level; i++) {
			if (i < fInstantLoad->step)
				fInstantLoad->SetHighColor(255, 90, 90);
			else if ((i / fInstantLoad->step) & 1)
				fInstantLoad->SetHighColor(90, 255, 90);
			else
				fInstantLoad->SetHighColor(40, 200, 40);
			fInstantLoad->FillRect(BRect(3 + i * 4, 2, 5 + i * 4, 19));
		}
	}
	/* the level is lower than before, we need to erase some bars. */
	else {
		fInstantLoad->SetHighColor(0, 0, 0);
		for (i = level; i < fInstantLoadLevel; i++)
			fInstantLoad->FillRect(BRect(3 + i * 4, 2, 5 +i * 4, 19));
	}
	/* we want that drawing to be completed as soon as possible */
	Flush();

	fInstantLoadLevel = level;
	Unlock();
}


void
ChartWindow::PrintStatNumbers(float fps)
{
	char		text_frames[6];
	char		text_cpu_load[6];
	float		frame_rate, load;
	bigtime_t	timeout;

	/* rules use to determine the stat numbers : if the target framerate
	   is greater than the simulate one, then we consider that 100.0 cpu
	   was used, and we only got the simulate framerate. */
	if (fps <= fCurrentSettings.refresh_rate) {
		load = 100.0;
		frame_rate = fps + 0.05;
	}
	/* if the target framerate is less than the simulate one, then we
	   calculate what fraction of the cpu would have been required to
	   deliver the target framerate, and we said that the target framerate
	   was delivered. */
	else {
		load = (100.0*fCurrentSettings.refresh_rate)/fps + 0.05;
		frame_rate = fCurrentSettings.refresh_rate + 0.05;
	}

	/* convert numbers in strings */
	sprintf(text_cpu_load, "%3.1f", load);
	sprintf(text_frames, "%3.1f", frame_rate);

	/* same remark than for DrawInstantLoad. We want to avoid to
	   block if using DirectWindow mode. */
	if (fCurrentSettings.display == DISPLAY_BITMAP)
		timeout = 100000;
	else
		timeout = 0;

	if (LockWithTimeout(timeout) == B_OK) {
		fFramesView->SetText(text_frames);
		fCpuLoadView->SetText(text_cpu_load);
		Unlock();
	}
}


//	#pragma mark Engine setting related functions.


void
ChartWindow::InitGeometry()
{
	/* calculate some parameters required for the 3d processing */
	float dz = sqrt(1.0 - (DH_REF*DH_REF + DV_REF*DV_REF) * (0.5 + 0.5/Z_CUT_RATIO) * (0.5 + 0.5/Z_CUT_RATIO));
	fDepthRef = dz / (1.0 - 1.0/Z_CUT_RATIO);

	/* set the position of the pyramid of vision, so that it was always
	   possible to include it into a 1x1x1 cube parallel to the 3 main
	   axis. */
	fGeometry.z_max = fDepthRef;
	fGeometry.z_min = fDepthRef/Z_CUT_RATIO;

	/* used for lighting processing */
	fGeometry.z_max_square = fGeometry.z_max * fGeometry.z_max;

	/* preprocess that for the fast clipping based on the pyramid of vision */
	fGeometry.xz_max = (0.5*DH_REF)/fGeometry.z_max;
	fGeometry.xz_min = -fGeometry.xz_max;
	fGeometry.yz_max = (0.5*DV_REF)/fGeometry.z_max;
	fGeometry.yz_min = -fGeometry.yz_max;
}

/* second part of the asynchronous setting mechanism. This will be
   called once during every loop of the animation engine, at a time
   when the engine is not using the setting for realtime processing.
   Each setting will be checked for potential change, and action
   will be taken if needed. The window can be locked at that time
   because the structure of the animation engine loop guarantees
   that DirectConnected can not stay blocked at the same time that
   this method is executed. */
void
ChartWindow::ChangeSetting(setting new_set)
{
	int32			i, color_count, old_step;
	int32			color_index[7];

	/* check for change of window/fullscreen/fullscreen demo mode */
	if (fCurrentSettings.fullscreen_mode != new_set.fullscreen_mode) {
		switch (new_set.fullscreen_mode) {
		case WINDOW_MODE :
			fPreviousFullscreenMode = WINDOW_MODE;
			ResizeTo(fPreviousFrame.Width(), fPreviousFrame.Height());
			MoveTo(fPreviousFrame.left, fPreviousFrame.top);
			break;
		case FULLSCREEN_MODE :
			{
				fPreviousFullscreenMode = FULLSCREEN_MODE;
				if (fCurrentSettings.fullscreen_mode == WINDOW_MODE)
					fPreviousFrame = Frame();
				BScreen	a_screen(this);
				MoveTo(a_screen.Frame().left, a_screen.Frame().top);
				ResizeTo(a_screen.Frame().Width(), a_screen.Frame().Height());
			}
			break;
		case FULLDEMO_MODE :
			{
				fPreviousFullscreenMode = fCurrentSettings.fullscreen_mode;
				if (fCurrentSettings.fullscreen_mode == WINDOW_MODE)
					fPreviousFrame = Frame();
				BScreen	b_screen(this);
				ResizeTo(b_screen.Frame().Width() + LEFT_WIDTH, b_screen.Frame().Height() + TOP_LEFT_LIMIT);
				MoveTo(b_screen.Frame().left - LEFT_WIDTH, b_screen.Frame().top - TOP_LEFT_LIMIT);
			}
			break;
		}
	}

	/* check for change in the target refresh rate */
	if (fCurrentSettings.refresh_rate != new_set.refresh_rate) {
		fCurrentSettings.refresh_rate = new_set.refresh_rate;
		old_step = fInstantLoad->step;
		fInstantLoad->step = (int32)((fCurrentSettings.refresh_rate+6.0)/12.0);
		if (fInstantLoad->step < 1)
			fInstantLoad->step = 1;
		if (LockWithTimeout(200000) == B_OK) {
			if (old_step != fInstantLoad->step)
				fInstantLoad->Invalidate();
			fRefreshButton->SetEnabledOff(ButtonPicture(false, REFRESH_BUTTON_PICT));
			fRefreshButton->SetEnabledOn(ButtonPicture(true, REFRESH_BUTTON_PICT));
			fRefreshButton->Invalidate();
			Unlock();
		}
		if (fCurrentSettings.animation != ANIMATION_OFF)
			fFrameDelay = (bigtime_t)(1000000.0/new_set.refresh_rate);
	}

	/* check for change in the star colors list */
	for (i=0; i<7; i++)
		if (fCurrentSettings.colors[i] != new_set.colors[i]) {
			/* if any, get the list of usable color index... */
			color_count = 0;
			for (i=0; i<7; i++)
				if (new_set.colors[i])
					color_index[color_count++] = i;
			/* check that at least one color is enabled */
			if (color_count == 0)
				color_index[color_count++] = 6;
			/* set a new color distribution in the starfield */
			SetStarColors(color_index, color_count);
			break;
		}

	/* check for change of the special effect setting */
	if (new_set.special != fCurrentSettings.special)
		InitSpecials(new_set.special);

	/* check for change of the display method */
	if (new_set.display != fCurrentSettings.display) {
		if (new_set.display == DISPLAY_BITMAP) {
			/* check the settings of the offscreen bitmap */
			CheckBitmap(new_set.depth, new_set.width, new_set.height);
			/* synchronise the camera geometry and the offscreen buffer geometry */
			SetGeometry(fBitmapBuffer.buffer_width, fBitmapBuffer.buffer_height);
			/* reset the offscreen background and cancel the erasing */
			SetBitmapBackGround();
			fStars.erase_count = 0;
			fSpecials.erase_count = 0;
		}
		if (new_set.display == DISPLAY_DIRECT) {
			/* this need to be atomic in regard of DirectConnected */
			while (acquire_sem(fDrawingLock) == B_INTERRUPTED)
				;
			/* synchronise the camera geometry and the direct buffer geometry */
			SetGeometry(fDirectBuffer.buffer_width, fDirectBuffer.buffer_height);
			/* cancel erasing of stars not in visible part of the direct window */
			RefreshClipping(&fDirectBuffer, &fStars);
			RefreshClipping(&fDirectBuffer, &fSpecials);
			release_sem(fDrawingLock);
		}
	}

	/* check for change of the animation mode. */
	if (new_set.animation != fCurrentSettings.animation) {
		/* when there is no camera animation, we loop only
		   10 times per second. */
		if (new_set.animation == ANIMATION_OFF)
			fFrameDelay = 100000;
		else
			fFrameDelay = (bigtime_t)(1000000.0/new_set.refresh_rate);
		/* reset the free camera animation context for a fresh start */
		if (new_set.animation == ANIMATION_FREE_MOVE) {
			fDynamicAlpha = 0.0;
			fDynamicTheta = 0.0;
			fDynamicPhi = 0.0;
			fCountAlpha = 0;
			fCountTheta = 0;
			fCountPhi = 0;
		}
	}

	/* check for change of starfield model */
	if (new_set.space_model != fCurrentSettings.space_model) {
		/* Generate a new starfield. Also reset the special animation */
		InitStars(new_set.space_model);
		InitSpecials(new_set.special);
	}

	/* check for change of the background color */
	if ((new_set.back_color.red != fCurrentSettings.back_color.red) ||
		(new_set.back_color.green != fCurrentSettings.back_color.green) ||
		(new_set.back_color.blue != fCurrentSettings.back_color.blue)) {
		if (LockWithTimeout(200000) == B_OK) {
			BScreen		screen(this);
			/* set the background color and it's 8 bits index equivalent */
			fCurrentSettings.back_color = new_set.back_color;
			fBackColorIndex = screen.IndexForColor(new_set.back_color);
			/* set the nackground color of the view (directwindow mode) */
			fChartView->SetViewColor(new_set.back_color);
			/* change the color of the picture button used in the UI */
			fColorButton->SetEnabledOff(ButtonPicture(false, COLOR_BUTTON_PICT));
			fColorButton->SetEnabledOn(ButtonPicture(true, COLOR_BUTTON_PICT));
			fColorButton->Invalidate();
			/* update all dependencies in the offscreen buffer descriptor */
			SetColorSpace(&fBitmapBuffer, fBitmapBuffer.depth);
			/* update all dependencies in the directwindow buffer descriptor */
			while (acquire_sem(fDrawingLock) == B_INTERRUPTED)
				;
			SetColorSpace(&fDirectBuffer, fDirectBuffer.depth);
			release_sem(fDrawingLock);
			/* in offscreen mode, erase the background and cancel star erasing */
			if (new_set.display == DISPLAY_BITMAP) {
				SetBitmapBackGround();
				fStars.erase_count = 0;
				fSpecials.erase_count = 0;
			}
			/* in directwindow mode, just force an update */
			else
				fChartView->Invalidate();
			Unlock();
		}
	}

	/* check for change of the star animation density */
	if (new_set.star_density != fCurrentSettings.star_density) {
		if (LockWithTimeout(200000) == B_OK) {
			fCurrentSettings.star_density = new_set.star_density;
			/* change the picture button used in the UI */
			fDensityButton->SetEnabledOff(ButtonPicture(false, DENSITY_BUTTON_PICT));
			fDensityButton->SetEnabledOn(ButtonPicture(true, DENSITY_BUTTON_PICT));
			fDensityButton->Invalidate();
			Unlock();
		}
		fStars.count = new_set.star_density;
	}

	/* check for change in the buffer format for the offscreen bitmap.
	   DirectWindow depth change are always handle in realtime */
	if (new_set.depth != fCurrentSettings.depth) {
		CheckBitmap(new_set.depth, new_set.width, new_set.height);
		/* need to reset the buffer if it's currently used for display */
		if (new_set.display == DISPLAY_BITMAP) {
			SetBitmapBackGround();
			fStars.erase_count = 0;
			fSpecials.erase_count = 0;
		}
	}

	/* check for change in the drawing area of the offscreen bitmap */
	if ((new_set.width != fCurrentSettings.width) || (new_set.height != fCurrentSettings.height)) {
		CheckBitmap(new_set.depth, new_set.width, new_set.height);
		fBitmapBuffer.buffer_width = new_set.width;
		fBitmapBuffer.buffer_height = new_set.height;
		if (new_set.display == DISPLAY_BITMAP)
			SetGeometry(fBitmapBuffer.buffer_width, fBitmapBuffer.buffer_height);
		SetBitmapClipping(new_set.width, new_set.height);
	}

	/* copy the new state as the new current state */
	fCurrentSettings.Set(&new_set);
}

/* Initialise the starfield in the different modes */
void
ChartWindow::InitStars(int32 space_model)
{
	star		*s;
	int32		step;
	int32		amas_select[32];
	float		dx, dy, dz, dist, fact, alpha, r;
	float		factor[8];
	uint32		i, index, i_step;
	TPoint		amas[8];

	switch (space_model) {
	/* Create a random starfield */
	case SPACE_CHAOS :
		FillStarList(fStars.list, STAR_DENSITY_MAX);
		fKeyPointCount = 0;
		break;

	/* Create a starfield with big concentration of stars (amas) */
	case SPACE_AMAS :
	case SPACE_SPIRAL :
		/* pick 8 random position for the amas */
		FillStarList(fStars.list, 8);
		for (i=0; i<8; i++) {
			amas[i].x = fStars.list[i].x;
			amas[i].y = fStars.list[i].y;
			amas[i].z = fStars.list[i].z;
			amas_select[i] = i;
			factor[i] = ((float)(fCrcAlea&2047) + 0.5)*(1.0/128.0) + 16.0/3.0;
			CrcStep();
			CrcStep();
		}
		/* make each amas ramdomly smaller or bigger */
		for (i=8; i<32; i++) {
			amas_select[i] = (fCrcAlea & 7);
			CrcStep();
		}

		/* create a random starfield */
		FillStarList(fStars.list, STAR_DENSITY_MAX);

		/* In spiral mode, only half the star will be put into the amas.
		   the other half will be put into the spiral galaxy. */
		if (space_model == SPACE_AMAS)
			i_step = 1;
		else
			i_step = 2;
		s = fStars.list;

		for (i=0; i<STAR_DENSITY_MAX; i+=i_step) {
			/* for every star, calculate its position relative to the
			   center of the corresponding amas. */
			index = amas_select[i&31];
			dx = s->x-amas[index].x;
			if (dx < -0.5) dx += 1.0;
			if (dx > 0.5)  dx -= 1.0;
			dy = s->y-amas[index].y;
			if (dy < -0.5) dy += 1.0;
			if (dy > 0.5)  dy -= 1.0;
			dz = s->z-amas[index].z;
			if (dz < -0.5) dz += 1.0;
			if (dz > 0.5)  dz -= 1.0;

			/* make the star randomly closer from its center, but keep
			   it on the same orientation. */
			step = 0;
			dist = (abs(dx) + abs(dy) + abs(dz))*factor[index];
			while (dist > 1.0) {
				dist *= 0.5;
				step++;
			}

			step -= (fCrcAlea&3);
			CrcStep();
			fact = 1.0;
			for (;step>=0; step--)
				fact *= 0.55;
			dx *= fact;
			dy *= fact;
			dz *= fact;

			/* put the star back in the [0-1]x[0-1]x[0-1] iteration of
			   the cubic torus. */
			s->x = amas[index].x + dx;
			if (s->x >= 1.0) s->x -= 1.0;
			if (s->x <= 0.0) s->x += 1.0;
			s->y = amas[index].y + dy;
			if (s->y >= 1.0) s->y -= 1.0;
			if (s->y <= 0.0) s->y += 1.0;
			s->z = amas[index].z + dz;
			if (s->z >= 1.0) s->z -= 1.0;
			if (s->z <= 0.0) s->z += 1.0;

			s += i_step;
		}

		/* record the center of the amas as key points for the free
		   camera animation mode. */
		for (i=0; i<8; i++)
			fKeyPoints[i] = amas[i];
		fKeyPointCount = 8;

		/* no further processing needed in amas only mode. */
		if (space_model == SPACE_AMAS)
			break;

		/* in spiral mode, the second half of the star will be distributed
		   on random spiral like galaxy. */
		s = fStars.list+1;
		for (i=1; i<STAR_DENSITY_MAX; i+=2) {
			/* some random point (probability 50 %) will be move into a
			   big amas at the center of the spiral galaxy. */
			if (fCrcAlea & 2048) {
				/* for every star, calculate its position relative to the
				   center of the galaxy. */
				dx = s->x - 0.5;
				dy = s->y - 0.5;
				dz = s->z - 0.5;

				/* make the star randomly closer from its center, but keep
				   it on the same orientation. */
				step = 0;
				dist = (dx*dx + dy*dy + dz*dz) * (32.0/0.75);
				while (dist > 1.0) {
					dist *= 0.5;
					step++;
				}

				step -= (fCrcAlea&3);
				CrcStep();
				fact = 0.5;
				for (;step>=0; step--)
					fact *= 0.55;
				dx *= fact;
				dy *= fact;
				dz *= fact;
			}
			else {
				/* other star are put at a random place somewhere on one of
				   teh two spiral arms... */
				alpha = 3.4 * s->x * (s->x*0.5 + 1.0);
				if (fCrcAlea & 64)
					alpha += 3.14159;
				r = s->x * 0.34 + 0.08;
				r += (s->y-0.725 + 0.03 * (float)(fCrcAlea & 15))*0.04*(1.2+r);
				r *= 0.5;
				dx = (s->z-0.8 + 0.04 * (float)(fCrcAlea & 15)) * (2.0 - abs(s->y - 0.5)) * (0.025*0.5);
				dy = cos(alpha) * r;
				dz = sin(alpha) * r;
			}
			CrcStep();

			/* put the star back in the [0-1]x[0-1]x[0-1] iteration of
			   the cubic torus. */
			s->x = 0.5 + dx;
			s->y = 0.5 + dy;
			s->z = 0.5 + dz;
			s += 2;
		}

		/* add the center of the galaxy to the key point list for free camera
		   animation mode */
		fKeyPoints[8].x = 0.5;
		fKeyPoints[8].y = 0.5;
		fKeyPoints[8].z = 0.5;
		/* add seven other galaxy star to the key point list */
		for (i=9; i<16; i++) {
			fKeyPoints[i].x = fStars.list[i*(STAR_DENSITY_MAX/18)].x;
			fKeyPoints[i].y = fStars.list[i*(STAR_DENSITY_MAX/18)].y;
			fKeyPoints[i].z = fStars.list[i*(STAR_DENSITY_MAX/18)].z;
		}
		fKeyPointCount = 16;
		break;
	}

	/* In all starfield modes, for all stars, peek a random brightness level */
	for (i=0; i<STAR_DENSITY_MAX; i++) {
		fStars.list[i].size = (float)((fCrcAlea&15)+17)*(1.0/56.0);
		if ((fCrcAlea & 0xc0) == 0)
			fStars.list[i].size *= 2.0;
		if ((fCrcAlea & 0x3f00) == 0)
			fStars.list[i].size *= 3.0;
		CrcStep();
	}
}

/* Fill a list of star with random position in the [0-1]x[0-1]x[0-1] cube */
void
ChartWindow::FillStarList(star *list, int32 count)
{
	int32		i;

	for (i=0; i<count; i++) {
		list[i].x = ((float)(fCrcAlea&2047) + 0.5)*(1.0/2048.0);
		CrcStep();
	}
	for (i=0; i<count; i++) {
		list[i].y = ((float)(fCrcAlea&2047) + 0.5)*(1.0/2048.0);
		CrcStep();
	}
	for (i=0; i<count; i++) {
		list[i].z = ((float)(fCrcAlea&2047) + 0.5)*(1.0/2048.0);
		CrcStep();
	}
}

/* initialise anything needed to enable a specific special animation */
void
ChartWindow::InitSpecials(int32 code)
{
	int			i, j;
	float		alpha, ksin, kcos, coeff;
	TPoint		dx, dy;
	TMatrix		matrix;

	switch (code) {
	/* turn special animation off */
	case SPECIAL_NONE :
		fSpecials.count = 0;
		break;

	/* Initialise the pixel-comet animation */
	case SPECIAL_COMET :
		/* Get a bunchof random values by getting some radom stars */
		fSpecials.count = 512;
		FillStarList(fSpecials.list, 4);
		/* for both comets... */
		for (j=0; j<2; j++) {
			/* select the initial position of the comet head */
			fComet[j].x = fSpecials.list[j].x;
			fComet[j].y = fSpecials.list[j].y;
			fComet[j].z = fSpecials.list[j].z;
			fSpecials.list[0].size = 1.4;

			/* select the speed vector of the comet */
			matrix.Set(fSpecials.list[j+2].x * 6.28319, fSpecials.list[j+2].y * 3.14159 - 1.5708, 0.0);
			fDeltaComet[j] = matrix.Axis(0) * 0.0015;
			dx = matrix.Axis(1);
			dy = matrix.Axis(2);

			for (i=j+2; i<fSpecials.count; i+=2) {
				/* make the pixel invisible at first */
				fSpecials.list[i].x = -10.0;
				fSpecials.list[i].y = 0.0;
				fSpecials.list[i].z = 0.0;
				/* spread the initial count on a linear scale (to make pixel
				   appear progressively */
				fSpecialList[i].comet.count = i/2;
				/* spread the pixel trace count randomly on a [93-124] range */
				fSpecialList[i].comet.count0 = (fCrcAlea & 31) + 93;
				CrcStep();
				/* pick a random ejection angle */
				alpha = ((fCrcAlea>>8) & 1023) * (6.283159/1024.0);
				CrcStep();

				/* pick a random ejection speed */
				coeff = 0.000114 + 0.0000016 * (float)((fCrcAlea>>17) & 31);
				if ((fCrcAlea & 7) > 4) coeff *= 0.75;
				if ((fCrcAlea & 7) == 7) coeff *= 0.65;
				CrcStep();

				/* calculate the ejection speed vector */
				ksin = sin(alpha) * coeff;
				kcos = cos(alpha) * coeff;
				fSpecialList[i].comet.dx = dx.x * kcos + dy.x * ksin;
				fSpecialList[i].comet.dy = dx.y * kcos + dy.y * ksin;
				fSpecialList[i].comet.dz = dx.z * kcos + dy.z * ksin;
			}
		}
		break;

	/* Add a list of random star (used for nova effect by modifying their
	   brightness level in real time) close from the first stars of the
	   starfield. */
	case SPECIAL_NOVAS :
		fSpecials.count = 96;
		for (i=0; i<fSpecials.count; i++) {
			fSpecialList[i].nova.count = i + 40;
			fSpecialList[i].nova.count0 = (fCrcAlea & 63) + 28;
			CrcStep();
			fSpecials.list[i].x = fStars.list[i].x + (fCrcAlea & 1)*0.02 - 0.01;
			CrcStep();
			fSpecials.list[i].y = fStars.list[i].y + (fCrcAlea & 1)*0.02 - 0.01;
			CrcStep();
			fSpecials.list[i].z = fStars.list[i].z + (fCrcAlea & 1)*0.02 - 0.01;
			CrcStep();
			fSpecials.list[i].size = 0.0;
		}
		break;

	/* not implemented */
	case SPECIAL_BATTLE :
		fSpecials.count = 0;
		break;
	}
}

/* select a color for each star (and special animation point) by
   looping through the color index list. */
void
ChartWindow::SetStarColors(int32 *color_list, int32 color_count)
{
	int32		i, index;

	index = 0;
	for (i=0; i<STAR_DENSITY_MAX; i++) {
		fStars.list[i].color_type = color_list[index];
		index++;
		if (index >= color_count)
			index = 0;
	}
	for (i=0; i<SPECIAL_COUNT_MAX; i++) {
		fSpecials.list[i].color_type = color_list[index];
		index++;
		if (index >= color_count)
			index = 0;
	}
}


void
ChartWindow::SetGeometry(int32 dh, int32 dv)
{
	float zoom;

	/* calculate the zoom factor for the 3d projection */
	fGeometry.zoom_factor = (float)dh*(fDepthRef/DH_REF);
	zoom = (float)dv*(fDepthRef/DV_REF);
	if (zoom > fGeometry.zoom_factor)
		fGeometry.zoom_factor = zoom;

	/* offset of the origin in the view area */
	fGeometry.offset_h = (float)dh * 0.5;
	fGeometry.offset_v = (float)dv * 0.5;

	/* sub-pixel precision double-sampling */
	fGeometry.zoom_factor *= 2.0;
	fGeometry.offset_h = fGeometry.offset_h * 2.0 - 1.0;
	fGeometry.offset_v = fGeometry.offset_v * 2.0 - 1.0;
}


void
ChartWindow::SetColorSpace(buffer *buf, color_space depth)
{
	bool swap_needed;
	int32 red_shift = 0, green_shift = 0;
	int32 blue_shift = 0, alpha_shift = 0;
	int32 step_doubling = 0;

	int32 red_divide_shift = 0, green_divide_shift = 0;
	int32 blue_divide_shift = 0, alpha_divide_shift = 0;
	int32 i;
	uint32 color;
	uint32 *col;
	BScreen screen(this);
	rgb_color ref_color;

	/* depending the colorspace of the target buffer, set parameters used
	   to encode the RGBA information for various color information in
	   the right format. */
	buf->depth = depth;
	switch (depth) {
		case B_RGBA32_BIG :
		case B_RGB32_BIG :
		case B_RGBA32 :
		case B_RGB32 :
			buf->depth_mode = PIXEL_4_BYTES;
			buf->bytes_per_pixel = 4;
			red_shift = 16;
			green_shift = 8;
			blue_shift = 0;
			alpha_shift = 24;
			red_divide_shift = 0;
			green_divide_shift = 0;
			blue_divide_shift = 0;
			alpha_divide_shift = 0;
			step_doubling = 32;
			break;
		case B_RGB16_BIG :
		case B_RGB16 :
			buf->depth_mode = PIXEL_2_BYTES;
			buf->bytes_per_pixel = 2;
			red_shift = 11;
			red_divide_shift = 3;
			green_shift = 5;
			green_divide_shift = 2;
			blue_shift = 0;
			blue_divide_shift = 3;
			alpha_shift = 32;
			alpha_divide_shift = 8;
			step_doubling = 16;
			break;
		case B_RGB15 :
		case B_RGBA15 :
		case B_RGB15_BIG :
		case B_RGBA15_BIG :
			buf->depth_mode = PIXEL_2_BYTES;
			buf->bytes_per_pixel = 2;
			red_shift = 10;
			red_divide_shift = 3;
			green_shift = 5;
			green_divide_shift = 3;
			blue_shift = 0;
			blue_divide_shift = 3;
			alpha_shift = 15;
			alpha_divide_shift = 7;
			step_doubling = 16;
			break;
		case B_CMAP8 :
		default:
			buf->depth_mode = PIXEL_1_BYTE;
			buf->bytes_per_pixel = 1;
			break;
	}

	/* Check if the endianess of the buffer is different from the
	   endianess use by the processor to encode the color information */
	switch (depth) {
		case B_RGBA32_BIG :
		case B_RGB32_BIG :
		case B_RGB16_BIG :
		case B_RGB15_BIG :
		case B_RGBA15_BIG :
			swap_needed = true;
			break;
		case B_RGBA32 :
		case B_RGB32 :
		case B_RGB16 :
		case B_RGB15 :
		case B_RGBA15 :
		case B_CMAP8 :
		default:
			swap_needed = false;
			break;
	}

#if B_HOST_IS_BENDIAN
	swap_needed = ~swap_needed;
#endif
	/* fill the color tables (8 light level for 7 colors, and also encode
	   the background color */
	col = buf->colors[0];
	switch (buf->depth_mode) {
		case PIXEL_1_BYTE :
			/* 8 bits, indexed mode */
			for (i=0; i<7*8; i++) {
				ref_color = color_list[i>>3];
				ref_color.red   = (ref_color.red*light_gradient[i&7])>>16;
				ref_color.green = (ref_color.green*light_gradient[i&7])>>16;
				ref_color.blue  = (ref_color.blue*light_gradient[i&7])>>16;
				color = screen.IndexForColor(ref_color);
				col[i] = (color<<24) | (color<<16) | (color<<8) | color;
			}
			color = screen.IndexForColor(fCurrentSettings.back_color);
			buf->back_color = (color<<24) | (color<<16) | (color<<8) | color;
			break;
		case PIXEL_2_BYTES :
		case PIXEL_4_BYTES :
			/* 15, 16 or 32 bytes, RGB modes. Those modes just directly encode
			   part of the bits of the initial rgba_color, at the right bit
			   position */
			for (i=0; i<7*8; i++) {
				ref_color = color_list[i>>3];
				ref_color.red   = (ref_color.red*light_gradient[i&7])>>16;
				ref_color.green = (ref_color.green*light_gradient[i&7])>>16;
				ref_color.blue  = (ref_color.blue*light_gradient[i&7])>>16;
				color = ((uint8)ref_color.red >> red_divide_shift) << red_shift;
				color |= ((uint8)ref_color.green >> green_divide_shift) << green_shift;
				color |= ((uint8)ref_color.blue >> blue_divide_shift) << blue_shift;
				color |= ((uint8)ref_color.alpha >> alpha_divide_shift) << alpha_shift;
				col[i] = (color<<step_doubling) | color;
			}
			color = ((uint8)fCurrentSettings.back_color.red >> red_divide_shift) << red_shift;
			color |= ((uint8)fCurrentSettings.back_color.green >> green_divide_shift) << green_shift;
			color |= ((uint8)fCurrentSettings.back_color.blue >> blue_divide_shift) << blue_shift;
			color |= ((uint8)fCurrentSettings.back_color.alpha >> alpha_divide_shift) << alpha_shift;
			buf->back_color = (color<<step_doubling) | color;
			break;
	}

	/* do the endianess swap if needed */
	if (swap_needed) {
		col = buf->colors[0];
		for (i = 0; i < 7*8; i++) {
			B_SWAP_INT32(col[i]);
		}
		B_SWAP_INT32(buf->back_color);
	}
}


/*!
	For each different offset used to access a pixel of the star matrix,
	create a buffer pointer based on the main buffer pointer offset by
	the pixel matrix offset. That way, any pixel of the matrix can be
	address later by just picking the right pointer and indexing it by
	the global star offset
*/
void
ChartWindow::SetPatternBits(buffer *buf)
{
	for (int32 i=0; i<32; i++) {
		buf->pattern_bits[i] = (void*)((char*)buf->bits +
			buf->bytes_per_row * pattern_dv[i] +
			buf->bytes_per_pixel * pattern_dh[i]);
	}
}


//	#pragma mark Engine processing related functions.


/*!
	That's the main thread controling the animation and synchronising
	the engine state with the changes coming from the UI.
*/
long
ChartWindow::Animation(void *data)
{
	int32			i, cur_4_frames_index, cur_last_fps, count_fps;
	float			time_factor = 0, total_fps;
	float			last_fps[4];
	bigtime_t		next_stat;
	bigtime_t		timer, time_left, current;
	bigtime_t		before_frame, after_frame, fps;
	bigtime_t		last_4_frames[4];
	ChartWindow		*w;

	w = (ChartWindow*)data;

	/* init refresh rate control */
	timer = system_time();
	w->fFrameDelay = 100000;

	/* init performance timing control variables */
	next_stat = timer + STAT_DELAY;
	cur_4_frames_index = 0;
	cur_last_fps = 0;
	for (i=0; i<4; i++)
		last_fps[i] = 0.0;
	total_fps = 0.0;
	count_fps = 0;

	/* here start the loop doing all the good stuff */
	while (!w->fKillThread) {

		/* start the performance mesurement here */
		before_frame = system_time();

		/* credit the timer by the current delay between frame */
		timer += w->fFrameDelay;

		/* change the settings, if needed */
		w->ChangeSetting(w->fNextSettings);

		/* draw the next frame */
		if (w->fCurrentSettings.display == DISPLAY_BITMAP) {
			w->RefreshStars(&w->fBitmapBuffer, time_factor * 2.4);
			if (w->LockWithTimeout(200000) == B_OK) {
				w->fChartView->DrawBitmap(w->fOffscreen);
				w->Unlock();
			}
		}
		else if (w->fCurrentSettings.display == DISPLAY_DIRECT) {
			/* This part get the drawing-lock to guarantee that the
			   directbuffer context won't change during the drawing
			   operations. During that period, no Window should be
			   done to avoid any potential deadlock. */
			while (acquire_sem(w->fDrawingLock) == B_INTERRUPTED)
				;

			if (w->fDirectConnected)
				w->RefreshStars(&w->fDirectBuffer, time_factor * 2.4);

			release_sem(w->fDrawingLock);
		}

		/* do the camera animation */
		w->CameraAnimation(time_factor);

		/* end the performance mesurement here */
		after_frame = system_time();

		/* performance timing calculation here (if display enabled). */
		if (w->fCurrentSettings.display != DISPLAY_OFF) {
			/* record frame duration into a 2 levels 4 entries ring buffer */
			last_4_frames[cur_4_frames_index] = after_frame - before_frame;
			cur_4_frames_index++;
			if (cur_4_frames_index == 4) {
				cur_4_frames_index = 0;
				last_fps[cur_last_fps++ & 3] =
					last_4_frames[0]+last_4_frames[1]+last_4_frames[2]+last_4_frames[3];
				/* the instant load is calculated based on the average duration
				   of the last 16 frames. */
				fps = (bigtime_t) (16e6 / (last_fps[0]+last_fps[1]+last_fps[2]+last_fps[3]));
				w->DrawInstantLoad(fps);

				total_fps += fps;
				count_fps += 1;

				/* The statistic numbers are based on the ratio between the real
				   duration and the frame count during a period of approximately
				   STAT_DELAY microseconds. */
				if (after_frame > next_stat) {
					w->PrintStatNumbers(total_fps/(float)count_fps);
					next_stat = after_frame+STAT_DELAY;
					total_fps = 0.0;
					count_fps = 0;
				}
			}
		}

		/* do a pause if necessary */
		current = system_time();
		time_left = timer-current;
		if (time_left > 2000) {
			snooze(time_left);
			time_left = 0;
		}
		else if (time_left < -5000)
			timer = current;

		/* this factor controls the dynamic timing configuration, that
		   slow down or speed up the whole animation step to compensate
		   for varaiation of the framerate. */
		time_factor = (float)(system_time() - before_frame) * (1.0/4e4);
	}
	return 0;
}

/* This is the second thread doing star animation. It's just a poor
   slave of the Animation thread. It's directly synchronised with its
   master, and will only do some star animation processing whenever
   its master allows him to do so. */
long
ChartWindow::Animation2(void *data)
{
	ChartWindow *w = (ChartWindow*)data;
	while (!w->fKillThread) {
		/* This thread need to both wait for its master to unblock
		   him to do some real work, or for the main control to
		   set the fKillThread flag, asking it to quit. */
		status_t status;
		do {
			status = acquire_sem_etc(w->fSecondThreadLock, 1, B_TIMEOUT, 500000);
			if (w->fKillThread)
				return 0;
		} while (status == B_TIMED_OUT || status == B_INTERRUPTED);

		/* the duration of the processing is needed to control the
		   dynamic load split (see RefreshStar) */
		bigtime_t before = system_time();
		RefreshStarPacket(w->fSecondThreadBuffer, &w->fStars2, &w->fGeometry);
		RefreshStarPacket(w->fSecondThreadBuffer, &w->fSpecials2, &w->fGeometry);
		bigtime_t after = system_time();

		w->fSecondThreadDelay = after-before;

		release_sem(w->fSecondThreadRelease);
	}
	return 0;
}


void
ChartWindow::SetCubeOffset()
{
	int32		i;
	TPoint		min, max, dx, dy, dz, p1;

	/* calculate the shortest aligned cube encapsulating the pyramid
	   of vision, by calculating the min and max on the 3 main axis
	   of the coordinates of the 8 extremities of the pyramid of
	   vision (as limited by its 4 sides and the rear and front
	   cut plan) */
	min.x = min.y = min.z = 10.0;
	max.x = max.y = max.z = -10.0;

	dx = fCamera.Axis(0)*(DH_REF*0.5);
	dy = fCamera.Axis(1)*(DV_REF*0.5);
	dz = fCamera.Axis(2)*fDepthRef;

	for (i=0; i<8; i++) {
		/* left side / right side */
		if (i&1) p1 = dz + dx;
		else	 p1 = dz - dx;
		/* top side / bottom side */
		if (i&2) p1 = p1 + dy;
		else	 p1 = p1 - dy;
		/* rear cut plan / front cut plan */
		if (i&4) p1 = p1 * (1.0 / Z_CUT_RATIO);
		/* relative to the position of the camera */
		p1 = p1 + fOrigin;

		if (min.x > p1.x) min.x = p1.x;
		if (min.y > p1.y) min.y = p1.y;
		if (min.z > p1.z) min.z = p1.z;
		if (max.x < p1.x) max.x = p1.x;
		if (max.y < p1.y) max.y = p1.y;
		if (max.z < p1.z) max.z = p1.z;
	}

	/* offset the camera origin by +1 or -1 on any axis (which
	   doesn't change its relative position in the cubic torus
	   as the cubic torus repeat itself identicaly for any move
	   of +1 or -1 on any axis), to get the bounding cube into
	   [0-2[ x [0-2[ x [0-2[. As the pyramid of vision is just
	   small enough to gurantee that its bounding box will never
	   be larger than 1 on any axis, it's always possible. */
	while (min.x < 0.0) {
		min.x += 1.0;
		max.x += 1.0;
		fOrigin.x += 1.0;
	}
	while (min.y < 0.0) {
		min.y += 1.0;
		max.y += 1.0;
		fOrigin.y += 1.0;
	}
	while (min.z < 0.0) {
		min.z += 1.0;
		max.z += 1.0;
		fOrigin.z += 1.0;
	}
	while (max.x >= 2.0) {
		min.x -= 1.0;
		max.x -= 1.0;
		fOrigin.x -= 1.0;
	}
	while (max.y >= 2.0) {
		min.y -= 1.0;
		max.y -= 1.0;
		fOrigin.y -= 1.0;
	}
	while (max.z >= 2.0) {
		min.z -= 1.0;
		max.z -= 1.0;
		fOrigin.z -= 1.0;
	}

	/* set the cutting plans. For example, if the bouding box of
	   the pyramid of vision of the camera imcludes only X in
	   [0.43 ; 1.37], we know that points with X in [0 ; 0.4] are
	   not visible from the camera. So we will offset them by +1
	   in [1.0 ; 1.4] where they will be visible. Same process
	   on other axis. That way, we have to test every star of the
	   starfield in one position and only one. */
	fCut.x = (min.x + max.x - 1.0) * 0.5;
	fCut.y = (min.y + max.y - 1.0) * 0.5;
	fCut.z = (min.z + max.z - 1.0) * 0.5;

	/* Make sure those new settings are copied into the struct
	   used by the embedded C-engine. */
	SyncGeo();
}

/* move the camera around, as defined by the animation popup.
   This is adjusted by a time factor to compensate for change
   in the framerate. */
void
ChartWindow::CameraAnimation(float time_factor)
{
	TPoint			move;

	switch (fCurrentSettings.animation) {
	/* Slow rotation around the "center" of the visible area. */
	case ANIMATION_ROTATE :
		/* turn around a point at 0.45 in front of the camera */
		move = fCamera.Axis(2);
		move = move * 0.45;
		fOrigin = fOrigin + move;

		/* turn around the alpha angle of the spheric rotation
		   matrix */
		fCameraAlpha += 0.011*time_factor;
		if (fCameraAlpha > 2*3.14159)
			fCameraAlpha -= 2*3.14159;

		/* set the other two angles close from hardcoded values */
		if (fCameraTheta < 0.18)
			fCameraTheta += 0.003*time_factor;
		if (fCameraTheta > 0.22)
			fCameraTheta -= 0.003*time_factor;

		if (fCameraPhi < -0.02)
			fCameraPhi += 0.003*time_factor;
		if (fCameraPhi > 0.02)
			fCameraPhi -= 0.003*time_factor;

		fCamera.Set(fCameraAlpha, fCameraTheta, fCameraPhi);
		fCameraInvert = fCamera.Transpose();
		move = fCamera.Axis(2);
		move = move * -0.45;
		fOrigin = fOrigin + move;
		/* As we moved or rotated the camera, we need to process
		   again the parameters specific to the pyramid of vision. */
		SetCubeOffset();
		break;

	case ANIMATION_SLOW_MOVE :
		/* Just move forward, at slow speed */
		move = fCamera.Axis(2);
		move = move * 0.006*time_factor;
		fOrigin = fOrigin + move;
		SetCubeOffset();
		break;

	case ANIMATION_FAST_MOVE :
		/* Just move forward, at fast speed */
		move = fCamera.Axis(2);
		move = move * 0.018*time_factor;
		fOrigin = fOrigin + move;
		SetCubeOffset();
		break;

	case ANIMATION_FREE_MOVE :
		/* go into advanced selection process no more than once every
		   0.5 time unit (average time). */
		fLastDynamicDelay += time_factor;
		if (fLastDynamicDelay > 0.5) {
			fLastDynamicDelay -= 0.5;
			if (fLastDynamicDelay > 0.2)
				fLastDynamicDelay = 0.2;

			/* if we're not following any target, then just turn
			   randomly (modifying only the direction of the
			   acceleration) */
			if (fTrackingTarget < 0) {
				if ((fCrcAlea & 0x4200) == 0) {
					if (fCrcAlea & 0x8000)
						fCountAlpha += 1 - (fCountAlpha/4);
					else
						fCountAlpha += -1 - (fCountAlpha/4);
					CrcStep();
					if (fCrcAlea & 0x8000)
						fCountTheta += 1 - (fCountTheta/4);
					else
						fCountTheta += -1 - (fCountTheta/4);
					CrcStep();
					if (fCrcAlea & 0x8000)
						fCountPhi += 1 - (fCountPhi/4);
					else
						fCountPhi += -1 - (fCountPhi/4);
					CrcStep();
				}
				CrcStep();
			}
			/* if following a target, try to turn in its direction */
			else
				FollowTarget();

			/* Change target everyonce in a while... */
			if ((fCrcAlea & 0xf80) == 0)
				SelectNewTarget();

			/* depending the direction of acceleration, increase or
			   reduce the angular speed of the 3 spherical angles. */
			if (fCountAlpha < 0)
				fDynamicAlpha += -0.0005 - fDynamicAlpha * 0.025;
			else if (fCountAlpha > 0)
				fDynamicAlpha += 0.0005 - fDynamicAlpha * 0.025;

			if (fCountTheta < 0)
				fDynamicTheta += -0.0002 - fDynamicTheta * 0.025;
			else if (fCountTheta > 0)
				fDynamicTheta += 0.0002 - fDynamicTheta * 0.025;

			if (fCountPhi < 0)
				fDynamicPhi += -0.00025 - fDynamicPhi * 0.025;
			else if (fCountPhi >0)
				fDynamicPhi += 0.00025 - fDynamicPhi * 0.025;
		}

		/* turn the camera following the specified angular speed */
		fCameraAlpha += fDynamicAlpha*time_factor;
		if (fCameraAlpha < 0.0)
			fCameraAlpha += 2*3.14159;
		else if (fCameraAlpha > 2*3.14159)
			fCameraAlpha -= 2*3.14159;

		fCameraTheta += fDynamicTheta*time_factor;
		if (fCameraTheta < 0.0)
			fCameraTheta += 2*3.14159;
		else if (fCameraTheta > 2*3.14159)
			fCameraTheta -= 2*3.14159;

		fCameraPhi += fDynamicPhi*time_factor;
		if (fCameraPhi < 0.0)
			fCameraPhi += 2*3.14159;
		else if (fCameraPhi > 2*3.14159)
			fCameraPhi -= 2*3.14159;

		/* Set the new rotation matrix of the camera */
		fCamera.Set(fCameraAlpha, fCameraTheta, fCameraPhi);
		fCameraInvert = fCamera.Transpose();

		/* move the camera forward at medium speed */
		move = fCamera.Axis(2);
		move = move * 0.0115*time_factor;
		fOrigin = fOrigin + move;
		SetCubeOffset();
		break;
	}
}


void
ChartWindow::SelectNewTarget()
{
	float		ratio, ratio_min;
	float		dist, lateral, axial, ftmp;
	int32		i, index_min;
	TPoint		axis, pt, vect;

	axis = fCamera.Axis(2);
	ratio_min = 1e6;
	index_min = -3;

	for (i=-2; i<fKeyPointCount; i++) {
		/* if they're used, the comets are two good potential
		   targets. */
		if (i < 0) {
			if (fCurrentSettings.special == SPECIAL_COMET)
				pt = fComet[i+2];
			else
				continue;
		}
		/* other potential targets are the key_points defined
		   in the star field. */
		else
			pt = fKeyPoints[i];

		/* Qualify the interest of the potential target in
		   relationship with its distance and its proximity to
		   the axis of the camera. */
		if (pt.x < fCut.x)
			pt.x += 1.0;
		if (pt.y < fCut.y)
			pt.y += 1.0;
		if (pt.z < fCut.z)
			pt.z += 1.0;
		pt = pt - fOrigin;
		dist = pt.Length();
		ftmp = 1.0/dist;
		pt.x *= ftmp;
		pt.y *= ftmp;
		pt.z *= ftmp;
		vect = pt ^ axis;
		lateral = axis.Length();
		axial = pt.x*axis.x + pt.y*axis.y + pt.z*axis.z;
		ratio = (lateral/axial) * sqrt(dist);

		/* keep track of the best possible choice */
		if ((dist > 0.05) && (ratio < ratio_min)) {
			ratio_min = ratio;
			index_min = i;
		}
	}

	/* record what target has been chosen */
	fTrackingTarget = index_min+2;
}

/* Try to change the angular acceleration to aim in direction
   of the current target. */
void
ChartWindow::FollowTarget()
{
	float		x0, y0, x, y, z, cphi, sphi;
	TPoint		pt;

	/* get the target point */
	if (fTrackingTarget < 2)
		pt = fComet[fTrackingTarget];
	else
		pt = fKeyPoints[fTrackingTarget-2];
	/* move it in the right iteration of the cubic torus (the
	   one iteration that is the most likely to be close from
	   the pyramid of vision. */
	if (pt.x < fCut.x)
		pt.x += 1.0;
	if (pt.y < fCut.y)
		pt.y += 1.0;
	if (pt.z < fCut.z)
		pt.z += 1.0;
	/* convert the target coordinates in the camera referential */
	pt = pt - fOrigin;
	x = fCameraInvert.m[0][0]*pt.x + fCameraInvert.m[1][0]*pt.y + fCameraInvert.m[2][0]*pt.z;
	y = fCameraInvert.m[0][1]*pt.x + fCameraInvert.m[1][1]*pt.y + fCameraInvert.m[2][1]*pt.z;
	z = fCameraInvert.m[0][2]*pt.x + fCameraInvert.m[1][2]*pt.y + fCameraInvert.m[2][2]*pt.z;
	if (z <= 0.001) {
		/* need to do a U-turn (better to do it using theta). */
		fCountAlpha = 0;
		fCountTheta = -1;
		fCountPhi = 0;
	}
	else {
		/* need to do a direction adjustement (play with
		   alpha and theta) */
		cphi = cos(fCameraPhi);
		sphi = sin(fCameraPhi);
		x0 = x*cphi - y*sphi;
		y0 = x*sphi + y*cphi;

		/* need to move first on the left/right axis */
		if (abs(x0) > abs(y0)) {
			if (x0 > 0)
				fCountAlpha = -1;
			else
				fCountAlpha = 1;
			fCountTheta = 0;
			fCountPhi = 0;
		}
		/* need to move first on the top/bottom axis */
		else {
			if (y0 > 0)
				fCountTheta = -1;
			else
				fCountTheta = 1;
			fCountAlpha = 0;
			fCountPhi = 0;
		}
	}
}

/* Do whatever special processing is required to do special
   animation. This used a time_step (or time_factor) to
   compensate for change in the framerate of the animation. */
void
ChartWindow::AnimSpecials(float time_step)
{
	int			i, j;
	star		*s;
	float		delta;
	special		*sp;

	switch (fCurrentSettings.special) {
	case SPECIAL_COMET :
		/* for both comets... */
		for (j=0; j<2; j++) {
			/* move the comet forward, at its specific speed */
			fComet[j] = fComet[j] + fDeltaComet[j] * time_step;
			/* Insure that the comet stays in the [0-1]x[0-1]x[0-1]
			   iteration of the cubic torus. */
			if (fComet[j].x < 0.0) fComet[j].x += 1.0;
			else if (fComet[j].x > 1.0) fComet[j].x -= 1.0;
			if (fComet[j].y < 0.0) fComet[j].y += 1.0;
			else if (fComet[j].y > 1.0) fComet[j].y -= 1.0;
			if (fComet[j].z < 0.0) fComet[j].z += 1.0;
			else if (fComet[j].z > 1.0) fComet[j].z -= 1.0;
			/* set the position of the star used to represent the
			   head of the comet. */
			fSpecials.list[j].x = fComet[j].x;
			fSpecials.list[j].y = fComet[j].y;
			fSpecials.list[j].z = fComet[j].z;

			/* for other point, the ones that are ejected from the
			   comet, depending for allow long they have been ejected... */
			s = fSpecials.list+j+2;
			sp = fSpecialList+j+2;
			for (i=j+2; i<fSpecials.count; i+=2) {
				sp->comet.count -= (int32)time_step;
				/* they are reset and reejected again, just a little in
				   the back of the head of the comet */
				if (sp->comet.count <= 0.0) {
					delta = (0.6 + (float)(fCrcAlea & 31) * (1.0/32.0)) * time_step;
					s->x = fComet[j].x + 6.0 * sp->comet.dx - fDeltaComet[j].x * delta;
					s->y = fComet[j].y + 6.0 * sp->comet.dy - fDeltaComet[j].y * delta;
					s->z = fComet[j].z + 6.0 * sp->comet.dz - fDeltaComet[j].z * delta;
					s->size = 0.6;
					sp->comet.count = (int32)(sp->comet.count0 + (fCrcAlea & 63));
					CrcStep();
				}
				/* or they just move at their own (ejection) speed */
				else {
					s->x += sp->comet.dx * time_step;
					s->y += sp->comet.dy * time_step;
					s->z += sp->comet.dz * time_step;
					s->size *= (1.0 - 0.031 * time_step + 0.001 * time_step * time_step);
				}
				sp+=2;
				s+=2;
			}
		}
		break;

	case SPECIAL_NOVAS :
		/* Novas are just stars (usualy invisible) that periodically
		   become much brighter during a suddent flash, then disappear
		   again until their next cycle */
		sp = fSpecialList;
		for (i=0; i<fSpecials.count; i++) {
			sp->nova.count -= time_step;
			if (sp->nova.count <= 0.0) {
				fSpecials.list[i].x -= 10.0;
				sp->nova.count = sp->nova.count0 + (fCrcAlea & 31);
				CrcStep();
			}
			else if (sp->nova.count < 16.0) {
				if (fSpecials.list[i].x < 0.0)
					fSpecials.list[i].x += 10.0;
				fSpecials.list[i].size = sp->nova.count;
			}
			sp++;
		}
		break;

	case SPECIAL_BATTLE :
		/* not implemented */
		break;
	}
}


/* Sync the embedded camera state with the window class camera
   state (before calling the embedded C-engine in ChartRender.c */
void
ChartWindow::SyncGeo()
{
	fGeometry.x = fOrigin.x;
	fGeometry.y = fOrigin.y;
	fGeometry.z = fOrigin.z;
	fGeometry.cutx = fCut.x;
	fGeometry.cuty = fCut.y;
	fGeometry.cutz = fCut.z;
	memcpy(fGeometry.m, fCameraInvert.m, sizeof(float)*9);
}


void
ChartWindow::RefreshStars(buffer *buf, float time_step)
{
	/* do the specials animation (single-threaded) */
	AnimSpecials(time_step);

	/* do the projection, clipping, erase and redraw
	   of all stars. This operation is done by the
	   embedded C-engine. This code only control the
	   dynamic load split between the two threads, when
	   needed. */
	if (fCurrentSettings.second_thread) {
		int32 star_threshold = (int32)((float)fStars.count * fSecondThreadThreshold + 0.5);
		int32 special_threshold = (int32)((float)fSpecials.count * fSecondThreadThreshold + 0.5);

		/* split the work load (star and special animation)
		   between the two threads, proportionnaly to the
		   last split factor determined during the last
		   cycle. */
		star_packet stars1;
		stars1.list = fStars.list;
		stars1.count = star_threshold;
		stars1.erase_count = star_threshold;
		if (stars1.erase_count > fStars.erase_count)
			stars1.erase_count = fStars.erase_count;

		fStars2.list = fStars.list + star_threshold;
		fStars2.count = fStars.count - star_threshold;
		fStars2.erase_count = fStars.erase_count - star_threshold;
		if (fStars2.erase_count < 0)
			fStars2.erase_count = 0;

		star_packet specials1;
		specials1.list = fSpecials.list;
		specials1.count = special_threshold;
		specials1.erase_count = special_threshold;
		if (specials1.erase_count > fSpecials.erase_count)
			specials1.erase_count = fSpecials.erase_count;

		fSpecials2.list = fSpecials.list + special_threshold;
		fSpecials2.count = fSpecials.count - special_threshold;
		fSpecials2.erase_count = fSpecials.erase_count - special_threshold;
		if (fSpecials2.erase_count < 0)
			fSpecials2.erase_count = 0;

		fSecondThreadBuffer = buf;

		/* release the slave thread */
		release_sem(fSecondThreadLock);

		/* do its own part (time it) */
		bigtime_t before = system_time();
		RefreshStarPacket(buf, &stars1, &fGeometry);
		RefreshStarPacket(buf, &specials1, &fGeometry);
		bigtime_t after = system_time();

		/* wait for completion of the second thread */
		while (acquire_sem(fSecondThreadRelease) == B_INTERRUPTED)
			;

		/* calculate the new optimal split ratio depending
		   of the previous one and the time used by both
		   threads to do their work. */
		float ratio = ((float)fSecondThreadDelay/(float)(after-before)) *
				(fSecondThreadThreshold/(1.0-fSecondThreadThreshold));
		fSecondThreadThreshold = ratio / (1.0+ratio);

	} else {
		 /* In single-threaded mode, nothing fancy to be done. */
		RefreshStarPacket(buf, &fStars, &fGeometry);
		RefreshStarPacket(buf, &fSpecials, &fGeometry);
	}

	/* All the stars that were drawn will have to be erased during
	   the next frame. */
	fStars.erase_count = fStars.count;
	fSpecials.erase_count = fSpecials.count;
}


//	#pragma mark Offscreen bitmap configuration related functions.


void
ChartWindow::CheckBitmap(color_space depth, int32 width, int32 height)
{
	color_space		cur_depth;

	if (LockWithTimeout(200000) != B_OK)
		return;
	/* If there was no offscreen before, or if it was too small
	   or in the wrong depth, then... */
	if (fOffscreen == NULL)
		cur_depth = B_NO_COLOR_SPACE;
	else
		cur_depth = fBitmapBuffer.depth;
	if ((cur_depth != depth) || (width > fMaxWidth) || (height > fMaxHeight)) {
		/* We free the old one if needed... */
		if (fOffscreen)
			delete fOffscreen;
		/* We chose a new size (resizing are done by big step to
		   avoid resizing to often)... */
		while ((width > fMaxWidth) || (height > fMaxHeight)) {
			fMaxWidth += WINDOW_H_STEP;
			fMaxHeight += WINDOW_V_STEP;
		}
		/* And we try to allocate a new BBitmap at the new size. */
		fOffscreen = new BBitmap(BRect(0, 0, fMaxWidth-1, fMaxHeight-1), depth);
		if (!fOffscreen->IsValid()) {
			/* If we failed, the offscreen is released and the buffer
			   clipping is set as empty. */
			delete fOffscreen;
			fOffscreen = NULL;
			fBitmapBuffer.depth = B_NO_COLOR_SPACE;
			fBitmapBuffer.clip_bounds.top = 0;
			fBitmapBuffer.clip_bounds.left = 0;
			fBitmapBuffer.clip_bounds.right = -1;
			fBitmapBuffer.clip_bounds.bottom = -1;
		}
		else {
			/* If we succeed, then initialise the generic buffer
			   descriptor, we set the clipping to the required size,
			   and we set the buffer background color. */
			fBitmapBuffer.bits = fOffscreen->Bits();
			fBitmapBuffer.bytes_per_row = fOffscreen->BytesPerRow();
			fBitmapBuffer.buffer_width = fCurrentSettings.width;
			fBitmapBuffer.buffer_height = fCurrentSettings.height;
			SetColorSpace(&fBitmapBuffer, fOffscreen->ColorSpace());
			SetPatternBits(&fBitmapBuffer);
			SetBitmapClipping(fCurrentSettings.width, fCurrentSettings.height);
			SetBitmapBackGround();
		}
	}
	Unlock();
}


void
ChartWindow::SetBitmapClipping(int32 width, int32 height)
{
	/* Set the bitmap buffer clipping to the required size of
	   the buffer (even if the allocated buffer is larger) */
	fBitmapBuffer.clip_list_count = 1;
	fBitmapBuffer.clip_bounds.top = 0;
	fBitmapBuffer.clip_bounds.left = 0;
	fBitmapBuffer.clip_bounds.right = width-1;
	fBitmapBuffer.clip_bounds.bottom = height-1;
	fBitmapBuffer.clip_list[0].top = fBitmapBuffer.clip_bounds.top;
	fBitmapBuffer.clip_list[0].left = fBitmapBuffer.clip_bounds.left;
	fBitmapBuffer.clip_list[0].right = fBitmapBuffer.clip_bounds.right;
	fBitmapBuffer.clip_list[0].bottom = fBitmapBuffer.clip_bounds.bottom;
}


void
ChartWindow::SetBitmapBackGround()
{
	int32		i, count;
	uint32		*bits;
	uint32		color;

	/* set the bitmap buffer to the right background color */
	bits = (uint32*)fOffscreen->Bits();
	count = fOffscreen->BitsLength()/4;
	color = fBitmapBuffer.back_color;

	for (i=0; i<count; i++)
		bits[i] = color;
}


//	#pragma mark DirectWindow related functions.


void
ChartWindow::DirectConnected(direct_buffer_info *info)
{
	/* block the animation thread. */
	while (acquire_sem(fDrawingLock) == B_INTERRUPTED)
		;
	/* update the direct screen infos. */
	SwitchContext(info);
	/* unblock the animation thread. */
	release_sem(fDrawingLock);
}

/* This function update the internal graphic context of the ChartWindow
   object to reflect the infos send through the DirectConnected API.
   It also update the state of stars (and erase some of them) to
   insure a clean transition during resize. As this function is called
   in DirectConnected, it's a bad idea to do any heavy drawing (long)
   operation. But as we only update the stars (the background will be
   update a little later by the view system), it's not a big deal. */
void
ChartWindow::SwitchContext(direct_buffer_info *info)
{
	uint32 i, j;

	/* you need to use that mask to read the buffer state. */
	switch (info->buffer_state & B_DIRECT_MODE_MASK) {
	/* start a direct screen connection. */
	case B_DIRECT_START :
		/* set the status as connected, and continue as a modify */
		fDirectConnected = true;

	/* change the state of a direct screen connection. */
	case B_DIRECT_MODIFY :
		/* update the description of the abstract buffer representing
		   the direct window connection. DirectConnected returns the
		   description of the full content area. As we want to use
		   only the animation view part of the window, we will need
		   to compensate for that when update the descriptor. */

		/* This calculate the base address of the animation view, taking into
		   account the base address of the screen buffer, the position of the
		   window and the position of the view in the window */
		fDirectBuffer.bits = (void*)((char*)info->bits +
			(info->window_bounds.top + TOP_LEFT_LIMIT) * info->bytes_per_row +
			(info->window_bounds.left + LEFT_WIDTH) * (info->bits_per_pixel>>3));
		/* Bytes per row and pixel-format are the same than the window values */
		fDirectBuffer.bytes_per_row = info->bytes_per_row;
		SetColorSpace(&fDirectBuffer, info->pixel_format);
		SetPatternBits(&fDirectBuffer);

		/* the width and height of the animation view are linked to the width
		   and height of the window itself, reduced by the size of the borders
		   reserved for the UI. */
		fDirectBuffer.buffer_width =
			info->window_bounds.right-info->window_bounds.left+1 - LEFT_WIDTH;
		fDirectBuffer.buffer_height =
			info->window_bounds.bottom-info->window_bounds.top+1 - TOP_LEFT_LIMIT;

		/* Now, we go through the clipping list and "clip" the clipping
		   rectangle to the animation view boundary. */
		j = 0;
		for (i=0; i<info->clip_list_count; i++) {
			fDirectBuffer.clip_list[j].top = info->clip_list[i].top - info->window_bounds.top;
			if (fDirectBuffer.clip_list[j].top < TOP_LEFT_LIMIT)
				fDirectBuffer.clip_list[j].top = TOP_LEFT_LIMIT;
			fDirectBuffer.clip_list[j].left = info->clip_list[i].left - info->window_bounds.left;
			if (fDirectBuffer.clip_list[j].left < LEFT_WIDTH)
				fDirectBuffer.clip_list[j].left = LEFT_WIDTH;
			fDirectBuffer.clip_list[j].right = info->clip_list[i].right - info->window_bounds.left;
			fDirectBuffer.clip_list[j].bottom = info->clip_list[i].bottom - info->window_bounds.top;

			/* All clipped rectangle that are not empty are recorded in
			   the buffer clipping list. We keep only the 64 first (as
			   a reasonnable approximation of most cases), but the rectangle
			   list could easily be made dynamic if needed. Those clipping
			   rectangle are offset to animation view coordinates */
			if ((fDirectBuffer.clip_list[j].top <= fDirectBuffer.clip_list[j].bottom) &&
				(fDirectBuffer.clip_list[j].left <= fDirectBuffer.clip_list[j].right)) {
				fDirectBuffer.clip_list[j].top -= TOP_LEFT_LIMIT;
				fDirectBuffer.clip_list[j].left -= LEFT_WIDTH;
				fDirectBuffer.clip_list[j].right -= LEFT_WIDTH;
				fDirectBuffer.clip_list[j].bottom -= TOP_LEFT_LIMIT;
				j++;
				if (j == 64)
					break;
			}
		}
		/* record the count of clipping rect in the new clipping list (less
		   or equal to the window clipping list count, as some rectangle can
		   be made invisible by the extra animation view clipping */
		fDirectBuffer.clip_list_count = j;

		/* the bounding box of the clipping list need to be calculated again
		   from scratch. Clipping the bounding box of the window clipping
		   region to the animation view can give us an incorrect (larger)
		   bounding box. Remember that the bounding box of a region is
		   required to be minimal */
		fDirectBuffer.clip_bounds.top = 20000;
		fDirectBuffer.clip_bounds.left = 20000;
		fDirectBuffer.clip_bounds.right = -20000;
		fDirectBuffer.clip_bounds.bottom = -20000;

		for (i=0; i<fDirectBuffer.clip_list_count; i++) {
			if (fDirectBuffer.clip_bounds.top > fDirectBuffer.clip_list[i].top)
				fDirectBuffer.clip_bounds.top = fDirectBuffer.clip_list[i].top;
			if (fDirectBuffer.clip_bounds.left > fDirectBuffer.clip_list[i].left)
				fDirectBuffer.clip_bounds.left = fDirectBuffer.clip_list[i].left;
			if (fDirectBuffer.clip_bounds.right < fDirectBuffer.clip_list[i].right)
				fDirectBuffer.clip_bounds.right = fDirectBuffer.clip_list[i].right;
			if (fDirectBuffer.clip_bounds.bottom < fDirectBuffer.clip_list[i].bottom)
				fDirectBuffer.clip_bounds.bottom = fDirectBuffer.clip_list[i].bottom;
		}

		/* If the bounding box is empty, nothing is visible and all erasing
		   should be canceled */
		if ((fDirectBuffer.clip_bounds.top > fDirectBuffer.clip_bounds.bottom) ||
			(fDirectBuffer.clip_bounds.left > fDirectBuffer.clip_bounds.right)) {
			fStars.erase_count = 0;
			goto nothing_visible;
		}

		if (fCurrentSettings.display == DISPLAY_DIRECT) {
			/* When the direct display mode is used, the geometry changes
			   need to be immediatly applied to the engine. */
			SetGeometry(fDirectBuffer.buffer_width, fDirectBuffer.buffer_height);
			/* if the buffer was reset then
			   we cancel the erasing of the stars for the next frame. */
			if (info->buffer_state & B_BUFFER_RESET) {
				fStars.erase_count = 0;
			}
			/* In the other case, we need to cancel the erasing of star that
			   were drawn at the previous frame, but are no longer visible */
			else if (info->buffer_state & B_CLIPPING_MODIFIED) {
				RefreshClipping(&fDirectBuffer, &fStars);
				RefreshClipping(&fDirectBuffer, &fSpecials);
			}
		}
		break;

	/* stop a direct screen connection */
	case B_DIRECT_STOP :
		/* set the status as not connected */
		fDirectConnected = false;
	nothing_visible:
		/* set an empty clipping */
		fDirectBuffer.clip_list_count = 1;
		fDirectBuffer.clip_bounds.top = 0;
		fDirectBuffer.clip_bounds.left = 0;
		fDirectBuffer.clip_bounds.right = -1;
		fDirectBuffer.clip_bounds.bottom = -1;
		fDirectBuffer.clip_list[0].top = 0;
		fDirectBuffer.clip_list[0].left = 0;
		fDirectBuffer.clip_list[0].right = -1;
		fDirectBuffer.clip_list[0].bottom = -1;
		break;
	}
}


/*! copy a setting into another */
void
ChartWindow::setting::Set(setting *master)
{
	memcpy(this, master, sizeof(setting));
}


/*! Pseudo-random generator increment function.	*/
void
ChartWindow::CrcStep()
{
	fCrcAlea <<= 1;
	if (fCrcAlea < 0)
		fCrcAlea ^= CRC_KEY;
}
