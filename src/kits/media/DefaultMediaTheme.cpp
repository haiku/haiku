/* 
** Copyright 2003, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/


#include "DefaultMediaTheme.h"
#include "debug.h"

#include <ParameterWeb.h>

#include <Slider.h>
#include <StringView.h>
#include <Button.h>
#include <TextControl.h>
#include <OptionPopUp.h>
#include <Box.h>
#include <CheckBox.h>


using namespace BPrivate;


DefaultMediaTheme::DefaultMediaTheme()
	: BMediaTheme("BeOS Theme", "BeOS built-in theme version 0.1")
{
	CALLED();
}


BControl *
DefaultMediaTheme::MakeControlFor(BParameter *parameter)
{
	CALLED();

	BRect rect(0, 0, 150, 100);
	return MakeViewFor(parameter, &rect);
}


BView *
DefaultMediaTheme::MakeViewFor(BParameterWeb *web, const BRect *hintRect)
{
	CALLED();

	if (web == NULL)
		return NULL;

	BRect rect;
	if (hintRect)
		rect = *hintRect;
	else
		rect.Set(0, 0, 150, 100);

	BView *view = new BView(rect, "web", B_FOLLOW_ALL, B_WILL_DRAW);
	view->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));

	rect.OffsetTo(B_ORIGIN);

	for (int32 i = 0; i < web->CountGroups(); i++) {
		BParameterGroup *group = web->GroupAt(i);
		if (group == NULL)
			continue;

		BView *groupView = MakeViewFor(group, &rect);
		if (groupView == NULL)
			continue;

		view->AddChild(groupView);

		rect.OffsetBy(0, groupView->Bounds().Height() + 5);

		if (groupView->Bounds().Width() + 5 > rect.Width())
			rect.right = groupView->Bounds().Width() - 1;
	}

	view->ResizeTo(rect.right + 10, rect.top + 5);

	return view;
}


BView *
DefaultMediaTheme::MakeViewFor(BParameterGroup *group, const BRect *hintRect)
{
	CALLED();

	BRect rect(*hintRect);
	rect.InsetBySelf(5, 5);

	BBox *view = new BBox(rect, group->Name());
	view->SetLabel(group->Name());

	// Add the sub-group views

	rect.OffsetTo(B_ORIGIN);
	rect.InsetBySelf(5, 5);
	rect.top += 10;	// ToDo: font height
	rect.right = rect.left + 100;

	for (int32 i = 0; i < group->CountGroups(); i++) {
		BParameterGroup *subGroup = group->GroupAt(i);
		if (subGroup == NULL)
			continue;

		BView *groupView = MakeViewFor(subGroup, &rect);
		if (groupView == NULL)
			continue;

		view->AddChild(groupView);

		rect.OffsetBy(groupView->Bounds().Width() + 5, 0);

		if (groupView->Bounds().Height() > rect.Height())
			rect.bottom = groupView->Bounds().Height() - 1;
	}

	view->ResizeTo(rect.left + 10, rect.bottom + 25);
	rect.left = 5;

	// add the parameter views part of the group

	if (view->CountChildren() > 0)
		rect.OffsetBy(0, rect.Height() + 5);

	for (int32 i = 0; i < group->CountParameters(); i++) {
		BParameter *parameter = group->ParameterAt(i);
		if (parameter == NULL)
			continue;

		BControl *control = MakeViewFor(parameter, &rect);
		if (control == NULL)
			continue;

		view->AddChild(control);

		rect.OffsetBy(0, control->Bounds().Height() + 5);

		if (control->Bounds().Width() + 5 > rect.Width())
			rect.right = control->Bounds().Width() - 1;
	}

	if (group->CountParameters() > 0)
		view->ResizeTo(rect.right + 10, rect.top + 5);

	return view;
}


BControl *
DefaultMediaTheme::MakeViewFor(BParameter *parameter, const BRect *hintRect)
{
	BRect rect;
	if (hintRect)
		rect = *hintRect;
	else
		rect.Set(0, 0, 150, 100);

	switch (parameter->Type()) {
		case BParameter::B_NULL_PARAMETER:
			// there is no default view for a null parameter
			return NULL;

		case BParameter::B_DISCRETE_PARAMETER:
		{
			BDiscreteParameter &discrete = static_cast<BDiscreteParameter &>(*parameter);

			if (discrete.CountItems() > 0) {
				// create a pop up menu field

				BOptionPopUp *popUp = new BOptionPopUp(rect, discrete.Name(), discrete.Name(), NULL);

				for (int32 i = 0; i < discrete.CountItems(); i++) {
					popUp->AddOption(discrete.ItemNameAt(i), discrete.ItemValueAt(i));
				}

				popUp->ResizeToPreferred();

				return popUp;
			} else {
				// create a checkbox item

				BCheckBox *checkBox = new BCheckBox(rect, discrete.Name(), discrete.Name(), NULL);
				checkBox->ResizeToPreferred();

				return checkBox;
			}
		}

		case BParameter::B_CONTINUOUS_PARAMETER:
		{
			BSlider *slider = new BSlider(rect, parameter->Name(), parameter->Name(),
				NULL, 0, 100);
			slider->ResizeToPreferred();

			return slider;
		}

		default:
			ERROR("BMediaTheme: Don't know parameter type: 0x%lx\n", parameter->Type());
	}
	return NULL;
}

