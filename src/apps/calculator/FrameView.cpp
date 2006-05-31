/*
 * Copyright (c) 2006, Haiku, Inc.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Peter Wagner <pwagner@stanfordalumni.org>
 */
#include <Button.h>
#include "FrameView.h"

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - *
	A FrameView is a subclass of a BView that draws a simple beveled
	rectangle just inside the view's bounds.

	The bevel_style parameter is currently unused--the only style available
	is a 1-pixel inset bevel.

	The contents of the view are erased to the "view color."  The left and
	top edges are drawn in the view's high color and the right and bottom
	edges are drawn inthe view's low color.  You may set the high, low,
	and view colors after creation to customize the FrameView's look.
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

BArchivable *FrameView::Instantiate(BMessage *archive) 
{ 
    if ( validate_instantiation(archive, "FrameView")) 
        return new FrameView(archive); 
    return NULL; 
}


FrameView::FrameView(BRect frame, const char *name,
					ulong resizeMask, int in_bevel_indent)
	   	   : BView(frame, name, resizeMask, B_WILL_DRAW|B_FRAME_EVENTS)
{
	static const rgb_color c = {215, 215, 215, 0};
	ColoringBasis(c);
	bevel_indent = in_bevel_indent;
}


FrameView::FrameView(BMessage *data)
	: BView(data)
{
	static const rgb_color c = {215, 215, 215, 0};
	ColoringBasis(c);
	
	int32 i;
	if(data->FindInt32("bevel_indent", &i) != B_OK)
		i = 1;
	if(i < 0)
		i = 0;
	if(i > 10)
		i = 10;
	bevel_indent = i;
}


void FrameView::MouseDown(BPoint where)
{
	// Pass unused mouse clicks up the view hierarchy.
	BView *view = Parent();
	if(view != NULL)
		view->MouseDown(where);
}


FrameView::FrameView(const BPoint &where, const char *name, uint32 resizeMask,
					 int items_wide, int items_high,
					 int bevel, int key_border, int key_height, int key_width,
					 const ClusterInfo button_info[])
		: BView(BRect(where.x-bevel, where.y-bevel,
					  where.x+bevel+key_border+items_wide*(key_border+key_width)-1,
					  where.y+bevel+key_border+items_high*(key_border+key_height)-1),
				name, resizeMask, B_WILL_DRAW|B_FRAME_EVENTS)
{
	static const rgb_color c = {215, 215, 215, 0};
	ColoringBasis(c);
	bevel_indent = bevel;

	for (int y=0; y<items_high; y++) {
		for (int x=0; x<items_wide; x++) {
			const ClusterInfo *info = &(button_info[x + y*items_wide]);
			char name[5];
			char *p=name;
			for (int i=0; i<4; i++)
				if (((info->message >> (24-8*i)) & 0x00FF) != 0)
					*p++ = (char)(info->message >> (24-8*i));
			*p = '\0';
			if (info->message != 0) {
				BRect r;
				r.left = bevel_indent + key_border + x*(key_border+key_width);
				r.right = r.left + key_width;
				r.top = bevel_indent + key_border + y*(key_border+key_height);
				r.bottom = r.top + key_height;
				BMessage *msg = new BMessage(info->message);
				BButton *button = new BButton(r, name, info->label, msg);
				AddChild(button);
			}
		}
	}
}


status_t FrameView::Archive(BMessage *msg, bool deep) const
{
	BView::Archive(msg, deep);
	msg->AddString("class", "FrameView");
	msg->AddInt32("bevel_indent", (int32)bevel_indent);
	return B_OK;
}


#define min(x,y) ((x<y)?x:y)
#define max(x,y) ((x>y)?x:y)

void FrameView::ColoringBasis(rgb_color basis_color)
{
	rgb_color view_color = basis_color;
	view_color.red = min(view_color.red+15, 255);
	view_color.green = min(view_color.green+15, 255);
	view_color.blue = min(view_color.blue+15, 255);
	SetViewColor(view_color);

	rgb_color hi_color = basis_color;
	hi_color.red = max(hi_color.red-45, 0);
	hi_color.green = max(hi_color.green-45, 0);
	hi_color.blue = max(hi_color.blue-45, 0);
	SetHighColor(hi_color);

	rgb_color low_color = basis_color;
	low_color.red = min(low_color.red+45, 255);
	low_color.green = min(low_color.green+45, 255);
	low_color.blue = min(low_color.blue+45, 255);
	SetLowColor(low_color);
}


void FrameView::AttachedToWindow()
{
	BView *view = Parent();
	if (view != NULL)
		ColoringBasis(view->ViewColor());
}


void FrameView::Draw(BRect)
{
	BRect r = Bounds();

//	r.InsetBy(bevel_indent/2, bevel_indent/2);
//	SetPenSize(bevel_indent);

	for (int i=0; i<bevel_indent; i++) {
		StrokeLine(r.RightTop(), r.RightBottom(), B_SOLID_LOW);
		StrokeLine(r.RightBottom(), r.LeftBottom(), B_SOLID_LOW);
		StrokeLine(r.LeftTop(), r.RightTop(), B_SOLID_HIGH);
		StrokeLine(r.LeftBottom(), r.LeftTop(), B_SOLID_HIGH);
		r.InsetBy(1,1);
	}
}


void FrameView::FrameResized(float new_width, float new_height)
{
	BRect r = Bounds();
	r.InsetBy(bevel_indent,bevel_indent);
	rgb_color old_color = HighColor();
	SetHighColor(ViewColor());
	FillRect(r);
	SetHighColor(old_color);
	Draw(Bounds());
}
