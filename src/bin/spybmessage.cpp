#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <OS.h>
#include <image.h>
#include <Application.h>
#include <StorageDefs.h>
#include <MessageFilter.h>
#include <MessageRunner.h>
#include <List.h>
#include <Window.h>

extern char **original_argv;
extern char **argv_save;
extern char **__libc_argv;
extern int original_argc;
extern int __libc_argc;

#define MAX_VIEWS 10

int gStartWin = 0;
char *gMatchViewNames[MAX_VIEWS];
int gNumViewNames = 0;

char gExePath[B_PATH_NAME_LENGTH];
image_id gExeImg = -1;
int (*gExeMainProc)(int argc, char **argv);

thread_id gWaitForBAppThID = -1;
sem_id gStdoutLock;

//extern "C" EnqueueMessage__Q28BPrivate13BLooperTargetP8BMessage(BLooper *_this, BMessage *message);

//EnqueueMessage__Q28BPrivate13BLooperTargetP8BMessage(BLooper *_this, BMessage *message);

BMessageFilter *gAppFilter;
bool quitting = false;

filter_result bapp_filter(BMessage *message, 
			  BHandler **target, 
			  BMessageFilter *filter)
{
  if (message->what == 'plop') /* our doesn't count */
    return B_DISPATCH_MESSAGE;
  acquire_sem(gStdoutLock);
  fprintf(stdout, "\033[31mMessage for BApplication:\033[0m\n");
  message->PrintToStream();
  release_sem(gStdoutLock);
  return B_DISPATCH_MESSAGE;
}

filter_result bwin_filter(BMessage *message, 
			  BHandler **target, 
			  BMessageFilter *filter)
{
  BWindow *win;
  BHandler *hand = NULL;
  if (target)
    hand = *target;
  win = dynamic_cast<BWindow *>(filter->Looper());
  acquire_sem(gStdoutLock);
  fprintf(stdout, 
	  "\033[31mMessage for View \"%s\" of Window \"%s\":\033[0m\n", 
	  hand?hand->Name():NULL,
	  win?win->Title():NULL);
  message->PrintToStream();
  release_sem(gStdoutLock);
  return B_DISPATCH_MESSAGE;
}

class MyHandler : public BHandler {
public:
  MyHandler();
  ~MyHandler();
  void MessageReceived(BMessage *msg);
private:
  BList fWindowList;
};

MyHandler::MyHandler()
  :BHandler("spying handler")
{
  fWindowList.MakeEmpty();
}

MyHandler::~MyHandler()
{
}

void MyHandler::MessageReceived(BMessage *msg)
{
  int i;
  BMessageFilter *afilter;
  switch (msg->what) {
  case 'plop':
    i = be_app->CountWindows();
    for (; i; i--) {
      BWindow *win = be_app->WindowAt(i-1);
      if (win && !fWindowList.HasItem(win)) {
	fWindowList.AddItem(win);
	afilter = new BMessageFilter(B_ANY_DELIVERY, 
				     B_ANY_SOURCE, 
				     bwin_filter);
	win->Lock();
	win->AddCommonFilter(afilter);
	win->Unlock();
      }
    }
    break;
  }
  BHandler::MessageReceived(msg);
}

int32 wait_for_loopers(void *arg)
{
  MyHandler *myh;
  /* wait for BApplication */
  while (!be_app)
    snooze(50000);
  gAppFilter = new BMessageFilter(B_ANY_DELIVERY, B_ANY_SOURCE, bapp_filter);
  myh = new MyHandler;
  be_app->Lock();
  be_app->AddCommonFilter(gAppFilter);
  be_app->AddHandler(myh);
  be_app->Unlock();
  new BMessageRunner(BMessenger(myh), new BMessage('plop'), 100000);
  
  return 0;
}

static int usage(char *argv0)
{
  printf("usage:\n");
  printf("%s app [args...]\n", argv0);
  return 0;
}

int main(int argc, char **argv)
{
  int i;
  status_t err;
  char *trapp_name;
  if (argc < 2)
    return usage(argv[0]);
  trapp_name = argv[0];
  
  for (i = 1; i < argc; i++) {
    if (strncmp(argv[i], "-", 1))
      break;
    else if (!strcmp(argv[i], "-view")) {
      if (gNumViewNames >= MAX_VIEWS) {
	printf("too many view names\n");
	return 1;
      }
      i++;
      if (i >= argc) {
	printf("missing arg to -view\n");
	return 1;
      }
      gMatchViewNames[gNumViewNames] = argv[i];
      gNumViewNames++;
    } else if (!strcmp(argv[i], "-firstw")) {
      i++;
      if (i >= argc) {
	printf("missing arg to -firstw\n");
	return 1;
      }
      gStartWin = atoi(argv[i]);
    } else {
      return usage(argv[0]);
    }
  }
  if (argc - i < 1)
    return usage(argv[0]);
  argv += i;
  argc -= i;
	
  for (i = 0; i < argc; i++)
    printf("argv[%d] = %s\n", i, argv[i]);
  gExePath[0] = '\0';
  if (strncmp(argv[0], "/", 1)) {
    getcwd(gExePath, B_PATH_NAME_LENGTH-10);
    strcat(gExePath, "/");
  }
  strncat(gExePath, argv[0], B_PATH_NAME_LENGTH-1-strlen(gExePath));
  printf("cmd = %s\n", gExePath);
  gExeImg = load_add_on(gExePath);
  if (gExeImg < B_OK) {
    fprintf(stderr, "load_add_on: %s\n", strerror(gExeImg));
    return 1;
  }
	
  // original are static...
  //printf("original: %d; %s\n", original_argc, *original_argv);
  fprintf(stderr, "libc: %d; %s\n", __libc_argc, *__libc_argv);
  fprintf(stderr, "save: %s\n", *argv_save);

  //argv[0] = trapp_name;
  __libc_argv = argv;
  __libc_argc = argc;
  argv_save = argv;
	
  gStdoutLock = create_sem(1, "spybmsg_stdout_lock");

  err = get_image_symbol(gExeImg, "main", B_SYMBOL_TYPE_TEXT, (void **)&gExeMainProc);
  if (err < B_OK) {
    fprintf(stderr, "get_image_symbol(main): %s\n", strerror(gExeImg));
    return 1;
  }
  printf("main @ %p\n", gExeMainProc);
  
  resume_thread(spawn_thread(wait_for_loopers, 
			     "waiting for BLoopers", 
			     B_NORMAL_PRIORITY, NULL));
  
  i = gExeMainProc(argc, argv);
  
  return i;
}

