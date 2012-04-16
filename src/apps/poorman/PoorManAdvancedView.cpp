/* PoorManAdvancedView.cpp
 *
 *	Philip Harrison
 *	Started: 5/12/2004
 *	Version: 0.1
 */

#include <Box.h>
#include <Catalog.h>
#include <Locale.h>

#include "constants.h"
#include "PoorManAdvancedView.h"
#include "PoorManWindow.h"
#include "PoorManApplication.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "PoorMan"


PoorManAdvancedView::PoorManAdvancedView(BRect rect, const char *name)
	: BView(rect, name, B_FOLLOW_ALL, B_WILL_DRAW)
{
	PoorManWindow	*	win;
	win = ((PoorManApplication *)be_app)->GetPoorManWindow();

	SetViewColor(BACKGROUND_COLOR);

	// Console Logging BBox
	BRect maxRect;
	maxRect = rect;
	maxRect.top -= 5.0;
	maxRect.left -= 5.0;
	maxRect.right -= 7.0;
	maxRect.bottom -= 118.0;

	BBox * connectionOptions = new BBox(maxRect, B_TRANSLATE("Connections"));
	connectionOptions->SetLabel(STR_BBX_CONNECTION);
	AddChild(connectionOptions);

	BRect sliderRect;
	sliderRect = connectionOptions->Bounds();
	sliderRect.InsetBy(10.0f, 10.0f);
	sliderRect.top += 10;
	sliderRect.bottom = sliderRect.top + 50.0;

	maxConnections = new StatusSlider(sliderRect, "Max Slider", STR_SLD_LABEL,
		STR_SLD_STATUS_LABEL, new BMessage(MSG_PREF_ADV_SLD_MAX_CONNECTION), 1, 200);

	// labels below the slider 1 and 200
	maxConnections->SetLimitLabels("1", "200");
	SetMaxSimutaneousConnections(win->MaxConnections());
	connectionOptions->AddChild(maxConnections);
}

void
PoorManAdvancedView::SetMaxSimutaneousConnections(int32 num)
{
	if (num <= 0 || num > 200)
		maxConnections->SetValue(32);
	else
		maxConnections->SetValue(num);
}
