// InfoWindow.cpp

#include "InfoWindow.h"

// Interface Kit
#include <Screen.h>
#include <ScrollBar.h>
#include <View.h>

__USE_CORTEX_NAMESPACE

#include <Debug.h>
#define D_ALLOC(x) //PRINT (x)
#define D_HOOK(x) //PRINT (x)
#define D_INTERNAL(x) //PRINT (x)
#define D_MESSAGE(x) //PRINT (x)

// -------------------------------------------------------- //
// ctor/dtor
// -------------------------------------------------------- //

InfoWindow::InfoWindow(
	BRect frame)
	: BWindow(frame, 
			  "", B_DOCUMENT_WINDOW, 0),
	  m_zoomed(false),
	  m_zooming(false) {
	D_ALLOC(("InfoWindow::InfoWindow()\n"));

}

InfoWindow::~InfoWindow() {
	D_ALLOC(("InfoWindow::~InfoWindow()\n"));

}

// -------------------------------------------------------- //
// BWindow impl
// -------------------------------------------------------- //

void
InfoWindow::FrameResized(
	float width,
	float height) {
	D_HOOK(("InfoWindow::FrameResized()\n"));

	if (!m_zooming) {
		m_zoomed = false;
	}
	else {
		m_zooming = false;
	}
}

void
InfoWindow::Show() {
	D_HOOK(("InfoWindow::Show()\n"));

	_constrainToScreen();
	m_manualSize = Bounds().OffsetToCopy(0.0, 0.0);

	BWindow::Show();
}

void
InfoWindow::Zoom(
	BPoint origin,
	float width,
	float height) {
	D_HOOK(("InfoWindow::Zoom()\n"));

	m_zooming = true;

	BScreen screen(this);
	if (!screen.Frame().Contains(Frame())) {
		m_zoomed = false;
	}

	if (!m_zoomed) {
		// resize to the ideal size
		m_manualSize = Bounds();
		float width, height;
		FindView("InfoView")->GetPreferredSize(&width, &height);
		width += B_V_SCROLL_BAR_WIDTH;
		ResizeTo(width, height);
		_constrainToScreen();
		m_zoomed = true;
	}
	else {
		// resize to the most recent manual size
		ResizeTo(m_manualSize.Width(), m_manualSize.Height());
		m_zoomed = false;
	}
}

// -------------------------------------------------------- //
// internal operations
// -------------------------------------------------------- //

void
InfoWindow::_constrainToScreen() {
	D_INTERNAL(("InfoWindow::_constrainToScreen()\n"));
	
	BScreen screen(this);
	BRect screenRect = screen.Frame();
	BRect windowRect = Frame();

	// if the window is outside the screen rect
	// move it to the default position
	if (!screenRect.Intersects(windowRect)) {
		windowRect.OffsetTo(screenRect.LeftTop());
		MoveTo(windowRect.LeftTop());
		windowRect = Frame();
	}

	// if the window is larger than the screen rect
	// resize it to fit at each side
	if (!screenRect.Contains(windowRect)) {
		if (windowRect.left < screenRect.left) {
			windowRect.left = screenRect.left + 5.0;
			MoveTo(windowRect.LeftTop());
			windowRect = Frame();
		}
		if (windowRect.top < screenRect.top) {
			windowRect.top = screenRect.top + 5.0;
			MoveTo(windowRect.LeftTop());
			windowRect = Frame();
		}
		if (windowRect.right > screenRect.right) {
			windowRect.right = screenRect.right - 5.0;
		}
		if (windowRect.bottom > screenRect.bottom) {
			windowRect.bottom = screenRect.bottom - 5.0;
		}
		ResizeTo(windowRect.Width(), windowRect.Height());
	}
}

// END -- InfoWindow.cpp --
