#include "OBApplication.h"
#include "OBWindow.h"
#include "OBView.h"

class OBTestApp : public OBApplication
{
public:
	OBTestApp(void);
	~OBTestApp(void);
};

OBWindow *win;

OBTestApp::OBTestApp(void) : OBApplication("application/OBTestApp")
{
	win=new OBWindow(BRect(100,100,200,200), "OBWindow1",B_BORDERED_WINDOW_LOOK,
		B_NORMAL_WINDOW_FEEL,0);
	win->Show();
}

OBTestApp::~OBTestApp(void)
{
}

int main(void)
{
	OBTestApp *app=new OBTestApp();
	app->Run();
	delete app;
}
