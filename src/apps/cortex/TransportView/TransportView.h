// TransportView.h
// * PURPOSE
//   UI component (view) providing access to a selected
//   NodeGroup's properties & transport controls.
//
// * HISTORY
//   e.moon		18aug99		Begun.

#ifndef __TransportView_H__
#define __TransportView_H__

#include <list>

#include <View.h>
#include <PopUpMenu.h>

class BButton;
class BStringView;
class BTextControl;
class BMenuField;
class BMenu;

#include "cortex_defs.h"
__BEGIN_CORTEX_NAMESPACE

class NodeManager;
class NodeGroup;

class NumericValControl;

class _GroupInfoView;
class TransportWindow;

class TransportView :
	public	BView {
	typedef	BView _inherited;

	enum message_t {
		// _value: string
		M_SET_NAME					= TransportView_message_base
	};

public:											// *** ctors/dtor
	virtual ~TransportView(); //nyi
	
	TransportView(
		NodeManager*						nodeManager,
		const char*							name); //nyi
	
public:											// *** BView
	virtual void AttachedToWindow();
	virtual void AllAttached();
	virtual void DetachedFromWindow();
	virtual void FrameResized(
		float										width,
		float										height);
	virtual void KeyDown(
		const char*							bytes,
		int32										count);
	virtual void MouseDown(
		BPoint									where);
		
public:											// *** BHandler
	virtual void MessageReceived(
		BMessage*								message); //nyi
		
private:										// *** BHandler impl.
	void _handleSelectGroup(
		BMessage*								message);
		
protected:									// *** internal operations

	// select the given group; initialize controls
	// (if 0, gray out all controls)
	void _selectGroup(
		uint32									groupID);

	void _observeGroup();
	void _releaseGroup();
	
	void _initTimeSources();

protected:									// *** controls

	void _constructControls(); //nyi
	
	void _disableControls();
	void _enableControls();

	void _updateTransportButtons();
	void _updateTimeSource();
	void _updateRunMode();
	
	// convert a position control's value to bigtime_t
	// [e.moon 11oct99]
	bigtime_t _scalePosition(
		double									value);
	
	void _populateRunModeMenu(
		BMenu*									menu);
	void _populateTimeSourceMenu(
		BMenu*									menu);
		
	// add the given invoker to be retargeted to this
	// view (used for controls whose messages need a bit more
	// processing before being forwarded to the NodeGroup.)
	void _addLocalTarget(
		BInvoker*								invoker);

	void _addGroupTarget(
		BInvoker*								invoker);
		
	void _refreshTransportSettings();
	
	// [e.moon 2dec99]
	void _timeSourceCreated(
		BMessage*								message); //nyi
	void _timeSourceDeleted(
		BMessage*								message); //nyi
		
protected:									// *** layout

	void _initLayout();
	void _updateLayout();

private:
	friend _GroupInfoView;
	friend TransportWindow;

	// logical
	NodeManager*							m_manager;
	NodeGroup*								m_group;

	// controls
	_GroupInfoView*						m_infoView;
	BMenuField*								m_runModeView;
	BMenuField*								m_timeSourceView;

	BStringView*							m_fromLabel;
	NumericValControl*				m_regionStartView;
	BStringView*							m_toLabel;
	NumericValControl*				m_regionEndView;
	
	BButton*									m_startButton;
	BButton*									m_stopButton;
	BButton*									m_prerollButton;
	
	typedef list<BInvoker*>		target_set;
	target_set								m_localTargets;
	target_set								m_groupTargets;
	
	// layout
	class _layout_state;
	_layout_state*						m_layout;
};

__END_CORTEX_NAMESPACE
#endif /*__TransportView_H__*/
