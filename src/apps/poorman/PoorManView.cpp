/* PoorManView.cpp
 *
 *	Philip Harrison
 *	Started: 4/25/2004
 *	Version: 0.1
 */
 
#ifndef POOR_MAN_VIEW_H
#include "PoorManView.h"
#endif

PoorManView::PoorManView(BRect rect, const char *name)
	: BView(rect, name, B_FOLLOW_ALL, B_WILL_DRAW)
{
}

void 
PoorManView::AttachedToWindow()
{
}
