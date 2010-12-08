#include "HalftoneView.h"

#include <Bitmap.h>
#include <StringView.h>


HalftonePreviewView::HalftonePreviewView(BRect frame, const char* name,
	uint32 resizeMask, uint32 flags)
	:
	BView(frame, name, resizeMask, flags)
{
}


void
HalftonePreviewView::Preview(float gamma, float min,
	Halftone::DitherType ditherType, bool color)
{
	const color_space kColorSpace = B_RGB32;
	const float right = Bounds().Width();
	const float bottom = Bounds().Height();
	BRect rect(0, 0, right, bottom);
	
	BBitmap testImage(rect, kColorSpace, true);
	BBitmap preview(rect, kColorSpace);
	BView view(rect, "", B_FOLLOW_ALL, B_WILL_DRAW);
	
	// create test image
	testImage.Lock();
	testImage.AddChild(&view);
	
	// color bars
	const int height = Bounds().IntegerHeight()+1;
	const int width  = Bounds().IntegerWidth()+1;
	const int delta  = height / 4;
	const float red_bottom   = delta - 1;
	const float green_bottom = red_bottom + delta;
	const float blue_bottom  = green_bottom + delta;
	const float gray_bottom  = height - 1;
	
	for (int x = 0; x <= right; x ++) {
		uchar value = x * 255 / width;
		
		BPoint from(x, 0);
		BPoint to(x, red_bottom);
		// red
		view.SetHighColor(255, value, value);
		view.StrokeLine(from, to);
		// green
		from.y = to.y+1;
		to.y = green_bottom;
		view.SetHighColor(value, 255, value);
		view.StrokeLine(from, to);
		// blue
		from.y = to.y+1;
		to.y = blue_bottom;
		view.SetHighColor(value, value, 255);
		view.StrokeLine(from, to);
		// gray
		from.y = to.y+1;
		to.y = gray_bottom;
		view.SetHighColor(value, value, value);
		view.StrokeLine(from, to);
	}

	view.Sync();
	testImage.RemoveChild(&view);
	testImage.Unlock();
	
	// create preview image 
	Halftone halftone(kColorSpace, gamma, min, ditherType);
	halftone.SetBlackValue(Halftone::kLowValueMeansBlack);

	const int widthBytes = (width + 7) / 8; // byte boundary
	uchar* buffer = new uchar[widthBytes];
	
	const uchar* src = (uchar*)testImage.Bits();
	uchar* dstRow = (uchar*)preview.Bits();
	
	const int numPlanes = color ? 3 : 1;
	if (color) {
		halftone.SetPlanes(Halftone::kPlaneRGB1);
	}

	for (int y = 0; y < height; y ++) {
		for (int plane = 0; plane < numPlanes;  plane ++) {
			// halftone the preview image
			halftone.Dither(buffer, src, 0, y, width);
			
			// convert the plane(s) to RGB32
			ColorRGB32Little* dst = (ColorRGB32Little*)dstRow;
			const uchar* bitmap = buffer;
			for (int x = 0; x < width; x ++, dst ++) {
				const int bit = 7 - (x % 8);
				const bool isSet = (*bitmap & (1 << bit)) != 0;
				uchar value = isSet ? 255 : 0;
				
				if (color) {
					switch (plane) {
						case 0: dst->red = value;
							break;
						case 1: dst->green = value;
							break;
						case 2: dst->blue = value;
							break;
					}
				} else {
					dst->red = dst->green = dst->blue = value;
				}
				
				if (bit == 0) {
					bitmap ++;
				}
			}
		}
		
		// next row
		src += testImage.BytesPerRow();
		dstRow += preview.BytesPerRow();
	}

	delete[] buffer;
	
	SetViewBitmap(&preview);
	Invalidate();
}


HalftoneView::HalftoneView(BRect frame, const char* name, uint32 resizeMask,
	uint32 flags)
	:
	BView(frame, name, resizeMask, flags)
{
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));

	BRect r(frame);
	float size, max;
	
	r.OffsetTo(0, 0);
	const int height = r.IntegerHeight()+1;
	const int delta  = height / 4;
	const float red_top   = 0;
	const float green_top = delta;
	const float blue_top  = green_top + delta;
	const float gray_top  = r.bottom - delta;
	
	const char* kRedLabel   = "Red: ";
	const char* kGreenLabel = "Green: ";
	const char* kBlueLabel  = "Blue: ";
	const char* kGrayLabel  = "Black: ";
	

	BFont font(be_plain_font);
	font_height fh;
	font.GetHeight(&fh);

	max = size = font.StringWidth(kRedLabel);
	r.Set(0, 0, size, fh.ascent + fh.descent);
	r.OffsetTo(0, red_top);
	r.right = r.left + size;
	AddChild(new BStringView(r, "red", kRedLabel));

	size = font.StringWidth(kGreenLabel);
	r.Set(0, 0, size, fh.ascent + fh.descent);
	if (max < size) max = size;
	r.OffsetTo(0, green_top);
	r.right = r.left + size;
	AddChild(new BStringView(r, "green", kGreenLabel));

	size = font.StringWidth(kBlueLabel);
	r.Set(0, 0, size, fh.ascent + fh.descent);
	if (max < size) max = size;
	r.OffsetTo(0, blue_top);
	r.right = r.left + size;
	AddChild(new BStringView(r, "blue", kBlueLabel));

	size = font.StringWidth(kGrayLabel);
	r.Set(0, 0, size, fh.ascent + fh.descent);
	if (max < size) max = size;
	r.OffsetTo(0, gray_top);
	r.right = r.left + size;
	AddChild(new BStringView(r, "gray", kGrayLabel));

	r = frame;
	r.OffsetTo(max, 0);
	r.right -= max;
	fPreview = new HalftonePreviewView(r, "preview", resizeMask, flags);
	AddChild(fPreview);
}


void
HalftoneView::Preview(float gamma, float min,
	Halftone::DitherType ditherType, bool color)
{
	fPreview->Preview(gamma, min, ditherType, color);
}
