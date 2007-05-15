/*
 * Copyright 2007, Ingo Weinhold <bonefish@cs.tu-berlin.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include <math.h>
#include <stdio.h>

#include <Application.h>
#include <Button.h>
#include <Invoker.h>
#include <LayoutUtils.h>
#include <List.h>
#include <Region.h>
#include <String.h>
#include <View.h>
#include <Window.h>


// internal messages
enum {
	MSG_LAYOUT_CONTAINER			= 'layc',
	MSG_2D_SLIDER_VALUE_CHANGED		= '2dsv',
	MSG_UNLIMITED_MAX_SIZE_CHANGED	= 'usch',
};


// missing operators in BPoint

BPoint
operator+(const BPoint& p, const BSize& size)
{
	return BPoint(p.x + size.width, p.y + size.height);
}


// View
class View {
public:
	View()
		: fFrame(0, 0, 0, 0),
		  fContainer(NULL),
		  fParent(NULL),
		  fChildren(),
		  fViewColor(B_TRANSPARENT_32_BIT),
		  fLayoutValid(false)
	{
	}

	View(BRect frame)
		: fFrame(frame),
		  fContainer(NULL),
		  fParent(NULL),
		  fChildren(),
		  fViewColor(B_TRANSPARENT_32_BIT),
		  fLayoutValid(false)
	{
	}

	virtual ~View()
	{
		// delete children
		for (int32 i = CountChildren() - 1; i >= 0; i--)
			delete RemoveChild(i);
	}

	virtual void SetFrame(BRect frame)
	{
		if (frame != fFrame && frame.IsValid()) {
			BRect oldFrame(frame);
			Invalidate();
			fFrame = frame;
			Invalidate();

			FrameChanged(oldFrame, frame);
		}

		// relayout if necessary
		if (!fLayoutValid) {
			Layout();
			fLayoutValid = true;
		}
	}

	BRect Frame() const
	{
		return fFrame;
	}

	BRect Bounds() const
	{
		return BRect(fFrame).OffsetToCopy(B_ORIGIN);
	}

	void SetLocation(BPoint location)
	{
		SetFrame(fFrame.OffsetToCopy(location));
	}

	BPoint Location() const
	{
		return fFrame.LeftTop();
	}

	void SetSize(BSize size)
	{
		BRect frame(fFrame);
		frame.right = frame.left + size.width;
		frame.bottom = frame.top + size.height;
		SetFrame(frame);
	}

	BSize Size() const
	{
		return Frame().Size();
	}

	virtual BSize MinSize()
	{
		return BSize(-1, -1);
	}

	virtual BSize MaxSize()
	{
		return BSize(B_SIZE_UNLIMITED, B_SIZE_UNLIMITED);
	}

	virtual BSize PreferredSize()
	{
		return MinSize();
	}

	virtual BAlignment Alignment()
	{
		return BAlignment(B_ALIGN_HORIZONTAL_CENTER, B_ALIGN_VERTICAL_CENTER);
	}

	BPoint LocationInContainer() const
	{
		BPoint location = fFrame.LeftTop();
		return (fParent ? fParent->LocationInContainer() + location : location);
	}

	BRect FrameInContainer() const
	{
		BRect frame(fFrame);
		return frame.OffsetToCopy(LocationInContainer());
	}

	BPoint ConvertFromContainer(BPoint point) const
	{
		return point - LocationInContainer();
	}

	BRect ConvertFromContainer(BRect rect) const
	{
		return rect.OffsetBySelf(-LocationInContainer());
	}

	BPoint ConvertToContainer(BPoint point) const
	{
		return point + LocationInContainer();
	}

	BRect ConvertToContainer(BRect rect) const
	{
		return rect.OffsetBySelf(LocationInContainer());
	}

	View* Parent() const
	{
		return fParent;
	}

	BView* Container() const
	{
		return fContainer;
	}

	bool AddChild(View* child)
	{
		if (!child)
			return false;

		if (child->Parent() || child->Container()) {
			fprintf(stderr, "View::AddChild(): view %p already has a parent "
				"or is the container view\n", child);
			return false;
		}

		if (!fChildren.AddItem(child))
			return false;

		child->_AddedToParent(this);

		child->Invalidate();
		InvalidateLayout();

		return true;
	}

	bool RemoveChild(View* child)
	{
		if (!child)
			return false;

		return RemoveChild(IndexOfChild(child));
	}

	View* RemoveChild(int32 index)
	{
		if (index < 0 || index >= fChildren.CountItems())
			return NULL;

		View* child = ChildAt(index);
		child->Invalidate();
		child->_RemovingFromParent();
		fChildren.RemoveItem(index);

		InvalidateLayout();

		return child;
	}

	int32 CountChildren() const
	{
		return fChildren.CountItems();
	}

	View* ChildAt(int32 index) const
	{
		return (View*)fChildren.ItemAt(index);
	}

	View* ChildAt(BPoint point) const
	{
		for (int32 i = 0; View* child = ChildAt(i); i++) {
			if (child->Frame().Contains(point))
				return child;
		}

		return NULL;
	}

	View* AncestorAt(BPoint point, BPoint* localPoint = NULL) const
	{
		if (!Bounds().Contains(point))
			return NULL;

		View* view = const_cast<View*>(this);

		// Iterate deeper down the hierarchy, until we reach a view that
		// doesn't have a child at the location.
		while (true) {
			View* child = view->ChildAt(point);
			if (!child) {
				if (localPoint)
					*localPoint = point;
				return view;
			}

			view = child;
			point -= view->Frame().LeftTop();
		}
	}

	int32 IndexOfChild(View* child) const
	{
		return (child ? fChildren.IndexOf(child) : -1);
	}

	void Invalidate(BRect rect)
	{
		if (fContainer) {
			rect = rect & Bounds();
			fContainer->Invalidate(rect.OffsetByCopy(LocationInContainer()));
		}
	}

	void Invalidate()
	{
		Invalidate(Bounds());
	}

	virtual void InvalidateLayout()
	{
		if (fLayoutValid) {
			fLayoutValid = false;
			if (fParent)
				fParent->InvalidateLayout();
		}
	}

	bool IsLayoutValid() const
	{
		return fLayoutValid;
	}

	void SetViewColor(rgb_color color)
	{
		fViewColor = color;
	}

	virtual void Draw(BView* container, BRect updateRect)
	{
	}

	virtual void MouseDown(BPoint where, uint32 buttons, int32 modifiers)
	{
	}

	virtual void MouseUp(BPoint where, uint32 buttons, int32 modifiers)
	{
	}

	virtual void MouseMoved(BPoint where, uint32 buttons, int32 modifiers)
	{
	}

	virtual void AddedToContainer()
	{
	}

	virtual void RemovingFromContainer()
	{
	}

	virtual void FrameChanged(BRect oldFrame, BRect newFrame)
	{
	}

	virtual void Layout()
	{
		// simply trigger relayouting the children
		for (int32 i = 0; View* child = ChildAt(i); i++)
			child->SetFrame(child->Frame());
	}

protected:
	virtual void _AddedToParent(View* parent)
	{
		fParent = parent;

		if (parent->Container()) {
			Invalidate();
			_AddedToContainer(parent->Container());
		}
	}

	virtual void _RemovingFromParent()
	{
		if (fContainer)
			_RemovingFromContainer();

		fParent = NULL;
	}

	virtual void _AddedToContainer(BView* container)
	{
		fContainer = container;

		AddedToContainer();

		for (int32 i = 0; View* child = ChildAt(i); i++)
			child->_AddedToContainer(fContainer);
	}

	virtual void _RemovingFromContainer()
	{
		for (int32 i = 0; View* child = ChildAt(i); i++)
			child->_RemovingFromContainer();

		RemovingFromContainer();

		fContainer = NULL;
	}

	void _Draw(BView* container, BRect updateRect)
	{
		// compute the clipping region
		BRegion region(Bounds());
		for (int32 i = 0; View* child = ChildAt(i); i++)
			region.Exclude(child->Frame());

		if (region.Frame().IsValid()) {
			// set the clipping region
			container->ConstrainClippingRegion(&region);

			// draw the background, if it isn't transparent
			if (fViewColor.alpha != 0) {
				container->SetLowColor(fViewColor);
				container->FillRect(updateRect, B_SOLID_LOW);
			}

			// draw this view
			Draw(container, updateRect);

			// revert the clipping region
			region.Set(Bounds());
			container->ConstrainClippingRegion(&region);
		}

		// draw the children
		if (CountChildren() > 0) {
			container->PushState();

			for (int32 i = 0; View* child = ChildAt(i); i++) {
				BRect childFrame = child->Frame();
				BRect childUpdateRect = updateRect & childFrame;
				if (childUpdateRect.IsValid()) {
					// set origin
					childUpdateRect.OffsetBy(-childFrame.LeftTop());
					container->SetOrigin(childFrame.LeftTop());

					// draw
					child->_Draw(container, childUpdateRect);
				}
			}

			container->PopState();
		}
	}

private:
	BRect		fFrame;
	BView*		fContainer;
	View*		fParent;
	BList		fChildren;
	rgb_color	fViewColor;
	bool		fLayoutValid;
};


// ViewContainer
class ViewContainer : public BView, public View {
public:
	ViewContainer(BRect frame)
		: BView(frame, "view container", B_FOLLOW_NONE, B_WILL_DRAW),
		  View(frame.OffsetToCopy(B_ORIGIN)),
		  fMouseFocus(NULL)
	{
		BView::SetViewColor(B_TRANSPARENT_32_BIT);
		_AddedToContainer(this);
	}

	virtual void InvalidateLayout()
	{
		if (View::IsLayoutValid()) {
			View::InvalidateLayout();

			// trigger asynchronous re-layout
			if (Window())
				Window()->PostMessage(MSG_LAYOUT_CONTAINER, this);
		}
	}

	virtual void MessageReceived(BMessage* message)
	{
		switch (message->what) {
			case MSG_LAYOUT_CONTAINER:
				View::Layout();
				break;
			default:
				BView::MessageReceived(message);
				break;
		}
	}

	virtual void AllAttached()
	{
		Window()->PostMessage(MSG_LAYOUT_CONTAINER, this);
	}

	virtual void Draw(BRect updateRect)
	{
		View::_Draw(this, updateRect);
	}

	virtual void MouseDown(BPoint where)
	{
		// get mouse buttons and modifiers
		uint32 buttons;
		int32 modifiers;
		_GetButtonsAndModifiers(buttons, modifiers);

		// get mouse focus
		if (!fMouseFocus && (buttons & B_PRIMARY_MOUSE_BUTTON)) {
			fMouseFocus = AncestorAt(where);
			if (fMouseFocus)
				SetMouseEventMask(B_POINTER_EVENTS);
		}

		// call hook
		if (fMouseFocus) {
			fMouseFocus->MouseDown(fMouseFocus->ConvertFromContainer(where),
				buttons, modifiers);
		}
	}

	virtual void MouseUp(BPoint where)
	{
		if (!fMouseFocus)
			return;

		// get mouse buttons and modifiers
		uint32 buttons;
		int32 modifiers;
		_GetButtonsAndModifiers(buttons, modifiers);

		// call hook
		if (fMouseFocus) {
			fMouseFocus->MouseUp(fMouseFocus->ConvertFromContainer(where),
				buttons, modifiers);
		}

		// unset the mouse focus when the primary button has been released
		if (!(buttons & B_PRIMARY_MOUSE_BUTTON))
			fMouseFocus = NULL;
	}

	virtual void MouseMoved(BPoint where, uint32 code, const BMessage* message)
	{
		if (!fMouseFocus)
			return;

		// get mouse buttons and modifiers
		uint32 buttons;
		int32 modifiers;
		_GetButtonsAndModifiers(buttons, modifiers);

		// call hook
		if (fMouseFocus) {
			fMouseFocus->MouseMoved(fMouseFocus->ConvertFromContainer(where),
				buttons, modifiers);
		}
	}

	virtual void Draw(BView* container, BRect updateRect)
	{
	}

	virtual void MouseDown(BPoint where, uint32 buttons, int32 modifiers)
	{
	}

	virtual void MouseUp(BPoint where, uint32 buttons, int32 modifiers)
	{
	}

	virtual void MouseMoved(BPoint where, uint32 buttons, int32 modifiers)
	{
	}

private:
	void _GetButtonsAndModifiers(uint32& buttons, int32& modifiers)
	{
		buttons = 0;
		modifiers = 0;

		if (BMessage* message = Window()->CurrentMessage()) {
			if (message->FindInt32("buttons", (int32*)&buttons) != B_OK)
				buttons = 0;
			if (message->FindInt32("modifiers", modifiers) != B_OK)
				modifiers = 0;
		}
	}

private:
	View*	fMouseFocus;
};


// TwoDimensionalSliderView
class TwoDimensionalSliderView : public View, public BInvoker {
public:
	TwoDimensionalSliderView(BMessage* message = NULL,
		BMessenger target = BMessenger())
		: View(BRect(0, 0, 4, 4)),
		  BInvoker(message, target),
		  fMinLocation(0, 0),
		  fMaxLocation(0, 0),
		  fDragging(false)
	{
		SetViewColor((rgb_color){0, 120, 0, 255});
	}

	void SetLocationRange(BPoint minLocation, BPoint maxLocation)
	{
		if (maxLocation.x < minLocation.x)
			maxLocation.x = minLocation.x;
		if (maxLocation.y < minLocation.y)
			maxLocation.y = minLocation.y;

		fMinLocation = minLocation;
		fMaxLocation = maxLocation;

		// force valid value
		SetValue(Value());
	}

	BPoint MinLocation() const
	{
		return fMinLocation;
	}

	BPoint MaxLocation() const
	{
		return fMaxLocation;
	}

	BPoint Value() const
	{
		return Location() - fMinLocation;
	}

	void SetValue(BPoint value)
	{
		BPoint location = fMinLocation + value;
		if (location.x < fMinLocation.x)
			location.x = fMinLocation.x;
		if (location.y < fMinLocation.y)
			location.y = fMinLocation.y;
		if (location.x > fMaxLocation.x)
			location.x = fMaxLocation.x;
		if (location.y > fMaxLocation.y)
			location.y = fMaxLocation.y;

		if (location != Location()) {
			SetFrame(Frame().OffsetToCopy(location));

			// send the message
			if (Message()) {
				BMessage message(*Message());
				message.AddPoint("value", Value());
				InvokeNotify(&message);
			}
		}
	}

	virtual void MouseDown(BPoint where, uint32 buttons, int32 modifiers)
	{
		if (fDragging)
			return;

		fOriginalLocation = Frame().LeftTop();
		fOriginalPoint = ConvertToContainer(where);
		fDragging = true;
	}

	virtual void MouseUp(BPoint where, uint32 buttons, int32 modifiers)
	{
		if (!fDragging || (buttons & B_PRIMARY_MOUSE_BUTTON))
			return;

		fDragging = false;
	}

	virtual void MouseMoved(BPoint where, uint32 buttons, int32 modifiers)
	{
		if (!fDragging)
			return;

		BPoint moved = ConvertToContainer(where) - fOriginalPoint;
		SetValue(fOriginalLocation - fMinLocation + moved);
	}

private:
	BPoint	fMinLocation;
	BPoint	fMaxLocation;
	bool	fDragging;
	BPoint	fOriginalPoint;
	BPoint	fOriginalLocation;
};


// WrapperView
class WrapperView : public View {
public:
	WrapperView(BView* view)
		: View(),
		  fView(view),
		  fInsets(1, 1, 1, 1)
	{
		SetViewColor((rgb_color){255, 0, 0, 255});
	}

	BView* GetView() const
	{
		return fView;
	}

	virtual BSize MinSize()
	{
		return _FromViewSize(fView->MinSize());
	}

	virtual BSize MaxSize()
	{
		return _FromViewSize(fView->MaxSize());
	}

	virtual BSize PreferredSize()
	{
		return _FromViewSize(fView->PreferredSize());
	}

	virtual void AddedToContainer()
	{
		_UpdateViewFrame();

		Container()->AddChild(fView);
	}

	virtual void RemovingFromContainer()
	{
		Container()->RemoveChild(fView);
	}

	virtual void FrameChanged(BRect oldFrame, BRect newFrame)
	{
		_UpdateViewFrame();
	}

private:
	void _UpdateViewFrame()
	{
		BRect frame(_ViewFrameInContainer());
		fView->MoveTo(frame.LeftTop());
		fView->ResizeTo(frame.Width(), frame.Height());
	}

	BRect _ViewFrame() const
	{
		BRect viewFrame(Bounds());
		viewFrame.left += fInsets.left;
		viewFrame.top += fInsets.top;
		viewFrame.right -= fInsets.right;
		viewFrame.bottom -= fInsets.bottom;

		return viewFrame;
	}

	BRect _ViewFrameInContainer() const
	{
		return ConvertToContainer(_ViewFrame());
	}

	BSize _FromViewSize(BSize size) const
	{
		float horizontalInsets = fInsets.left + fInsets.right - 1;
		float verticalInsets = fInsets.top + fInsets.bottom - 1;
		return BSize(BLayoutUtils::AddDistances(size.width, horizontalInsets),
			BLayoutUtils::AddDistances(size.height, verticalInsets));
	}

private:
	BView*	fView;
	BRect	fInsets;
};


// StringView
class StringView : public View {
public:
	StringView(const char* string)
		: View(),
		  fString(string),
		  fAlignment(B_ALIGN_LEFT),
		  fStringAscent(0),
		  fStringDescent(0),
		  fStringWidth(0),
		  fExplicitMinSize(B_SIZE_UNSET, B_SIZE_UNSET)
	{
		SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
		fTextColor = (rgb_color){ 0, 0, 0, 255 };
	}

	void SetString(const char* string)
	{
		fString = string;

		_UpdateStringMetrics();
		Invalidate();
		InvalidateLayout();
	}

	void SetAlignment(alignment align)
	{
		if (align != fAlignment) {
			fAlignment = align;
			Invalidate();
		}
	}

	void SetTextColor(rgb_color color)
	{
		fTextColor = color;
		Invalidate();
	}

	virtual BSize MinSize()
	{
		BSize size(fExplicitMinSize);
		if (!size.IsWidthSet())
			size.width = fStringWidth - 1;
		if (!size.IsHeightSet())
			size.height = fStringAscent + fStringDescent - 1;
		return size;
	}

	void SetExplicitMinSize(BSize size)
	{
		fExplicitMinSize = size;
	}

	virtual void AddedToContainer()
	{
		_UpdateStringMetrics();
	}

	virtual void Draw(BView* container, BRect updateRect)
	{
		BSize size(Size());
		int widthDiff = (int)size.width + 1 - (int)fStringWidth;
		int heightDiff = (int)size.height + 1
			- (int)(fStringAscent + (int)fStringDescent);
		BPoint base;

		// horizontal alignment
		switch (fAlignment) {
			case B_ALIGN_RIGHT:
				base.x = widthDiff;
				break;
			case B_ALIGN_LEFT:
			default:
				base.x = 0;
				break;
		}

		base.y = heightDiff / 2 + fStringAscent;

		container->SetHighColor(fTextColor);
		container->DrawString(fString.String(), base);
	}

private:
	void _UpdateStringMetrics()
	{
		BView* container = Container();
		if (!container)
			return;

		BFont font;
		container->GetFont(&font);

		font_height fh;
		font.GetHeight(&fh);

		fStringAscent = ceilf(fh.ascent);
		fStringDescent = ceilf(fh.descent);
		fStringWidth = font.StringWidth(fString.String());
	}

private:
	BString		fString;
	alignment	fAlignment;
	rgb_color	fTextColor;
	float		fStringAscent;
	float		fStringDescent;
	float		fStringWidth;
	BSize		fExplicitMinSize;
};


// CheckBox
class CheckBox : public View, public BInvoker {
public:
	CheckBox(BMessage* message = NULL, BMessenger target = BMessenger())
		: View(BRect(0, 0, 12, 12)),
		  BInvoker(message, target),
		  fSelected(false),
		  fPressed(false)
	{
		SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	}

	void SetSelected(bool selected)
	{
		if (selected != fSelected) {
			fSelected = selected;
			Invalidate();

			// send the message
			if (Message()) {
				BMessage message(*Message());
				message.AddBool("selected", IsSelected());
				InvokeNotify(&message);
			}
		}
	}

	bool IsSelected() const
	{
		return fSelected;
	}

	virtual BSize MinSize()
	{
		return BSize(12, 12);
	}

	virtual BSize MaxSize()
	{
		return MinSize();
	}

	virtual void Draw(BView* container, BRect updateRect)
	{
		BRect rect(Bounds());

		if (fPressed)
			container->SetHighColor((rgb_color){ 120, 0, 0, 255 });
		else
			container->SetHighColor((rgb_color){ 0, 0, 0, 255 });

		container->StrokeRect(rect);

		if (fPressed ? fPressedSelected : fSelected) {
			rect.InsetBy(2, 2);
			container->StrokeLine(rect.LeftTop(), rect.RightBottom());
			container->StrokeLine(rect.RightTop(), rect.LeftBottom());
		}
	}

	virtual void MouseDown(BPoint where, uint32 buttons, int32 modifiers)
	{
		if (fPressed)
			return;

		fPressed = true;
		fPressedSelected = fSelected;
		_PressedUpdate(where);
	}

	virtual void MouseUp(BPoint where, uint32 buttons, int32 modifiers)
	{
		if (!fPressed || (buttons & B_PRIMARY_MOUSE_BUTTON))
			return;

		_PressedUpdate(where);
		fPressed = false;
		SetSelected(fPressedSelected);
		Invalidate();
	}

	virtual void MouseMoved(BPoint where, uint32 buttons, int32 modifiers)
	{
		if (!fPressed)
			return;

		_PressedUpdate(where);
	}

private:
	void _PressedUpdate(BPoint where)
	{
		bool pressedSelected = Bounds().Contains(where) ^ fSelected;
		if (pressedSelected != fPressedSelected) {
			fPressedSelected = pressedSelected;
			Invalidate();
		}
	}

private:
	bool	fSelected;
	bool	fPressed;
	bool	fPressedSelected;
};


// GroupView
class GroupView : public View {
public:
	GroupView(enum orientation orientation, int32 lineCount = 1)
		: View(BRect(0, 0, 0, 0)),
		  fOrientation(orientation),
		  fLineCount(lineCount),
		  fColumnSpacing(0),
		  fRowSpacing(0),
		  fInsets(0, 0, 0, 0),
		  fMinMaxValid(false),
		  fColumnInfos(NULL),
		  fRowInfos(NULL)
	{
		SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));

		if (fLineCount < 1)
			fLineCount = 1;
	}

	virtual ~GroupView()
	{
		delete fColumnInfos;
		delete fRowInfos;
	}

	void SetSpacing(float horizontal, float vertical)
	{
		if (horizontal != fColumnSpacing || vertical != fRowSpacing) {
			fColumnSpacing = horizontal;
			fRowSpacing = vertical;

			InvalidateLayout();
		}
	}

	void SetInsets(float left, float top, float right, float bottom)
	{
		BRect newInsets(left, top, right, bottom);
		if (newInsets != fInsets) {
			fInsets = newInsets;
			InvalidateLayout();
		}
	}

	virtual BSize MinSize()
	{
		_ValidateMinMax();
		return _AddInsetsAndSpacing(BSize(fMinWidth - 1, fMinHeight - 1));
	}

	virtual BSize MaxSize()
	{
		_ValidateMinMax();
		return _AddInsetsAndSpacing(BSize(fMaxWidth - 1, fMaxHeight - 1));
	}

	virtual BSize PreferredSize()
	{
		_ValidateMinMax();
		return _AddInsetsAndSpacing(BSize(fPreferredWidth - 1,
			fPreferredHeight - 1));
	}

	virtual BAlignment Alignment()
	{
		return BAlignment(B_ALIGN_USE_FULL_WIDTH, B_ALIGN_USE_FULL_HEIGHT);
	}


	virtual void Layout()
	{
		_ValidateMinMax();
			// actually a little late already

		BSize size = _SubtractInsetsAndSpacing(Size());
		_LayoutLine(size.IntegerWidth() + 1, fColumnInfos, fColumnCount);
		_LayoutLine(size.IntegerHeight() + 1, fRowInfos, fRowCount);

		// layout children
		BPoint location = fInsets.LeftTop();
		for (int32 column = 0; column < fColumnCount; column++) {
			LayoutInfo& columnInfo = fColumnInfos[column];
			location.y = fInsets.top;
			for (int32 row = 0; row < fRowCount; row++) {
				View* child = _ChildAt(column, row);
				if (!child)
					continue;

				// get the grid cell frame
				BRect cellFrame(location,
					BSize(columnInfo.size - 1, fRowInfos[row].size - 1));

				// align the child frame in the grid cell
				BRect childFrame = BLayoutUtils::AlignInFrame(cellFrame,
					child->MaxSize(), child->Alignment());

				// layout child
				child->SetFrame(childFrame);

				location.y += fRowInfos[row].size + fRowSpacing;
			}

			location.x += columnInfo.size + fColumnSpacing;
		}
	}

private:
	struct LayoutInfo {
		int32	min;
		int32	max;
		int32	preferred;
		int32	size;

		LayoutInfo()
			: min(0),
			  max(B_SIZE_UNLIMITED),
			  preferred(0)
		{
		}

		void AddConstraints(float addMin, float addMax, float addPreferred)
		{
			if (addMin >= min)
				min = (int32)addMin + 1;
			if (addMax <= max)
				max = (int32)addMax + 1;
			if (addPreferred >= preferred)
				preferred = (int32)addPreferred + 1;
		}

		void Normalize()
		{
			if (max > min)
				max = min;
			if (preferred < min)
				preferred = min;
			if (preferred > max)
				preferred = max;
		}
	};

private:
	void _ValidateMinMax()
	{
		if (fMinMaxValid)
			return;

		delete fColumnInfos;
		delete fRowInfos;

		fColumnCount = _ColumnCount();
		fRowCount = _RowCount();

		fColumnInfos = new LayoutInfo[fColumnCount];
		fRowInfos = new LayoutInfo[fRowCount];

		// collect the children's min/max constraints
		for (int32 column = 0; column < fColumnCount; column++) {
			for (int32 row = 0; row < fRowCount; row++) {
				View* child = _ChildAt(column, row);
				if (!child)
					continue;

				BSize min = child->MinSize();
				BSize max = child->MaxSize();
				BSize preferred = child->PreferredSize();

				// apply constraints to column/row info
				fColumnInfos[column].AddConstraints(min.width, max.width,
					preferred.width);
				fRowInfos[row].AddConstraints(min.height, max.height,
					preferred.height);
			}
		}

		// normalize the column/row constraints and compute sum min/max
		fMinWidth = 0;
		fMinHeight = 0;
		fMaxWidth = 0;
		fMaxHeight = 0;
		fPreferredWidth = 0;
		fPreferredHeight = 0;

		for (int32 column = 0; column < fColumnCount; column++) {
			fColumnInfos[column].Normalize();
			fMinWidth = BLayoutUtils::AddSizesInt32(fMinWidth,
				fColumnInfos[column].min);
			fMaxWidth = BLayoutUtils::AddSizesInt32(fMaxWidth,
				fColumnInfos[column].max);
			fPreferredWidth = BLayoutUtils::AddSizesInt32(fPreferredWidth,
				fColumnInfos[column].preferred);
		}

		for (int32 row = 0; row < fRowCount; row++) {
			fRowInfos[row].Normalize();
			fMinHeight = BLayoutUtils::AddSizesInt32(fMinHeight,
				fRowInfos[row].min);
			fMaxHeight = BLayoutUtils::AddSizesInt32(fMaxHeight,
				fRowInfos[row].max);
			fPreferredHeight = BLayoutUtils::AddSizesInt32(fPreferredHeight,
				fRowInfos[row].preferred);
		}

		fMinMaxValid = true;
	}

	void _LayoutLine(int32 size, LayoutInfo* infos, int32 infoCount)
	{
		BList infosToLayout;
		for (int32 i = 0; i < infoCount; i++) {
			infos[i].size = 0;
			infosToLayout.AddItem(infos + i);
		}

		// Distribute the available space over the infos. Each iteration we
		// try to distribute the remaining space evenly. We respect min and
		// max constraints, though, add up the space we failed to assign
		// due to the constraints, and use the sum as remaining space for the
		// next iteration (can be negative). Then we ignore infos that can't
		// shrink or grow anymore (depending on what would be needed).
		while (!infosToLayout.IsEmpty()) {
			BList canShrinkInfos;
			BList canGrowInfos;

			int32 sizeDiff = 0;
			int32 infosToLayoutCount = infosToLayout.CountItems();
			for (int32 i = 0; i < infosToLayoutCount; i++) {
				LayoutInfo* info = (LayoutInfo*)infosToLayout.ItemAt(i);
				info->size += (i + 1) * size / infosToLayoutCount
					- i * size / infosToLayoutCount;
				if (info->size < info->min) {
					sizeDiff -= info->min - info->size;
					info->size = info->min;
				} else if (info->size > info->max) {
					sizeDiff += info->size - info->max;
					info->size = info->max;
				}

				if (info->size > info->min)
					canShrinkInfos.AddItem(info);
				if (info->size < info->max)
					canGrowInfos.AddItem(info);
			}

			size = sizeDiff;
			if (size == 0)
				break;

			if (size > 0)
				infosToLayout = canGrowInfos;
			else
				infosToLayout = canShrinkInfos;
		}

		// If unassigned space is remaining, that means, that the group has
		// been resized beyond its max size. We distribute the excess space
		// evenly.
		if (size > 0) {
			for (int32 i = 0; i < infoCount; i++) {
				infos[i].size += (i + 1) * size / infoCount
					- i * size / infoCount;
			}
		}
	}

	virtual BSize _AddInsetsAndSpacing(BSize size)
	{
		size.width = BLayoutUtils::AddDistances(size.width,
			fInsets.left + fInsets.right - 1
			+ (fColumnCount - 1) * fColumnSpacing);
		size.height = BLayoutUtils::AddDistances(size.height,
			fInsets.top + fInsets.bottom - 1
			+ (fRowCount - 1) * fRowSpacing);
		return size;
	}

	virtual BSize _SubtractInsetsAndSpacing(BSize size)
	{
		size.width = BLayoutUtils::SubtractDistances(size.width,
			fInsets.left + fInsets.right - 1
			+ (fColumnCount - 1) * fColumnSpacing);
		size.height = BLayoutUtils::SubtractDistances(size.height,
			fInsets.top + fInsets.bottom - 1
			+ (fRowCount - 1) * fRowSpacing);
		return size;
	}

	int32 _RowCount() const
	{
		int32 childCount = CountChildren();
		int32 count;
		if (fOrientation == B_HORIZONTAL)
			count = min_c(fLineCount, childCount);
		else
			count = (childCount + fLineCount - 1) / fLineCount;

		return max_c(count, 1);
	}

	int32 _ColumnCount() const
	{
		int32 childCount = CountChildren();
		int32 count;
		if (fOrientation == B_HORIZONTAL)
			count = (childCount + fLineCount - 1) / fLineCount;
		else
			count = min_c(fLineCount, childCount);

		return max_c(count, 1);
	}

	View* _ChildAt(int32 column, int32 row) const
	{
		if (fOrientation == B_HORIZONTAL)
			return ChildAt(column * fLineCount + row);
		else
			return ChildAt(row * fLineCount + column);
	}

private:
	enum orientation	fOrientation;
	int32				fLineCount;
	float				fColumnSpacing;
	float				fRowSpacing;
	BRect				fInsets;
	bool				fMinMaxValid;
	int32				fMinWidth;
	int32				fMinHeight;
	int32				fMaxWidth;
	int32				fMaxHeight;
	int32				fPreferredWidth;
	int32				fPreferredHeight;
	LayoutInfo*			fColumnInfos;
	LayoutInfo*			fRowInfos;
	int32				fColumnCount;
	int32				fRowCount;
};


// Glue
class Glue : public View {
public:
	Glue()
		: View()
	{
		SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	}
};


// Strut
class Strut : public View {
public:
	Strut(float pixelWidth, float pixelHeight)
		: View(),
		  fSize(pixelWidth >= 0 ? pixelWidth - 1 : B_SIZE_UNSET,
		  	pixelHeight >= 0 ? pixelHeight - 1 : B_SIZE_UNSET)
	{
		SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	}

	virtual BSize MinSize()
	{
		return BLayoutUtils::ComposeSize(fSize, BSize(-1, -1));
	}

	virtual BSize MaxSize()
	{
		return BLayoutUtils::ComposeSize(fSize,
			BSize(B_SIZE_UNLIMITED, B_SIZE_UNLIMITED));
	}

private:
	BSize	fSize;
};


// HStrut
class HStrut : public Strut {
public:
	HStrut(float width)
		: Strut(width, -1)
	{
	}
};


// VStrut
class VStrut : public Strut {
public:
	VStrut(float height)
		: Strut(-1, height)
	{
	}
};


// TestWindow
class TestWindow : public BWindow {
public:
	TestWindow()
		: BWindow(BRect(100, 100, 700, 500), "Widget Layout",
			B_TITLED_WINDOW,
			B_ASYNCHRONOUS_CONTROLS | B_QUIT_ON_WINDOW_CLOSE | B_NOT_RESIZABLE
			| B_NOT_ZOOMABLE)
	{
		fViewContainer = new ViewContainer(Bounds());
		fViewContainer->View::SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
		AddChild(fViewContainer);

		BRect rect(10, 10, 400, 300);
		View* view = new View(rect);
		fViewContainer->View::AddChild(view);
		view->SetViewColor((rgb_color){200, 200, 240, 255});

		// wrapper view
		fWrapperView = new WrapperView(
			new BButton(BRect(0, 0, 9, 9), "test button", "Ooh, press me!",
				(BMessage*)NULL, B_FOLLOW_NONE));
		fWrapperView->SetLocation(BPoint(10, 10));
		view->AddChild(fWrapperView);
		fWrapperView->SetSize(fWrapperView->PreferredSize());

		// slider view
		fSliderView = new TwoDimensionalSliderView(
			new BMessage(MSG_2D_SLIDER_VALUE_CHANGED), this);
		fSliderView->SetLocationRange(BPoint(30, 40), BPoint(150, 130));
		view->AddChild(fSliderView);

		_UpdateSliderConstraints();

		// size views
		GroupView* sizeViewsGroup = new GroupView(B_VERTICAL, 6);
		sizeViewsGroup->SetSpacing(0, 8);
		fViewContainer->View::AddChild(sizeViewsGroup);

		// min
		_CreateSizeViews(sizeViewsGroup, "min:  ", fMinWidthView,
			fMinHeightView);

		// max (with checkbox)
		GroupView* unlimitedMaxGroup = new GroupView(B_HORIZONTAL);
		fUnlimitedMaxSizeCheckBox = new CheckBox(
			new BMessage(MSG_UNLIMITED_MAX_SIZE_CHANGED), this);
		unlimitedMaxGroup->AddChild(fUnlimitedMaxSizeCheckBox);
		unlimitedMaxGroup->AddChild(new StringView("  override"));

		_CreateSizeViews(sizeViewsGroup, "max:  ", fMaxWidthView,
			fMaxHeightView, unlimitedMaxGroup);

		// preferred
		_CreateSizeViews(sizeViewsGroup, "preferred:  ", fPreferredWidthView,
			fPreferredHeightView);

		// current
		_CreateSizeViews(sizeViewsGroup, "current:  ", fCurrentWidthView,
			fCurrentHeightView);

		sizeViewsGroup->SetFrame(BRect(BPoint(rect.left, rect.bottom + 10),
			sizeViewsGroup->PreferredSize()));

		_UpdateSizeViews();
	}

	virtual void DispatchMessage(BMessage* message, BHandler* handler)
	{
		switch (message->what) {
			case B_LAYOUT_WINDOW:
				if (!fWrapperView->GetView()->IsLayoutValid()) {
					_UpdateSliderConstraints();
					_UpdateSizeViews();
				}
				break;
		}

		BWindow::DispatchMessage(message, handler);
	}

	virtual void MessageReceived(BMessage* message)
	{
		switch (message->what) {
			case MSG_2D_SLIDER_VALUE_CHANGED:
			{
				BPoint sliderLocation(fSliderView->Location());
				BPoint wrapperLocation(fWrapperView->Location());
				BSize size(sliderLocation.x - wrapperLocation.x - 1,
					sliderLocation.y - wrapperLocation.y - 1);
				fWrapperView->SetSize(size);
				_UpdateSizeViews();
				break;
			}

			case MSG_UNLIMITED_MAX_SIZE_CHANGED:
				if (fUnlimitedMaxSizeCheckBox->IsSelected())  {
					fWrapperView->GetView()->SetExplicitMaxSize(
						BSize(B_SIZE_UNLIMITED, B_SIZE_UNLIMITED));
				} else {
					fWrapperView->GetView()->SetExplicitMaxSize(
						BSize(B_SIZE_UNSET, B_SIZE_UNSET));
				}
				break;

			default:
				BWindow::MessageReceived(message);
				break;
		}
	}

private:
	void _UpdateSliderConstraints()
	{
		BPoint wrapperLocation(fWrapperView->Location());
		BSize minWrapperSize(fWrapperView->MinSize());
		BSize maxWrapperSize(fWrapperView->MaxSize());

		BPoint minSliderLocation = wrapperLocation + minWrapperSize
			+ BPoint(1, 1);
		BPoint maxSliderLocation = wrapperLocation + maxWrapperSize
			+ BPoint(1, 1);

		BSize sliderSize(fSliderView->Size());
		BSize parentSize(fSliderView->Parent()->Size());
		if (maxSliderLocation.x + sliderSize.width > parentSize.width)
			maxSliderLocation.x = parentSize.width - sliderSize.width;
		if (maxSliderLocation.y + sliderSize.height > parentSize.height)
			maxSliderLocation.y = parentSize.height - sliderSize.height;

		fSliderView->SetLocationRange(minSliderLocation, maxSliderLocation);
	}

	void _CreateSizeViews(View* group, const char* labelText,
		StringView*& widthView, StringView*& heightView,
		View* additionalView = NULL)
	{
		// label
		StringView* label = new StringView(labelText);
		label->SetAlignment(B_ALIGN_RIGHT);
		group->AddChild(label);

		// width view
		widthView = new StringView("9999999999.9");
		widthView->SetAlignment(B_ALIGN_RIGHT);
		group->AddChild(widthView);
		widthView->SetExplicitMinSize(widthView->PreferredSize());

		// "," label
		StringView* labelView = new StringView(",  ");
		group->AddChild(labelView);

		// height view
		heightView = new StringView("9999999999.9");
		heightView->SetAlignment(B_ALIGN_RIGHT);
		group->AddChild(heightView);
		heightView->SetExplicitMinSize(heightView->PreferredSize());

		// spacing
		group->AddChild(new HStrut(20));

		// glue or unlimited max size check box
		if (additionalView)
			group->AddChild(additionalView);
		else
			group->AddChild(new Glue());
	}

	void _UpdateSizeView(StringView* view, float size)
	{
		char buffer[32];
		if (size < B_SIZE_UNLIMITED) {
			sprintf(buffer, "%.1f", size);
			view->SetString(buffer);
		} else
			view->SetString("unlimited");
	}

	void _UpdateSizeViews(StringView* widthView, StringView* heightView,
		BSize size)
	{
		_UpdateSizeView(widthView, size.width);
		_UpdateSizeView(heightView, size.height);
	}

	void _UpdateSizeViews()
	{
		_UpdateSizeViews(fMinWidthView, fMinHeightView,
			fWrapperView->GetView()->MinSize());
		_UpdateSizeViews(fMaxWidthView, fMaxHeightView,
			fWrapperView->GetView()->MaxSize());
		_UpdateSizeViews(fPreferredWidthView, fPreferredHeightView,
			fWrapperView->GetView()->PreferredSize());
		_UpdateSizeViews(fCurrentWidthView, fCurrentHeightView,
			fWrapperView->GetView()->Frame().Size());
	}

private:
	ViewContainer*				fViewContainer;
	WrapperView*				fWrapperView;
	TwoDimensionalSliderView*	fSliderView;
	StringView*					fMinWidthView;
	StringView*					fMinHeightView;
	StringView*					fMaxWidthView;
	StringView*					fMaxHeightView;
	StringView*					fPreferredWidthView;
	StringView*					fPreferredHeightView;
	StringView*					fCurrentWidthView;
	StringView*					fCurrentHeightView;
	CheckBox*					fUnlimitedMaxSizeCheckBox;
};


int
main()
{
	BApplication app("application/x-vnd.haiku.widget-layout-test");

	BWindow* window = new TestWindow;
	window->Show();

	app.Run();

	return 0;
}
