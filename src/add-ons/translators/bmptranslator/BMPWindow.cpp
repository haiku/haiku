// BMPWindow.cpp

#include "BMPWindow.h"

BMPWindow::BMPWindow(BRect area) :
	BWindow(area, "BMPTranslator", B_TITLED_WINDOW, B_NOT_RESIZABLE | B_NOT_ZOOMABLE)
{
}

BMPWindow::~BMPWindow()
{
	be_app->PostMessage(B_QUIT_REQUESTED);
}