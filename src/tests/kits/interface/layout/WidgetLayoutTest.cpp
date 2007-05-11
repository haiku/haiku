/*
 * Copyright 2007, Ingo Weinhold <bonefish@cs.tu-berlin.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include <stdio.h>

#include <Application.h>
#include <Button.h>
#include <Invoker.h>
#include <LayoutUtils.h>
#include <List.h>
#include <Region.h>
#include <View.h>
#include <Window.h>


// internal messages
enum {
	MSG_2D_SLIDER_VALUE_CHANGED	= '2dsv',
};


// missing operators in BPoint

BPoint
operator-(const BPoint& p)
{
	return BPoint(-p.x, -p.y);
}


BPoint
operator+(const BPoint& p, const BSize& size)
{
	return BPoint(p.x + size.width, p.y + size.height);
}


// View
class View {
public:
	View(BRect frame)
		: fFrame(frame),
		  fContainer(NULL),
		  fParent(NULL),
		  fChildren(),
		  fViewColor(B_TRANSPARENT_32_BIT)
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
		return BSize(Frame());
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
//printf("%p->View::_Draw(): ", this);
//updateRect.PrintToStream();

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

	virtual void Draw(BRect updateRect)
	{
		View::_Draw(this, updateRect);
	}

	virtual void MouseDown(BPoint where)
	{
//printf("ViewContainer::MouseDown((%f, %f))\n", where.x, where.y);
		// get mouse buttons and modifiers
		uint32 buttons;
		int32 modifiers;
		_GetButtonsAndModifiers(buttons, modifiers);

		// get mouse focus
		if (!fMouseFocus && (buttons & B_PRIMARY_MOUSE_BUTTON))
			fMouseFocus = AncestorAt(where);
//printf("  mouse focus: %p (this: %p)\n", fMouseFocus, this);

		// call hook
		if (fMouseFocus) {
			fMouseFocus->MouseDown(fMouseFocus->ConvertFromContainer(where),
				buttons, modifiers);
		}
	}

	virtual void MouseUp(BPoint where)
	{
//printf("ViewContainer::MouseUp((%f, %f))\n", where.x, where.y);
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
//printf("ViewContainer::MouseMoved((%f, %f))\n", where.x, where.y);
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
	WrapperView(BRect frame, BView* view)
		: View(frame),
		  fView(view),
		  fInsets(1, 1, 1, 1)
	{
		SetViewColor((rgb_color){255, 0, 0, 255});
	}

	BView* GetView() const
	{
		return fView;
	}

	BSize MinSize() const
	{
		return _FromViewSize(fView->MinSize());
	}

	BSize MaxSize() const
	{
		return _FromViewSize(fView->MaxSize());
	}

	BSize PreferredSize() const
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


// TestWindow
class TestWindow : public BWindow {
public:
	TestWindow()
		: BWindow(BRect(100, 100, 700, 500), "Widget Layout",
			B_TITLED_WINDOW,
			B_ASYNCHRONOUS_CONTROLS | B_QUIT_ON_WINDOW_CLOSE | B_NOT_RESIZABLE
			| B_NOT_ZOOMABLE)
	{
		ViewContainer* viewContainer = new ViewContainer(Bounds());
		viewContainer->View::SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
		AddChild(viewContainer);

		View* view = new View(BRect(10, 10, 400, 300));
		viewContainer->View::AddChild(view);
		view->SetViewColor((rgb_color){200, 200, 240, 255});

		// wrapper view
		fWrapperView = new WrapperView(BRect(10, 10, 100, 100),
			new BButton(BRect(0, 0, 9, 9), "test button", "Ooh, press me!",
				(BMessage*)NULL, B_FOLLOW_NONE,
				B_WILL_DRAW | B_NAVIGABLE | B_FULL_UPDATE_ON_RESIZE));
fWrapperView->GetView()->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, B_SIZE_UNLIMITED));
		view->AddChild(fWrapperView);
		fWrapperView->SetSize(fWrapperView->PreferredSize());

		// slider view
		fSliderView = new TwoDimensionalSliderView(
			new BMessage(MSG_2D_SLIDER_VALUE_CHANGED), this);
		fSliderView->SetLocationRange(BPoint(30, 40), BPoint(150, 130));
		view->AddChild(fSliderView);

		_UpdateSliderConstraints();
	}

	virtual void MessageReceived(BMessage* message)
	{
		switch (message->what) {
			case MSG_2D_SLIDER_VALUE_CHANGED:
			{
//				message->PrintToStream();
				BPoint sliderLocation(fSliderView->Location());
				BPoint wrapperLocation(fWrapperView->Location());
				BSize size(sliderLocation.x - wrapperLocation.x - 1,
					sliderLocation.y - wrapperLocation.y - 1);
				fWrapperView->SetSize(size);
				break;
			}

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

private:
	WrapperView*				fWrapperView;
	TwoDimensionalSliderView*	fSliderView;
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
