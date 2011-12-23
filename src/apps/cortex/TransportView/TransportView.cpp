/*
 * Copyright (c) 1999-2000, Eric Moon.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions, and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions, and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF TITLE, NON-INFRINGEMENT, MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
 * TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


// TransportView.cpp

#include "TransportView.h"

#include "RouteApp.h"
#include "RouteWindow.h"

#include "RouteAppNodeManager.h"
#include "NodeGroup.h"

#include "NumericValControl.h"
#include "TextControlFloater.h"

#include <Button.h>
#include <Debug.h>
#include <Font.h>
#include <Invoker.h>
#include <StringView.h>
#include <MediaRoster.h>
#include <MenuField.h>
#include <MenuItem.h>
#include <PopUpMenu.h>
#include <String.h>
#include <TextControl.h>

#include <algorithm>
#include <functional>

using namespace std;

__USE_CORTEX_NAMESPACE

// -------------------------------------------------------- //
// _GroupInfoView
// -------------------------------------------------------- //

__BEGIN_CORTEX_NAMESPACE
class _GroupInfoView :
	public	BView {
	typedef	BView _inherited;
	
public:												// ctor/dtor
	_GroupInfoView(
		BRect											frame,
		TransportView*						parent,
		const char*								name,
		uint32										resizeMode =B_FOLLOW_LEFT|B_FOLLOW_TOP,
		uint32										flags =B_WILL_DRAW|B_FRAME_EVENTS) :
		
		BView(frame, name, resizeMode, flags),
		m_parent(parent),
		m_plainFont(be_plain_font),
		m_boldFont(be_bold_font) {
		
		_initViews();
		_initColors();
		_updateLayout();
	}

public:												// BView
	virtual void FrameResized(
		float											width,
		float											height) {

		_inherited::FrameResized(width, height);
		_updateLayout();
		Invalidate();
	}
	
	virtual void GetPreferredSize(
		float*										width,
		float*										height) {
		font_height fh;
		m_plainFont.GetHeight(&fh);
		
		*width = 0.0;
		*height = (fh.ascent+fh.descent+fh.leading) * 2;
		*height += 4.0; //+++++
	}
	
	
	virtual void Draw(
		BRect											updateRect) {
		
		NodeGroup* g = m_parent->m_group;
		BRect b = Bounds();
		
		// border
		rgb_color hi = tint_color(ViewColor(), B_LIGHTEN_2_TINT);
		rgb_color lo = tint_color(ViewColor(), B_DARKEN_2_TINT);
		SetHighColor(lo);
		StrokeLine(
			b.LeftTop(), b.RightTop());
		StrokeLine(
			b.LeftTop(), b.LeftBottom());

		SetHighColor(hi);
		StrokeLine(
			b.LeftBottom(), b.RightBottom());
		StrokeLine(
			b.RightTop(), b.RightBottom());
			
		SetHighColor(255,255,255,255);
		
		// background +++++
		
		// name
		BString name = g ? g->name() : "(no group)";
		// +++++ constrain width
		SetFont(&m_boldFont);
		DrawString(name.String(), m_namePosition);
		
		SetFont(&m_plainFont);
		
		// node count
		BString nodeCount;
		if(g)
			nodeCount << g->countNodes();
		else
			nodeCount << '0';
		nodeCount << ((nodeCount == "1") ? " node." : " nodes.");
		// +++++ constrain width
		DrawString(nodeCount.String(), m_nodeCountPosition);
		
		// status
		BString status = "No errors.";
		// +++++ constrain width
		DrawString(status.String(), m_statusPosition);
	}
	
	virtual void MouseDown(
		BPoint										point) {
		
		NodeGroup* g = m_parent->m_group;
		if(!g)
			return;
		
		font_height fh;
		m_boldFont.GetHeight(&fh);
		
		BRect nameBounds(
			m_namePosition.x,
			m_namePosition.y - fh.ascent,
			m_namePosition.x + m_maxNameWidth,
			m_namePosition.y + (fh.ascent+fh.leading-4.0));
		if(nameBounds.Contains(point)) {
			ConvertToScreen(&nameBounds);
			nameBounds.OffsetBy(-7.0, -3.0);
			new TextControlFloater(
				nameBounds,
				B_ALIGN_LEFT,
				&m_boldFont,
				g->name(),
				m_parent,
				new BMessage(TransportView::M_SET_NAME));
		}
	}

public:												// implementation
	void _initViews() {
		// +++++
	}

	void _initColors() {
		// +++++ these colors need to be centrally defined
		SetViewColor(16, 64, 96, 255);
		SetLowColor(16, 64, 96, 255);
		SetHighColor(255,255,255,255);
	}

	void _updateLayout() {
		float _edge_pad_x = 3.0;
		float _edge_pad_y = 1.0;
		
		BRect b = Bounds();
		font_height fh;
		m_plainFont.GetHeight(&fh);
		
		float realWidth = b.Width() - (_edge_pad_x * 2);

		m_maxNameWidth = realWidth * 0.7;
		m_maxNodeCountWidth = realWidth - m_maxNameWidth;
		m_namePosition.x = _edge_pad_x;
		m_namePosition.y = _edge_pad_x + fh.ascent - 2.0;
		m_nodeCountPosition = m_namePosition;
		m_nodeCountPosition.x = m_maxNameWidth;
		
		m_maxStatusWidth = realWidth;
		m_statusPosition.x = _edge_pad_x;
		m_statusPosition.y = b.Height() - (fh.descent + fh.leading + _edge_pad_y);
	}

private:
	TransportView*							m_parent;
	
	BFont												m_plainFont;
	BFont												m_boldFont;
	
	BPoint											m_namePosition;
	float												m_maxNameWidth;
	
	BPoint											m_nodeCountPosition;
	float												m_maxNodeCountWidth;
	
	BPoint											m_statusPosition;
	float												m_maxStatusWidth;
};
__END_CORTEX_NAMESPACE

// -------------------------------------------------------- //
// *** ctors
// -------------------------------------------------------- //
	
TransportView::TransportView(
	NodeManager*						manager,
	const char*							name) :
	
	BView(
		BRect(),
		name,
		B_FOLLOW_ALL_SIDES,
		B_WILL_DRAW|B_FRAME_EVENTS),
	m_manager(manager),
	m_group(0) {

	// initialize
	_initLayout();
	_constructControls();
//	_updateLayout(); deferred until AttachedToWindow(): 24aug99
	_disableControls();
	
	SetViewColor(
		tint_color(
			ui_color(B_PANEL_BACKGROUND_COLOR),
			B_LIGHTEN_1_TINT));
}

// -------------------------------------------------------- //
// *** BView
// -------------------------------------------------------- //

void TransportView::AttachedToWindow() {
	_inherited::AttachedToWindow();
	
	// finish layout
	_updateLayout();
	
	// watch the node manager (for time-source create/delete notification)
	RouteApp* app = dynamic_cast<RouteApp*>(be_app);
	ASSERT(app);
	add_observer(this, app->manager);
}

void TransportView::AllAttached() {
	_inherited::AllAttached();
	
	// set message targets for view-configuation controls
	for(target_set::iterator it = m_localTargets.begin();
		it != m_localTargets.end(); ++it) {
		ASSERT(*it);
		(*it)->SetTarget(this);
	}
}

void TransportView::DetachedFromWindow() {
	_inherited::DetachedFromWindow();
		
	RouteApp* app = dynamic_cast<RouteApp*>(be_app);
	ASSERT(app);
	remove_observer(this, app->manager);
}

void TransportView::FrameResized(
	float										width,
	float										height) {

	_inherited::FrameResized(width, height);
//	_updateLayout();
}

void TransportView::KeyDown(
	const char*							bytes,
	int32										count) {

	switch(bytes[0]) {
		case B_SPACE: {
			RouteApp* app = dynamic_cast<RouteApp*>(be_app);
			ASSERT(app);
			BMessenger(app->routeWindow).SendMessage(
				RouteWindow::M_TOGGLE_GROUP_ROLLING);
			break;
		}

		default:
			_inherited::KeyDown(bytes, count);
	}	
}


void TransportView::MouseDown(
	BPoint									where) {
	
	MakeFocus(true);
}


// -------------------------------------------------------- //
// *** BHandler
// -------------------------------------------------------- //

void TransportView::MessageReceived(
	BMessage*								message) {
	status_t err;
	uint32 groupID;
	
//	PRINT((
//		"TransportView::MessageReceived()\n"));
//	message->PrintToStream();
	
	switch(message->what) {

		case NodeGroup::M_RELEASED:
			{
				err = message->FindInt32("groupID", (int32*)&groupID);
				if(err < B_OK) {
					PRINT((
						"* TransportView::MessageReceived(NodeGroup::M_RELEASED)\n"
						"  no groupID!\n"));
				}
				if(!m_group || groupID != m_group->id()) {
					PRINT((
						"* TransportView::MessageReceived(NodeGroup::M_RELEASED)\n"
						"  mismatched groupID.\n"));
					break;
				}
				
				_releaseGroup();
//				
//				BMessage m(M_REMOVE_OBSERVER);
//				m.AddMessenger("observer", BMessenger(this));
//				message->SendReply(&m);
			}
			break;
			
		case NodeGroup::M_OBSERVER_ADDED:
			err = message->FindInt32("groupID", (int32*)&groupID);
			if(err < B_OK) {
				PRINT((
					"* TransportView::MessageReceived(NodeGroup::M_OBSERVER_ADDED)\n"
					"  no groupID!\n"));
				break;
			}
			if(!m_group || groupID != m_group->id()) {
				PRINT((
					"* TransportView::MessageReceived(NodeGroup::M_OBSERVER_ADDED)\n"
					"  mismatched groupID; ignoring.\n"));
				break;
			}
			
			_enableControls();
			break;
			
		case NodeGroup::M_TRANSPORT_STATE_CHANGED:
			uint32 groupID;
			err = message->FindInt32("groupID", (int32*)&groupID);
			if(err < B_OK) {
				PRINT((
					"* TransportView::MessageReceived(NodeGroup::M_TRANSPORT_STATE_CHANGED)\n"
					"  no groupID!\n"));
				break;
			}
			if(!m_group || groupID != m_group->id()) {
				PRINT((
					"* TransportView::MessageReceived(NodeGroup::M_TRANSPORT_STATE_CHANGED)\n"
					"  mismatched groupID; ignoring.\n"));
				break;
			}
				
			_updateTransportButtons();
			break;
			
		case NodeGroup::M_TIME_SOURCE_CHANGED:
			//_updateTimeSource(); +++++ check group?
			break;
			
		case NodeGroup::M_RUN_MODE_CHANGED:
			//_updateRunMode(); +++++ check group?
			break;
			
		// * CONTROL PROCESSING
		
		case NodeGroup::M_SET_START_POSITION: {

			if(!m_group)
				break;

			bigtime_t position = _scalePosition(
				m_regionStartView->value());
			message->AddInt64("position", position);
			BMessenger(m_group).SendMessage(message);
			
			// update start-button duty/label [e.moon 11oct99]
			if(m_group->transportState() == NodeGroup::TRANSPORT_STOPPED)
				_updateTransportButtons();

			break;
		}

		case NodeGroup::M_SET_END_POSITION: {

			if(!m_group)
				break;

			bigtime_t position = _scalePosition(
				m_regionEndView->value());
			message->AddInt64("position", position);
			BMessenger(m_group).SendMessage(message);

			// update start-button duty/label [e.moon 11oct99]
			if(m_group->transportState() == NodeGroup::TRANSPORT_STOPPED)
				_updateTransportButtons();

			break;
		}
		
		case M_SET_NAME:
			{
				const char* name;
				err = message->FindString("_value", &name);
				if(err < B_OK) {
					PRINT((
						"! TransportView::MessageReceived(M_SET_NAME): no _value!\n"));
					break;
				}
				if(m_group) {
					m_group->setName(name);
					m_infoView->Invalidate();
				}
			}
			break;
		
		case RouteAppNodeManager::M_TIME_SOURCE_CREATED:
			_timeSourceCreated(message);
			break;

		case RouteAppNodeManager::M_TIME_SOURCE_DELETED:
			_timeSourceDeleted(message);
			break;

		default:
			_inherited::MessageReceived(message);
			break;
	}
}

// -------------------------------------------------------- //
// *** BHandler impl.
// -------------------------------------------------------- //

void TransportView::_handleSelectGroup(
	BMessage*								message) {

	uint32 groupID;
	status_t err = message->FindInt32("groupID", (int32*)&groupID);
	if(err < B_OK) {
		PRINT((
			"* TransportView::_handleSelectGroup(): no groupID\n"));
		return;
	}
	
	if(m_group && groupID != m_group->id())
		_releaseGroup();
		
	_selectGroup(groupID);
	
	Invalidate();
}

// -------------------------------------------------------- //
// *** internal operations
// -------------------------------------------------------- //

// select the given group; initialize controls
// (if 0, gray out all controls)
void TransportView::_selectGroup(
	uint32									groupID) {
	status_t err;
	
	if(m_group)
		_releaseGroup();

	if(!groupID) {
		_disableControls();
		return;
	}

	err = m_manager->findGroup(groupID, &m_group);
	if(err < B_OK) {
		PRINT((
			"* TransportView::_selectGroup(%ld): findGroup() failed:\n"
			"  %s\n",
			groupID,
			strerror(err)));
		return;
	}
	
	_observeGroup();
}

void TransportView::_observeGroup() {
	ASSERT(m_group);
	
	add_observer(this, m_group);
}

void TransportView::_releaseGroup() {
	ASSERT(m_group);
	
	remove_observer(this, m_group);
	m_group = 0;
}
// -------------------------------------------------------- //
// *** CONTROLS
// -------------------------------------------------------- //

const char _run_mode_strings[][20] = {
	"Offline",
	"Decrease precision",
	"Increase latency",
	"Drop data",
	"Recording"
};
const int _run_modes = 5;

//const char _time_source_strings[][20] = {
//	"DAC time source",
//	"System clock"
//};
//const int _time_sources = 2;

const char* _region_start_label = "From:";
const char* _region_end_label = "To:";

void TransportView::_constructControls() {

	BMessage* m;

	// * create and populate, but don't position, the views:

//	// create and populate, but don't position, the views:
//	m_nameView = new BStringView(
//		BRect(),
//		"nameView",
//		"Group Name",
//		B_FOLLOW_NONE);
//	AddChild(m_nameView);

	m_infoView = new _GroupInfoView(
		BRect(),
		this,
		"infoView");
	AddChild(m_infoView);
	
	m_runModeView = new BMenuField(
		BRect(),
		"runModeView",
		"Run mode:",
		new BPopUpMenu("runModeMenu"));
	_populateRunModeMenu(
		m_runModeView->Menu());
	AddChild(m_runModeView);

	m_timeSourceView = new BMenuField(
		BRect(),
		"timeSourceView",
		"Time source:",
		new BPopUpMenu("timeSourceMenu"));
	_populateTimeSourceMenu(
		m_timeSourceView->Menu());
	AddChild(m_timeSourceView);


	m_fromLabel = new BStringView(BRect(), 0, "Roll from");
	AddChild(m_fromLabel);
	
	
	m = new BMessage(NodeGroup::M_SET_START_POSITION);
	m_regionStartView = new NumericValControl(
		BRect(),
		"regionStartView",
		m,
		2, 4, // * DIGITS
		false, ValControl::ALIGN_FLUSH_LEFT);
		
	// redirect view back to self.  this gives me the chance to 
	// augment the message with the position before sending 
	_addLocalTarget(m_regionStartView);
	AddChild(m_regionStartView);

	m_toLabel = new BStringView(BRect(), 0, "to");
	AddChild(m_toLabel);
	
	m = new BMessage(NodeGroup::M_SET_END_POSITION);
	m_regionEndView = new NumericValControl(
		BRect(),
		"regionEndView",
		m,
		2, 4, // * DIGITS
		false, ValControl::ALIGN_FLUSH_LEFT);
		
	// redirect view back to self.  this gives me the chance to 
	// augment the message with the position before sending 
	_addLocalTarget(m_regionEndView);
	AddChild(m_regionEndView);

//	m = new BMessage(NodeGroup::M_SET_START_POSITION);
//	m_regionStartView = new BTextControl(
//		BRect(),
//		"regionStartView",
//		_region_start_label,
//		"0",
//		m);
//
//	_addGroupTarget(m_regionStartView);
////	m_regionStartView->TextView()->SetAlignment(B_ALIGN_RIGHT);
//	
//	AddChild(m_regionStartView);
//
//	m = new BMessage(NodeGroup::M_SET_END_POSITION);
//	m_regionEndView = new BTextControl(
//		BRect(),
//		"regionEndView",
//		_region_end_label,
//		"0",
//		m);
//
//	_addGroupTarget(m_regionEndView);
////	m_regionEndView->TextView()->SetAlignment(B_ALIGN_RIGHT);
//	AddChild(m_regionEndView);
	
	m = new BMessage(NodeGroup::M_START);
	m_startButton = new BButton(
		BRect(),
		"startButton",
		"Start",
		m);
	_addGroupTarget(m_startButton);
	AddChild(m_startButton);

	m = new BMessage(NodeGroup::M_STOP);
	m_stopButton = new BButton(
		BRect(),
		"stopButton",
		"Stop",
		m);
	_addGroupTarget(m_stopButton);
	AddChild(m_stopButton);

	m = new BMessage(NodeGroup::M_PREROLL);
	m_prerollButton = new BButton(
		BRect(),
		"prerollButton",
		"Preroll",
		m);
	_addGroupTarget(m_prerollButton);
	AddChild(m_prerollButton);
}

// convert a position control's value to bigtime_t
// [e.moon 11oct99]
bigtime_t TransportView::_scalePosition(
	double									value) {
	
	return bigtime_t(value * 1000000.0);
}

void TransportView::_populateRunModeMenu(
	BMenu*									menu) {

	BMessage* m;
	for(int n = 0; n < _run_modes; ++n) {
		m = new BMessage(NodeGroup::M_SET_RUN_MODE);
		m->AddInt32("runMode", n+1);

		BMenuItem* i = new BMenuItem(
			_run_mode_strings[n], m);
		menu->AddItem(i);
		_addGroupTarget(i);
	}
}

void TransportView::_populateTimeSourceMenu(
	BMenu*									menu) {

	// find the standard time sources
	status_t err;
	media_node dacTimeSource, systemTimeSource;
	BMessage* m;
	BMenuItem* i;
	err = m_manager->roster->GetTimeSource(&dacTimeSource);
	if(err == B_OK) {
		m = new BMessage(NodeGroup::M_SET_TIME_SOURCE);
		m->AddData(
			"timeSourceNode",
			B_RAW_TYPE,
			&dacTimeSource,
			sizeof(media_node));
		i = new BMenuItem(
			"DAC time source",
			m);
		menu->AddItem(i);
		_addGroupTarget(i);
	}

	err = m_manager->roster->GetSystemTimeSource(&systemTimeSource);
	if(err == B_OK) {
		m = new BMessage(NodeGroup::M_SET_TIME_SOURCE);
		m->AddData(
			"timeSourceNode",
			B_RAW_TYPE,
			&systemTimeSource,
			sizeof(media_node));
		i = new BMenuItem(
			"System clock",
			m);
		menu->AddItem(i);
		_addGroupTarget(i);
	}

	// find other time source nodes

	Autolock _l(m_manager);
	void* cookie = 0;
	NodeRef* r;
	char nameBuffer[B_MEDIA_NAME_LENGTH+16];
	bool needSeparator = (menu->CountItems() > 0);
	while(m_manager->getNextRef(&r, &cookie) == B_OK) {
		if(!(r->kind() & B_TIME_SOURCE))
			continue;
		if(r->id() == dacTimeSource.node)
			continue;
		if(r->id() == systemTimeSource.node)
			continue;

		if(needSeparator) {
			needSeparator = false;
			menu->AddItem(new BSeparatorItem());
		}

		m = new BMessage(NodeGroup::M_SET_TIME_SOURCE);
		m->AddData(
			"timeSourceNode",
			B_RAW_TYPE,
			&r->node(),
			sizeof(media_node));
		sprintf(nameBuffer, "%s: %ld",
			r->name(),
			r->id());
		i = new BMenuItem(
			nameBuffer,
			m);
		menu->AddItem(i);
		_addGroupTarget(i);
	}
	
//	BMessage* m;
//	for(int n = 0; n < _time_sources; ++n) {
//		m = new BMessage(NodeGroup::M_SET_TIME_SOURCE);
//		m->AddData(
//			"timeSourceNode",
//			B_RAW_TYPE,
//			&m_timeSourcePresets[n],
//			sizeof(media_node));
//		// +++++ copy media_node of associated time source!
////		m->AddInt32("timeSource", n);
//		
//		BMenuItem* i = new BMenuItem(
//				_time_source_strings[n], m);
//		menu->AddItem(i);
//		_addGroupTarget(i);
//	}
}

// add the given invoker to be retargeted to this
// view (used for controls whose messages need a bit more
// processing before being forwarded to the NodeManager.)

void TransportView::_addLocalTarget(
	BInvoker*								invoker) {
	
	m_localTargets.push_back(invoker);
	if(Window())
		invoker->SetTarget(this);
}

void TransportView::_addGroupTarget(
	BInvoker*								invoker) {
	
	m_groupTargets.push_back(invoker);
	if(m_group)
		invoker->SetTarget(m_group);
}

void TransportView::_refreshTransportSettings() {
	if(m_group)
		_updateTransportButtons();
}


void TransportView::_disableControls() {

//	PRINT((
//		"*** _disableControls()\n"));

	BWindow* w = Window();
	if(w)
		w->BeginViewTransaction();

//	m_nameView->SetText("(no group)");
	m_infoView->Invalidate();
	
	m_runModeView->SetEnabled(false);
	m_runModeView->Menu()->Superitem()->SetLabel("(none)");

	m_timeSourceView->SetEnabled(false);
	m_timeSourceView->Menu()->Superitem()->SetLabel("(none)");

	m_regionStartView->SetEnabled(false);
	m_regionStartView->setValue(0);

	m_regionEndView->SetEnabled(false);
	m_regionEndView->setValue(0);
	
	m_startButton->SetEnabled(false);
	m_stopButton->SetEnabled(false);
	m_prerollButton->SetEnabled(false);

	if(w)
		w->EndViewTransaction();


//	PRINT((
//		"*** DONE: _disableControls()\n"));
}

void TransportView::_enableControls() {

//	PRINT((
//		"*** _enableControls()\n"));
//
	ASSERT(m_group);
	Autolock _l(m_group); // +++++ why?
	BWindow* w = Window();
	if(w) {
		w->BeginViewTransaction();

		// clear focused view
		// 19sep99: TransportWindow is currently a B_AVOID_FOCUS window; the
		// only way views get focused is if they ask to be (which ValControl
		// currently does.)
		BView* focused = w->CurrentFocus();
		if(focused)
			focused->MakeFocus(false);
	}	

//	BString nameViewText = m_group->name();
//	nameViewText << ": " << m_group->countNodes() << " nodes.";
//	m_nameView->SetText(nameViewText.String());
	
	m_infoView->Invalidate();
	
	m_runModeView->SetEnabled(true);
	_updateRunMode();

	m_timeSourceView->SetEnabled(true);
	_updateTimeSource();

	_updateTransportButtons();
	

	// +++++ ick. hard-coded scaling factors make me queasy
	
	m_regionStartView->SetEnabled(true);
	m_regionStartView->setValue(
		((double)m_group->startPosition()) / 1000000.0);
	
	m_regionEndView->SetEnabled(true);
	m_regionEndView->setValue(
		((double)m_group->endPosition()) / 1000000.0);

	// target controls on group
	for(target_set::iterator it = m_groupTargets.begin();
		it != m_groupTargets.end(); ++it) {
		ASSERT(*it);
		(*it)->SetTarget(m_group);
	}

	if(w) {
		w->EndViewTransaction();
	}	

//	PRINT((
//		"*** DONE: _enableControls()\n"));
}

void TransportView::_updateTransportButtons() {

	ASSERT(m_group);
	
	switch(m_group->transportState()) {
		case NodeGroup::TRANSPORT_INVALID:
		case NodeGroup::TRANSPORT_STARTING:
		case NodeGroup::TRANSPORT_STOPPING:
			m_startButton->SetEnabled(false);
			m_stopButton->SetEnabled(false);
			m_prerollButton->SetEnabled(false);
			break;
			
		case NodeGroup::TRANSPORT_STOPPED:
			m_startButton->SetEnabled(true);
			m_stopButton->SetEnabled(false);
			m_prerollButton->SetEnabled(true);
			break;

		case NodeGroup::TRANSPORT_RUNNING:
		case NodeGroup::TRANSPORT_ROLLING:
			m_startButton->SetEnabled(false);
			m_stopButton->SetEnabled(true);
			m_prerollButton->SetEnabled(false);
			break;
	}

	// based on group settings, set the start button to do either
	// a simple start or a roll (atomic start/stop.) [e.moon 11oct99]
	ASSERT(m_startButton->Message());
	
	// get current region settings
	bigtime_t startPosition = _scalePosition(m_regionStartView->value());
	bigtime_t endPosition = _scalePosition(m_regionEndView->value());
	
	// get current run-mode setting
	uint32 runMode = 0;
	BMenuItem* i = m_runModeView->Menu()->FindMarked();
	ASSERT(i);
	runMode = m_runModeView->Menu()->IndexOf(i) + 1;

	if(
		endPosition > startPosition &&
		(runMode == BMediaNode::B_OFFLINE ||
			!m_group->canCycle())) {
			
		m_startButton->SetLabel("Roll");
		m_startButton->Message()->what = NodeGroup::M_ROLL;

	} else {
		m_startButton->SetLabel("Start");
		m_startButton->Message()->what = NodeGroup::M_START;
	}
}

void TransportView::_updateTimeSource() {
	ASSERT(m_group);

	media_node tsNode;
	status_t err = m_group->getTimeSource(&tsNode);
	if(err < B_OK) {
		PRINT((
			"! TransportView::_updateTimeSource(): m_group->getTimeSource():\n"
			"  %s\n",
			strerror(err)));
		return;
	}

	BMenu* menu = m_timeSourceView->Menu();
	ASSERT(menu);
	if(tsNode == media_node::null) {
		menu->Superitem()->SetLabel("(none)");
		return;
	}

	// find menu item
	int32 n;
	for(n = menu->CountItems()-1; n >= 0; --n) {
		const void* data;
		media_node itemNode;
		BMessage* message = menu->ItemAt(n)->Message();
		if(!message)
			continue;
			
		ssize_t size = sizeof(media_node);
		err = message->FindData(			
			"timeSourceNode",
			B_RAW_TYPE,
			&data,
			&size);
		if(err < B_OK)
			continue;
			
		memcpy(&itemNode, data, sizeof(media_node));
		if(itemNode != tsNode)
			continue;
		
		// found it
		PRINT((
			"### _updateTimeSource: %s\n",
			menu->ItemAt(n)->Label()));
		menu->ItemAt(n)->SetMarked(true);
		break;
	}	
//	ASSERT(m_timeSourcePresets);
//	int n;
//	for(n = 0; n < _time_sources; ++n) {
//		if(m_timeSourcePresets[n] == tsNode) {
//			BMenuItem* i = m_timeSourceView->Menu()->ItemAt(n);
//			ASSERT(i);
//			i->SetMarked(true);
//			break;
//		}
//	}
	if(n < 0)
		menu->Superitem()->SetLabel("(???)");

}
void TransportView::_updateRunMode() {
	ASSERT(m_group);
	
	BMenu* menu = m_runModeView->Menu();
	uint32 runMode = m_group->runMode() - 1; // real run mode starts at 1
	ASSERT((uint32)menu->CountItems() > runMode);
	menu->ItemAt(runMode)->SetMarked(true);
}

//void TransportView::_initTimeSources() {
//
//	status_t err;
//	media_node node;
//	err = m_manager->roster->GetTimeSource(&node);
//	if(err == B_OK) {
//		m_timeSources.push_back(node.node);
//	}
//	
//	err = m_manager->roster->GetSystemTimeSource(&m_timeSourcePresets[1]);
//	if(err < B_OK) {
//		PRINT((
//			"* TransportView::_initTimeSources():\n"
//			"  GetSystemTimeSource() failed: %s\n", strerror(err)));
//	}
//}

// [e.moon 2dec99]
void TransportView::_timeSourceCreated(
	BMessage*								message) {

	status_t err;
	media_node_id id;
	err = message->FindInt32("nodeID", &id);
	if(err < B_OK)
		return;
		
//	PRINT(("### _timeSourceCreated(): %ld\n", id));
	NodeRef* ref;
	err = m_manager->getNodeRef(id, &ref);
	if(err < B_OK) {
		PRINT((
			"!!! TransportView::_timeSourceCreated(): node %ld doesn't exist\n",
			id));
		return; 
	}

	char nameBuffer[B_MEDIA_NAME_LENGTH+16];
	BMessage* m = new BMessage(NodeGroup::M_SET_TIME_SOURCE);
	m->AddData(
		"timeSourceNode",
		B_RAW_TYPE,
		&ref->node(),
		sizeof(media_node));
	sprintf(nameBuffer, "%s: %ld",
		ref->name(),
		ref->id());
	BMenuItem* i = new BMenuItem(
		nameBuffer,
		m);
	BMenu* menu = m_timeSourceView->Menu();
	menu->AddItem(i);
	_addGroupTarget(i);
}

// +++++
void TransportView::_timeSourceDeleted(
	BMessage*								message) {

	status_t err;
	media_node_id id;
	err = message->FindInt32("nodeID", &id);
	if(err < B_OK)
		return;
		
//	PRINT(("### _timeSourceDeleted(): %ld\n", id));

	BMenu* menu = m_timeSourceView->Menu();
	ASSERT(menu);

	// find menu item
	int32 n;
	for(n = menu->CountItems()-1; n >= 0; --n) {
		const void* data;
		media_node itemNode;
		BMessage* message = menu->ItemAt(n)->Message();
		if(!message)
			continue;
			
		ssize_t size = sizeof(media_node);
		err = message->FindData(			
			"timeSourceNode",
			B_RAW_TYPE,
			&data,
			&size);
		if(err < B_OK)
			continue;
			
		memcpy(&itemNode, data, sizeof(media_node));
		if(itemNode.node != id)
			continue;
		
		// found it; remove the item
		menu->RemoveItem(n);
		break;
	}
}

// -------------------------------------------------------- //
// *** LAYOUT ***
// -------------------------------------------------------- //

const float _edge_pad_x 								= 3.0;
const float _edge_pad_y 								= 3.0;

const float _column_pad_x 							= 4.0;

const float _field_pad_x 								= 20.0;

const float _text_view_pad_x						= 10.0;

const float _control_pad_x 							= 5.0;
const float _control_pad_y 							= 10.0;
const float _menu_field_pad_x 					= 30.0;

const float _label_pad_x 								= 8.0;

const float _transport_pad_y 						= 4.0;
const float _transport_button_width			= 60.0;
const float _transport_button_height		= 22.0;
const float _transport_button_pad_x			= 4.0;

const float _lock_button_width					= 42.0;
const float _lock_button_height					= 16.0;

class TransportView::_layout_state {
public:
	_layout_state() {}

	// +++++	
};

inline float _menu_width(
	const BMenu* menu,
	const BFont* font) {

	float max = 0.0;
	
	int total = menu->CountItems();
	for(int n = 0; n < total; ++n) {
		float w = font->StringWidth(
			menu->ItemAt(n)->Label());
		if(w > max)
			max = w;
	}
	
	return max;
}

// -------------------------------------------------------- //

void TransportView::_initLayout() {
	m_layout = new _layout_state();
}


void TransportView::_updateLayout() // +++++
{
	BWindow* window = Window();
	if(window)
		window->BeginViewTransaction();

	// calculate the ideal size of the view
	// * max label width
	float maxLabelWidth = 0.0;
	float w;
	
	maxLabelWidth = be_bold_font->StringWidth(
		m_runModeView->Label());
		
	w = be_bold_font->StringWidth(
		m_timeSourceView->Label());
	if(w > maxLabelWidth)
		maxLabelWidth = w;

//	w = be_bold_font->StringWidth(
//		m_regionStartView->Label());
//	if(w > maxLabelWidth)
//		maxLabelWidth = w;
//
//	w = be_bold_font->StringWidth(
//		m_regionEndView->Label());
//	if(w > maxLabelWidth)
//		maxLabelWidth = w;

	// * max field width
	float maxFieldWidth = 0.0;
	maxFieldWidth = _menu_width(
		m_runModeView->Menu(), be_plain_font);

	w = _menu_width(
		m_timeSourceView->Menu(), be_plain_font);
	if(w > maxFieldWidth)
		maxFieldWidth = w;

	// * column width
	float columnWidth =
		maxLabelWidth +
		maxFieldWidth + _label_pad_x + _field_pad_x;

	// figure columns
	float column1_x = _edge_pad_x;
	float column2_x = column1_x + columnWidth + _column_pad_x;

	// * sum to figure view width
	float viewWidth =
		column2_x + columnWidth + _edge_pad_x;

	// make room for buttons
	float buttonSpan =
		(_transport_button_width*3) +
		(_transport_button_pad_x*2);
	if(columnWidth < buttonSpan)
		viewWidth += (buttonSpan - columnWidth);

//	float insideWidth = viewWidth - (_edge_pad_x*2);
		
	// * figure view height a row at a time
	font_height fh;
	be_plain_font->GetHeight(&fh);
	float lineHeight = fh.ascent + fh.descent + fh.leading;

	float prefInfoWidth, prefInfoHeight;
	m_infoView->GetPreferredSize(&prefInfoWidth, &prefInfoHeight);
	float row_1_height = max(prefInfoHeight, _transport_button_height);
	
	float row1_y = _edge_pad_y;
	float row2_y = row1_y + row_1_height + _transport_pad_y;
	float row3_y = row2_y + lineHeight + _control_pad_y;
//	float row4_y = row3_y + lineHeight + _control_pad_y + _transport_pad_y;
//	float row5_y = row4_y + lineHeight + _control_pad_y;
	float viewHeight = row3_y + lineHeight + _control_pad_y + _edge_pad_y;
	
	// Place controls
	m_infoView->MoveTo(
		column1_x+1.0, row1_y+1.0);
	m_infoView->ResizeTo(
		columnWidth, prefInfoHeight);
		
	BRect br(
		column2_x, row1_y,
		column2_x+_transport_button_width,
		row1_y+_transport_button_height);
	if(prefInfoHeight > _transport_button_height)
		br.OffsetBy(0.0, (prefInfoHeight - _transport_button_height)/2);

	m_startButton->MoveTo(br.LeftTop());
	m_startButton->ResizeTo(br.Width(), br.Height());
	br.OffsetBy(_transport_button_width + _transport_button_pad_x, 0.0);

	m_stopButton->MoveTo(br.LeftTop());
	m_stopButton->ResizeTo(br.Width(), br.Height());
	br.OffsetBy(_transport_button_width + _transport_button_pad_x, 0.0);

	m_prerollButton->MoveTo(br.LeftTop());
	m_prerollButton->ResizeTo(br.Width(), br.Height());

	m_runModeView->MoveTo(
		column2_x, row2_y);
	m_runModeView->ResizeTo(
		columnWidth, lineHeight);
	m_runModeView->SetDivider(
		maxLabelWidth+_label_pad_x);
	m_runModeView->SetAlignment(
		B_ALIGN_LEFT);
		
	m_timeSourceView->MoveTo(
		column2_x, row3_y);
	m_timeSourceView->ResizeTo(
		columnWidth, lineHeight);
	m_timeSourceView->SetDivider(
		maxLabelWidth+_label_pad_x);
	m_timeSourceView->SetAlignment(
		B_ALIGN_LEFT);

//	float regionControlWidth = columnWidth;
//	float regionControlHeight = lineHeight + 4.0;

//	m_regionStartView->TextView()->SetResizingMode(
//		B_FOLLOW_LEFT_RIGHT|B_FOLLOW_TOP);

	// "FROM"
	
	BPoint rtLeftTop(column1_x, row2_y + 5.0);
	BPoint rtRightBottom;
	
	m_fromLabel->MoveTo(rtLeftTop);
	m_fromLabel->ResizeToPreferred();
	rtRightBottom = rtLeftTop + BPoint(
		m_fromLabel->Bounds().Width(),
		m_fromLabel->Bounds().Height());
	
	
	// (region-start)

	rtLeftTop.x = rtRightBottom.x+4;
	
	m_regionStartView->MoveTo(rtLeftTop + BPoint(0.0, 2.0));
	m_regionStartView->ResizeToPreferred();
	rtRightBottom = rtLeftTop + BPoint(
		m_regionStartView->Bounds().Width(),
		m_regionStartView->Bounds().Height());
	
//	m_regionStartView->SetDivider(
//		maxLabelWidth);
//	m_regionStartView->TextView()->ResizeTo(
//		regionControlWidth-(maxLabelWidth+_text_view_pad_x),
//		regionControlHeight-4.0);

	// "TO"

	rtLeftTop.x = rtRightBottom.x + 6;

	m_toLabel->MoveTo(rtLeftTop);
	m_toLabel->ResizeToPreferred();
	rtRightBottom = rtLeftTop + BPoint(
		m_toLabel->Bounds().Width(),
		m_toLabel->Bounds().Height());
	
	// (region-end)

	rtLeftTop.x = rtRightBottom.x + 4;
	
	m_regionEndView->MoveTo(rtLeftTop + BPoint(0.0, 2.0));
	m_regionEndView->ResizeToPreferred();
//	m_regionEndView->SetDivider(
//		maxLabelWidth);
//	m_regionEndView->TextView()->ResizeTo(
//		regionControlWidth-(maxLabelWidth+_text_view_pad_x),
//		regionControlHeight-4.0);


	BRect b = Bounds();
	float targetWidth = (b.Width() < viewWidth) ?
		viewWidth :
		b.Width();
	float targetHeight = (b.Height() < viewHeight) ?
		viewHeight :
		b.Height();
	
	// Resize view to fit contents
	ResizeTo(targetWidth, targetHeight);

	if(window) {
		window->ResizeTo(targetWidth, targetHeight);
	}

//	// +++++ testing NumericValControl [23aug99]
//	float valWidth, valHeight;
//	m_valView->GetPreferredSize(&valWidth, &valHeight);
//	PRINT((
//		"\n\nm_valView preferred size: %.1f x %.1f\n\n",
//		valWidth, valHeight));
//
	if(window)
		window->EndViewTransaction();
}

// -------------------------------------------------------- //
// *** dtor
// -------------------------------------------------------- //

TransportView::~TransportView() {
	if(m_group)
		_releaseGroup();
	if(m_layout)
		delete m_layout;
}
	

// END -- TransportView.cpp --
