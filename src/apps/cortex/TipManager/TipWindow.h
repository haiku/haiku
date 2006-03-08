// TipWindow.h
// * PURPOSE
//   A floating window used to display floating tips
//   (aka 'ToolTips'.)  May be subclassed to provide
//   a custom tip view or to extend the window's
//   behavior.
//
// * HISTORY
//   e.moon		17oct99		Begun (based on TipFloater.)

#ifndef __TipWindow_H__
#define __TipWindow_H__

#include <String.h>
#include <Window.h>

#include "cortex_defs.h"
__BEGIN_CORTEX_NAMESPACE

class TipView;

class TipWindow :
	public	BWindow {
	typedef	BWindow _inherited;
	
public:											// *** dtor/ctors
	virtual ~TipWindow();
	TipWindow(
		const char*							text=0);

public:											// *** operations (LOCK REQUIRED)

	const char* text() const;
	virtual void setText(
		const char*							text);

protected:									// *** hooks
	// override to substitute your own view class
	virtual TipView* createTipView();

public:											// *** BWindow

	// initializes the tip view
	virtual void Show();

	// remove tip view? +++++
	virtual void Hide();

	// hides the window when the user switches workspaces
	// +++++ should it be restored when the user switches back?
	virtual void WorkspaceActivated(
		int32										workspace,
		bool										active);

private:										// implementation
	TipView*									m_tipView;
	BString										m_text;
	
	void _createTipView();
	void _destroyTipView();
};

__END_CORTEX_NAMESPACE
#endif /*__TipWindow_H__*/
