/* 
** Copyright 2008, Stefano Ceccherini (stefano.ceccherini@gmail.com).
** All rights reserved.
** Distributed under the terms of the MIT License.
*/

#include <Application.h>
#include <Font.h>

#include <stdio.h>
#include <stdlib.h>

#include <WidthBuffer.h>


class App : public BApplication {
public:
	App();
	~App();
	virtual void ReadyToRun();
	
private:
	_BWidthBuffer_ *fWidthBuffer;
	thread_id fThread;
	
	int32 TesterFunc();
	static int32 _thread(void *data);
	
};


int main()
{
	App app;
	app.Run();
	return 0;
}


App::App()
	:BApplication("application/x-vnd-WidthBufferTest")
{
	fWidthBuffer = new _BWidthBuffer_;
	fThread = spawn_thread(App::_thread, "widthbuffer tester", 
				B_NORMAL_PRIORITY, this);
}

 
void
App::ReadyToRun()
{
	resume_thread(fThread);
}


App::~App()
{
	status_t status;
	wait_for_thread(fThread, &status);
	delete fWidthBuffer;
}


int32
App::TesterFunc()
{
	FILE *file = fopen("/boot/beos/etc/termcap", "r");
	if (file != NULL) {
		char buffer[512];
		while (fgets(buffer, 512, file)) {
			float width = fWidthBuffer->StringWidth(buffer, 0,
							strlen(buffer), be_plain_font);
			printf("string: %s, width: %f\n", buffer, width);
		}
		fclose(file);
	}

	PostMessage(B_QUIT_REQUESTED);
	
	return 0;
}


int32
App::_thread(void *data)
{
	return static_cast<App *>(data)->TesterFunc();
}

