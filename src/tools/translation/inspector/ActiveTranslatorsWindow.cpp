/*****************************************************************************/
// ActiveTranslatorsWindow
// Written by Michael Wilber, OBOS Translation Kit Team
//
// ActiveTranslatorsWindow.cpp
//
// BWindow class for displaying information about the currently open
// document 
//
//
// Copyright (c) 2003 OpenBeOS Project
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense, 
// and/or sell copies of the Software, and to permit persons to whom the 
// Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included 
// in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
// OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL 
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.
/*****************************************************************************/

#include "Constants.h"
#include "ActiveTranslatorsWindow.h"
#include <Application.h>
#include <ScrollView.h>
#include <Message.h>
#include <String.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>

ActiveTranslatorsWindow::ActiveTranslatorsWindow(BRect rect, const char *name)
	: BWindow(rect, name, B_FLOATING_WINDOW, B_NOT_RESIZABLE | B_NOT_ZOOMABLE)
{
	BRect rctframe = Bounds();
	rctframe.InsetBy(5, 5);
	rctframe.right -= B_V_SCROLL_BAR_WIDTH;
	
	fplist = new BOutlineListView(rctframe, "translators_list",
		B_MULTIPLE_SELECTION_LIST);

	fplist->AddItem(fpuserItem = new BStringItem("User Translators"));
	fplist->AddItem(fpsystemItem = new BStringItem("System Translators"));
	AddTranslatorsToList("/boot/home/config/add-ons/Translators", fpuserItem);
	AddTranslatorsToList("/system/add-ons/Translators", fpsystemItem);
	
	AddChild(new BScrollView("scroll_list", fplist, B_FOLLOW_LEFT | B_FOLLOW_TOP,
		0, false, true));
	
	SetSizeLimits(100, 10000, 100, 10000);
	
	Show();
}

void
ActiveTranslatorsWindow::AddTranslatorsToList(const char *path,
	BStringItem *pparent)
{
	DIR *dir = opendir(path);
	if (!dir) {
		return;
	}
	struct dirent *dent;
	struct stat stbuf;
	char cwd[PATH_MAX] = "";
	while (NULL != (dent = readdir(dir))) {
		strcpy(cwd, path);
		strcat(cwd, "/");
		strcat(cwd, dent->d_name);
		status_t err = stat(cwd, &stbuf);

		if (!err && S_ISREG(stbuf.st_mode) &&
			strcmp(dent->d_name, ".") && strcmp(dent->d_name, ".."))
			fplist->AddUnder(new BStringItem(dent->d_name), pparent);
	}
	closedir(dir);
}

ActiveTranslatorsWindow::~ActiveTranslatorsWindow()
{
}

void
ActiveTranslatorsWindow::FrameResized(float width, float height)
{
}

void
ActiveTranslatorsWindow::MessageReceived(BMessage *pmsg)
{
	switch (pmsg->what) {
		default:
			BWindow::MessageReceived(pmsg);
			break;
	}
}

void
ActiveTranslatorsWindow::Quit()
{
	// tell the app to forget about this window
	be_app->PostMessage(M_ACTIVE_TRANSLATORS_WINDOW_QUIT);
	BWindow::Quit();
}
