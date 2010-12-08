#include "PagesView.h"

static const float kWidth  = 80;
static const float kHeight = 30;

static const float kPageWidth  = 16;
static const float kPageHeight = 16;

static const float kPageHorizontalIndent = 7;
static const float kPageVerticalIndent = 5;

PagesView::PagesView(BRect frame, const char* name, uint32 resizeMask, uint32 flags)
	:	BView(frame, name, resizeMask, flags),
		fCollate(false),
		fReverse(false)
{
	SetViewColor(B_TRANSPARENT_COLOR);
}
	
void PagesView::GetPreferredSize(float *width, float *height)
{
	*width  = kWidth;
	*height = kHeight;
}

void PagesView::Draw(BRect rect)
{
	rgb_color backgroundColor = {255, 255, 255};
	// inherit view color from parent if available
	if (Parent() != NULL) {
		backgroundColor = Parent()->ViewColor();
	}
	SetHighColor(backgroundColor);
	FillRect(rect);
	
	BFont font(be_plain_font);
	font.SetSize(8);
	SetFont(&font);
	
	BPoint position(3, 3);
	if (fCollate) {
		BPoint next(kPageWidth + kPageHorizontalIndent * 2 + 10, 0);
		_DrawPages(position, 1, 3);
		position += next;
		_DrawPages(position, 1, 3);
	} else {
		BPoint next(kPageWidth + kPageHorizontalIndent * 1 + 10, 0);
		for (int i = 1; i <= 3; i ++) {
			int page = fReverse ? 4 - i : i;
			_DrawPages(position, page, 2);
			position += next;
		}
	}
}

void PagesView::_DrawPages(BPoint position, int number, int count)
{
	position.x += kPageHorizontalIndent * (count - 1);
	BPoint next(-kPageHorizontalIndent, kPageVerticalIndent);
	if (fCollate) {
		for (int i = 0; i < count; i ++) {
			int page;
			if (fReverse) {
				page = 1 + i;
			} else {
				page = count - i;
			}
			_DrawPage(position, page);
			position += next;
		}
	} else {
		for (int i = 0; i < count; i ++) {
			_DrawPage(position, number);
			position += next;
		}
	}
}

void PagesView::_DrawPage(BPoint position, int number)
{
	const rgb_color pageBackgroundColor = {255, 255, 255};
	const rgb_color pageBorderColor = {0, 0, 0};
	const rgb_color pageNumberColor = {0, 0, 0};
	
	BPoint page[5];
	page[0].x = position.x + 3;
	page[0].y = position.y;
	page[4].x = position.x;
	page[4].y = position.y + 3;
	page[1].x = position.x + kPageWidth - 1;
	page[1].y = position.y;
	page[2].x = position.x + kPageWidth - 1;
	page[2].y = position.y + kPageHeight - 1;
	page[3].x = position.x;
	page[3].y = position.y + kPageHeight - 1;
	
	SetHighColor(pageBackgroundColor);
	FillPolygon(page, 5);
	
	SetHighColor(pageBorderColor);
	StrokePolygon(page, 5);	
	StrokeLine(position + BPoint(3, 1), position + BPoint(3, 3));
	StrokeLine(position + BPoint(3, 3), position + BPoint(1, 3));
	
	SetHighColor(pageNumberColor);
	SetLowColor(pageBackgroundColor);
	DrawChar('0' + number, 
		position + 
		BPoint(kPageWidth - kPageHorizontalIndent + 1, kPageHeight - 2));
}
	
void PagesView::SetCollate(bool collate)
{
	fCollate = collate;
	Invalidate();
}

void PagesView::SetReverse(bool reverse)
{
	fReverse = reverse;
	Invalidate();
}
