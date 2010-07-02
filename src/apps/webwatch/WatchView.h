/*
 * Copyright (c) 1999-2003 Matthijs Hollemans
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a 
 * copy of this software and associated documentation files (the "Software"), 
 * to deal in the Software without restriction, including without limitation 
 * the rights to use, copy, modify, merge, publish, distribute, sublicense, 
 * and/or sell copies of the Software, and to permit persons to whom the 
 * Software is furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in 
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR 
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE 
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER 
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER 
 * DEALINGS IN THE SOFTWARE.
 */

#ifndef WATCHVIEW_H
#define WATCHVIEW_H

#include <Catalog.h>
#include <Locale.h>
#include <View.h>

class WatchView : public BView 
{
public:

	// This constructor is used by the WatchApp class to install the
	// WebWatch replicant into the Deskbar.
	WatchView();

	// This constructor is used to "rehydrate" the archived WatchView
	// object after the Deskbar (or any other BShelf) has received it.
	WatchView(BMessage* message);

	// Called by the Deskbar's shelf after it has received the message
	// that contains our replicant. (To enable the Deskbar to find this
	// function, we export it from the executable.) 
	static WatchView* Instantiate(BMessage* archive);

	// Archives the data that we need in order to instantiate ourselves.
	virtual status_t Archive(BMessage* archive, bool deep = true) const;

	virtual void Draw(BRect updateRect);
	virtual void MouseDown(BPoint point);
	virtual void MessageReceived(BMessage* message);

	// Periodically checks whether the Internet Time has changed, and tells
	// the view to redraw itself. The time between these checks depends on
	// the pulse rate of the Deskbar window. By default the pulse rate is 500
	// milliseconds, which is fine for our purposes. Note that other Deskbar
	// replicants could possibly change the pulse rate of the Deskbar window.
	virtual void Pulse();

private:

	typedef BView super;

	void OnAboutRequested();
	void OnQuitRequested();

	// Calculates the current Internet Time.
	int32 GetInternetTime();
	
	// The Internet Time from the last call to Pulse().
	int32 oldTime;
};

#endif // WATCHVIEW_H
