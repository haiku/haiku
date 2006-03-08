// ValCtrlLayoutEntry.cpp
// e.moon 29jan99

#include "ValCtrlLayoutEntry.h"
#include "ValControlSegment.h"

#include <Debug.h>
#include <View.h>

__USE_CORTEX_NAMESPACE

/*static*/
ValCtrlLayoutEntry ValCtrlLayoutEntry::decimalPoint(
	DECIMAL_POINT_ENTRY, LAYOUT_NO_PADDING);

/*static*/
void ValCtrlLayoutEntry::InitChildFrame(ValCtrlLayoutEntry& e) {
	if(e.pView) {
//		PRINT((
//			"### ValCtrlLayoutEntry::InitChildFrame(): (%.1f,%.1f)-(%.1f,%.1f)\n",
//			e.frame.left, e.frame.top, e.frame.right, e.frame.bottom));
		e.pView->MoveTo(e.frame.LeftTop());
		e.pView->ResizeTo(e.frame.Width(), e.frame.Height());
	}
}


// END -- ValCtrlLayoutEntry.cpp -- 