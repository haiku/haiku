/*
 * Copyright 2016, 2026, Haiku Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Kacper Kasper, kacperkasper@gmail.com
 */


#include <Application.h>
#include <GradientRadial.h>
#include <GradientLinear.h>
#include <Shape.h>
#include <Polygon.h>
#include <String.h>
#include <View.h>
#include <Window.h>

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>


class View : public BView {
	public:
		View(BRect rect);
		virtual ~View();

		virtual void Draw(BRect);
		BGradientLinear g[3];
		BPoint bezierPoints[3][4];
		BShape shapes[3];
		BPolygon polygon;
};

View::View(BRect rect)
	: BView(rect, "gradients", B_FOLLOW_ALL, B_WILL_DRAW)
{
	SetViewColor(0, 255, 0);
	for(int i = 0; i < 3; i++) {
		g[i].SetStart(BPoint(0, i * 100));
		g[i].SetEnd(BPoint(0, (i + 1) * 100));
		g[i].AddColor((rgb_color) { 255, 0, 0, 255 }, 255.0);
		g[i].AddColor((rgb_color) { 255, 255, 255, 0 }, 0.0);
		bezierPoints[i][0] = BPoint(540, i * 100 + 10);
		bezierPoints[i][1] = BPoint(610, i * 100 + 10);
		bezierPoints[i][2] = BPoint(580, (i + 1) * 100);
		bezierPoints[i][3] = BPoint(630, (i + 1) * 100);
		shapes[i].MoveTo(BPoint(640, i * 100 + 10));
		shapes[i].LineTo(BPoint(670, i * 100 + 40));
		shapes[i].LineTo(BPoint(700, i * 100 + 40));
		shapes[i].LineTo(BPoint(640, i * 100 + 100));
		shapes[i].Close();
	}
	BPoint p[4] = { BPoint(0, 0), BPoint(0, 50), BPoint(90, 90), BPoint(50, 50) };
	polygon.AddPoints(p, 4);
}


View::~View()
{
}


void
View::Draw(BRect)
{
	SetHighColor(0, 0, 0);
	BPoint text(5, 60);
	BRect rect(10, 10, 100, 100);
	BRect gradient(110, 10, 200, 100);
	rect.OffsetBy(30, 0);
	gradient.OffsetBy(30, 0);
	int opacity[3] = {255, 254, 100};
	for(int i = 0; i < 3; i++) {
		BString s;
		s << opacity[i];
		DrawString(s, 3, text);
		BeginLayer(opacity[i]);
		FillRect(rect);
		FillRect(gradient, g[i]);
		FillRoundRect(gradient.OffsetByCopy(100, 0), 10, 10, g[i]);
		FillEllipse(gradient.OffsetByCopy(200, 0), g[i]);
		FillArc(gradient.OffsetByCopy(300, 0), 30, 120, g[i]);
		FillBezier(bezierPoints[i], g[i]);
		BShape sh = shapes[i];
		MovePenTo(B_ORIGIN);
		FillShape(&sh, g[i]);
		BPolygon p = polygon;
		p.MapTo(BRect(0, 0, 90, 90), gradient.OffsetByCopy(600, 0));
		FillPolygon(&p, g[i]);
		StrokeLine(BPoint(840, i * 100 + 10), BPoint(930, i * 100 + 100), g[i]);
		EndLayer();
		rect.OffsetBy(0, 100);
		gradient.OffsetBy(0, 100);
		text.y += 100;
	}
}


//	#pragma mark -


class Window : public BWindow {
	public:
		Window();
		virtual ~Window();

		virtual bool QuitRequested();
};


Window::Window()
	: BWindow(BRect(100, 100, 1040, 410), "Layers-Test",
			B_TITLED_WINDOW, B_ASYNCHRONOUS_CONTROLS)
{
	BView *view = new View(Bounds());
	AddChild(view);
}


Window::~Window()
{
}


bool
Window::QuitRequested()
{
	be_app->PostMessage(B_QUIT_REQUESTED);
	return true;
}


//	#pragma mark -


class Application : public BApplication {
	public:
		Application();

		virtual void ReadyToRun();
};


Application::Application()
	: BApplication("application/x-vnd.haiku-layers_test")
{
}


void
Application::ReadyToRun()
{
	Window *window = new Window();
	window->Show();
}


//	#pragma mark -


int
main(int argc, char **argv)
{
	Application app;

	app.Run();
	return 0;
}
