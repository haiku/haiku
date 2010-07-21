/* 
 * Copyright 2001 Werner Freytag - please read to the LICENSE file
 *
 * Copyright 2002-2006, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved.
 *		
 */

#include "ColorField.h"

#include <stdio.h>

#include <Bitmap.h>
#include <OS.h>
#include <Region.h>
#include <Window.h>

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
ColorField::ColorField(BPoint offset_point, selected_color_mode mode,
					   float fixed_value, orientation orient)
	: BControl(BRect(0.0, 0.0, MAX_X + 4.0, MAX_Y + 4.0).OffsetToCopy(offset_point),
			   "ColorField", "", new BMessage(MSG_COLOR_FIELD),
			   B_FOLLOW_LEFT | B_FOLLOW_TOP,
			   B_WILL_DRAW | B_FRAME_EVENTS),
	  fMode(mode),
	  fFixedValue(fixed_value),
	  fOrientation(orient),
	  fMarkerPosition(0.0, 0.0),
	  fLastMarkerPosition(-1.0, -1.0),
	  fMouseDown(false),
	  fUpdateThread(B_ERROR),
	  fUpdatePort(B_ERROR)
{
	SetViewColor(B_TRANSPARENT_32_BIT);

	for (int i = 0; i < 2; ++i) {
		fBgBitmap[i] = new BBitmap(Bounds(), B_RGB32, true);

		fBgBitmap[i]->Lock();
		fBgView[i] = new BView(Bounds(), "", B_FOLLOW_NONE, B_WILL_DRAW);
		fBgBitmap[i]->AddChild(fBgView[i]);
		fBgView[i]->SetOrigin(2.0, 2.0);
		fBgBitmap[i]->Unlock();
	}

	_DrawBorder();

	fUpdatePort = create_port(100, "color field update port");

	fUpdateThread = spawn_thread(ColorField::_UpdateThread,
								 "color field update thread", 10, this);
	resume_thread(fUpdateThread);

//	Update(3);
}

// destructor
ColorField::~ColorField()
{
	if (fUpdatePort >= B_OK)
		delete_port(fUpdatePort);
	if (fUpdateThread >= B_OK)
		kill_thread(fUpdateThread);

	delete fBgBitmap[0];
	delete fBgBitmap[1];
}

#if LIB_LAYOUT
// layoutprefs
minimax
ColorField::layoutprefs()
{
	if (fOrientation == B_VERTICAL) {
		mpm.mini.x = 4 + MAX_X / 17;
		mpm.mini.y = 4 + MAX_Y / 5;
	} else {
		mpm.mini.x = 4 + MAX_X / 5;
		mpm.mini.y = 4 + MAX_Y / 17;
	}
	mpm.maxi.x = 4 + MAX_X;
	mpm.maxi.y = 4 + MAX_Y;

	mpm.weight = 1.0;

	return mpm;
}

// layout
BRect
ColorField::layout(BRect frame)
{
	MoveTo(frame.LeftTop());

	// reposition marker
	fMarkerPosition.x *= (frame.Width() - 4.0) / (Bounds().Width() - 4.0);
	fMarkerPosition.y *= (frame.Height() - 4.0) / (Bounds().Height() - 4.0);

	ResizeTo(frame.Width(), frame.Height());
	_DrawBorder();
	Update(3);
	return Frame();
}
#endif // LIB_LAYOUT

// Invoke
status_t
ColorField::Invoke(BMessage *msg)
{	
	if (!msg)
		msg = Message();

	msg->RemoveName("value");

	float v1 = 0;
	float v2 = 0;
	
	switch (fMode) {
		case R_SELECTED:
		case G_SELECTED:
		case B_SELECTED:
			v1 = fMarkerPosition.x / Width();
			v2 = 1.0 - fMarkerPosition.y / Height();
			break;

		case H_SELECTED:
			if (fOrientation == B_VERTICAL) {
				v1 = fMarkerPosition.x / Width();
				v2 = 1.0 - fMarkerPosition.y / Height();
			} else {
				v1 = fMarkerPosition.y / Height();
				v2 = 1.0 - fMarkerPosition.x / Width();
			}
			break;
		
		case S_SELECTED:
		case V_SELECTED:
			v1 = fMarkerPosition.x / Width() * 6.0;
			v2 = 1.0 - fMarkerPosition.y / Height();
			break;

	}
	
	msg->AddFloat("value", v1);
	msg->AddFloat("value", v2);
	
	return BControl::Invoke(msg);
}

// AttachedToWindow
void
ColorField::AttachedToWindow()
{
	Update(3);
}

// Draw
void
ColorField::Draw(BRect updateRect)
{
	Update(0);
}

// MouseDown
void
ColorField::MouseDown(BPoint where)
{
	Window()->Activate();
	
	fMouseDown = true;
	SetMouseEventMask(B_POINTER_EVENTS, B_SUSPEND_VIEW_FOCUS|B_LOCK_WINDOW_FOCUS );
	PositionMarkerAt( where );

	if (Message()) {
		BMessage message(*Message());
		message.AddBool("begin", true);
		Invoke(&message);
	} else
		Invoke();
}

// MouseUp
void
ColorField::MouseUp(BPoint where)
{
	fMouseDown = false;
}

// MouseMoved
void
ColorField::MouseMoved(BPoint where, uint32 code, const BMessage *a_message)
{
	if (a_message || !fMouseDown ) {
		BView::MouseMoved( where, code, a_message);
		return;
	}
	
	PositionMarkerAt(where);
	Invoke();
}

// Update
void
ColorField::Update(int depth)
{
	// depth: 0 = only onscreen redraw, 1 = only cursor 1, 2 = full update part 2, 3 = full

	if (depth == 3) {
		write_port(fUpdatePort, MSG_UPDATE, NULL, 0);
		return;
	}

	if (depth >= 1) {
		fBgBitmap[1]->Lock();
		
		fBgView[1]->DrawBitmap( fBgBitmap[0], BPoint(-2.0, -2.0) );
	
		fBgView[1]->SetHighColor( 0, 0, 0 );
		fBgView[1]->StrokeEllipse( fMarkerPosition, 5.0, 5.0 );
		fBgView[1]->SetHighColor( 255.0, 255.0, 255.0 );
		fBgView[1]->StrokeEllipse( fMarkerPosition, 4.0, 4.0 );
			
		fBgView[1]->Sync();

		fBgBitmap[1]->Unlock();
	}
	
	if (depth != 0 && depth != 2 && fMarkerPosition != fLastMarkerPosition) {
	
		fBgBitmap[1]->Lock();

		DrawBitmap( fBgBitmap[1],
			BRect(-3.0, -3.0, 7.0, 7.0).OffsetByCopy(fMarkerPosition),
			BRect(-3.0, -3.0, 7.0, 7.0).OffsetByCopy(fMarkerPosition));
		DrawBitmap( fBgBitmap[1],
			BRect(-3.0, -3.0, 7.0, 7.0).OffsetByCopy(fLastMarkerPosition),
			BRect(-3.0, -3.0, 7.0, 7.0).OffsetByCopy(fLastMarkerPosition));

		fLastMarkerPosition = fMarkerPosition;

		fBgBitmap[1]->Unlock();

	} else
		DrawBitmap(fBgBitmap[1]);

}

// SetModeAndValue
void
ColorField::SetModeAndValue(selected_color_mode mode, float fixed_value)
{
	float R(0), G(0), B(0);
	float H(0), S(0), V(0);
	
	fBgBitmap[0]->Lock();

	float width = Width();
	float height = Height();

	switch (fMode) {
		
		case R_SELECTED: {
			R = fFixedValue * 255;
			G = round(fMarkerPosition.x / width * 255.0);
			B = round(255.0 - fMarkerPosition.y / height * 255.0);
		}; break;
		
		case G_SELECTED: {
			R = round(fMarkerPosition.x / width * 255.0);
			G = fFixedValue * 255;
			B = round(255.0 - fMarkerPosition.y / height * 255.0);
		}; break;

		case B_SELECTED: {
			R = round(fMarkerPosition.x / width * 255.0);
			G = round(255.0 - fMarkerPosition.y / height * 255.0);
			B = fFixedValue * 255;
		}; break;

		case H_SELECTED: {
			H = fFixedValue;
			S = fMarkerPosition.x / width;
			V = 1.0 - fMarkerPosition.y / height;
		}; break;

		case S_SELECTED: {
			H = fMarkerPosition.x / width * 6.0;
			S = fFixedValue;
			V = 1.0 - fMarkerPosition.y / height;
		}; break;

		case V_SELECTED: {
			H = fMarkerPosition.x / width * 6.0;
			S = 1.0 - fMarkerPosition.y / height;
			V = fFixedValue;
		}; break;
	}

	if (fMode & (H_SELECTED | S_SELECTED | V_SELECTED)) {
		HSV_to_RGB(H, S, V, R, G, B);
		R *= 255.0; G *= 255.0; B *= 255.0;
	}
	
	rgb_color color = { round(R), round(G), round(B), 255 };

	fBgBitmap[0]->Unlock();

	if (fFixedValue != fixed_value || fMode != mode) {
		fFixedValue = fixed_value;
		fMode = mode;
	
		Update(3);
	}

	SetMarkerToColor(color);
}

// SetFixedValue
void
ColorField::SetFixedValue(float fixed_value)
{
	if (fFixedValue != fixed_value) {
		fFixedValue = fixed_value;
		Update(3);
	}
}

// SetMarkerToColor
void
ColorField::SetMarkerToColor(rgb_color color)
{
	float h, s, v;
	RGB_to_HSV( (float)color.red / 255.0, (float)color.green / 255.0, (float)color.blue / 255.0, h, s, v );
	
	fLastMarkerPosition = fMarkerPosition;

	float width = Width();
	float height = Height();

	switch (fMode) {
		case R_SELECTED: {
			fMarkerPosition = BPoint(color.green / 255.0 * width,
									 (255.0 - color.blue) / 255.0 * height);
		} break;
		
		case G_SELECTED: {
			fMarkerPosition = BPoint(color.red / 255.0 * width,
									 (255.0 - color.blue) / 255.0 * height);
		} break;

		case B_SELECTED: {
			fMarkerPosition = BPoint(color.red / 255.0 * width,
									 (255.0 - color.green) / 255.0 * height);
		} break;
		
		case H_SELECTED: {
			if (fOrientation == B_VERTICAL)
				fMarkerPosition = BPoint(s * width, height - v * height);
			else
				fMarkerPosition = BPoint(width - v * width, s * height);
		} break;
		
		case S_SELECTED: {
			fMarkerPosition = BPoint(h / 6.0 * width, height - v * height);
		} break;

		case V_SELECTED: {
			fMarkerPosition = BPoint( h / 6.0 * width, height - s * height);
		} break;
	}

	Update(1);
}

// PositionMarkerAt
void
ColorField::PositionMarkerAt( BPoint where )
{
	BRect rect = Bounds().InsetByCopy( 2.0, 2.0 ).OffsetToCopy(0.0, 0.0);
	where = BPoint(max_c(min_c(where.x - 2.0, rect.right), 0.0),
				   max_c(min_c(where.y - 2.0, rect.bottom), 0.0) );
	
	fLastMarkerPosition = fMarkerPosition;
	fMarkerPosition = where;
	Update(1);

}

// Width
float
ColorField::Width() const
{
	return Bounds().IntegerWidth() + 1 - 4;
}

// Height
float
ColorField::Height() const
{
	return Bounds().IntegerHeight() + 1 - 4;
}

// set_bits
void
set_bits(uint8* bits, uint8 r, uint8 g, uint8 b)
{
	bits[0] = b;
	bits[1] = g;
	bits[2] = r;
//	bits[3] = 255;
}

// _UpdateThread
status_t
ColorField::_UpdateThread(void* data)
{
	// initializing
	ColorField* colorField = (ColorField *)data;
	
	bool looperLocked = colorField->LockLooper();

	BBitmap* bitmap = colorField->fBgBitmap[0];
	port_id	port = colorField->fUpdatePort;
	orientation orient = colorField->fOrientation;

	if (looperLocked)
		colorField->UnlockLooper();
	
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
		
		if (colorField->LockLooper()) {
		
			uint 	colormode = colorField->fMode;
			float	fixedvalue = colorField->fFixedValue;
		
			int width = (int)colorField->Width();
			int height = (int)colorField->Height();
		     
			colorField->UnlockLooper();
		
			bitmap->Lock();
	//bigtime_t now = system_time();
			uint8* bits = (uint8*)bitmap->Bits();
			uint32 bpr = bitmap->BytesPerRow();
			// offset 2 pixels from top and left
			bits += 2 * 4 + 2 * bpr;
			
			switch (colormode) {
				
				case R_SELECTED: {
					R = round(fixedvalue * 255);
					for (int y = height; y >= 0; y--) {
						uint8* bitsHandle = bits;
						int B = y / height * 255;
						for (int x = 0; x <= width; ++x) {
							int G = x / width * 255;
							set_bits(bitsHandle, R, G, B);
							bitsHandle += 4;
						}
						bits += bpr;
					}
				}; break;
				
				case G_SELECTED: {
					G = round(fixedvalue * 255);
					for (int y = height; y >= 0; y--) {
						uint8* bitsHandle = bits;
						int B = y / height * 255;
						for (int x = 0; x <= width; ++x) {
							int R = x / width * 255;
							set_bits(bitsHandle, R, G, B);
							bitsHandle += 4;
						}
						bits += bpr;
					}
				}; break;
				
				case B_SELECTED: {
					B = round(fixedvalue * 255);
					for (int y = height; y >= 0; y--) {
						uint8* bitsHandle = bits;
						int G = y / height * 255;
						for (int x = 0; x <= width; ++x) {
							int R = x / width * 255;
							set_bits(bitsHandle, R, G, B);
							bitsHandle += 4;
						}
						bits += bpr;
					}
				}; break;
				
				case H_SELECTED: {
					h = fixedvalue;
					if (orient == B_VERTICAL) {
						for (int y = 0; y <= height; ++y) {
							v = (float)(height - y) / height;
							uint8* bitsHandle = bits;
							for (int x = 0; x <= width; ++x) {
								s = (float)x / width;
								HSV_to_RGB( h, s, v, r, g, b );
								set_bits(bitsHandle, round(r * 255.0), round(g * 255.0), round(b * 255.0));
								bitsHandle += 4;
							}
							bits += bpr;
						}
					} else {
						for (int y = 0; y <= height; ++y) {
							s = (float)y / height;
							uint8* bitsHandle = bits;
							for (int x = 0; x <= width; ++x) {
								v = (float)(width - x) / width;
								HSV_to_RGB( h, s, v, r, g, b );
								set_bits(bitsHandle, round(r * 255.0), round(g * 255.0), round(b * 255.0));
								bitsHandle += 4;
							}
							bits += bpr;
						}
					}
				}; break;
				
				case S_SELECTED: {
					s = fixedvalue;
					for (int y = 0; y <= height; ++y) {
						v = (float)(height - y) / height;
						uint8* bitsHandle = bits;
						for (int x = 0; x <= width; ++x) {
							h = 6.0 / width * x;
							HSV_to_RGB( h, s, v, r, g, b );
							set_bits(bitsHandle, round(r * 255.0), round(g * 255.0), round(b * 255.0));
							bitsHandle += 4;
						}
						bits += bpr;
					}
				}; break;
				
				case V_SELECTED: {
					v = fixedvalue;
					for (int y = 0; y <= height; ++y) {
						s = (float)(height - y) / height;
						uint8* bitsHandle = bits;
						for (int x = 0; x <= width; ++x) {
							h = 6.0 / width * x;
							HSV_to_RGB( h, s, v, r, g, b );
							set_bits(bitsHandle, round(r * 255.0), round(g * 255.0), round(b * 255.0));
							bitsHandle += 4;
						}
						bits += bpr;
					}
				}; break;
			}
	
	//printf("color field update: %lld\n", system_time() - now);
			bitmap->Unlock();
	
			if (colorField->LockLooper()) {
				colorField->Update(2);
				colorField->UnlockLooper();
			}
		}
	}
	return B_OK;
}

// _DrawBorder
void
ColorField::_DrawBorder()
{
	bool looperLocked = LockLooper();
		
	fBgBitmap[1]->Lock();

	rgb_color background = ui_color(B_PANEL_BACKGROUND_COLOR);
	rgb_color shadow = tint_color(background, B_DARKEN_1_TINT);
	rgb_color darkShadow = tint_color(background, B_DARKEN_3_TINT);
	rgb_color light = tint_color(background, B_LIGHTEN_MAX_TINT);

	BRect r(fBgView[1]->Bounds());
	r.OffsetBy(-2.0, -2.0);
	BRegion region(r);
	fBgView[1]->ConstrainClippingRegion(&region);

	r = Bounds();
	r.OffsetBy(-2.0, -2.0);

	stroke_frame(fBgView[1], r, shadow, shadow, light, light);
	r.InsetBy(1.0, 1.0);
	stroke_frame(fBgView[1], r, darkShadow, darkShadow, background, background);
	r.InsetBy(1.0, 1.0);

	region.Set(r);
	fBgView[1]->ConstrainClippingRegion(&region);

	fBgBitmap[1]->Unlock();

	if (looperLocked)
		UnlockLooper();
}
