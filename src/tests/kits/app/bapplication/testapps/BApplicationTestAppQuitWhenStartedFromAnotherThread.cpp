#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <Application.h>

static thread_id gBAppThreadID;
static thread_id gMainThreadID;
static int bapp_ref_count = 0;

static int32
bapp_quit_thread(void *arg)
{
	printf("%s: calling Lock()\n", __FUNCTION__);
	be_app->Lock();
	printf("%s: calling Quit()\n", __FUNCTION__);
	be_app->Quit();
	return 0;
}


static void
sighandler(int sig)
{
	resume_thread(spawn_thread(bapp_quit_thread, "BApplication->Quit()", B_NORMAL_PRIORITY, NULL));
}


static int32
bapp_thread(void *arg)
{
	signal(SIGINT, sighandler); /* on ^C exit the bapp_thread too */
	printf("%s: calling Lock()\n", __FUNCTION__);
	be_app->Lock();
	printf("%s: calling Run()\n", __FUNCTION__);
	be_app->Run();
	printf("%s: exitting...\n", __FUNCTION__);
	return 0;
}


void
beos_launch_bapplication(void)
{
	bapp_ref_count++;
	if (be_app)
		return; /* done already! */
	new BApplication("application/x-vnd.mmu_man-threaded-BApp-test");
	gBAppThreadID = spawn_thread(bapp_thread, "BApplication(test)", B_NORMAL_PRIORITY, (void *)find_thread(NULL));
	if (gBAppThreadID < B_OK)
		return; /* #### handle errors */
	if (resume_thread(gBAppThreadID) < B_OK)
		return;
	// This way we ensure we don't create BWindows before be_app is created
	// (creating it in the thread doesn't ensure this)
	printf("%s: calling Unlock()\n", __FUNCTION__);
	be_app->Unlock();
}


void beos_uninit_bapplication(void)
{
	status_t err;
	if (--bapp_ref_count)
		return;
	if (!be_app->Lock())
		debugger("cannot lock BApp!");
	printf("%s: calling Quit()\n", __FUNCTION__);
	be_app->Quit();
	printf("%s: waiting for app thread\n", __FUNCTION__);
	wait_for_thread(gBAppThreadID, &err);
	delete be_app;
	be_app = NULL;
}


int main()
{
	printf("%s: calling beos_launch_bapplication()\n", __FUNCTION__);
	beos_launch_bapplication();
	sleep(10);
	printf("%s: calling beos_uninit_bapplication()\n", __FUNCTION__);
	beos_uninit_bapplication();
}
