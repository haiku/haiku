/*
 * Copyright (c) 2003-4 Kian Duffy <myob@users.sourceforge.net> 
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files or portions
 * thereof (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so, subject
 * to the following conditions:
 *
 *  * Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 *  * Redistributions in binary form must reproduce the above copyright notice
 *    in the  binary, as well as this list of conditions and the following
 *    disclaimer in the documentation and/or other materials provided with
 *    the distribution.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */
#include <Application.h>
#include <StringView.h>
#include "AboutWindow.h"


// Set up the window


aboutWindow::aboutWindow(const char *)	
	: BWindow(BRect(100,100,390,230), "About", B_TITLED_WINDOW, B_NOT_RESIZABLE)
{
	
// Set up the main view	
	BView* icon = new BView(BRect(0,0,390,230), "File Info", B_FOLLOW_ALL, B_WILL_DRAW);
	

// Set up the colour of the panel and set it to being displayed
	AddChild(icon);
	
	Show();


text1 = new BStringView(BRect(15,10,390,30), "text", "Haiku Terminal", B_FOLLOW_NONE, B_WILL_DRAW);
AddChild(text1);

text2 = new BStringView(BRect(15,30,390,50), "text", "Based on MuTerminal 2.3", B_FOLLOW_NONE, B_WILL_DRAW);
AddChild(text2);

text3 = new BStringView(BRect(15,50,390,70), "text", "By Kazuho Okui and Takashi Murai", B_FOLLOW_NONE, B_WILL_DRAW);
AddChild(text3);

text4 = new BStringView(BRect(15,70,390,90), "text", "Haiku Modifications by Kian Duffy, Daniel Furrer", B_FOLLOW_NONE, B_WILL_DRAW);
AddChild(text4);




// Set up the BMessage handlers
}

void 
aboutWindow::MessageReceived(BMessage* msg)
{
	
	switch(msg->what)
	{
	
	
	
	 default:
	 	BWindow::MessageReceived(msg);
	 	break;
	}
}
// Quit Button implementation	


void aboutWindow::Quit() 
{
	
	BWindow::Quit();
}



