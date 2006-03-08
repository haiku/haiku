// ParameterWindow.h (Cortex/ParameterWindow)
//
// * PURPOSE
//	 Window subclass to contain BMediaTheme generated 'parameter 
//	 views' and offers elegant resizing/zooming behaviour.
//
//	 The ParameterWindow currently listens to the MediaRoster for
// 	 changes of the parameter web, but this should change when the
//   provided BMediaTheme becomes more 'intelligent'.
//
//	 Support for selecting alternate themes is planned, but not
//	 yet finished (see the 'Themes' menu)
//
// * HISTORY
//   c.lenz		24nov99		Begun
//   c.lenz		17feb00		Added scrollbar support, migrated the
//							basic management functionality to new
//							class ParameterWindowManager
//

#ifndef __ParameterWindow_H__
#define __ParameterWindow_H__

// Interface Kit
#include <Window.h>
// Media Kit
#include <MediaNode.h>

class BMessenger;

#include "cortex_defs.h"
__BEGIN_CORTEX_NAMESPACE

class NodeRef;

class ParameterWindow : 
	public		BWindow {

public:						// *** messages

	enum message_t {
		
		// OUTBOUND
		// fields:
		//	B_INT32_TYPE		"nodeID
		M_CLOSED = ParameterWindow_message_base,

		// OUTBOUND
		// fields:
		//	B_INT32_TYPE		"nodeID"
		//  B_MESSENGER_TYPE	"messenger"
		M_CONTROL_PANEL_STARTED,

		// INBOUND
		// fields:
		//	B_BOOL_TYPE			"replace"
		M_START_CONTROL_PANEL,

		// INBOUND
		// fields:
		//  B_INT32_TYPE		"themeID"
		M_THEME_SELECTED
	};

public:						// *** ctor/dtor

							ParameterWindow(
								BPoint position,
								live_node_info &nodeInfo,
								BMessenger *notifyTarget = 0);

	virtual					~ParameterWindow();

public:						// *** BWindow impl

	// remember that frame was changed manually
	virtual void			FrameResized(
								float width,
								float height);

	// closes the window when the node is released
	virtual void			MessageReceived(
								BMessage *message);

	// stop watching the MediaRoster, tell the notifyTarget
	virtual bool			QuitRequested();

	// extend basic Zoom() functionality to 'minimize' the
	// window when currently at max size
	virtual void			Zoom(
								BPoint origin,
								float width,
								float height);

private:					// *** internal operations

	// is called from the ctor for window-positioning
	void					_init();

	// adds or updates the parameter view using the given
	// theme
	void					_updateParameterView(
								BMediaTheme *theme = 0);

	// resizes the window to fit in the current screen rect
	void					_constrainToScreen();

	// tries to start the nodes control panel through the
	// MediaRoster
	status_t				_startControlPanel();

private:					// *** data members

	media_node				m_node;

	BView				   *m_parameters;

	BMessenger			   *m_notifyTarget;
	
	BRect					m_manualSize;

	BRect					m_idealSize;

	bool					m_zoomed;

	bool					m_zooming;
};

__END_CORTEX_NAMESPACE
#endif /* __ParameterWindow_H__ */
