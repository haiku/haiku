/*
 * Copyright 2003-2009, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include "DefaultMediaTheme.h"

#include <Box.h>
#include <Button.h>
#include <ChannelSlider.h>
#include <CheckBox.h>
#include <MediaRoster.h>
#include <MenuField.h>
#include <MessageFilter.h>
#include <OptionPopUp.h>
#include <ParameterWeb.h>
#include <ScrollBar.h>
#include <Slider.h>
#include <StringView.h>
#include <TabView.h>
#include <TextControl.h>
#include <Window.h>

#include "debug.h"


using namespace BPrivate;


namespace BPrivate {

class DynamicScrollView : public BView {
	public:
		DynamicScrollView(const char *name, BView *target);
		virtual ~DynamicScrollView();

		virtual void AttachedToWindow(void);
		virtual void FrameResized(float width, float height);
		virtual void FrameMoved(BPoint newPosition);
		virtual void GetPreferredSize(float *_width, float *_height);

		void SetContentBounds(BRect bounds);
		BRect ContentBounds() const { return fContentBounds; }

	private:
		void UpdateBars();

		BScrollBar	*fHorizontalScrollBar, *fVerticalScrollBar;
		BRect		fContentBounds;
		BView		*fTarget;
		bool		fIsDocumentScroller;
};

class GroupView : public BView {
	public:
		GroupView(BRect frame, const char *name);
		virtual ~GroupView();

		virtual void AttachedToWindow();
		virtual void AllAttached();
		virtual void GetPreferredSize(float *_width, float *_height);

		virtual	BSize MinSize();
		virtual	BSize MaxSize();
		virtual	BSize PreferredSize();

		void SetContentBounds(BRect bounds);
		BRect ContentBounds() const { return fContentBounds; }

	private:
		BRect		fContentBounds;
};

class TabView : public BTabView {
	public:
		TabView(BRect frame, const char *name, button_width width = B_WIDTH_FROM_LABEL,
			uint32 resizingMode = B_FOLLOW_ALL, uint32 flags = B_FULL_UPDATE_ON_RESIZE
				| B_WILL_DRAW | B_NAVIGABLE_JUMP | B_FRAME_EVENTS | B_NAVIGABLE);

		virtual void FrameResized(float width, float height);
		virtual void Select(int32 tab);
};

class SeparatorView : public BView {
	public:
		SeparatorView(BRect frame);
		virtual ~SeparatorView();

		virtual void Draw(BRect updateRect);

	private:
		bool	fVertical;
};

class TitleView : public BView {
	public:
		TitleView(BRect frame, const char *title);
		virtual ~TitleView();

		virtual void Draw(BRect updateRect);
		virtual void GetPreferredSize(float *width, float *height);

	private:
		const char *fTitle;
};

class CheckBox : public BCheckBox {
	public:
		CheckBox(BRect area, const char* name, const char* label,
			BDiscreteParameter &parameter);
		virtual ~CheckBox();

		virtual void AttachedToWindow();
		virtual void DetachedFromWindow();
	private:
		BDiscreteParameter &fParameter;
};

class OptionPopUp : public BOptionPopUp {
	public:
		OptionPopUp(BRect area, const char* name, const char* label,
			BDiscreteParameter &parameter);
		virtual ~OptionPopUp();

		virtual void AttachedToWindow();
		virtual void DetachedFromWindow();
	private:
		BDiscreteParameter &fParameter;
};

class Slider : public BSlider {
	public:
		Slider(BRect area, const char* name, const char*label, int32 minValue,
			int32 maxValue, BContinuousParameter &parameter);
		virtual ~Slider();

		virtual void AttachedToWindow();
		virtual void DetachedFromWindow();
	private:
		BContinuousParameter &fParameter;
};

class ChannelSlider : public BChannelSlider {
	public:
		ChannelSlider(BRect area, const char* name, const char* label,
			orientation orientation, int32 channels,
			BContinuousParameter &parameter);
		virtual ~ChannelSlider();

		virtual void AttachedToWindow();
		virtual void DetachedFromWindow();
	private:
		BContinuousParameter &fParameter;
};

class MessageFilter : public BMessageFilter {
	public:
		static MessageFilter *FilterFor(BView *view, BParameter &parameter);

	protected:
		MessageFilter();
};

class ContinuousMessageFilter : public MessageFilter {
	public:
		ContinuousMessageFilter(BControl *control,
			BContinuousParameter &parameter);
		virtual ~ContinuousMessageFilter();

		virtual filter_result Filter(BMessage *message, BHandler **target);

	private:
		void _UpdateControl();

		BControl				*fControl;
		BContinuousParameter	&fParameter;
};

class DiscreteMessageFilter : public MessageFilter {
	public:
		DiscreteMessageFilter(BControl *control, BDiscreteParameter &parameter);
		virtual ~DiscreteMessageFilter();

		virtual filter_result Filter(BMessage *message, BHandler **target);

	private:
		BDiscreteParameter	&fParameter;
};

}	// namespace BPrivate


const uint32 kMsgParameterChanged = '_mPC';


static bool
parameter_should_be_hidden(BParameter &parameter)
{
	// ToDo: note, this is probably completely stupid, but it's the only
	// way I could safely remove the null parameters that are not shown
	// by the R5 media theme
	if (parameter.Type() != BParameter::B_NULL_PARAMETER
		|| strcmp(parameter.Kind(), B_WEB_PHYSICAL_INPUT))
		return false;

	for (int32 i = 0; i < parameter.CountOutputs(); i++) {
		if (!strcmp(parameter.OutputAt(0)->Kind(), B_INPUT_MUX))
			return true;
	}

	return false;
}


static void
start_watching_for_parameter_changes(BControl* control, BParameter &parameter)
{
	BMediaRoster* roster = BMediaRoster::CurrentRoster();
	if (roster != NULL) {
		roster->StartWatching(control, parameter.Web()->Node(),
			B_MEDIA_NEW_PARAMETER_VALUE);
	}
}


static void
stop_watching_for_parameter_changes(BControl* control, BParameter &parameter)
{
	BMediaRoster* roster = BMediaRoster::CurrentRoster();
	if (roster != NULL) {
		roster->StopWatching(control, parameter.Web()->Node(),
			B_MEDIA_NEW_PARAMETER_VALUE);
	}
}

//	#pragma mark -


DynamicScrollView::DynamicScrollView(const char *name, BView *target)
	: BView(target->Frame(), name, B_FOLLOW_ALL, B_WILL_DRAW | B_FRAME_EVENTS),
	fHorizontalScrollBar(NULL),
	fVerticalScrollBar(NULL),
	fTarget(target),
	fIsDocumentScroller(false)
{
	fContentBounds.Set(-1, -1, -1, -1);
	AdoptViewColors(fTarget);
	target->MoveTo(B_ORIGIN);
	AddChild(target);
}


DynamicScrollView::~DynamicScrollView()
{
}


void
DynamicScrollView::AttachedToWindow(void)
{
	BRect frame = ConvertToScreen(Bounds());
	BRect windowFrame = Window()->Frame();

	fIsDocumentScroller = Parent() == NULL
		&& Window() != NULL
		&& Window()->Look() == B_DOCUMENT_WINDOW_LOOK
		&& frame.right == windowFrame.right
		&& frame.bottom == windowFrame.bottom;

	UpdateBars();
}


void
DynamicScrollView::FrameResized(float width, float height)
{
	UpdateBars();
}


void
DynamicScrollView::FrameMoved(BPoint newPosition)
{
	UpdateBars();
}


void
DynamicScrollView::GetPreferredSize(float *_width, float *_height)
{
	float width = 50;
	if (fVerticalScrollBar)
		width += B_V_SCROLL_BAR_WIDTH;
	float height = 50;
	if (fHorizontalScrollBar)
		height += B_H_SCROLL_BAR_HEIGHT;
	if (_width)
		*_width = width;
	if (_height)
		*_height = height;
}


void
DynamicScrollView::SetContentBounds(BRect bounds)
{
	fContentBounds = bounds;
	if (Window())
		UpdateBars();
}


void
DynamicScrollView::UpdateBars()
{
	// we need the size that the view wants to have, and the one
	// it could have (without the space for the scroll bars)

	float width, height;
	if (fContentBounds == BRect(-1, -1, -1, -1))
		fTarget->GetPreferredSize(&width, &height);
	else {
		width = fContentBounds.Width();
		height = fContentBounds.Height();
	}

	BRect bounds = Bounds();

	// do we have to remove a scroll bar?

	bool horizontal = width > bounds.Width();
	bool vertical = height > bounds.Height();

	if (!horizontal && fHorizontalScrollBar != NULL) {
		RemoveChild(fHorizontalScrollBar);
		delete fHorizontalScrollBar;
		fHorizontalScrollBar = NULL;
	}

	if (!vertical && fVerticalScrollBar != NULL) {
		RemoveChild(fVerticalScrollBar);
		delete fVerticalScrollBar;
		fVerticalScrollBar = NULL;
	}

	// or do we have to add a scroll bar?

	if (horizontal && fHorizontalScrollBar == NULL) {
		BRect rect = Bounds();
		rect.top = rect.bottom + 1 - B_H_SCROLL_BAR_HEIGHT;
		if (vertical || fIsDocumentScroller)
			rect.right -= B_V_SCROLL_BAR_WIDTH;

		fHorizontalScrollBar = new BScrollBar(rect, "horizontal", fTarget, 0,
			width, B_HORIZONTAL);
		AddChild(fHorizontalScrollBar);
	}

	if (vertical && fVerticalScrollBar == NULL) {
		BRect rect = Bounds();
		rect.left = rect.right + 1 - B_V_SCROLL_BAR_WIDTH;
		if (horizontal || fIsDocumentScroller)
			rect.bottom -= B_H_SCROLL_BAR_HEIGHT;

		fVerticalScrollBar = new BScrollBar(rect, "vertical", fTarget, 0,
			height, B_VERTICAL);
		AddChild(fVerticalScrollBar);
	}

	// update the scroll bar range & proportions and layout views

	bounds = Bounds();
	if (fHorizontalScrollBar != NULL)
		bounds.bottom -= B_H_SCROLL_BAR_HEIGHT + 1;
	if (fVerticalScrollBar != NULL)
		bounds.right -= B_V_SCROLL_BAR_WIDTH + 1;

	fTarget->MoveTo(bounds.LeftTop());
	fTarget->ResizeTo(bounds.Width(), bounds.Height());

	if (fHorizontalScrollBar != NULL) {
		float delta = width - bounds.Width();
		if (delta < 0)
			delta = 0;

		fHorizontalScrollBar->SetRange(0, delta);
		fHorizontalScrollBar->SetSteps(1, bounds.Width());
		fHorizontalScrollBar->SetProportion(bounds.Width() / width);

		float barWidth = Bounds().Width();
		if (vertical) {
			// scrollbars overlap one pixel of the frame
			barWidth += 1;
		}
		if (vertical || fIsDocumentScroller)
			barWidth -= B_V_SCROLL_BAR_WIDTH + 1;

		fHorizontalScrollBar->MoveTo(bounds.left, bounds.bottom + 1);
		fHorizontalScrollBar->ResizeTo(barWidth, B_H_SCROLL_BAR_HEIGHT);
	}
	if (fVerticalScrollBar != NULL) {
		float delta = height - bounds.Height();
		if (delta < 0)
			delta = 0;

		fVerticalScrollBar->SetRange(0, delta);
		fVerticalScrollBar->SetSteps(1, bounds.Height());
		fVerticalScrollBar->SetProportion(bounds.Height() / height);

		float barHeight = Bounds().Height();
		if (horizontal) {
			// scrollbars overlap one pixel of the frame
			barHeight += 1;
		}
		if (horizontal || fIsDocumentScroller)
			barHeight -= B_H_SCROLL_BAR_HEIGHT + 1;

		fVerticalScrollBar->MoveTo(bounds.right + 1, bounds.top);
		fVerticalScrollBar->ResizeTo(B_V_SCROLL_BAR_WIDTH, barHeight);
	}
}


//	#pragma mark -


GroupView::GroupView(BRect frame, const char *name)
	: BView(frame, name, B_FOLLOW_NONE, B_WILL_DRAW)
{
	SetViewUIColor(B_PANEL_BACKGROUND_COLOR);
}


GroupView::~GroupView()
{
}


void
GroupView::AttachedToWindow()
{
	for (int32 i = CountChildren(); i-- > 0;) {
		BControl *control = dynamic_cast<BControl *>(ChildAt(i));
		if (control == NULL)
			continue;

		control->SetTarget(control);
	}
}


void
GroupView::AllAttached()
{
}


void
GroupView::GetPreferredSize(float *_width, float *_height)
{
	if (_width)
		*_width = fContentBounds.Width();

	if (_height)
		*_height = fContentBounds.Height();
}


BSize
GroupView::MinSize()
{
	return BSize(100, 100);
}


BSize
GroupView::PreferredSize()
{
	return MinSize();
}


BSize
GroupView::MaxSize()
{
	BSize max;
	GetPreferredSize(&max.width, &max.height);
	return max;
}


void
GroupView::SetContentBounds(BRect bounds)
{
	fContentBounds = bounds;
}


//	#pragma mark -


/** BTabView is really stupid - it doesn't even resize its content
 *	view when it is resized itself.
 *	This derived class fixes this issue, and also resizes all tab
 *	content views to the size of the container view when they are
 *	selected (does not take their resize flags into account, though).
 */

TabView::TabView(BRect frame, const char *name, button_width width,
	uint32 resizingMode, uint32 flags)
	: BTabView(frame, name, width, resizingMode, flags)
{
}


void
TabView::FrameResized(float width, float height)
{
	BRect rect = Bounds();
	rect.top += TabHeight();
	rect.InsetBy(3.0f, 3.0f);
		//ContainerView is inseted by 3.0 in BTabView::_InitObject()

	ContainerView()->ResizeTo(rect.Width(), rect.Height());
}


void
TabView::Select(int32 tab)
{
	BTabView::Select(tab);

	BView *view = ViewForTab(Selection());
	if (view != NULL) {
		BRect rect = ContainerView()->Bounds();
		view->ResizeTo(rect.Width(), rect.Height());
	}
}


//	#pragma mark -


SeparatorView::SeparatorView(BRect frame)
	: BView(frame, "-", B_FOLLOW_NONE, B_WILL_DRAW)
{
	fVertical = frame.Width() < frame.Height();
	SetViewColor(B_TRANSPARENT_COLOR);
}


SeparatorView::~SeparatorView()
{
}


void
SeparatorView::Draw(BRect updateRect)
{
	rgb_color color = ui_color(B_PANEL_BACKGROUND_COLOR);
	BRect rect = updateRect & Bounds();

	SetHighColor(tint_color(color, B_DARKEN_1_TINT));
	if (fVertical)
		StrokeLine(BPoint(0, rect.top), BPoint(0, rect.bottom));
	else
		StrokeLine(BPoint(rect.left, 0), BPoint(rect.right, 0));

	SetHighColor(tint_color(color, B_LIGHTEN_1_TINT));
	if (fVertical)
		StrokeLine(BPoint(1, rect.top), BPoint(1, rect.bottom));
	else
		StrokeLine(BPoint(rect.left, 1), BPoint(rect.right, 1));
}


//	#pragma mark -


TitleView::TitleView(BRect frame, const char *title)
	: BView(frame, title, B_FOLLOW_LEFT_RIGHT, B_WILL_DRAW)
{
	fTitle = strdup(title);
	AdoptSystemColors();
}


TitleView::~TitleView()
{
	free((char *)fTitle);
}


void
TitleView::Draw(BRect updateRect)
{
	BRect rect(Bounds());

	SetDrawingMode(B_OP_COPY);
	SetHighColor(tint_color(ViewColor(), B_LIGHTEN_2_TINT));
	DrawString(fTitle, BPoint(rect.left + 1, rect.bottom - 8));

	SetDrawingMode(B_OP_OVER);
	SetHighColor(80, 20, 20);
	DrawString(fTitle, BPoint(rect.left, rect.bottom - 9));
}


void
TitleView::GetPreferredSize(float *_width, float *_height)
{
	if (_width)
		*_width = StringWidth(fTitle) + 2;

	if (_height) {
		font_height fontHeight;
		GetFontHeight(&fontHeight);

		*_height = fontHeight.ascent + fontHeight.descent + fontHeight.leading
			+ 8;
	}
}


//	#pragma mark -


CheckBox::CheckBox(BRect area, const char* name, const char* label,
	BDiscreteParameter &parameter)
	: BCheckBox(area, name, label, NULL),
	fParameter(parameter)
{
}


CheckBox::~CheckBox()
{
}


void
CheckBox::AttachedToWindow()
{
	start_watching_for_parameter_changes(this, fParameter);
}


void
CheckBox::DetachedFromWindow()
{
	stop_watching_for_parameter_changes(this, fParameter);
}


OptionPopUp::OptionPopUp(BRect area, const char* name, const char* label,
	BDiscreteParameter &parameter)
	: BOptionPopUp(area, name, label, NULL),
	fParameter(parameter)
{
}


OptionPopUp::~OptionPopUp()
{
}


void
OptionPopUp::AttachedToWindow()
{
	start_watching_for_parameter_changes(this, fParameter);
}


void
OptionPopUp::DetachedFromWindow()
{
	stop_watching_for_parameter_changes(this, fParameter);
}


Slider::Slider(BRect area, const char* name, const char* label, int32 minValue,
	int32 maxValue, BContinuousParameter &parameter)
	: BSlider(area, name, label, NULL, minValue, maxValue),
	fParameter(parameter)
{
}


Slider::~Slider()
{
}


void
Slider::AttachedToWindow()
{
	start_watching_for_parameter_changes(this, fParameter);
}


void
Slider::DetachedFromWindow()
{
	stop_watching_for_parameter_changes(this, fParameter);
}


ChannelSlider::ChannelSlider(BRect area, const char* name, const char* label,
	orientation orientation, int32 channels, BContinuousParameter &parameter)
	: BChannelSlider(area, name, label, NULL, orientation, channels),
	fParameter(parameter)
{
}


ChannelSlider::~ChannelSlider()
{
}


void
ChannelSlider::AttachedToWindow()
{
	start_watching_for_parameter_changes(this, fParameter);
}


void
ChannelSlider::DetachedFromWindow()
{
	stop_watching_for_parameter_changes(this, fParameter);
}


//	#pragma mark -


MessageFilter::MessageFilter()
	: BMessageFilter(B_ANY_DELIVERY, B_ANY_SOURCE)
{
}


MessageFilter *
MessageFilter::FilterFor(BView *view, BParameter &parameter)
{
	BControl *control = dynamic_cast<BControl *>(view);
	if (control == NULL)
		return NULL;

	switch (parameter.Type()) {
		case BParameter::B_CONTINUOUS_PARAMETER:
			return new ContinuousMessageFilter(control,
				static_cast<BContinuousParameter &>(parameter));

		case BParameter::B_DISCRETE_PARAMETER:
			return new DiscreteMessageFilter(control,
				static_cast<BDiscreteParameter &>(parameter));

		case BParameter::B_NULL_PARAMETER: /* fall through */
		default:
			return NULL;
	}
}


//	#pragma mark -


ContinuousMessageFilter::ContinuousMessageFilter(BControl *control,
		BContinuousParameter &parameter)
	: MessageFilter(),
	fControl(control),
	fParameter(parameter)
{
	// initialize view for us
	control->SetMessage(new BMessage(kMsgParameterChanged));

	if (BSlider *slider = dynamic_cast<BSlider *>(fControl))
		slider->SetModificationMessage(new BMessage(kMsgParameterChanged));
	else if (BChannelSlider *slider = dynamic_cast<BChannelSlider *>(fControl))
		slider->SetModificationMessage(new BMessage(kMsgParameterChanged));
	else
		ERROR("ContinuousMessageFilter: unknown continuous parameter view\n");

	// set initial value
	_UpdateControl();
}


ContinuousMessageFilter::~ContinuousMessageFilter()
{
}


filter_result
ContinuousMessageFilter::Filter(BMessage *message, BHandler **target)
{
	if (*target != fControl)
		return B_DISPATCH_MESSAGE;

	if (message->what == kMsgParameterChanged) {
		// update parameter from control
		// TODO: support for response!

		float value[fParameter.CountChannels()];

		if (BSlider *slider = dynamic_cast<BSlider *>(fControl)) {
			value[0] = (float)(slider->Value() / 1000.0);
		} else if (BChannelSlider *slider
				= dynamic_cast<BChannelSlider *>(fControl)) {
			for (int32 i = 0; i < fParameter.CountChannels(); i++)
				value[i] = (float)(slider->ValueFor(i) / 1000.0);
		}

		TRACE("ContinuousMessageFilter::Filter: update view %s, %" B_PRId32
			" channels\n", fControl->Name(), fParameter.CountChannels());

		if (fParameter.SetValue((void *)value, sizeof(value),
				-1) < B_OK) {
			ERROR("ContinuousMessageFilter::Filter: Could not set parameter "
				"value for %p\n", &fParameter);
			return B_DISPATCH_MESSAGE;
		}
		return B_SKIP_MESSAGE;
	}
	if (message->what == B_MEDIA_NEW_PARAMETER_VALUE) {
		// update view from parameter -- if the message concerns us
		const media_node* node;
		int32 parameterID;
		ssize_t size;
		if (message->FindInt32("parameter", &parameterID) != B_OK
			|| fParameter.ID() != parameterID
			|| message->FindData("node", B_RAW_TYPE, (const void**)&node,
					&size) != B_OK
			|| fParameter.Web()->Node() != *node)
			return B_DISPATCH_MESSAGE;

		_UpdateControl();
		return B_SKIP_MESSAGE;
	}

	return B_DISPATCH_MESSAGE;
}


void
ContinuousMessageFilter::_UpdateControl()
{
	// TODO: response support!

	float value[fParameter.CountChannels()];
	size_t size = sizeof(value);
	if (fParameter.GetValue((void *)&value, &size, NULL) < B_OK) {
		ERROR("ContinuousMessageFilter: Could not get value for continuous "
			"parameter %p (name '%s', node %d)\n", &fParameter,
			fParameter.Name(), (int)fParameter.Web()->Node().node);
		return;
	}

	if (BSlider *slider = dynamic_cast<BSlider *>(fControl)) {
		slider->SetValue((int32) (1000 * value[0]));
		slider->SetModificationMessage(new BMessage(kMsgParameterChanged));
	} else if (BChannelSlider *slider
			= dynamic_cast<BChannelSlider *>(fControl)) {
		for (int32 i = 0; i < fParameter.CountChannels(); i++) {
			slider->SetValueFor(i, (int32) (1000 * value[i]));
		}
	}
}


//	#pragma mark -


DiscreteMessageFilter::DiscreteMessageFilter(BControl *control,
		BDiscreteParameter &parameter)
	: MessageFilter(),
	fParameter(parameter)
{
	// initialize view for us
	control->SetMessage(new BMessage(kMsgParameterChanged));

	// set initial value
	size_t size = sizeof(int32);
	int32 value;
	if (parameter.GetValue((void *)&value, &size, NULL) < B_OK) {
		ERROR("DiscreteMessageFilter: Could not get value for discrete "
			"parameter %p (name '%s', node %d)\n", &parameter,
			parameter.Name(), (int)(parameter.Web()->Node().node));
		return;
	}

	if (BCheckBox *checkBox = dynamic_cast<BCheckBox *>(control)) {
		checkBox->SetValue(value);
	} else if (BOptionPopUp *popUp = dynamic_cast<BOptionPopUp *>(control)) {
		popUp->SelectOptionFor(value);
	} else
		ERROR("DiscreteMessageFilter: unknown discrete parameter view\n");
}


DiscreteMessageFilter::~DiscreteMessageFilter()
{
}


filter_result
DiscreteMessageFilter::Filter(BMessage *message, BHandler **target)
{
	BControl *control;

	if ((control = dynamic_cast<BControl *>(*target)) == NULL)
		return B_DISPATCH_MESSAGE;

	if (message->what == B_MEDIA_NEW_PARAMETER_VALUE) {
		TRACE("DiscreteMessageFilter::Filter: Got a new parameter value\n");
		const media_node* node;
		int32 parameterID;
		ssize_t size;
		if (message->FindInt32("parameter", &parameterID) != B_OK
			|| fParameter.ID() != parameterID
			|| message->FindData("node", B_RAW_TYPE, (const void**)&node,
					&size) != B_OK
			|| fParameter.Web()->Node() != *node)
			return B_DISPATCH_MESSAGE;

		int32 value = 0;
		size_t valueSize = sizeof(int32);
		if (fParameter.GetValue((void*)&value, &valueSize, NULL) < B_OK) {
			ERROR("DiscreteMessageFilter: Could not get value for continuous "
			"parameter %p (name '%s', node %d)\n", &fParameter,
			fParameter.Name(), (int)fParameter.Web()->Node().node);
			return B_SKIP_MESSAGE;
		}
		if (BCheckBox* checkBox = dynamic_cast<BCheckBox*>(control)) {
			checkBox->SetValue(value);
		} else if (BOptionPopUp* popUp = dynamic_cast<BOptionPopUp*>(control)) {
			popUp->SetValue(value);
		}

		return B_SKIP_MESSAGE;
	}

	if (message->what != kMsgParameterChanged)
		return B_DISPATCH_MESSAGE;

	// update view

	int32 value = 0;

	if (BCheckBox *checkBox = dynamic_cast<BCheckBox *>(control)) {
		value = checkBox->Value();
	} else if (BOptionPopUp *popUp = dynamic_cast<BOptionPopUp *>(control)) {
		popUp->SelectedOption(NULL, &value);
	}

	TRACE("DiscreteMessageFilter::Filter: update view %s, value = %"
		B_PRId32 "\n", control->Name(), value);

	if (fParameter.SetValue((void *)&value, sizeof(value), -1) < B_OK) {
		ERROR("DiscreteMessageFilter::Filter: Could not set parameter value for %p\n", &fParameter);
		return B_DISPATCH_MESSAGE;
	}

	return B_SKIP_MESSAGE;
}


//	#pragma mark -


DefaultMediaTheme::DefaultMediaTheme()
	: BMediaTheme("Haiku theme", "Haiku built-in theme version 0.1")
{
	CALLED();
}


BControl *
DefaultMediaTheme::MakeControlFor(BParameter *parameter)
{
	CALLED();

	BRect rect(0, 0, 150, 100);
	return MakeViewFor(parameter, &rect);
}


BView *
DefaultMediaTheme::MakeViewFor(BParameterWeb *web, const BRect *hintRect)
{
	CALLED();

	if (web == NULL)
		return NULL;

	BRect rect;
	if (hintRect)
		rect = *hintRect;

	BRect bestRect;

	// do we have more than one attached parameter group?
	// if so, use a tabbed view with a tab for each group

	TabView *tabView = NULL;

	if (web->CountGroups() > 1)
		tabView = new TabView(rect, "web");

	rect.OffsetTo(B_ORIGIN);

	for (int32 i = 0; i < web->CountGroups(); i++) {
		BParameterGroup *group = web->GroupAt(i);
		if (group == NULL)
			continue;

		BView *groupView = MakeViewFor(*group, hintRect ? &rect : NULL);
		if (groupView == NULL)
			continue;

		if (GroupView *view = dynamic_cast<GroupView *>(groupView)) {
			// the top-level group views must not be larger than their hintRect,
			// but unlike their children, they should follow all sides when
			// their parent is resized
			if (hintRect != NULL)
				view->ResizeTo(rect.Width() - 10, rect.Height() - 10);
			view->SetResizingMode(B_FOLLOW_ALL);
		}

		if (tabView == NULL) {
			// if we don't need a container to put that view into,
			// we're done here (but the groupView may span over the
			// whole hintRect)
			if (groupView->Frame().LeftTop() == BPoint(5, 5)) {
				// remove insets, as they are not needed
				groupView->MoveBy(-5, -5);
				groupView->ResizeBy(10, 10);
			}

			return new DynamicScrollView(groupView->Name(), groupView);
		}

		DynamicScrollView *scrollView = new DynamicScrollView(groupView->Name(), groupView);
		tabView->AddTab(scrollView);

		if (!hintRect) {
			bestRect = bestRect | scrollView->Bounds();
		}
	}

	if (tabView != NULL) {
		// this adjustment must be kept in sync with TabView::FrameResized
		bestRect.bottom += tabView->TabHeight();
		bestRect.InsetBy(-3.0,-3.0);

		tabView->ResizeTo(bestRect.Width(), bestRect.Height());
		tabView->FrameResized(bestRect.Width(), bestRect.Height());
			//needed since we're not attached to a window yet
	}

	return tabView;
}


BView *
DefaultMediaTheme::MakeViewFor(BParameterGroup& group, const BRect* hintRect)
{
	CALLED();

	if (group.Flags() & B_HIDDEN_PARAMETER)
		return NULL;

	BRect rect;
	if (hintRect != NULL)
		rect = *hintRect;

	GroupView *view = new GroupView(rect, group.Name());

	// Create the parameter views - but don't add them yet

	rect.OffsetTo(B_ORIGIN);
	rect.InsetBySelf(5, 5);

	BList views;
	for (int32 i = 0; i < group.CountParameters(); i++) {
		BParameter *parameter = group.ParameterAt(i);
		if (parameter == NULL)
			continue;

		BView *parameterView = MakeSelfHostingViewFor(*parameter,
			hintRect ? &rect : NULL);
		if (parameterView == NULL)
			continue;

		parameterView->AdoptViewColors(view);
			// ToDo: dunno why this is needed, but the controls
			// sometimes (!) have a white background without it

		views.AddItem(parameterView);
	}

	// Identify a title view, and add it at the top if present

	TitleView *titleView = dynamic_cast<TitleView *>((BView *)views.ItemAt(0));
	if (titleView != NULL) {
		view->AddChild(titleView);
		rect.OffsetBy(0, titleView->Bounds().Height());
	}

	// Add the sub-group views

	rect.right = rect.left + 20;
	rect.bottom = rect.top + 20;
	float lastHeight = 0;

	for (int32 i = 0; i < group.CountGroups(); i++) {
		BParameterGroup *subGroup = group.GroupAt(i);
		if (subGroup == NULL)
			continue;

		BView *groupView = MakeViewFor(*subGroup, &rect);
		if (groupView == NULL)
			continue;

		if (i > 0) {
			// add separator view
			BRect separatorRect(groupView->Frame());
			separatorRect.left -= 3;
			separatorRect.right = separatorRect.left + 1;
			if (lastHeight > separatorRect.Height())
				separatorRect.bottom = separatorRect.top + lastHeight;

			view->AddChild(new SeparatorView(separatorRect));
		}

		view->AddChild(groupView);

		rect.OffsetBy(groupView->Bounds().Width() + 5, 0);

		lastHeight = groupView->Bounds().Height();
		if (lastHeight > rect.Height())
			rect.bottom = rect.top + lastHeight - 1;
	}

	view->ResizeTo(rect.left + 10, rect.bottom + 5);
	view->SetContentBounds(view->Bounds());

	if (group.CountParameters() == 0)
		return view;

	// add the parameter views part of the group

	if (group.CountGroups() > 0) {
		rect.top = rect.bottom + 10;
		rect.bottom = rect.top + 20;
	}

	bool center = false;

	for (int32 i = 0; i < views.CountItems(); i++) {
		BView *parameterView = static_cast<BView *>(views.ItemAt(i));

		if (parameterView->Bounds().Width() + 5 > rect.Width())
			rect.right = parameterView->Bounds().Width() + rect.left + 5;

		// we don't need to add the title view again
		if (parameterView == titleView)
			continue;

		// if there is a BChannelSlider (ToDo: or any vertical slider?)
		// the views will be centered
		if (dynamic_cast<BChannelSlider *>(parameterView) != NULL)
			center = true;

		parameterView->MoveTo(parameterView->Frame().left, rect.top);
		view->AddChild(parameterView);

		rect.OffsetBy(0, parameterView->Bounds().Height() + 5);
	}

	if (views.CountItems() > (titleView != NULL ? 1 : 0))
		view->ResizeTo(rect.right + 5, rect.top + 5);

	// center the parameter views if needed, and tweak some views

	float width = view->Bounds().Width();

	for (int32 i = 0; i < views.CountItems(); i++) {
		BView *subView = static_cast<BView *>(views.ItemAt(i));
		BRect frame = subView->Frame();

		if (center)
			subView->MoveTo((width - frame.Width()) / 2, frame.top);
		else {
			// tweak the PopUp views to look better
			if (dynamic_cast<BOptionPopUp *>(subView) != NULL)
				subView->ResizeTo(width, frame.Height());
		}
	}

	view->SetContentBounds(view->Bounds());
	return view;
}


/*!	This creates a view that handles all incoming messages itself - that's
	what is meant with self-hosting.
*/
BView *
DefaultMediaTheme::MakeSelfHostingViewFor(BParameter& parameter,
	const BRect* hintRect)
{
	if (parameter.Flags() & B_HIDDEN_PARAMETER
		|| parameter_should_be_hidden(parameter))
		return NULL;

	BView *view = MakeViewFor(&parameter, hintRect);
	if (view == NULL) {
		// The MakeViewFor() method above returns a BControl - which we
		// don't need for a null parameter; that's why it returns NULL.
		// But we want to see something anyway, so we add a string view
		// here.
		if (parameter.Type() == BParameter::B_NULL_PARAMETER) {
			if (parameter.Group()->ParameterAt(0) == &parameter) {
				// this is the first parameter in this group, so
				// let's use a nice title view

				TitleView *titleView = new TitleView(BRect(0, 0, 10, 10), parameter.Name());
				titleView->ResizeToPreferred();

				return titleView;
			}
			BStringView *stringView = new BStringView(BRect(0, 0, 10, 10),
				parameter.Name(), parameter.Name());
			stringView->SetAlignment(B_ALIGN_CENTER);
			stringView->ResizeToPreferred();

			return stringView;
		}

		return NULL;
	}

	MessageFilter *filter = MessageFilter::FilterFor(view, parameter);
	if (filter != NULL)
		view->AddFilter(filter);

	return view;
}


BControl *
DefaultMediaTheme::MakeViewFor(BParameter *parameter, const BRect *hintRect)
{
	BRect rect;
	if (hintRect)
		rect = *hintRect;
	else
		rect.Set(0, 0, 50, 100);

	switch (parameter->Type()) {
		case BParameter::B_NULL_PARAMETER:
			// there is no default view for a null parameter
			return NULL;

		case BParameter::B_DISCRETE_PARAMETER:
		{
			BDiscreteParameter &discrete = static_cast<BDiscreteParameter &>(*parameter);

			if (!strcmp(discrete.Kind(), B_ENABLE)
				|| !strcmp(discrete.Kind(), B_MUTE)
				|| discrete.CountItems() == 0) {
				// create a checkbox item

				BCheckBox *checkBox = new CheckBox(rect, discrete.Name(),
					discrete.Name(), discrete);
				checkBox->ResizeToPreferred();

				return checkBox;
			} else {
				// create a pop up menu field

				// ToDo: replace BOptionPopUp (or fix it in Haiku...)
				// this is a workaround for a bug in BOptionPopUp - you need to
				// know the actual width before creating the object - very nice...

				BFont font;
				float width = 0;
				for (int32 i = 0; i < discrete.CountItems(); i++) {
					float labelWidth = font.StringWidth(discrete.ItemNameAt(i));
					if (labelWidth > width)
						width = labelWidth;
				}
				width += font.StringWidth(discrete.Name()) + 55;
				rect.right = rect.left + width;

				BOptionPopUp *popUp = new OptionPopUp(rect, discrete.Name(),
					discrete.Name(), discrete);

				for (int32 i = 0; i < discrete.CountItems(); i++) {
					popUp->AddOption(discrete.ItemNameAt(i), discrete.ItemValueAt(i));
				}

				popUp->ResizeToPreferred();

				return popUp;
			}
		}

		case BParameter::B_CONTINUOUS_PARAMETER:
		{
			BContinuousParameter &continuous = static_cast<BContinuousParameter &>(*parameter);

			if (!strcmp(continuous.Kind(), B_MASTER_GAIN)
				|| !strcmp(continuous.Kind(), B_GAIN)) {
				BChannelSlider *slider = new ChannelSlider(rect,
					continuous.Name(), continuous.Name(), B_VERTICAL,
					continuous.CountChannels(), continuous);

				char minLabel[64], maxLabel[64];

				const char *unit = continuous.Unit();
				if (unit[0]) {
					// if we have a unit, print it next to the limit values
					sprintf(minLabel, "%g %s", continuous.MinValue(), continuous.Unit());
					sprintf(maxLabel, "%g %s", continuous.MaxValue(), continuous.Unit());
				} else {
					sprintf(minLabel, "%g", continuous.MinValue());
					sprintf(maxLabel, "%g", continuous.MaxValue());
				}
				slider->SetLimitLabels(minLabel, maxLabel);

				float width, height;
				slider->GetPreferredSize(&width, &height);
				slider->ResizeTo(width, 190);

				// ToDo: take BContinuousParameter::GetResponse() & ValueStep() into account!

				for (int32 i = 0; i < continuous.CountChannels(); i++) {
					slider->SetLimitsFor(i, int32(continuous.MinValue() * 1000),
						int32(continuous.MaxValue() * 1000));
				}

				return slider;
			}

			BSlider *slider = new Slider(rect, parameter->Name(),
				parameter->Name(), 0, 100, continuous);

			float width, height;
			slider->GetPreferredSize(&width, &height);
			slider->ResizeTo(100, height);

			return slider;
		}

		default:
			ERROR("BMediaTheme: Don't know parameter type: 0x%x\n",
				parameter->Type());
	}
	return NULL;
}

