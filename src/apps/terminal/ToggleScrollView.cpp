#include <ScrollBar.h>
#include <ScrollView.h>
#include "ToggleScrollView.h"

ToggleScrollView::ToggleScrollView(const char * name, BView * target,
							uint32 flags = 0,
							bool horizontal = false, bool vertical = false,
							border_style border = B_FANCY_BORDER,
							bool auto_hide_horizontal = false,
							bool auto_hide_vertical = false)
	: BView(target->Bounds(),name,target->ResizingMode(),flags)
{
	_name = name; _target = target; _flags = flags; 
	_horizontal = horizontal; _vertical = vertical; _border = border; 
	_auto_hide_horizontal = auto_hide_horizontal;
	_auto_hide_vertical = auto_hide_vertical;
	_target = ResizeTarget(_target,_horizontal,_vertical);
	fScrollView = new BScrollView(_name,_target,_target->ResizingMode(),_flags,_horizontal,_vertical,_border);
	AddChild(fScrollView);
}

ToggleScrollView::~ToggleScrollView()
{
}

BArchivable *
ToggleScrollView::Instantiate(BMessage *data) { }

status_t		
ToggleScrollView::Archive(BMessage *data, bool deep = true) const { }

BScrollBar *
ToggleScrollView::ScrollBar(orientation flag) const {
	return fScrollView->ScrollBar(flag);
}

// extension to BScrollView API
void
ToggleScrollView::ToggleScrollBar(bool horizontal = false, bool vertical = false) {
	if (!horizontal && !vertical) {
		return;
	}
	float delta_x = 0, delta_y = 0;
	if (vertical) {
		if (ScrollBar(B_VERTICAL) != 0) {
			delta_x = B_V_SCROLL_BAR_WIDTH;
		} else {
			delta_x = -B_V_SCROLL_BAR_WIDTH;
		}
	}
	if (horizontal) {
		if (ScrollBar(B_HORIZONTAL) != 0) {
			delta_y = B_H_SCROLL_BAR_HEIGHT;
		} else {
			delta_y = -B_H_SCROLL_BAR_HEIGHT;
		}
	}
	_target->ResizeBy(delta_x,delta_y);
	BScrollView * oldView = fScrollView;
	if (oldView) {
		oldView->RemoveChild(_target);
		RemoveChild(oldView);
		delete oldView;
	}
	fScrollView = new BScrollView(_name,_target,_target->ResizingMode(),_horizontal,_vertical,_border);
	AddChild(fScrollView);
}

void			
ToggleScrollView::SetBorder(border_style border) {
	fScrollView->SetBorder(border);
}

border_style	
ToggleScrollView::Border() const {
	return fScrollView->Border();
}

status_t
ToggleScrollView::SetBorderHighlighted(bool state) {
	return fScrollView->SetBorderHighlighted(state);
}

bool			
ToggleScrollView::IsBorderHighlighted() const {
	return fScrollView->IsBorderHighlighted();
}

void			
ToggleScrollView::SetTarget(BView *new_target) {
	if (new_target != fScrollView->Target()) {
		_target = new_target;
		ResizeTarget(_target,_horizontal,_vertical);
		fScrollView->SetTarget(_target);
		SetResizingMode(_target->ResizingMode());
	}
}

BView *
ToggleScrollView::Target() const {
	return fScrollView->Target();
}

BHandler *
ToggleScrollView::ResolveSpecifier(BMessage *msg,
										int32 index,
										BMessage *specifier,
										int32 form,
										const char *property) {
	return fScrollView->ResolveSpecifier(msg,index,specifier,form,property);
}

void			
ToggleScrollView::ResizeToPreferred() {
	fScrollView->ResizeToPreferred();
}

void			
ToggleScrollView::GetPreferredSize(float *width, float *height) {
	fScrollView->GetPreferredSize(width,height);
}

void			
ToggleScrollView::MakeFocus(bool state = true) {
	fScrollView->MakeFocus(state);
}

// overloaded functions
void
ToggleScrollView::SetFlags(uint32 flags)
{
	_flags = flags;
	fScrollView->SetFlags(_flags);
	BView::SetFlags(_flags);
}

void
ToggleScrollView::SetResizingMode(uint32 mode)
{
	_target->SetResizingMode(mode);
	fScrollView->SetResizingMode(mode);
	BView::SetResizingMode(mode);
}

// private
BView * 
ToggleScrollView::ResizeTarget(BView * target, bool horizontal, bool vertical)
{
	float delta_x = 0, delta_y = 0;
	if (vertical) {
		delta_x = -B_V_SCROLL_BAR_WIDTH;
	}
	if (horizontal) {
		delta_y = -B_H_SCROLL_BAR_HEIGHT;
	}
	target->ResizeBy(delta_x,delta_y);
	target->FrameResized(target->Bounds().Width(),target->Bounds().Height());
	return target;
}
