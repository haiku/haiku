#include <Window.h>
#include <View.h>
#include <Box.h>
#include <Button.h>
#include <StringView.h>
#include "ScrollBarWindow.h"
#include "ScrollBarApp.h"
#include "ScrollBarView.h"

ScrollBarWindow::ScrollBarWindow(void) 
	: BWindow( BRect(50,50,398,325), "Scroll Bar", B_TITLED_WINDOW, B_NOT_RESIZABLE | B_NOT_ZOOMABLE )
{
	sbview=new ScrollBarView;
	AddChild(sbview);
}

ScrollBarWindow::~ScrollBarWindow(void)
{
}

bool
ScrollBarWindow::QuitRequested(void)
{
	scroll_bar_app->PostMessage(B_QUIT_REQUESTED);
	return true;
}
