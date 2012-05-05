/* 
 * Copyright 2001 Werner Freytag - please read to the LICENSE file
 *
 * Copyright 2002-2006, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved.
 *		
 */

#include "ColorSlider.h"

#include <stdio.h>

#include <Bitmap.h>
#include <OS.h>
#include <Window.h>
#include <math.h>

#include "selected_color_mode.h"
#include "support_ui.h"

#include "rgb_hsv.h"

#define round(x) (int)(x+.5)

enum {
	MSG_UPDATE			= 'Updt',
};

#define MAX_X 255
#define MAX_Y 255

// constructor
ColorSlider::ColorSlider(BPoint offset_point,
						 selected_color_mode mode,
						 float value1, float value2, orientation dir)
	:  BControl(BRect(0.0, 0.0, 35.0, 265.0).OffsetToCopy(offset_point),
				"ColorSlider", "", new BMessage(MSG_COLOR_SLIDER),
				B_FOLLOW_LEFT | B_FOLLOW_TOP, B_WILL_DRAW | B_FRAME_EVENTS),
	  fMode(mode),
	  fFixedValue1(value1),
	  fFixedValue2(value2),
	  fMouseDown(false),
	  fBgBitmap(new BBitmap(Bounds(), B_RGB32, true)),
	  fBgView(NULL),
	  fUpdateThread(0),
	  fUpdatePort(0),
	  fOrientation(dir)
{
	SetViewColor(B_TRANSPARENT_32_BIT);

	if (fBgBitmap->IsValid() && fBgBitmap->Lock()) {
		fBgView = new BView(Bounds(), "", B_FOLLOW_NONE, B_WILL_DRAW);
		fBgBitmap->AddChild(fBgView);
/*		if (fOrientation == B_VERTICAL)
			fBgView->SetOrigin(8.0, 2.0);
		else
			fBgView->SetOrigin(2.0, 2.0);*/
		fBgBitmap->Unlock();
	} else {
		delete fBgBitmap;
		fBgBitmap = NULL;
		fBgView = this;
	}

	fUpdatePort = create_port(100, "color slider update port");

	fUpdateThread = spawn_thread(ColorSlider::_UpdateThread, "color slider update thread", 10, this);
	resume_thread( fUpdateThread );

	Update(2);
}

// destructor
ColorSlider::~ColorSlider()
{
	if (fUpdatePort)
		delete_port(fUpdatePort);
	if (fUpdateThread)
		kill_thread(fUpdateThread);

	delete fBgBitmap;	
}

#if LIB_LAYOUT
// layoutprefs
minimax
ColorSlider::layoutprefs()
{
	if (fOrientation == B_VERTICAL) {
		mpm.mini.x = 36;
		mpm.maxi.x = 36;
		mpm.mini.y = 10 + MAX_Y / 17;
		mpm.maxi.y = 10 + MAX_Y;
	} else {
		mpm.mini.x = 10 + MAX_X / 17;
		mpm.maxi.x = 10 + MAX_X;
		mpm.mini.y = 10.0;
		mpm.maxi.y = 12.0;
	}
	mpm.weight = 1.0;

	return mpm;
}

// layout
BRect
ColorSlider::layout(BRect frame)
{
	MoveTo(frame.LeftTop());
	ResizeTo(frame.Width(), frame.Height());
//	Update(2);
	return Frame();
}
#endif // LIB_LAYOUT

// AttachedToWindow
void
ColorSlider::AttachedToWindow()
{
	BControl::AttachedToWindow();

	SetViewColor(B_TRANSPARENT_32_BIT);

	if (fBgBitmap && fBgBitmap->Lock()) {
		fBgView->SetHighColor(ui_color(B_PANEL_BACKGROUND_COLOR));
		fBgView->FillRect(Bounds());
		fBgBitmap->Unlock();
	} else {
		SetHighColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	}

	Update(2);
}


// Invoke
status_t
ColorSlider::Invoke(BMessage *msg)
{
	if (!msg) msg = Message();
	
	msg->RemoveName("value");
	msg->RemoveName("begin");
	
	switch (fMode) {
				
		case R_SELECTED:
		case G_SELECTED:
		case B_SELECTED: {
			msg->AddFloat("value", 1.0 - (float)Value() / 255);
		} break;
		
		case H_SELECTED: {
			msg->AddFloat("value", (1.0 - (float)Value() / 255) * 6);
		} break;
		
		case S_SELECTED:
		case V_SELECTED: {
			msg->AddFloat("value", 1.0 - (float)Value() / 255);
		} break;
		
	}

	// some other parts of WonderBrush rely on this.
	// if the flag is present, it triggers generating an undo action
	// fMouseDown is not set yet the first message is sent
	if (!fMouseDown)
		msg->AddBool("begin", true);

	return BControl::Invoke(msg);
}

// Draw
void
ColorSlider::Draw(BRect updateRect)
{
	Update(0);
}

// FrameResized
void
ColorSlider::FrameResized(float width, float height)
{
	if (fBgBitmap) {
		fBgBitmap->Lock();
		delete fBgBitmap;
	}
	fBgBitmap = new BBitmap(Bounds(), B_RGB32, true);
	if (fBgBitmap->IsValid() && fBgBitmap->Lock()) {
		fBgView = new BView(Bounds(), "", B_FOLLOW_NONE, B_WILL_DRAW);
		fBgBitmap->AddChild(fBgView);
/*		if (fOrientation == B_VERTICAL)
			fBgView->SetOrigin(8.0, 2.0);
		else
			fBgView->SetOrigin(2.0, 2.0);*/
		fBgBitmap->Unlock();
	} else {
		delete fBgBitmap;
		fBgBitmap = NULL;
		fBgView = this;
	}
	Update(2);
}

// MouseDown
void
ColorSlider::MouseDown(BPoint where)
{
	Window()->Activate();
	
	SetMouseEventMask(B_POINTER_EVENTS, B_SUSPEND_VIEW_FOCUS | B_LOCK_WINDOW_FOCUS);
	_TrackMouse(where);
	fMouseDown = true;
}

// MouseUp
void
ColorSlider::MouseUp(BPoint where)
{
	fMouseDown = false;
}

// MouseMoved
void
ColorSlider::MouseMoved( BPoint where, uint32 code, const BMessage* dragMessage)
{
	if (dragMessage || !fMouseDown)
		return;
	
	_TrackMouse(where);
}

// SetValue
void
ColorSlider::SetValue(int32 value)
{
	value = max_c(min_c(value, 255), 0);
	if (value != Value()) {
		BControl::SetValue(value);
	
		Update(1);
	}
}

// Update
void
ColorSlider::Update(int depth)
{
	// depth: 0 = onscreen only, 1 = bitmap 1, 2 = bitmap 0
	if (depth == 2) {
		write_port(fUpdatePort, MSG_UPDATE, NULL, 0);
		return;
	}

	if (!Parent())
		return;
	
	fBgBitmap->Lock();

//	BRect r(fBgView->Bounds());
	BRect r(Bounds());
	BRect bounds(r);
	if (fOrientation == B_VERTICAL) {
//		r.OffsetBy(-8.0, -2.0);
		r.InsetBy(6.0, 3.0);
//		bounds.Set(-8.0, -2.0, r.right - 8.0, r.bottom - 2.0);
//		bounds.OffsetBy(8.0, 2.0);
	} else {
//		r.OffsetBy(-2.0, -2.0);
//		bounds.Set(-2.0, -2.0, r.right - 2.0, r.bottom - 2.0);
//		bounds.OffsetBy(2.0, 2.0);
	}

	fBgBitmap->Unlock();

	rgb_color background = ui_color(B_PANEL_BACKGROUND_COLOR);
	rgb_color shadow = tint_color(background, B_DARKEN_1_TINT);
	rgb_color darkShadow = tint_color(background, B_DARKEN_3_TINT);
	rgb_color light = tint_color(background, B_LIGHTEN_MAX_TINT);

	if (depth >= 1) {

		fBgBitmap->Lock();
	

		// frame
		stroke_frame(fBgView, r, shadow, shadow, light, light);
		r.InsetBy(1.0, 1.0);
		stroke_frame(fBgView, r, darkShadow, darkShadow, background, background);

		if (fOrientation == B_VERTICAL) {
			// clear area left and right from slider
			fBgView->SetHighColor( background );
			fBgView->FillRect( BRect(bounds.left, bounds.top, bounds.left + 5.0, bounds.bottom) );
			fBgView->FillRect( BRect(bounds.right - 5.0, bounds.top, bounds.right, bounds.bottom) );
		}
/*
		// marker
		if (fOrientation == B_VERTICAL) {
			// clear area left and right from slider
			fBgView->SetHighColor( background );
			fBgView->FillRect( BRect(bounds.left, bounds.top, bounds.left + 5.0, bounds.bottom) );
			fBgView->FillRect( BRect(bounds.right - 5.0, bounds.top, bounds.right, bounds.bottom) );
			// draw the triangle markers
			fBgView->SetHighColor( 0, 0, 0 );
			float value = Value();
			fBgView->StrokeLine( BPoint(bounds.left, value - 2.0), BPoint(bounds.left + 5.0, value + 3.0));
			fBgView->StrokeLine( BPoint(bounds.left, value + 8.0));
			fBgView->StrokeLine( BPoint(bounds.left, value - 2.0));
	
			fBgView->StrokeLine( BPoint(bounds.right, value - 2.0), BPoint(bounds.right - 5.0, value + 3.0));
			fBgView->StrokeLine( BPoint(bounds.right, value + 8.0));
			fBgView->StrokeLine( BPoint(bounds.right, value - 2.0));
		} else {
			r.InsetBy(1.0, 1.0);
			float value = (Value() / 255.0) * (bounds.Width() - 4.0);
			if (value - 2 > r.left) {
				fBgView->SetHighColor( 255, 255, 255 );
				fBgView->StrokeLine( BPoint(value - 2, bounds.top + 2.0),
									 BPoint(value - 2, bounds.bottom - 2.0));
			}
			if (value - 1 > r.left) {
				fBgView->SetHighColor( 0, 0, 0 );
				fBgView->StrokeLine( BPoint(value - 1, bounds.top + 2.0),
									 BPoint(value - 1, bounds.bottom - 2.0));
			}
			if (value + 1 < r.right) {
				fBgView->SetHighColor( 0, 0, 0 );
				fBgView->StrokeLine( BPoint(value + 1, bounds.top + 2.0),
									 BPoint(value + 1, bounds.bottom - 2.0));
			}
			if (value + 2 < r.right) {
				fBgView->SetHighColor( 255, 255, 255 );
				fBgView->StrokeLine( BPoint(value + 2, bounds.top + 2.0),
									 BPoint(value + 2, bounds.bottom - 2.0));
			}
		}*/

		fBgView->Sync();

		fBgBitmap->Unlock();
	} else
		r.InsetBy(1.0, 1.0);

	DrawBitmap(fBgBitmap, BPoint(0.0, 0.0));

	// marker
	if (fOrientation == B_VERTICAL) {
		// draw the triangle markers
		SetHighColor( 0, 0, 0 );
		float value = Value();
		StrokeLine( BPoint(bounds.left, value),
					BPoint(bounds.left + 5.0, value + 5.0));
		StrokeLine( BPoint(bounds.left, value + 10.0));
		StrokeLine( BPoint(bounds.left, value));

		StrokeLine( BPoint(bounds.right, value),
					BPoint(bounds.right - 5.0, value + 5.0));
		StrokeLine( BPoint(bounds.right, value + 10.0));
		StrokeLine( BPoint(bounds.right, value));
	} else {
		r.InsetBy(1.0, 1.0);
		float value = (Value() / 255.0) * (bounds.Width() - 4.0);
		if (value > r.left) {
			SetHighColor( 255, 255, 255 );
			StrokeLine( BPoint(value, bounds.top + 2.0),
						BPoint(value, bounds.bottom - 2.0));
		}
		if (value + 1 > r.left) {
			SetHighColor( 0, 0, 0 );
			StrokeLine( BPoint(value + 1, bounds.top + 2.0),
						BPoint(value + 1, bounds.bottom - 2.0));
		}
		if (value + 3 < r.right) {
			SetHighColor( 0, 0, 0 );
			StrokeLine( BPoint(value + 3, bounds.top + 2.0),
						BPoint(value + 3, bounds.bottom - 2.0));
		}
		if (value + 4 < r.right) {
			SetHighColor( 255, 255, 255 );
			StrokeLine( BPoint(value + 4, bounds.top + 2.0),
						BPoint(value + 4, bounds.bottom - 2.0));
		}
	}
	SetOrigin(0.0, 0.0);
}

// SetModeAndValues
void
ColorSlider::SetModeAndValues(selected_color_mode mode,
							  float value1, float value2)
{
	float R(0), G(0), B(0);
	float h(0), s(0), v(0);
	
	fBgBitmap->Lock();
	
	switch (fMode) {
		
		case R_SELECTED: {
			R = 255 - Value();
			G = round(fFixedValue1 * 255.0);
			B = round(fFixedValue2 * 255.0);
		}; break;
		
		case G_SELECTED: {
			R = round(fFixedValue1 * 255.0);
			G = 255 - Value();
			B = round(fFixedValue2 * 255.0);
		}; break;
		
		case B_SELECTED: {
			R = round(fFixedValue1 * 255.0);
			G = round(fFixedValue2 * 255.0);
			B = 255 - Value();
		}; break;
		
		case H_SELECTED: {
			h = (1.0 - (float)Value()/255.0)*6.0;
			s = fFixedValue1;
			v = fFixedValue2;
		}; break;
		
		case S_SELECTED: {
			h = fFixedValue1;
			s = 1.0 - (float)Value()/255.0;
			v = fFixedValue2;
		}; break;
		
		case V_SELECTED: {
			h = fFixedValue1;
			s = fFixedValue2;
			v = 1.0 - (float)Value()/255.0;
		}; break;
	}

	if (fMode & (H_SELECTED|S_SELECTED|V_SELECTED) ) {
		HSV_to_RGB(h, s, v, R, G, B);
		R*=255.0; G*=255.0; B*=255.0;
	}
	
	rgb_color color = { round(R), round(G), round(B), 255 };
	
	fMode = mode;
	SetOtherValues(value1, value2);
	fBgBitmap->Unlock();

	SetMarkerToColor( color );
	Update(2);
}

// SetOtherValues
void
ColorSlider::SetOtherValues(float value1, float value2)
{
	fFixedValue1 = value1;
	fFixedValue2 = value2;

	if (fMode != H_SELECTED) {
		Update(2);
	}
}

// GetOtherValues
void
ColorSlider::GetOtherValues(float* value1, float* value2) const
{
	if (value1 && value2) {
		*value1 = fFixedValue1;
		*value2 = fFixedValue2;
	}
}

// SetMarkerToColor
void
ColorSlider::SetMarkerToColor(rgb_color color)
{
	float h = 0.0f;
	float s = 0.0f;
	float v = 0.0f;
	if ((fMode & (H_SELECTED | S_SELECTED | V_SELECTED)) != 0) {
		RGB_to_HSV((float)color.red / 255.0f, (float)color.green / 255.0f,
			(float)color.blue / 255.0f, h, s, v);
	}
	
	switch (fMode) {
		case R_SELECTED:
			SetValue(255 - color.red);
			break;
		
		case G_SELECTED:
			SetValue(255 - color.green);
			break;

		case B_SELECTED:
			SetValue(255 - color.blue);
			break;
		
		case H_SELECTED:
			SetValue(255.0 - round(h / 6.0 * 255.0));
			break;
		
		case S_SELECTED:
			SetValue(255.0 - round(s * 255.0));
			break;
		
		case V_SELECTED:
			SetValue(255.0 - round(v * 255.0));
			break;
	}
}

// _UpdateThread
status_t
ColorSlider::_UpdateThread(void* data)
{
	// initializing
	ColorSlider* colorSlider = (ColorSlider*)data;
	
	bool looperLocked = colorSlider->LockLooper();

	port_id	port = colorSlider->fUpdatePort;
	orientation orient = colorSlider->fOrientation;

	if (looperLocked)
		colorSlider->UnlockLooper();
	
	float h, s, v, r, g, b;
	int R, G, B;
		
	// drawing

    int32 msg_code;
    char msg_buffer;

	while (true) {

		port_info info;

		do {

			read_port(port, &msg_code, &msg_buffer, sizeof(msg_buffer));
			get_port_info(port, &info);
			
		} while (info.queue_count);
		
		if (colorSlider->LockLooper()) {
	
			uint 	colormode = colorSlider->fMode;
			float	fixedvalue1 = colorSlider->fFixedValue1;
			float	fixedvalue2 = colorSlider->fFixedValue2;
		    
			BBitmap* bitmap = colorSlider->fBgBitmap;
			BView* view = colorSlider->fBgView;

			bitmap->Lock();

			colorSlider->UnlockLooper();	
	
			view->BeginLineArray(256);
			
			switch (colormode) {
				
				case R_SELECTED: {
					G = round(fixedvalue1 * 255);
					B = round(fixedvalue2 * 255);
					for (int R = 0; R < 256; ++R) {
						_DrawColorLineY( view, R, R, G, B );
					}
				}; break;
				
				case G_SELECTED: {
					R = round(fixedvalue1 * 255);
					B = round(fixedvalue2 * 255);
					for (int G = 0; G < 256; ++G) {
						_DrawColorLineY( view, G, R, G, B );
					}
				}; break;
				
				case B_SELECTED: {
					R = round(fixedvalue1 * 255);
					G = round(fixedvalue2 * 255);
					for (int B = 0; B < 256; ++B) {
						_DrawColorLineY( view, B, R, G, B );
					}
				}; break;
				
				case H_SELECTED: {
					s = 1.0;//fixedvalue1;
					v = 1.0;//fixedvalue2;
					if (orient == B_VERTICAL) {
						for (int y = 0; y < 256; ++y) {
							HSV_to_RGB( (float)y*6.0/255.0, s, v, r, g, b );
							_DrawColorLineY( view, y, r*255, g*255, b*255 );
						}
					} else {
						for (int x = 0; x < 256; ++x) {
							HSV_to_RGB( (float)x*6.0/255.0, s, v, r, g, b );
							_DrawColorLineX( view, x, r*255, g*255, b*255 );
						}
					}
				}; break;
				
				case S_SELECTED: {
					h = fixedvalue1;
					v = 1.0;//fixedvalue2;
					for (int y = 0; y < 256; ++y) {
						HSV_to_RGB( h, (float)y/255, v, r, g, b );
						_DrawColorLineY( view, y, r*255, g*255, b*255 );
					}
				}; break;
				
				case V_SELECTED: {
					h = fixedvalue1;
					s = 1.0;//fixedvalue2;
					for (int y = 0; y < 256; ++y) {
						HSV_to_RGB( h, s, (float)y/255, r, g, b );
						_DrawColorLineY( view, y, r*255, g*255, b*255 );
					}
				}; break;
			}
		
			view->EndLineArray();
			view->Sync();
			bitmap->Unlock();

			if (colorSlider->LockLooper()) {
				colorSlider->Update(1);
				colorSlider->UnlockLooper();
			}
		}
	}
	return B_OK;
}

// _DrawColorLineY
void
ColorSlider::_DrawColorLineY(BView *view, float y,
							 int r, int g, int b)
{
	rgb_color color = { r, g, b, 255 };
	y = 255.0 - y;
	view->AddLine( BPoint(8.0, y + 5.0), BPoint(27.0, y + 5.0), color );
}

// _DrawColorLineX
void
ColorSlider::_DrawColorLineX(BView *view, float x,
							 int r, int g, int b)
{
	rgb_color color = { r, g, b, 255 };
	BRect bounds(view->Bounds());
	x = (255.0 - x) * (bounds.Width() - 2.0) / 255.0 + 2.0;
	view->AddLine( BPoint(x, bounds.top + 2.0), BPoint(x, bounds.bottom - 2.0), color );
}

// _TrackMouse
void
ColorSlider::_TrackMouse(BPoint where)
{
	if (fOrientation == B_VERTICAL) {
		SetValue((int)where.y - 2);
	} else {
		BRect b(Bounds());
		SetValue((int)(((where.x - 2.0) / b.Width()) * 255.0));
	}
	Invoke();
}
