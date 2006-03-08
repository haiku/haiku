// TipWindow.cpp

#include "TipWindow.h"
#include "TipView.h"

#include <Debug.h>

__USE_CORTEX_NAMESPACE

// -------------------------------------------------------- //
// *** dtor/ctors
// -------------------------------------------------------- //

TipWindow::~TipWindow() {}
TipWindow::TipWindow(
	const char*							text=0) :
	BWindow(
		BRect(0,0,0,0),
		"TipWindow",
		B_NO_BORDER_WINDOW_LOOK,
		B_FLOATING_ALL_WINDOW_FEEL,
		B_NOT_MOVABLE|B_AVOID_FOCUS/*,
		B_ALL_WORKSPACES*/),
	// the TipView is created on demand
	m_tipView(0) {

	if(text)
		m_text = text;
}

		
// -------------------------------------------------------- //
// *** operations (LOCK REQUIRED)
// -------------------------------------------------------- //

const char* TipWindow::text() const {
	return m_text.Length() ?
		m_text.String() :
		0;
}

void TipWindow::setText(
	const char*							text) {

	if(!m_tipView)
		_createTipView();

	m_text = text;	
	m_tipView->setText(text);
	
	// size to fit view
	float width, height;
	m_tipView->GetPreferredSize(&width, &height);
	m_tipView->ResizeTo(width, height);
	ResizeTo(width, height);
	
	m_tipView->Invalidate();
}

// -------------------------------------------------------- //
// *** hooks
// -------------------------------------------------------- //

// override to substitute your own view class
TipView* TipWindow::createTipView() {
	return new TipView;
}

// -------------------------------------------------------- //
// *** BWindow
// -------------------------------------------------------- //

// initializes the tip view
void TipWindow::Show() {

	// initialize the tip view if necessary
	if(!m_tipView)
		_createTipView();
	
	_inherited::Show();
}

// remove tip view? +++++
void TipWindow::Hide() {
	_inherited::Hide();
}


// hides the window when the user switches workspaces
// +++++ should it be restored when the user switches back?
void TipWindow::WorkspaceActivated(
	int32										workspace,
	bool										active) {

	// don't confuse the user
	if(!IsHidden())
		Hide();	

	_inherited::WorkspaceActivated(workspace, active);
}

// -------------------------------------------------------- //
// implementation
// -------------------------------------------------------- //

void TipWindow::_createTipView() {
	if(m_tipView)
		_destroyTipView();
	m_tipView = createTipView();
	ASSERT(m_tipView);
	
	AddChild(m_tipView);
	
	if(m_text.Length())
		m_tipView->setText(m_text.String());
	else
		m_tipView->setText("(no info)");
}

void TipWindow::_destroyTipView() {
	if(!m_tipView)
		return;
	RemoveChild(m_tipView);
	delete m_tipView;
	m_tipView = 0;
}

// END -- TipWindow.cpp --