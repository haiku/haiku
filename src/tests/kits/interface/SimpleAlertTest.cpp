/*
 * Copyright 2014, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include <Alert.h>
#include <Application.h>
#include <Button.h>
#include <LayoutBuilder.h>
#include <MenuField.h>
#include <Slider.h>
#include <String.h>
#include <StringView.h>
#include <Window.h>

#include <stdio.h>


const uint32 kMsgShowAlert = 'shAl';


//	#pragma mark -


class Window : public BWindow {
public:
								Window();

	virtual void				MessageReceived(BMessage* message);
	virtual	bool				QuitRequested();

private:
			button_width		_ButtonWidth();
			button_spacing		_ButtonSpacing();
			alert_type			_AlertType();

private:
			BMenuField*			fSizeField;
			BMenuField*			fTypeField;
			BMenuField*			fSpacingField;
			BSlider*			fCountSlider;
			BStringView*		fLastStringView;
};


Window::Window()
	:
	BWindow(BRect(100, 100, 620, 200), "Alert-Test", B_TITLED_WINDOW,
		B_ASYNCHRONOUS_CONTROLS | B_AUTO_UPDATE_SIZE_LIMITS)
{
	BMenu* sizeMenu = new BMenu("Button size");
	sizeMenu->SetLabelFromMarked(true);
	sizeMenu->SetRadioMode(true);
	sizeMenu->AddItem(new BMenuItem("As usual", NULL));
	sizeMenu->AddItem(new BMenuItem("From widest", NULL));
	sizeMenu->AddItem(new BMenuItem("From label", NULL));
	sizeMenu->ItemAt(0)->SetMarked(true);
	fSizeField = new BMenuField("Button size", sizeMenu);

	BMenu* typeMenu = new BMenu("Alert type");
	typeMenu->SetLabelFromMarked(true);
	typeMenu->SetRadioMode(true);
	typeMenu->AddItem(new BMenuItem("Empty", NULL));
	typeMenu->AddItem(new BMenuItem("Info", NULL));
	typeMenu->AddItem(new BMenuItem("Idea", NULL));
	typeMenu->AddItem(new BMenuItem("Warning", NULL));
	typeMenu->AddItem(new BMenuItem("Stop", NULL));
	typeMenu->ItemAt(0)->SetMarked(true);
	fTypeField = new BMenuField("Alert type", typeMenu);

	BMenu* spacingMenu = new BMenu("Button spacing");
	spacingMenu->SetLabelFromMarked(true);
	spacingMenu->SetRadioMode(true);
	spacingMenu->AddItem(new BMenuItem("Even", NULL));
	spacingMenu->AddItem(new BMenuItem("Offset", NULL));
	spacingMenu->ItemAt(0)->SetMarked(true);
	fSpacingField = new BMenuField("Button spacing", spacingMenu);

	fCountSlider = new BSlider("count", "Button count", NULL, 1, 3,
		B_HORIZONTAL);
	fCountSlider->SetValue(3);
	fCountSlider->SetLimitLabels("1", "3");
	fCountSlider->SetHashMarkCount(3);
	fCountSlider->SetHashMarks(B_HASH_MARKS_BOTTOM);

	fLastStringView = new BStringView("last pressed", "");
	fLastStringView->SetAlignment(B_ALIGN_CENTER);

	BButton* button = new BButton("Show alert", new BMessage(kMsgShowAlert));

	BLayoutBuilder::Group<>(this, B_VERTICAL)
		.Add(fSizeField)
		.Add(fTypeField)
		.Add(fSpacingField)
		.Add(fCountSlider)
		.AddGlue()
		.Add(fLastStringView)
		.Add(button)
		.SetInsets(B_USE_DEFAULT_SPACING);

	CenterOnScreen();
	SetFlags(Flags() | B_CLOSE_ON_ESCAPE);
}


void
Window::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case kMsgShowAlert:
		{
			int32 count = fCountSlider->Value();

			BAlert* alert = new BAlert("Test title", "Lorem ipsum dolor sit "
				"amet, consectetur adipiscing elit. Suspendisse vel iaculis "
				"quam. Donec faucibus erat nunc, ac ullamcorper justo sodales.",
				"short 1", count > 1 ? "a bit longer 2" : NULL,
				count > 2 ? "very very long button 3" : NULL,
				_ButtonWidth(), _ButtonSpacing(), _AlertType());
			alert->SetFlags(alert->Flags() | B_CLOSE_ON_ESCAPE);
			int result = alert->Go();
			if (result < 0) {
				fLastStringView->SetText("Canceled alert");
			} else {
				fLastStringView->SetText(BString().SetToFormat(
					"Pressed button %d", result + 1).String());
			}
			break;
		}
		default:
			BWindow::MessageReceived(message);
	}
}


bool
Window::QuitRequested()
{
	be_app->PostMessage(B_QUIT_REQUESTED);
	return true;
}


button_width
Window::_ButtonWidth()
{
	switch (fSizeField->Menu()->FindMarkedIndex()) {
		case 0:
		default:
			return B_WIDTH_AS_USUAL;
		case 1:
			return B_WIDTH_FROM_WIDEST;
		case 2:
			return B_WIDTH_FROM_LABEL;
	}
}


button_spacing
Window::_ButtonSpacing()
{
	return (button_spacing)fSpacingField->Menu()->FindMarkedIndex();
}


alert_type
Window::_AlertType()
{
	return (alert_type)fTypeField->Menu()->FindMarkedIndex();
}


//	#pragma mark -


class Application : public BApplication {
public:
								Application();

	virtual	void				ReadyToRun();
};


Application::Application()
	:
	BApplication("application/x-vnd.haiku-AlertTest")
{
}


void
Application::ReadyToRun(void)
{
	BWindow* window = new Window();
	window->Show();
}


//	#pragma mark -


int
main(int argc, char** argv)
{
	Application app;

	app.Run();
	return 0;
}
