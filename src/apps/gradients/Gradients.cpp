/*
 * Copyright (c) 2008, Haiku, Inc.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Artur Wyszynski <harakash@gmail.com>
 */

#include <Application.h>
#include <GradientLinear.h>
#include <GradientRadial.h>
#include <GradientRadialFocus.h>
#include <GradientDiamond.h>
#include <GradientConic.h>
#include <View.h>
#include <Screen.h>
#include <Window.h>
#include <MenuField.h>
#include <MenuItem.h>
#include <PopUpMenu.h>


#define MSG_LINEAR			'gtli'
#define MSG_RADIAL			'gtra'
#define MSG_RADIAL_FOCUS	'gtrf'
#define MSG_DIAMOND			'gtdi'
#define MSG_CONIC			'gtco'

class GradientsApp : public BApplication {
public:
							GradientsApp(void);
};


class GradientsView : public BView {
public:
							GradientsView(const BRect &r);
	virtual					~GradientsView(void);
	
	virtual void			Draw(BRect update);
			void			DrawLinear(BRect update);
			void			DrawRadial(BRect update);
			void			DrawRadialFocus(BRect update);
			void			DrawDiamond(BRect update);
			void			DrawConic(BRect update);
			void			SetType(gradient_type type);
	
private:
			gradient_type	fType;
};


class GradientsWindow : public BWindow {
public:
							GradientsWindow(void);

			bool			QuitRequested(void);
	virtual	void			MessageReceived(BMessage *msg);

private:
			BPopUpMenu*		fGradientsMenu;
			BMenuItem*		fLinearItem;
			BMenuItem*		fRadialItem;
			BMenuItem*		fRadialFocusItem;
			BMenuItem*		fDiamondItem;
			BMenuItem*		fConicItem;
			BMenuField*		fGradientsTypeField;
			GradientsView*	fGradientsView;
};


//	#pragma mark -


GradientsApp::GradientsApp(void)
	: BApplication("application/x-vnd.Haiku-Gradients")
{
	GradientsWindow *window = new GradientsWindow();
	window->Show();
}


//	#pragma mark -


GradientsWindow::GradientsWindow()
	: BWindow(BRect(0, 0, 230, 490), "Gradients Test", B_TITLED_WINDOW,
		B_NOT_RESIZABLE | B_NOT_ZOOMABLE)
{
	BRect field(10, 10, Bounds().Width() - 10, 30);
	fGradientsMenu = new BPopUpMenu("gradientsType");
	fLinearItem = new BMenuItem("Linear", new BMessage(MSG_LINEAR));
	fRadialItem = new BMenuItem("Radial", new BMessage(MSG_RADIAL));
	fRadialFocusItem = new BMenuItem("Radial Focus",
									 new BMessage(MSG_RADIAL_FOCUS));
	fDiamondItem = new BMenuItem("Diamond", new BMessage(MSG_DIAMOND));
	fConicItem = new BMenuItem("Conic", new BMessage(MSG_CONIC));
	fGradientsMenu->AddItem(fLinearItem);
	fGradientsMenu->AddItem(fRadialItem);
	fGradientsMenu->AddItem(fRadialFocusItem);
	fGradientsMenu->AddItem(fDiamondItem);
	fGradientsMenu->AddItem(fConicItem);
	fLinearItem->SetMarked(true);
	fGradientsTypeField = new BMenuField(field, "gradientsField",
										 "Gradient type:",
										 fGradientsMenu,
										 B_FOLLOW_LEFT | B_FOLLOW_BOTTOM,
										 B_WILL_DRAW | B_NAVIGABLE
										 | B_FRAME_EVENTS);
	fGradientsTypeField->SetViewColor(255, 255, 255);
	fGradientsTypeField->SetDivider(110);
	AddChild(fGradientsTypeField);
		
	BRect bounds = Bounds();
	bounds.top = 40;
	fGradientsView = new GradientsView(bounds);
	AddChild(fGradientsView);

	MoveTo((BScreen().Frame().Width() - Bounds().Width()) / 2,
		(BScreen().Frame().Height() - Bounds().Height()) / 2 );
}


bool
GradientsWindow::QuitRequested()
{
	be_app->PostMessage(B_QUIT_REQUESTED);
	return true;
}


void
GradientsWindow::MessageReceived(BMessage *msg)
{
	switch (msg->what) {
		case MSG_LINEAR:
			fGradientsView->SetType(B_GRADIENT_LINEAR);
			break;
		case MSG_RADIAL:
			fGradientsView->SetType(B_GRADIENT_RADIAL);
			break;
		case MSG_RADIAL_FOCUS:
			fGradientsView->SetType(B_GRADIENT_RADIAL_FOCUS);
			break;
		case MSG_DIAMOND:
			fGradientsView->SetType(B_GRADIENT_DIAMOND);
			break;
		case MSG_CONIC:
			fGradientsView->SetType(B_GRADIENT_CONIC);
			break;
		default:
			BWindow::MessageReceived(msg);
			break;
	}
}


// #pragma mark -


GradientsView::GradientsView(const BRect &rect)
	: BView(rect, "gradientsview", B_FOLLOW_ALL, B_WILL_DRAW | B_PULSE_NEEDED),
	fType(B_GRADIENT_LINEAR)
{
}


GradientsView::~GradientsView()
{
}


void
GradientsView::Draw(BRect update)
{
	switch (fType) {
		case B_GRADIENT_LINEAR: {
			DrawLinear(update);
			break;
		}
		case B_GRADIENT_RADIAL: {
			DrawRadial(update);
			break;
		}
		case B_GRADIENT_RADIAL_FOCUS: {
			DrawRadialFocus(update);
			break;
		}
		case B_GRADIENT_DIAMOND: {
			DrawDiamond(update);
			break;
		}
		case B_GRADIENT_CONIC: {
			DrawConic(update);
			break;
		}
		case B_GRADIENT_NONE:
		default: {
			break;
		}
	}
}

void
GradientsView::DrawLinear(BRect update)
{
	BGradientLinear gradient;
	rgb_color c;
	c.red = 255;
	c.green = 0;
	c.blue = 0;
	gradient.AddColor(c, 0);
	c.red = 0;
	c.green = 255;
	c.blue = 0;
	gradient.AddColor(c, 127);
	c.red = 0;
	c.green = 0;
	c.blue = 255;
	gradient.AddColor(c, 255);
	
	// RoundRect
	SetHighColor(0, 0, 0);
	FillRoundRect(BRect(10, 10, 110, 110), 5, 5);
	gradient.SetStart(BPoint(120, 10));
	gradient.SetEnd(BPoint(220, 110));
	FillRoundRect(BRect(120, 10, 220, 110), 5, 5, gradient);

	// Rect
	SetHighColor(0, 0, 0);
	FillRect(BRect(10, 120, 110, 220));
	gradient.SetStart(BPoint(120, 120));
	gradient.SetEnd(BPoint(220, 220));
	FillRect(BRect(120, 120, 220, 220), gradient);
	
	// Triangle
	SetHighColor(0, 0, 0);
	FillTriangle(BPoint(60, 230), BPoint(10, 330), BPoint(110, 330));
	gradient.SetStart(BPoint(60, 230));
	gradient.SetEnd(BPoint(60, 330));
	FillTriangle(BPoint(170, 230), BPoint(120, 330), BPoint(220, 330), gradient);

	// Ellipse
	SetHighColor(0, 0, 0);
	FillEllipse(BPoint(60, 390), 50, 50);
	gradient.SetStart(BPoint(60, 340));
	gradient.SetEnd(BPoint(60, 440));
	FillEllipse(BPoint(170, 390), 50, 50, gradient);
}


void
GradientsView::DrawRadial(BRect update)
{
	BGradientRadial gradient;
	rgb_color c;
	c.red = 255;
	c.green = 0;
	c.blue = 0;
	gradient.AddColor(c, 0);
	c.red = 0;
	c.green = 255;
	c.blue = 0;
	gradient.AddColor(c, 127);
	c.red = 0;
	c.green = 0;
	c.blue = 255;
	gradient.AddColor(c, 255);
	
	// RoundRect
	SetHighColor(0, 0, 0);
	FillRoundRect(BRect(10, 10, 110, 110), 5, 5);
	gradient.SetCenter(BPoint(170, 60));
	FillRoundRect(BRect(120, 10, 220, 110), 5, 5, gradient);
	
	// Rect
	SetHighColor(0, 0, 0);
	FillRect(BRect(10, 120, 110, 220));
	gradient.SetCenter(BPoint(170, 170));
	FillRect(BRect(120, 120, 220, 220), gradient);
	
	// Triangle
	SetHighColor(0, 0, 0);
	FillTriangle(BPoint(60, 230), BPoint(10, 330), BPoint(110, 330));
	gradient.SetCenter(BPoint(170, 280));
	FillTriangle(BPoint(170, 230), BPoint(120, 330), BPoint(220, 330), gradient);
	
	// Ellipse
	SetHighColor(0, 0, 0);
	FillEllipse(BPoint(60, 390), 50, 50);
	gradient.SetCenter(BPoint(170, 390));
	FillEllipse(BPoint(170, 390), 50, 50, gradient);
}


void
GradientsView::DrawRadialFocus(BRect update)
{
	BGradientRadialFocus gradient;
	rgb_color c;
	c.red = 255;
	c.green = 0;
	c.blue = 0;
	gradient.AddColor(c, 0);
	c.red = 0;
	c.green = 255;
	c.blue = 0;
	gradient.AddColor(c, 127);
	c.red = 0;
	c.green = 0;
	c.blue = 255;
	gradient.AddColor(c, 255);
	
	// RoundRect
	SetHighColor(0, 0, 0);
	FillRoundRect(BRect(10, 10, 110, 110), 5, 5);
	gradient.SetCenter(BPoint(170, 60));
	FillRoundRect(BRect(120, 10, 220, 110), 5, 5, gradient);
	
	// Rect
	SetHighColor(0, 0, 0);
	FillRect(BRect(10, 120, 110, 220));
	gradient.SetCenter(BPoint(170, 170));
	FillRect(BRect(120, 120, 220, 220), gradient);
	
	// Triangle
	SetHighColor(0, 0, 0);
	FillTriangle(BPoint(60, 230), BPoint(10, 330), BPoint(110, 330));
	gradient.SetCenter(BPoint(170, 280));
	FillTriangle(BPoint(170, 230), BPoint(120, 330), BPoint(220, 330), gradient);
	
	// Ellipse
	SetHighColor(0, 0, 0);
	FillEllipse(BPoint(60, 390), 50, 50);
	gradient.SetCenter(BPoint(170, 390));
	FillEllipse(BPoint(170, 390), 50, 50, gradient);
}


void
GradientsView::DrawDiamond(BRect update)
{
	BGradientDiamond gradient;
	rgb_color c;
	c.red = 255;
	c.green = 0;
	c.blue = 0;
	gradient.AddColor(c, 0);
	c.red = 0;
	c.green = 255;
	c.blue = 0;
	gradient.AddColor(c, 127);
	c.red = 0;
	c.green = 0;
	c.blue = 255;
	gradient.AddColor(c, 255);
	
	// RoundRect
	SetHighColor(0, 0, 0);
	FillRoundRect(BRect(10, 10, 110, 110), 5, 5);
	gradient.SetCenter(BPoint(170, 60));
	FillRoundRect(BRect(120, 10, 220, 110), 5, 5, gradient);
	
	// Rect
	SetHighColor(0, 0, 0);
	FillRect(BRect(10, 120, 110, 220));
	gradient.SetCenter(BPoint(170, 170));
	FillRect(BRect(120, 120, 220, 220), gradient);
	
	// Triangle
	SetHighColor(0, 0, 0);
	FillTriangle(BPoint(60, 230), BPoint(10, 330), BPoint(110, 330));
	gradient.SetCenter(BPoint(170, 280));
	FillTriangle(BPoint(170, 230), BPoint(120, 330), BPoint(220, 330), gradient);
	
	// Ellipse
	SetHighColor(0, 0, 0);
	FillEllipse(BPoint(60, 390), 50, 50);
	gradient.SetCenter(BPoint(170, 390));
	FillEllipse(BPoint(170, 390), 50, 50, gradient);
}


void
GradientsView::DrawConic(BRect update)
{
	BGradientConic gradient;
	rgb_color c;
	c.red = 255;
	c.green = 0;
	c.blue = 0;
	gradient.AddColor(c, 0);
	c.red = 0;
	c.green = 255;
	c.blue = 0;
	gradient.AddColor(c, 127);
	c.red = 0;
	c.green = 0;
	c.blue = 255;
	gradient.AddColor(c, 255);
	
	// RoundRect
	SetHighColor(0, 0, 0);
	FillRoundRect(BRect(10, 10, 110, 110), 5, 5);
	gradient.SetCenter(BPoint(170, 60));
	FillRoundRect(BRect(120, 10, 220, 110), 5, 5, gradient);
	
	// Rect
	SetHighColor(0, 0, 0);
	FillRect(BRect(10, 120, 110, 220));
	gradient.SetCenter(BPoint(170, 170));
	FillRect(BRect(120, 120, 220, 220), gradient);
	
	// Triangle
	SetHighColor(0, 0, 0);
	FillTriangle(BPoint(60, 230), BPoint(10, 330), BPoint(110, 330));
	gradient.SetCenter(BPoint(170, 280));
	FillTriangle(BPoint(170, 230), BPoint(120, 330), BPoint(220, 330), gradient);
	
	// Ellipse
	SetHighColor(0, 0, 0);
	FillEllipse(BPoint(60, 390), 50, 50);
	gradient.SetCenter(BPoint(170, 390));
	FillEllipse(BPoint(170, 390), 50, 50, gradient);
}


void
GradientsView::SetType(gradient_type type)
{
	fType = type;
	Invalidate();
}


// #pragma mark -


int
main()
{
	GradientsApp app;
	app.Run();
	return 0;
}
