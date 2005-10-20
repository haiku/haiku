/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#include <TranslatorAddOn.h>
#include <View.h>
#include <Window.h>
#include <Application.h>
#include <Alert.h>
#include <Screen.h>

#include <stdio.h>


BPoint get_window_origin();
void set_window_origin(BPoint pt);

class PPMWindow :
	public BWindow
{
public:
	PPMWindow(
			BRect area) :
		BWindow(area, "PPM Settings", B_TITLED_WINDOW, B_NOT_RESIZABLE | B_NOT_ZOOMABLE)
		{
		}
	~PPMWindow()
		{
			BPoint pt(0,0);
			ConvertToScreen(&pt);
			set_window_origin(pt);
			be_app->PostMessage(B_QUIT_REQUESTED);
		}
};

int
main()
{
	BApplication app("application/x-vnd.hplus-ppm-translator");
	BView * v = NULL;
	BRect r(0,0,225,175);
	if (MakeConfig(NULL, &v, &r)) {
		BAlert * err = new BAlert("Error", "Something is wrong with the PPMTranslator!", "OK");
		err->Go();
		return 1;
	}
	PPMWindow *w = new PPMWindow(r);
	v->ResizeTo(r.Width(), r.Height());
	w->AddChild(v);
	BPoint o = get_window_origin();
	{
		BScreen scrn;
		BRect f = scrn.Frame();
		f.InsetBy(10,23);
		/* if not in a good place, start where the cursor is */
		if (!f.Contains(o)) {
			uint32 i;
			v->GetMouse(&o, &i, false);
			o.x -= r.Width()/2;
			o.y -= r.Height()/2;
			/* clamp location to screen */
			if (o.x < f.left) o.x = f.left;
			if (o.y < f.top) o.y = f.top;
			if (o.x > f.right) o.x = f.right;
			if (o.y > f.bottom) o.y = f.bottom;
		}
	}
	w->MoveTo(o);
	w->Show();
	app.Run();
	return 0;
}

