/* PoorManAdvancedView.cpp
 *
 *	Philip Harrison
 *	Started: 5/12/2004
 *	Version: 0.1
 */

#include <Box.h>
#include <Catalog.h>
#include <LayoutBuilder.h>
#include <Locale.h>

#include "constants.h"
#include "PoorManAdvancedView.h"
#include "PoorManWindow.h"
#include "PoorManApplication.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "PoorMan"


PoorManAdvancedView::PoorManAdvancedView(const char* name)
	:
	BView(name, B_WILL_DRAW, NULL)
{
	PoorManWindow* win;
	win = ((PoorManApplication*)be_app)->GetPoorManWindow();

	BBox* connectionOptions = new BBox(B_TRANSLATE("Connections"));
	connectionOptions->SetLabel(STR_BBX_CONNECTION);

	fMaxConnections = new StatusSlider("Max Slider", STR_SLD_LABEL,
		STR_SLD_STATUS_LABEL,
		new BMessage(MSG_PREF_ADV_SLD_MAX_CONNECTION), 1, 200);

	// labels below the slider 1 and 200
	fMaxConnections->SetLimitLabels("1", "200");
	SetMaxSimutaneousConnections(win->MaxConnections());
	

	BGroupLayout* connectionOptionsLayout = new BGroupLayout(B_VERTICAL, 0);
	connectionOptions->SetLayout(connectionOptionsLayout);

	BLayoutBuilder::Group<>(this, B_VERTICAL)
		.AddGroup(connectionOptionsLayout)
			.SetInsets(B_USE_ITEM_INSETS)
			.AddStrut(B_USE_ITEM_SPACING)
			.Add(fMaxConnections)
			.End()
		.AddGlue()
		.SetInsets(B_USE_ITEM_INSETS);
}

void
PoorManAdvancedView::SetMaxSimutaneousConnections(int32 num)
{
	if (num <= 0 || num > 200)
		fMaxConnections->SetValue(32);
	else
		fMaxConnections->SetValue(num);
}
