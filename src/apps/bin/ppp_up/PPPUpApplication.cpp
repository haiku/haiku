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

#include <Application.h>
#include <Window.h>

#include "DialUpView.h"
#include "ConnectionWindow.h"


static const char *kSignature = "application/x-vnd.haiku.ppp_up";

extern "C" _EXPORT BView *instantiate_deskbar_item();


BView*
instantiate_deskbar_item()
{
	// TODO: implement me!
	return NULL;
}


static
status_t
report_thread(void *data)
{
	// TODO: add replicant when we finished connecting
	
	return B_OK;
}


class PPPUpApplication : public BApplication {
	public:
		PPPUpApplication(const char *name, ppp_interface_id id,
			thread_id replyThread);
		
		virtual void ReadyToRun();

	private:
		const char *fName;
		ppp_interface_id fID;
		thread_id fReplyThread;
		ConnectionWindow *fWindow;
};


int
main(const char *argv[], int argc)
{
	if(argc != 2)
		return -1;
	
	const char *name = argv[1];
	
	thread_id replyThread;
	ppp_interface_id id;
	receive_data(&replyThread, &id, sizeof(id));
	
	new PPPUpApplication(name, id, replyThread);
	
	be_app->Run();
	
	delete be_app;
	
	return 0;
}


PPPUpApplication::PPPUpApplication(const char *name, ppp_interface_id id,
		thread_id replyThread)
	: BApplication(kSignature),
	fName(name),
	fID(id),
	fReplyThread(replyThread),
	fWindow(NULL)
{
}


void
PPPUpApplication::ReadyToRun()
{
	// TODO: create report message thread (which in turn sends the reply)
	// TODO: create connection window
}
