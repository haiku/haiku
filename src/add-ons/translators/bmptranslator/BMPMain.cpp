// BMPMain.cpp

#include <Application.h>
#include <Screen.h>
#include <Alert.h>
#include "BMPTranslator.h"
#include "BMPWindow.h"
#include "BMPView.h"

int main()
{
	BApplication app("application/x-vnd.obos-bmp-translator");
	BMPTranslator *ptranslator = new BMPTranslator;
	BView *v = NULL;
	BRect r(0,0,200,100);
	if (ptranslator->MakeConfigurationView(NULL, &v, &r)) {
		BAlert *err = new BAlert("Error", "Something is wrong with the BMPTranslator!", "OK");
		err->Go();
		return 1;
	}
	ptranslator->Release();
		// release the translator even though I never really used it anyway
	BMPWindow *w = new BMPWindow(r);
	v->ResizeTo(r.Width(), r.Height());
	w->AddChild(v);
	BPoint o = B_ORIGIN;
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