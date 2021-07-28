/*
 * Copyright 2004, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Copyright 2015, Augustin Cavalier <waddlesplash>. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Effect from corTeX / Optimum.
 */


#include <AppKit.h>
#include <Catalog.h>
#include <ColorMenuItem.h>
#include <ControlLook.h>
#include <InterfaceKit.h>
#include <LayoutBuilder.h>
#include <MenuField.h>
#include <ScreenSaver.h>
#include <String.h>
#include <SupportDefs.h>
#include <Window.h>

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Nebula Screen Saver"


typedef struct
{
	int x, y, z, r;
} p3;

typedef float matrix[3][3];

#define GMAX 5000
p3 gal[GMAX];
float precos[512];
float presin[512];

typedef unsigned short word;

extern "C" {
#include "Draw.h"
#include "DrawStars.h"
}

const uint32 kMsgWidth  = 'widt';
const uint32 kMsgColorScheme = 'cols';
const uint32 kMsgBlankBorders = 'blbr';
const uint32 kMsgMotionBlur = 'blur';
const uint32 kMsgSpeed = 'sped';
const uint32 kMsgFrames = 'mfps';

float	gSpeed;
bool	gMotionBlur;
int32	gSettingsWidth;
int32	gWidth;
int32	gHeight;
float	gMaxFramesPerSecond;
BBitmap* gBitmap;
BScreenSaver* gScreenSaver;
uint32	gPalette[256];
int8	gPaletteScheme;
int8	gBlankBorders;
char*	gBuffer8;   /* working 8bit buffer */


inline float
ocos(float a)
{
	return (precos[(int)(a * 256 / M_PI) & 511]);
}

inline float
osin(float a)
{
	return (presin[(int)(a * 256 / M_PI) & 511]);
}


void
mulmat(matrix* a, matrix* b, matrix* c)
{
	int i, j;

	for (i = 0; i < 3; i++) {
		for (j = 0; j < 3; j++) {
			(*c)[i][j] = (*a)[i][0] * (*b)[0][j] +
						 (*a)[i][1] * (*b)[1][j] +
						 (*a)[i][2] * (*b)[2][j];
		}
	}
}


inline void
mulvec(matrix* a, float* x, float* y, float* z)
{
	float nx = *x, ny = *y, nz = *z;

	*x = nx * (*a)[0][0] + ny * (*a)[0][1] + nz * (*a)[0][2];
	*y = nx * (*a)[1][0] + ny * (*a)[1][1] + nz * (*a)[1][2];
	*z = nx * (*a)[2][0] + ny * (*a)[2][1] + nz * (*a)[2][2];
}


void
setrmat(float a, float b, float c, matrix* m)
{
	int i, j;
	for (i = 0; i < 3; i++)
		for (j = 0; j < 3; j++)
			(*m)[i][j] = (float)(i == j);

	if (a != 0) {
		(*m)[0][0] = cos(a);	(*m)[0][1] = sin(a);
		(*m)[1][0] = sin(a);	(*m)[1][1] = -cos(a);
		return;
	}
	if (b != 0) {
		(*m)[0][0] = cos(b);	(*m)[0][2] = sin(b);
		(*m)[2][0] = sin(b);	(*m)[2][2] = -cos(b);
		return;
	}
	(*m)[1][1] = cos(c);	(*m)[1][2] = sin(c);
	(*m)[2][1] = sin(c);	(*m)[2][2] = -cos(c);
}


void
rotate3d(float* xr, float* yr, float* zr,  /* point to rotate */
	float ax, float ay, float az)     /* the 3 angles (order ?..) */
{
	float xr2, yr2, zr2;

	xr2 = (*xr * ocos(az) + *yr * osin(az));
	yr2 = (*xr * osin(az) - *yr * ocos(az));
	*xr = xr2;
	*yr = yr2;

	xr2 = (*xr * ocos(ay) + *zr * osin(ay));
	zr2 = (*xr * osin(ay) - *zr * ocos(ay));
	*xr = xr2;
	*zr = zr2;

	zr2 = (*zr * ocos(ax) + *yr * osin(ax));
	yr2 = (*zr * osin(ax) - *yr * ocos(ax));
	*zr = zr2;
	*yr = yr2;
}


void
drawshdisk(int x0, int y0, int r)
{
	int x = 0;
	int y;
	int ly;		/* last y */
	int delta;
	int c;		/* color at center */
	int d;		/* delta */

#define SLIMIT 17
#define SRANGE 15

	if (r <= SLIMIT) {
		/* range checking is already (more or less) done... */
		draw_stars(gWidth, &gBuffer8[x0 + gWidth * y0], 10 + r * 5);
		//gBuffer8[x0 + W * y0] = 10 + r * 5;
		return;
	}

	if (r < SLIMIT + SRANGE)
		r = ((r - SLIMIT) * SLIMIT) / SRANGE + 1;

	y = ly = r;     /* AAaargh */
	delta = 3 - 2 * r;

	do {
		if (y != ly) {
			/* dont overlap these lines */
			c = ((r - y + 1) << 13) / r;
			d = -c / (x + 1);

			if (y == x + 1)		/* this would overlap with the next x lines */
				goto TOTO;		/* WHY NOT */

			/*  note : for "normal" numbers (not too big) :
				(unsigned int)(x) < M   <=>  0<=x<H
				(because if x<0, then (unsigned)(x) = 2**32-|x| which is
				BIG and thus >H )

				This is clearly a stupid, unmaintanable, unreadable
				"optimization". But i like it :)
			*/
			if ((uint32)(y0 - y - 1) < gHeight - 3)
				memshset(&gBuffer8[x0 + gWidth * (y0 - y + 1)], c, d, x);

			if ((uint32)(y0 + y - 1) < gHeight - 3)
				memshset(&gBuffer8[x0 + gWidth*(y0 + y)], c, d, x);
		}
		TOTO:
		c = ((r - x + 1) << 13) / r;
		d = -c / (y);

		if ((uint32)(y0 - x - 1) < gHeight - 3)
			memshset(&gBuffer8[x0 + gWidth*(y0 - x)], c, d, y);
		if ((uint32)(y0 + x - 1) < gHeight - 3)
			memshset(&gBuffer8[x0 + gWidth * (y0 + x + 1)], c, d, y);

		ly = y;
		if (delta < 0)
			delta += 4 * x + 6;
		else {
			delta += 4 * (x - y) + 10;
			y--;
		}
		x++;
	} while (x < y);
}


void
drawGalaxy()
{
	int r;
	int x, y;
	float rx, ry, rz;
	int i;
	float oa, ob, oc;
	float t;
	float a, b, c;
	matrix ma, mb, mc, mr;

	/* t is the parametric coordinate for the animation;
	change the scale value to change the speed of anim
	(independant of processor speed)
	*/
	static bigtime_t firstTime = system_time();
	t = ((double)gSpeed * system_time() - firstTime) / 1000000.0;
		//opti_scale_time(0.418, &demo_elapsed_time);

	a = 0.9 * t;
	b = t;
	c = 1.1 * t;

	setrmat(a, 0, 0, &ma);
	setrmat(0, b, 0, &mb);
	mulmat(&ma, &mb, &mc);
	setrmat(0, 0, c, &ma);
	mulmat(&ma, &mc, &mr);

	oa = 140 * osin(a);
	ob = 140 * ocos(b);
	oc = 240 * osin(c);

	if (gMotionBlur) {
		/* mblur does something like that:
		 * (or did, perhaps it's another version!..)

		for (i = 0; i < W * H; i++)
			gBuffer8[i]= (gBuffer8[i] >> 3) + (gBuffer8[i] >> 1);
		*/
		mblur (gBuffer8, gWidth * gHeight);
	} else
		memset(gBuffer8, 0, gWidth * gHeight);

	for (i = 0; i < GMAX; i++) {
		rx = gal[i].x;
		ry = gal[i].y;
		rz = gal[i].z;

		mulvec(&mr, &rx, &ry, &rz);

		rx += oa;
		ry += ob;
		rz += oc;
		rz += 300;

		if (rz > 5) {
			x = (int)(15 * rx / (rz / 5 + 1)) + gWidth / 2;
				/* tain jcomprend plus rien  */
			y = (int)(15 * ry/ (rz / 5 + 1)) + gHeight / 2;
				/* a ces formules de daube !! */
			r = (int)(3 * gal[i].r / (rz / 4 + 3)) + 2;

			if (x > 5 && x < gWidth - 6 && y > 5 && y < gHeight - 6)
//			if ((uint32)x < gWidth - 1 && (uint32)y < gHeight - 1)
				drawshdisk(x, y, r);
		}
	}
}


void
setPalette()
{
	int i;

	switch (gPaletteScheme) {
		case 0:		// yellow
		default:
			for (i = 0; i < 30; i++)
				gPalette[i] = (uint8)(i * 8 / 10) << 16
					| (uint8)(i * 6 / 10) << 8;
					// | (uint8)(i*3/10);

			for (i = 30; i < 256; i++) {
				uint8 r = (i);
				uint8 g = (i * i >> 8); //(i*8/10);
				uint8 b = i >= 240 ? (i - 240) << 3 : 0; //(i * 2 / 10);

				gPalette[i] = ((r << 16) | (g << 8) | (b));
			}
			break;

		case 1:		// blue
			for (i = 0; i < 30; i++)
				gPalette[i] = (uint8)(i * 8 / 10);
					// << 16 | (uint8)(i * 6 / 10) << 8;
					// | (uint8)(i * 3 / 10);

			for (i = 30; i < 256; i++) {
				uint8 b = (i);
				uint8 g = (i * i >> 8); //(i * 8 / 10);
				uint8 r = i >= 240 ? (i - 240) << 3 : 0; //(i * 2 / 10);

				gPalette[i] = ((r << 16) | (g << 8) | (b));
			}
			break;

		case 2:		// red
			for (i = 0; i < 128; i++)
				gPalette[i] = (uint8)i << 16;
					// << 16 | (uint8)(i * 6/10) << 8;
					// | (uint8)(i * 3 / 10);

			for (i = 128;i < 256; i++) {
				uint8 r = i;
				uint8 c = (uint8)((cos((i - 256) / 42.0) * 0.5 + 0.5) * 225);

				gPalette[i] = ((r << 16) | (c << 8) | c);
			}
/*			for (i = 192; i < 224; i++) {
				uint8 c = (i - 192);
				gPalette[i] = gPalette[i] & 0xff0000 | c << 8 | c;
			}
			for (i = 224; i < 256; i++) {
				uint8 c = (i-224) / 2;
				c = 32 + c * c * 6 / 10;
				gPalette[i] = gPalette[i] & 0xff0000 | c << 8 | c;
			}
*/			break;

		case 3:		// green
			for (i = 0; i < 30; i++)
				gPalette[i] = (uint8)(i * 8 / 10) << 8;
					// << 16 | (uint8)(i * 6 / 10) << 8;
					// | (uint8)(i * 3 / 10);

			for (i = 30; i < 256; i++) {
				uint8 g = (i);
				uint8 r = (i * i >> 8); //(i * 8 / 10);
				uint8 b = i >= 240 ? (i-240) << 3 : 0; //(i * 2 / 10);

				gPalette[i] = ((r << 16) | (g << 8) | (b));
			}
			break;

		case 4:		// grey
			for (i = 0; i < 256; i++) {
				uint8 c = i * 15 / 16 + 10;
				gPalette[i] = c << 16 | c << 8 | c;
			}
			break;
		case 5:		// cold
			for (i = 0; i < 30; i++)
				gPalette[i] = (uint8)(i * 8 / 10) << 16;
					// << 16 | (uint8)(i * 6 / 10) << 8;
					// | (uint8)(i * 3 / 10);

			for (i = 30; i < 256; i++) {
				uint8 r = i;
				uint8 c = (uint8)((cos((i - 255) / 82.0) * 0.5 + 0.5) * 255);

				gPalette[i] = ((r << 16) | (c << 8) | c);
			}
			break;

		case 6:		// original
			for (i = 0; i < 256; i++) {
				uint32 c = *(char *)&i;
				gPalette[i] = c << 16 | c << 8;
			}
			break;
	}
/*	for (i = 0;i < 256; i++) {
		uint8 r = (i);
		uint8 g = (i * i >> 8); //(i * 8 / 10);
		uint8 b = 0; //(i * 2 / 10);

		gPalette[i] = ((r << 16) | (g << 8) | (b));
	}
*/
/*	for (i = 240; i < 256; i++)
		gPalette[i] = (uint8)i << 16 | (uint8)i << 8 | (uint8)(i * 6 / 10);
*/
}


// #pragma mark - SimpleSlider


class SimpleSlider : public BSlider {
public:
	SimpleSlider(const char* label, BMessage* message)
	:
	BSlider(B_EMPTY_STRING, B_EMPTY_STRING, message, 1, 100, B_HORIZONTAL)
	{
		SetLimitLabels("1", "100");
		SetHashMarks(B_HASH_MARKS_BOTTOM);
		SetHashMarkCount(11);
		fLabel = label;
	};

	const char* UpdateText() const
	{
		fText.SetToFormat("%s: %d", fLabel, Value());
		return fText.String();
	};

private:
	mutable BString fText;
	const char* fLabel;
};


// #pragma mark - SettingsView


class SettingsView : public BView {
public:
								SettingsView(BRect frame);

	virtual	void				AttachedToWindow();
	virtual	void				MessageReceived(BMessage* message);

	private:
			BMenuField*			fWidthMenuField;
			BMenuField*			fColorMenuField;
			BMenuField*			fBorderMenuField;
			BCheckBox*			fMotionCheck;
			BSlider*			fSpeedSlider;
			BSlider*			fFramesSlider;
};


SettingsView::SettingsView(BRect frame)
	:
	BView(frame, "", B_FOLLOW_ALL, B_WILL_DRAW)
{
	SetViewUIColor(B_PANEL_BACKGROUND_COLOR);

	BStringView* titleString = new BStringView(B_EMPTY_STRING,
		B_TRANSLATE("Nebula"));
	titleString->SetFont(be_bold_font);

	BStringView* copyrightString = new BStringView(B_EMPTY_STRING,
		B_TRANSLATE("© 2001-2004 Axel Dörfler."));

	BPopUpMenu* popUpMenu;

	int32 widths[] = {
		0,
		320,
		512,
		576,
		640,
		800,
		1024,
		1152,
		1280,
		1400,
		1600
	};

	size_t widthsLength = sizeof(widths) / sizeof(widths[0]);

	popUpMenu = new BPopUpMenu("");
	for (size_t i = 0; i < widthsLength; i++) {
		BString label;
		if (widths[i] == 0)
			label.SetTo(B_TRANSLATE("screen resolution"));
		else
			label.SetToFormat(B_TRANSLATE("%" B_PRId32 " pixels"), widths[i]);

		BMessage* message = new BMessage(kMsgWidth);
		message->AddInt32("width", widths[i]);

		BMenuItem* item = new BMenuItem(label.String(), message);
		popUpMenu->AddItem(item);
		item->SetMarked(gSettingsWidth == widths[i]);
	}

	fWidthMenuField = new BMenuField("res", B_TRANSLATE("Internal width:"),
		popUpMenu);

	const char* colorSchemeLabels[] = {
		B_TRANSLATE("yellow"),
		B_TRANSLATE("cyan"),
		B_TRANSLATE("red"),
		B_TRANSLATE("green"),
		B_TRANSLATE("grey"),
		B_TRANSLATE("cold"),
		B_TRANSLATE("orange (original)")
	};

	rgb_color colorSchemeColors[] = {
		(rgb_color){ 255, 220, 0   },
		(rgb_color){ 127, 219, 255 },
		(rgb_color){ 255, 65,  54  },
		(rgb_color){ 46,  204, 64  },
		(rgb_color){ 170, 170, 170 },
		(rgb_color){ 234, 234, 234 },
		(rgb_color){ 255, 133, 27  }
	};

	popUpMenu = new BPopUpMenu("");
	for (int i = 0; i < 7; i++) {
		BMessage* message = new BMessage(kMsgColorScheme);
		message->AddInt8("scheme", (int8)i);
		BColorMenuItem* item = new BColorMenuItem(colorSchemeLabels[i],
			message, colorSchemeColors[i]);
		popUpMenu->AddItem(item);
		item->SetMarked(gPaletteScheme == i);
	}

	fColorMenuField = new BMenuField("col", B_TRANSLATE("Color:"), popUpMenu);

	const char* blankBorderFormats[] = {
		B_TRANSLATE("fullscreen, no borders"),
		B_TRANSLATE("16:9, wide-screen"),
		B_TRANSLATE("2:3.5, cinemascope"),
		B_TRANSLATE("only a slit")
	};

	popUpMenu = new BPopUpMenu("");
	for (int8 i = 0; i < 4; i++) {
		BMessage* message = new BMessage(kMsgBlankBorders);
		message->AddInt8("border", i);
		BMenuItem* item = new BMenuItem(blankBorderFormats[i], message);
		popUpMenu->AddItem(item);
		item->SetMarked(gBlankBorders == i);
	}

	fBorderMenuField = new BMenuField("cinema", B_TRANSLATE("Format:"),
		popUpMenu);

	fMotionCheck = new BCheckBox(B_EMPTY_STRING,
		B_TRANSLATE("Enable motion blur"), new BMessage(kMsgMotionBlur));
	fMotionCheck->SetValue((int)gMotionBlur);

	fSpeedSlider = new SimpleSlider(B_TRANSLATE("Speed"),
		new BMessage(kMsgSpeed));
	fSpeedSlider->SetValue((gSpeed - 0.002) / 0.05);

	fFramesSlider = new SimpleSlider(B_TRANSLATE("Maximum Frames Per Second"),
		new BMessage(kMsgFrames));
	fFramesSlider->SetValue(gMaxFramesPerSecond);

	BLayoutBuilder::Group<>(this, B_VERTICAL, B_USE_HALF_ITEM_SPACING)
		.SetInsets(B_USE_DEFAULT_SPACING)
		.Add(titleString)
		.Add(copyrightString)
		.AddStrut(roundf(be_control_look->DefaultItemSpacing() / 2))
		.AddGlue()
		.AddGrid(B_USE_DEFAULT_SPACING, B_USE_SMALL_SPACING)
			.Add(fColorMenuField->CreateLabelLayoutItem(), 0, 0)
			.AddGroup(B_HORIZONTAL, 0.0f, 1, 0)
				.Add(fColorMenuField->CreateMenuBarLayoutItem(), 0.0f)
				.AddGlue()
				.End()
			.Add(fWidthMenuField->CreateLabelLayoutItem(), 0, 1)
			.AddGroup(B_HORIZONTAL, 0.0f, 1, 1)
				.Add(fWidthMenuField->CreateMenuBarLayoutItem(), 0.0f)
				.AddGlue()
				.End()
			.Add(fBorderMenuField->CreateLabelLayoutItem(), 0, 2)
			.AddGroup(B_HORIZONTAL, 0.0f, 1, 2)
				.Add(fBorderMenuField->CreateMenuBarLayoutItem(), 0.0f)
				.AddGlue()
				.End()
			.Add(fMotionCheck, 1, 3)
			.End()
		.Add(fSpeedSlider)
		.Add(fFramesSlider)
	.End();
}


void
SettingsView::AttachedToWindow()
{
	fWidthMenuField->Menu()->SetTargetForItems(this);
	fColorMenuField->Menu()->SetTargetForItems(this);
	fBorderMenuField->Menu()->SetTargetForItems(this);
	fMotionCheck->SetTarget(this);
	fSpeedSlider->SetTarget(this);
	fFramesSlider->SetTarget(this);
}


void
SettingsView::MessageReceived(BMessage* message)
{
	switch(message->what) {
		case kMsgWidth:
			message->FindInt32("width", &gSettingsWidth);
			break;

		case kMsgColorScheme:
			if (message->FindInt8("scheme", &gPaletteScheme) == B_OK)
				setPalette();
			break;

		case kMsgBlankBorders:
			message->FindInt8("border", &gBlankBorders);
			break;

		case kMsgMotionBlur:
			gMotionBlur = fMotionCheck->Value() > 0;
			break;

		case kMsgSpeed:
			gSpeed = 0.002 + 0.05 * fSpeedSlider->Value();
			break;

		case kMsgFrames:
			gMaxFramesPerSecond = fFramesSlider->Value();
			gScreenSaver->SetTickSize(
				(bigtime_t)(1000000LL / gMaxFramesPerSecond));
			break;
	}
}



// #pragma mark - Nebula


class Nebula : public BScreenSaver {
public:
								Nebula(BMessage* message, image_id id);

	virtual	void				StartConfig(BView* view);
	virtual	status_t			SaveState(BMessage* state) const;

	virtual	status_t			StartSaver(BView* view, bool preview);
	virtual	void				StopSaver();
	virtual	void				Draw(BView* view, int32 frame);

private:
			float				fFactor;
			bool				fStarted;
};


Nebula::Nebula(BMessage* message, image_id id)
	:
	BScreenSaver(message, id),
	fStarted(false)
{
	message->FindFloat("speed", 0, &gSpeed);
	message->FindInt32("width", 0, &gSettingsWidth);
	message->FindBool("motionblur", 0, &gMotionBlur);
	message->FindFloat("max_fps", 0, &gMaxFramesPerSecond);
	message->FindInt8("scheme", 0, &gPaletteScheme);
	message->FindInt8("border", 0, &gBlankBorders);

	if (gSpeed < 0.01f)
		gSpeed = 0.4f;

	if (gMaxFramesPerSecond < 1.f)
		gMaxFramesPerSecond = 40.0f;

	gScreenSaver = this;
}


void
Nebula::StartConfig(BView* view)
{
	view->AddChild(new SettingsView(view->Bounds()));
}


status_t
Nebula::SaveState(BMessage* state) const
{
	state->AddFloat("speed", gSpeed);
	state->AddInt32("width", gSettingsWidth);
	state->AddBool("motionblur", gMotionBlur);
	state->AddFloat("max_fps", gMaxFramesPerSecond);
	state->AddInt8("scheme", gPaletteScheme);
	state->AddInt8("border", gBlankBorders);

	return B_OK;
}


status_t
Nebula::StartSaver(BView* view, bool preview)
{
	// initialize palette
	setPalette();

	int i;
	for (i = 0; i < 512; i++) {
		precos[i]=cos(i * M_PI / 256);
		presin[i]=sin(i * M_PI / 256);
	}

	// uniforme cubique
/*	for (i = 0;i < GMAX; i++) {
		gal[i].x = 1 * ((rand()&1023) - 512);
		gal[i].y = 1 * ((rand()&1023) - 512);
		gal[i].z = 1 * ((rand()&1023) - 512);
		gal[i].r = rand() & 63;
	}
*/

	for (i = 0; i < GMAX; i++) {
		float r, th, h, dth;

		r = rand() * 1.0 / RAND_MAX;
		r = (1 - r) * (1 - r) + 0.05;

		if (r < 0.12)
			th = rand() * M_PI * 2 / RAND_MAX;
		else {
			th = (rand() & 3) * M_PI_2 + r * r * 2;
			dth = rand() * 1.0 / RAND_MAX;
			dth = dth * dth * 2;
			th += dth;
		}
		gal[i].x = (int)(512 * r * cos(th));
		gal[i].z = (int)(512 * r * sin(th));
		h = (1 + cos(r * M_PI)) * 150;
		dth = rand() * 1.0 / RAND_MAX;
		gal[i].y = (int)(h * (dth - 0.5));
		gal[i].r = (int)((2 - r) * 60 + 31);
	}
	gal[0].x = gal[0].y = gal[0].z = 0;
	gal[0].r = 320;

	if (gSettingsWidth == 0)
		gWidth = view->Bounds().Width();
	else
		gWidth = gSettingsWidth;

	fFactor = (view->Bounds().Width()+1) / gWidth;
	if ((int)fFactor != fFactor)
		fFactor += 0.01;

	// 4:3
	gHeight = (int32)((view->Bounds().Height()+1)/fFactor + 0.5f);
	// calculate blank border format (if not in preview)
	if (!preview) switch (gBlankBorders) {
		case 1:		// 16:9
			gHeight = (int32)(gHeight * 0.703125 + 0.5);
			break;
		case 2:		// 2:3.5
			gHeight = (int32)(gHeight * 0.534 + 0.5);
			break;
		case 3:
			gHeight /= 5;
			break;
	}
	view->SetScale(fFactor);

	gBitmap = new BBitmap(BRect(0, 0, gWidth - 1, gHeight - 1), B_RGB32);
	gBuffer8 = (char*)malloc(gWidth * gHeight);

	SetTickSize((bigtime_t)(1000000LL / gMaxFramesPerSecond));
	fStarted = true;

	return B_OK;
}


void
Nebula::StopSaver()
{
	free(gBuffer8);
	gBuffer8 = NULL;

	delete gBitmap;
	gBitmap = NULL;
}


void
Nebula::Draw(BView* view, int32)
{
	if (fStarted) {
		view->SetHighColor(0, 0, 0, 0);
		view->FillRect(view->Frame());
		view->MovePenTo(0,
			(view->Bounds().Height() / fFactor - 1 - gHeight) / 2);

		fStarted = false;
	}
	uint32* buffer32 = (uint32*)gBitmap->Bits();

	drawGalaxy();

	for (int x = 0, end = gWidth * gHeight; x < end; x++)
		buffer32[x] = gPalette[(uint8)gBuffer8[x]];

	view->DrawBitmap(gBitmap);
}


// #pragma mark - instantiate_screen_saver


extern "C" _EXPORT BScreenSaver*
instantiate_screen_saver(BMessage* message, image_id image)
{
	return new Nebula(message, image);
}
