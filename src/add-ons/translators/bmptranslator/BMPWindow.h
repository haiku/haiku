// BMPWindow.h

#ifndef BMPWINDOW_H
#define BMPWINDOW_H

#include <Application.h>
#include <Window.h>
#include <View.h>

class BMPWindow : public BWindow {
public:
	BMPWindow(BRect area);
	~BMPWindow();
};

#endif