/*
 *  Printers Preference Application.
 *  Copyright (C) 2001 OpenBeOS. All Rights Reserved.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "Printers.h"

#ifndef PRINTERSWINDOW_H
	#include "PrintersWindow.h"
#endif

int main()
{
	new PrintersApp;
		be_app->Run();
	delete be_app;
	
	return 0;
}

PrintersApp::PrintersApp()
	: Inherited(PRINTERS_SIGNATURE)
{
}

void PrintersApp::ReadyToRun()
{
	PrintersWindow* win = new PrintersWindow(BRect(78.0, 71.0, 561.0, 409.0));
	win->Show();
}
