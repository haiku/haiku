/* -----------------------------------------------------------------------
 * Copyright (c) 2004 Waldemar Kornewald, Waldemar.Kornewald@web.de
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
 * ----------------------------------------------------------------------- */

#ifndef CONNECTION_VIEW__H
#define CONNECTION_VIEW__H

#include "DialUpView.h"


#define MSG_UPDATE		'UPDT'
	// sent when status changed
#define MSG_UP_FAILED	'UPFL'
	// sent when up failed


class ConnectionView : public BView {
		friend class ConnectionWindow;

	public:
		ConnectionView(BRect rect, ppp_interface_id id, thread_id thread,
			DialUpView *view);
		
		virtual void AttachedToWindow();
		virtual void MessageReceived(BMessage *message);

	private:
		void Update();
		void UpFailed();
		void Connect();
		void Cancel();
		void CleanUp();
		
		BString AttemptString() const;

	private:
		ppp_interface_id fID;
		thread_id fRequestThread;
		BView *fAuthenticationView, *fStatusView;
		DialUpView *fDUNView;
		BStringView *fAttemptView;
		
		BButton *fConnectButton, *fCancelButton;
		bool fConnecting;
};


#endif
