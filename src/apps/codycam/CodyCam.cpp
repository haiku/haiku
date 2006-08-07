#include "CodyCam.h"

#include <Alert.h>
#include <stdio.h>
#include <string.h>
#include <Button.h>
#include <TabView.h>
#include <Menu.h>
#include <MenuItem.h>
#include <MenuBar.h>
#include <PopUpMenu.h>
#include <MediaDefs.h>
#include <MediaNode.h>
#include <scheduler.h>
#include <MediaTheme.h>
#include <TimeSource.h>
#include <MediaRoster.h>
#include <TextControl.h>
#include <TranslationKit.h>
#include <unistd.h>


#define VIDEO_SIZE_X 320
#define VIDEO_SIZE_Y 240

#define WINDOW_SIZE_X (VIDEO_SIZE_X + 80)
#define WINDOW_SIZE_Y (VIDEO_SIZE_Y + 230)

#define WINDOW_OFFSET_X 28
#define WINDOW_OFFSET_Y 28

const int32 kBtnHeight = 20;
const int32 kBtnWidth = 60;
const int32 kBtnBuffer = 25;
const int32 kXBuffer = 10;
const int32 kYBuffer = 10;
const int32 kMenuHeight = 15;
const int32 kButtonHeight = 15;
const int32 kSliderViewRectHeight = 40;

const rgb_color kViewGray = { 216, 216, 216, 255};

static void ErrorAlert(const char * message, status_t err);
static status_t AddTranslationItems( BMenu * intoMenu, uint32 from_type);

#define	CALL		printf
#define ERROR		printf
#define FTPINFO		printf
#define	INFO		printf

//---------------------------------------------------------------
// The Application
//---------------------------------------------------------------

int main() {
	chdir("/boot/home");
	CodyCam app;
	app.Run();
	return 0;	
}

//---------------------------------------------------------------

CodyCam::CodyCam() :
	BApplication("application/x-vnd.Be.CodyCam"),
	fMediaRoster(NULL),
	fVideoConsumer(NULL),
	fWindow(NULL),
	fPort(0),
	mVideoControlWindow(NULL)
{
}

//---------------------------------------------------------------

CodyCam::~CodyCam()
{
	CALL("CodyCam::~CodyCam\n");

	// release the video consumer node
	// the consumer node cleans up the window
	if (fVideoConsumer) {
		fVideoConsumer->Release();
		fVideoConsumer = NULL;
	}
		
	CALL("CodyCam::~CodyCam - EXIT\n");
}

//---------------------------------------------------------------

void 
CodyCam::ReadyToRun()
{
	/* create the window for the app */
	fWindow = new VideoWindow(BRect(28, 28, 28 + (WINDOW_SIZE_X-1), 28 + (WINDOW_SIZE_Y-1)),
								(const char *)"CodyCam", B_TITLED_WINDOW, B_NOT_RESIZABLE | B_NOT_ZOOMABLE, &fPort);
								
	/* set up the node connections */
	status_t status = SetUpNodes();
	if (status != B_OK)
	{
		// This error is not needed because SetUpNodes handles displaying any
		// errors it runs into.
//		ErrorAlert("Error setting up nodes", status);
		return;
	}
	
	((VideoWindow *)fWindow)->ApplyControls();
	
}

//---------------------------------------------------------------

bool 
CodyCam::QuitRequested()
{
	TearDownNodes();
	snooze(100000);
	
	return true;
}

//---------------------------------------------------------------

void
CodyCam::MessageReceived(BMessage *message)
{
	switch (message->what)
	{
		case msg_start:
		{
			BTimeSource *timeSource = fMediaRoster->MakeTimeSourceFor(fTimeSourceNode);
			bigtime_t real = BTimeSource::RealTime();
			bigtime_t perf = timeSource->PerformanceTimeFor(real) + 10000;
			status_t status = fMediaRoster->StartNode(fProducerNode, perf);
			if (status != B_OK)
				ERROR("error starting producer!");
			timeSource->Release();
			break;
		}
		case msg_stop:
			fMediaRoster->StopNode(fProducerNode, 0, true);
			break;
		case msg_video:
		{
			if (mVideoControlWindow) {
				mVideoControlWindow->Activate();
				break;
			}
			BParameterWeb * web = NULL;
			BView * view = NULL;
			media_node node = fProducerNode;
			status_t err = fMediaRoster->GetParameterWebFor(node, &web);
			if ((err >= B_OK) &&
				(web != NULL))
			{
				view = BMediaTheme::ViewFor(web);
				mVideoControlWindow = new ControlWindow(
							BRect(2*WINDOW_OFFSET_X + WINDOW_SIZE_X, WINDOW_OFFSET_Y,
								2*WINDOW_OFFSET_X + WINDOW_SIZE_X + view->Bounds().right, WINDOW_OFFSET_Y + view->Bounds().bottom),
							view, node);
				fMediaRoster->StartWatching(BMessenger(NULL, mVideoControlWindow), node, B_MEDIA_WEB_CHANGED);
				mVideoControlWindow->Show();
			}
			break;
		}
		case msg_about:
		{
			(new BAlert("About CodyCam", "CodyCam\n\nThe Original BeOS WebCam", "Close"))->Go();
			break;
		}
		
		case msg_control_win:
		{
			// our control window is being asked to go away
			// set our pointer to NULL
			mVideoControlWindow = NULL;
			break;
		
		}
		
		default:
			BApplication::MessageReceived(message);
			break;
	}
}

//---------------------------------------------------------------

status_t 
CodyCam::SetUpNodes()
{
	status_t status = B_OK;

	/* find the media roster */
	fMediaRoster = BMediaRoster::Roster(&status);
	if (status != B_OK) {
		ErrorAlert("Can't find the media roster", status);
		return status;
	}	
	/* find the time source */
	status = fMediaRoster->GetTimeSource(&fTimeSourceNode);
	if (status != B_OK) {
		ErrorAlert("Can't get a time source", status);
		return status;
	}
	/* find a video producer node */
	INFO("CodyCam acquiring VideoInput node\n");
	status = fMediaRoster->GetVideoInput(&fProducerNode);
	if (status != B_OK) {
		ErrorAlert("Can't find a video source. You need a webcam to use CodyCam.", status);
		return status;
	}

	/* create the video consumer node */
	fVideoConsumer = new VideoConsumer("CodyCam", ((VideoWindow *)fWindow)->VideoView(), ((VideoWindow *)fWindow)->StatusLine(), NULL, 0);
	if (!fVideoConsumer) {
		ErrorAlert("Can't create a video window", B_ERROR);
		return B_ERROR;
	}
	
	/* register the node */
	status = fMediaRoster->RegisterNode(fVideoConsumer);
	if (status != B_OK) {
		ErrorAlert("Can't register the video window", status);
		return status;
	}
	fPort = fVideoConsumer->ControlPort();
	
	/* find free producer output */
	int32 cnt = 0;
	status = fMediaRoster->GetFreeOutputsFor(fProducerNode, &fProducerOut, 1,  &cnt, B_MEDIA_RAW_VIDEO);
	if (status != B_OK || cnt < 1) {
		status = B_RESOURCE_UNAVAILABLE;
		ErrorAlert("Can't find an available video stream", status);
		return status;
	}

	/* find free consumer input */
	cnt = 0;
	status = fMediaRoster->GetFreeInputsFor(fVideoConsumer->Node(), &fConsumerIn, 1, &cnt, B_MEDIA_RAW_VIDEO);
	if (status != B_OK || cnt < 1) {
		status = B_RESOURCE_UNAVAILABLE;
		ErrorAlert("Can't find an available connection to the video window", status);
		return status;
	}

	/* Connect The Nodes!!! */
	media_format format;
	format.type = B_MEDIA_RAW_VIDEO;
	media_raw_video_format vid_format = 
		{ 0, 1, 0, 239, B_VIDEO_TOP_LEFT_RIGHT, 1, 1, {B_RGB32, VIDEO_SIZE_X, VIDEO_SIZE_Y, VIDEO_SIZE_X*4, 0, 0}};
	format.u.raw_video = vid_format; 
	
	/* connect producer to consumer */
	status = fMediaRoster->Connect(fProducerOut.source, fConsumerIn.destination,
				&format, &fProducerOut, &fConsumerIn);
	if (status != B_OK) {
		ErrorAlert("Can't connect the video source to the video window", status);
		return status;
	}
	
	/* set time sources */
	status = fMediaRoster->SetTimeSourceFor(fProducerNode.node, fTimeSourceNode.node);
	if (status != B_OK) {
		ErrorAlert("Can't set the timesource for the video source", status);
		return status;
	}
	
	status = fMediaRoster->SetTimeSourceFor(fVideoConsumer->ID(), fTimeSourceNode.node);
	if (status != B_OK) {
		ErrorAlert("Can't set the timesource for the video window", status);
		return status;
	}
	
	/* figure out what recording delay to use */
	bigtime_t latency = 0;
	status = fMediaRoster->GetLatencyFor(fProducerNode, &latency);
	status = fMediaRoster->SetProducerRunModeDelay(fProducerNode, latency);

	/* start the nodes */
	bigtime_t initLatency = 0;
	status = fMediaRoster->GetInitialLatencyFor(fProducerNode, &initLatency);
	if (status < B_OK) {
		ErrorAlert("error getting initial latency for fCaptureNode", status);	
	}
	initLatency += estimate_max_scheduling_latency();
	
	BTimeSource *timeSource = fMediaRoster->MakeTimeSourceFor(fProducerNode);
	bool running = timeSource->IsRunning();
	
	/* workaround for people without sound cards */
	/* because the system time source won't be running */
	bigtime_t real = BTimeSource::RealTime();
	if (!running)
	{
		status = fMediaRoster->StartTimeSource(fTimeSourceNode, real);
		if (status != B_OK) {
			timeSource->Release();
			ErrorAlert("cannot start time source!", status);
			return status;
		}
		status = fMediaRoster->SeekTimeSource(fTimeSourceNode, 0, real);
		if (status != B_OK) {
			timeSource->Release();
			ErrorAlert("cannot seek time source!", status);
			return status;
		}
	}

	bigtime_t perf = timeSource->PerformanceTimeFor(real + latency + initLatency);
	timeSource->Release();
	
	/* start the nodes */
	status = fMediaRoster->StartNode(fProducerNode, perf);
	if (status != B_OK) {
		ErrorAlert("Can't start the video source", status);
		return status;
	}
	status = fMediaRoster->StartNode(fVideoConsumer->Node(), perf);
	if (status != B_OK) {
		ErrorAlert("Can't start the video window", status);
		return status;
	}
	
	return status;
}

//---------------------------------------------------------------

void 
CodyCam::TearDownNodes()
{
	CALL("CodyCam::TearDownNodes\n");
	if (!fMediaRoster)
		return;
	
	if (fVideoConsumer)
	{
		/* stop */	
		INFO("stopping nodes!\n");
//		fMediaRoster->StopNode(fProducerNode, 0, true);
		fMediaRoster->StopNode(fVideoConsumer->Node(), 0, true);
	
		/* disconnect */
		fMediaRoster->Disconnect(fProducerOut.node.node, fProducerOut.source,
								fConsumerIn.node.node, fConsumerIn.destination);
								
		if (fProducerNode != media_node::null) {
			INFO("CodyCam releasing fProducerNode\n");
			fMediaRoster->ReleaseNode(fProducerNode);
			fProducerNode = media_node::null;
		}
		fMediaRoster->ReleaseNode(fVideoConsumer->Node());		
		fVideoConsumer = NULL;
	}
}

//---------------------------------------------------------------
// Utility functions
//---------------------------------------------------------------

static void
ErrorAlert(const char * message, status_t err)
{
	(new BAlert("", message, "Quit"))->Go();
	
	printf("%s\n%s [%lx]", message, strerror(err), err);
	be_app->PostMessage(B_QUIT_REQUESTED);
}

//---------------------------------------------------------------

status_t 
AddTranslationItems( BMenu * intoMenu, uint32 from_type)
{ 

    BTranslatorRoster * use;
    char * translator_type_name;
    const char * translator_id_name;
    
    use = BTranslatorRoster::Default();
	translator_id_name = "be:translator"; 
	translator_type_name = "be:type";
	translator_id * ids = NULL;
	int32 count = 0;
	
	status_t err = use->GetAllTranslators(&ids, &count); 
	if (err < B_OK) return err; 
	for (int tix=0; tix<count; tix++) { 
		const translation_format * formats = NULL; 
		int32 num_formats = 0; 
		bool ok = false; 
		err = use->GetInputFormats(ids[tix], &formats, &num_formats); 
		if (err == B_OK) for (int iix=0; iix<num_formats; iix++) { 
			if (formats[iix].type == from_type) { 
				ok = true; 
				break; 
			} 
		} 
		if (!ok) continue; 
			err = use->GetOutputFormats(ids[tix], &formats, &num_formats); 
		if (err == B_OK) for (int oix=0; oix<num_formats; oix++) { 
 			if (formats[oix].type != from_type) { 
				BMessage * itemmsg; 
				itemmsg = new BMessage(msg_translate);
				itemmsg->AddInt32(translator_id_name, ids[tix]);
				itemmsg->AddInt32(translator_type_name, formats[oix].type); 
				intoMenu->AddItem(new BMenuItem(formats[oix].name, itemmsg)); 
				} 
		} 
	} 
	delete[] ids; 
	return B_OK; 
} 

//---------------------------------------------------------------
//  Video Window Class
//---------------------------------------------------------------

VideoWindow::VideoWindow (BRect frame, const char *title, window_type type, uint32 flags, port_id * consumerport) :
	BWindow(frame,title,type,flags),
	fPortPtr(consumerport),
	fView(NULL),
	fVideoView(NULL)
{
	fFtpInfo.port = 0;
	fFtpInfo.rate = 0x7fffffff;
	fFtpInfo.imageFormat = 0;
	fFtpInfo.translator = 0;
	fFtpInfo.passiveFtp = true;
	strcpy(fFtpInfo.fileNameText, "filename");
	strcpy(fFtpInfo.serverText, "server");
	strcpy(fFtpInfo.loginText, "login");
	strcpy(fFtpInfo.passwordText, "password");
	strcpy(fFtpInfo.directoryText, "directory");

	SetUpSettings("codycam", "");	

	BMenuBar* menuBar = new BMenuBar(BRect(0,0,0,0), "menu bar");
	AddChild(menuBar);
	
	BMenuItem* menuItem;
	BMenu* menu = new BMenu("File");

	menuItem = new BMenuItem("Video Preferences", new BMessage(msg_video), 'P');
	menuItem->SetTarget(be_app);
	menu->AddItem(menuItem);

	menu->AddSeparatorItem();

	menuItem = new BMenuItem("Start Video", new BMessage(msg_start), 'A');
	menuItem->SetTarget(be_app);
	menu->AddItem(menuItem);

	menuItem = new BMenuItem("Stop Video", new BMessage(msg_stop), 'O');
	menuItem->SetTarget(be_app);
	menu->AddItem(menuItem);

	menu->AddSeparatorItem();

	menuItem = new BMenuItem("About Codycam", new BMessage(msg_about), 'B');
	menuItem->SetTarget(be_app);
	menu->AddItem(menuItem);

	menu->AddSeparatorItem();

	menuItem = new BMenuItem("Quit", new BMessage(B_QUIT_REQUESTED), 'Q');
	menuItem->SetTarget(be_app);
	menu->AddItem(menuItem);

	menuBar->AddItem(menu);

	/* give it a gray background view */
	BRect aRect;
	aRect = Frame();
	aRect.OffsetTo(B_ORIGIN);
	aRect.top += menuBar->Frame().Height() + 1;
	fView	= new BView(aRect, "Background View", B_FOLLOW_ALL, B_WILL_DRAW);
	fView->SetViewColor(kViewGray);
	AddChild(fView);
	
	/* add some controls */
	BuildCaptureControls(fView);
	
	/* add another view to hold the video image */
	aRect = BRect(0, 0, VIDEO_SIZE_X - 1, VIDEO_SIZE_Y - 1);
	aRect.OffsetBy((WINDOW_SIZE_X - VIDEO_SIZE_X)/2, kYBuffer);
	
	fVideoView	= new BView(aRect, "Video View", B_FOLLOW_ALL, B_WILL_DRAW);
	fView->AddChild(fVideoView);

	Show();
}

//---------------------------------------------------------------

VideoWindow::~VideoWindow()
{
	QuitSettings();
}

//---------------------------------------------------------------

bool
VideoWindow::QuitRequested()
{
	be_app->PostMessage(B_QUIT_REQUESTED);
	return false;
}

//---------------------------------------------------------------

void
VideoWindow::MessageReceived(BMessage *message)
{
	BControl	*p;

	p = NULL;
	message->FindPointer((const char *)"source",(void **)&p);
	
	switch (message->what)
	{
		case msg_filename:
			if (p != NULL)
			{
				strncpy(fFtpInfo.fileNameText, ((BTextControl *)p)->Text(), 63);
				FTPINFO("file is '%s'\n", fFtpInfo.fileNameText);
			}
			break;
		case msg_rate_15s:
			FTPINFO("fifteen seconds\n");
			fFtpInfo.rate = (bigtime_t)(15 * 1000000);
			break;
		case msg_rate_30s:
			FTPINFO("thirty seconds\n");
			fFtpInfo.rate = (bigtime_t)(30 * 1000000);
			break;
		case msg_rate_1m:
			FTPINFO("one minute\n");
			fFtpInfo.rate = (bigtime_t)(1 * 60 * 1000000);
			break;
		case msg_rate_5m:
			FTPINFO("five minute\n");
			fFtpInfo.rate = (bigtime_t)(5 * 60 * 1000000);
			break;
		case msg_rate_10m:
			FTPINFO("ten minute\n");
			fFtpInfo.rate = (bigtime_t)(10 * 60 * 1000000);
			break;
		case msg_rate_15m:
			FTPINFO("fifteen minute\n");
			fFtpInfo.rate = (bigtime_t)(15 * 60 * 1000000);
			break;
		case msg_rate_30m:
			FTPINFO("thirty minute\n");
			fFtpInfo.rate = (bigtime_t)(30 * 60 * 1000000);
			break;
		case msg_rate_1h:
			FTPINFO("one hour\n");
			fFtpInfo.rate = (bigtime_t)(60LL * 60LL * 1000000LL);
			break;
		case msg_rate_2h:
			FTPINFO("two hour\n");
			fFtpInfo.rate = (bigtime_t)(2LL * 60LL * 60LL * 1000000LL);
			break;
		case msg_rate_4h:
			FTPINFO("four hour\n");
			fFtpInfo.rate = (bigtime_t)(4LL * 60LL * 60LL * 1000000LL);
			break;
		case msg_rate_8h:
			FTPINFO("eight hour\n");
			fFtpInfo.rate = (bigtime_t)(8LL * 60LL * 60LL * 1000000LL);
			break;
		case msg_rate_24h:
			FTPINFO("24 hour\n");
			fFtpInfo.rate = (bigtime_t)(24LL * 60LL * 60LL * 1000000LL);
			break;
		case msg_rate_never:
			FTPINFO("never\n");
			fFtpInfo.rate = (bigtime_t)(B_INFINITE_TIMEOUT);
			break;
		case msg_translate: 
			message->FindInt32("be:type", (int32 *)&(fFtpInfo.imageFormat));
			message->FindInt32("be:translator", &(fFtpInfo.translator)); 
			break;
		case msg_server:
			if (p != NULL)
			{
				strncpy(fFtpInfo.serverText, ((BTextControl *)p)->Text(), 64);
				FTPINFO("server = '%s'\n", fFtpInfo.serverText);
			}
			break;
		case msg_login:
			if (p != NULL)
			{
				strncpy(fFtpInfo.loginText, ((BTextControl *)p)->Text(), 64);
				FTPINFO("login = '%s'\n", fFtpInfo.loginText);
			}
			break;
		case msg_password:
			if (p != NULL)
			{
				strncpy(fFtpInfo.passwordText, ((BTextControl *)p)->Text(), 64);
				FTPINFO("password = '%s'\n", fFtpInfo.passwordText);
				if (Lock())
				{
					((BTextControl *)p)->SetText("<HIDDEN>");
					Unlock();
				}
			}
			break;
		case msg_directory:
			if (p != NULL)
			{
				strncpy(fFtpInfo.directoryText, ((BTextControl *)p)->Text(), 64);
				FTPINFO("directory = '%s'\n", fFtpInfo.directoryText);
			}
			break;
		case msg_passiveftp:
			if (p != NULL)
			{
				fFtpInfo.passiveFtp = ((BCheckBox *)p)->Value();
				if (fFtpInfo.passiveFtp)
					FTPINFO("using passive ftp\n");
			}
			break;
		default:
			BWindow::MessageReceived(message);
			return;
	}

	if (*fPortPtr)
		write_port(*fPortPtr, FTP_INFO, (void *)&fFtpInfo, sizeof(ftp_msg_info));

}

//---------------------------------------------------------------

BView *
VideoWindow::VideoView()
{
	return fVideoView;
}

//---------------------------------------------------------------

BStringView *
VideoWindow::StatusLine()
{
	return fStatusLine;
}

//---------------------------------------------------------------

void
VideoWindow::BuildCaptureControls(BView *theView)
{
	BRect aFrame, theFrame;

	theFrame = theView->Bounds();
	theFrame.top += VIDEO_SIZE_Y + 2*kYBuffer + 40;
	theFrame.left += kXBuffer;
	theFrame.right -= (WINDOW_SIZE_X/2 + 5);
	theFrame.bottom -= kXBuffer;
	
	fCaptureSetupBox = new BBox( theFrame, "Capture Controls", B_FOLLOW_ALL, B_WILL_DRAW);
	fCaptureSetupBox->SetLabel("Capture Controls");
	theView->AddChild(fCaptureSetupBox);
	
	aFrame = fCaptureSetupBox->Bounds();
	aFrame.InsetBy(kXBuffer,kYBuffer);	
	aFrame.top += kYBuffer/2;
	aFrame.bottom = aFrame.top + kMenuHeight;
	
	fFileName = new BTextControl(aFrame, "File Name", "File Name:", fFilenameSetting->Value(), new BMessage(msg_filename));

	fFileName->SetTarget(BMessenger(NULL, this));
	fFileName->SetDivider(fFileName->Divider() - 30);
	fCaptureSetupBox->AddChild(fFileName);	

	aFrame.top = aFrame.bottom + kYBuffer;
	aFrame.bottom = aFrame.top + kMenuHeight;

	fImageFormatMenu = new BPopUpMenu("Image Format Menu");	
	AddTranslationItems(fImageFormatMenu, B_TRANSLATOR_BITMAP);
	fImageFormatMenu->SetTargetForItems(this);
	if (fImageFormatMenu->FindItem("JPEG Image") != NULL)
		fImageFormatMenu->FindItem("JPEG Image")->SetMarked(true);
	else
		fImageFormatMenu->ItemAt(0)->SetMarked(true);		
	fImageFormatSelector = new BMenuField(aFrame, "Format", "Format:", fImageFormatMenu);
	fImageFormatSelector->SetDivider(fImageFormatSelector->Divider() - 30);
	fCaptureSetupBox->AddChild(fImageFormatSelector);
	
	aFrame.top = aFrame.bottom + kYBuffer;
	aFrame.bottom = aFrame.top + kMenuHeight;

	fCaptureRateMenu = new BPopUpMenu("Capture Rate Menu");
	fCaptureRateMenu->AddItem(new BMenuItem("Every 15 seconds",new BMessage(msg_rate_15s)));
	fCaptureRateMenu->AddItem(new BMenuItem("Every 30 seconds",new BMessage(msg_rate_30s)));
	fCaptureRateMenu->AddItem(new BMenuItem("Every minute",new BMessage(msg_rate_1m)));
	fCaptureRateMenu->AddItem(new BMenuItem("Every 5 minutes",new BMessage(msg_rate_5m)));
	fCaptureRateMenu->AddItem(new BMenuItem("Every 10 minutes",new BMessage(msg_rate_10m)));
	fCaptureRateMenu->AddItem(new BMenuItem("Every 15 minutes",new BMessage(msg_rate_15m)));
	fCaptureRateMenu->AddItem(new BMenuItem("Every 30 minutes",new BMessage(msg_rate_30m)));
	fCaptureRateMenu->AddItem(new BMenuItem("Every hour",new BMessage(msg_rate_1h)));
	fCaptureRateMenu->AddItem(new BMenuItem("Every 2 hours",new BMessage(msg_rate_2h)));
	fCaptureRateMenu->AddItem(new BMenuItem("Every 4 hours",new BMessage(msg_rate_4h)));
	fCaptureRateMenu->AddItem(new BMenuItem("Every 8 hours",new BMessage(msg_rate_8h)));
	fCaptureRateMenu->AddItem(new BMenuItem("Every 24 hours",new BMessage(msg_rate_24h)));
	fCaptureRateMenu->AddItem(new BMenuItem("Never",new BMessage(msg_rate_never)));
	fCaptureRateMenu->SetTargetForItems(this);
	fCaptureRateMenu->FindItem(fCaptureRateSetting->Value())->SetMarked(true);
	fCaptureRateSelector = new BMenuField(aFrame, "Rate", "Rate:", fCaptureRateMenu);
	fCaptureRateSelector->SetDivider(fCaptureRateSelector->Divider() - 30);
	fCaptureSetupBox->AddChild(fCaptureRateSelector);

	aFrame = theView->Bounds();
	aFrame.top += VIDEO_SIZE_Y + 2*kYBuffer + 40;
	aFrame.left += WINDOW_SIZE_X/2 + 5;
	aFrame.right -= kXBuffer;
	aFrame.bottom -= kYBuffer;
		
	fFtpSetupBox = new BBox( aFrame, "Ftp Setup", B_FOLLOW_ALL, B_WILL_DRAW);
	fFtpSetupBox->SetLabel("Ftp Setup");
	theView->AddChild(fFtpSetupBox);
	
	aFrame = fFtpSetupBox->Bounds();
	aFrame.InsetBy(kXBuffer,kYBuffer);	
	aFrame.top += kYBuffer/2;
	aFrame.bottom = aFrame.top + kMenuHeight;	
	aFrame.right = aFrame.left + 160;
	
	fServerName = new BTextControl(aFrame, "Server", "Server:", fServerSetting->Value(), new BMessage(msg_server));
	fServerName->SetTarget(this);
	fServerName->SetDivider(fServerName->Divider() - 30);
	fFtpSetupBox->AddChild(fServerName);	

	aFrame.top = aFrame.bottom + kYBuffer;
	aFrame.bottom = aFrame.top + kMenuHeight;
	
	fLoginId = new BTextControl(aFrame, "Login", "Login:", fLoginSetting->Value(), new BMessage(msg_login));
	fLoginId->SetTarget(this);
	fLoginId->SetDivider(fLoginId->Divider() - 30);
	fFtpSetupBox->AddChild(fLoginId);	

	aFrame.top = aFrame.bottom + kYBuffer;
	aFrame.bottom = aFrame.top + kMenuHeight;
	
	fPassword = new BTextControl(aFrame, "Password", "Password:", fPasswordSetting->Value(), new BMessage(msg_password));
	fPassword->SetTarget(this);
	fPassword->SetDivider(fPassword->Divider() - 30);
	fFtpSetupBox->AddChild(fPassword);	

	aFrame.top = aFrame.bottom + kYBuffer;
	aFrame.bottom = aFrame.top + kMenuHeight;
	
	fDirectory = new BTextControl(aFrame, "Directory", "Directory:", fDirectorySetting->Value(), new BMessage(msg_directory));
	fDirectory->SetTarget(this);
	fDirectory->SetDivider(fDirectory->Divider() - 30);
	fFtpSetupBox->AddChild(fDirectory);	

	aFrame.top = aFrame.bottom + kYBuffer;
	aFrame.bottom = aFrame.top + kMenuHeight;
	
	fPassiveFtp = new BCheckBox(aFrame, "Passive ftp", "Passive ftp", new BMessage(msg_passiveftp));
	fPassiveFtp->SetTarget(this);
	fPassiveFtp->SetValue(fPassiveFtpSetting->Value());
	fFtpSetupBox->AddChild(fPassiveFtp);	
	
	aFrame = theView->Bounds();
	aFrame.top += VIDEO_SIZE_Y + 2*kYBuffer;
	aFrame.left += kXBuffer;
	aFrame.right -= kXBuffer;
	aFrame.bottom = aFrame.top + kMenuHeight + 2*kYBuffer;
		
	fStatusBox = new BBox( aFrame, "Status", B_FOLLOW_ALL, B_WILL_DRAW);
	fStatusBox->SetLabel("Status");
	theView->AddChild(fStatusBox);
	
	aFrame = fStatusBox->Bounds();
	aFrame.InsetBy(kXBuffer,kYBuffer);	
	
	fStatusLine = new BStringView(aFrame,"Status Line","Waiting ...");
	fStatusBox->AddChild(fStatusLine);
}

//---------------------------------------------------------------

void
VideoWindow::ApplyControls()
{
	// apply controls
	fFileName->Invoke();		
	PostMessage(fImageFormatMenu->FindMarked()->Message());
	PostMessage(fCaptureRateMenu->FindMarked()->Message());
	fServerName->Invoke();
	fLoginId->Invoke();
	fPassword->Invoke();
	fDirectory->Invoke();
	fPassiveFtp->Invoke();
}

//---------------------------------------------------------------

void
VideoWindow::SetUpSettings(const char *filename, const char *dirname)
{
	fSettings = new Settings(filename, dirname);

	fSettings->Add(fServerSetting = new StringValueSetting("Server", "ftp.my.server",
		"server address expected", ""));
	fSettings->Add(fLoginSetting = new StringValueSetting("Login", "loginID",
		"login ID expected", ""));
	fSettings->Add(fPasswordSetting = new StringValueSetting("Password", "password",
		"password expected", ""));
	fSettings->Add(fDirectorySetting = new StringValueSetting("Directory", "web/images",
		"destination directory expected", ""));
	fSettings->Add(fPassiveFtpSetting = new BooleanValueSetting("PassiveFtp", 1));
	fSettings->Add(fFilenameSetting = new StringValueSetting("StillImageFilename", "codycam.jpg",
		"still image filename expected", ""));
	fSettings->Add(fCaptureRateSetting = new EnumeratedStringValueSetting("CaptureRate", "Every 5 minutes", kCaptureRate,
			"capture rate expected", "unrecognized capture rate specified"));

	fSettings->TryReadingSettings();
}

//---------------------------------------------------------------

void
VideoWindow::QuitSettings()
{
	fServerSetting->ValueChanged(fServerName->Text());
	fLoginSetting->ValueChanged(fLoginId->Text());
	fPasswordSetting->ValueChanged(fFtpInfo.passwordText);
	fDirectorySetting->ValueChanged(fDirectory->Text());
	fPassiveFtpSetting->ValueChanged(fPassiveFtp->Value());
	fFilenameSetting->ValueChanged(fFileName->Text());
	fCaptureRateSetting->ValueChanged(fCaptureRateMenu->FindMarked()->Label());
	
	fSettings->SaveSettings();
	delete fSettings;
}

//---------------------------------------------------------------

ControlWindow::ControlWindow(
	const BRect & frame,
	BView * controls,
	media_node node) :
	BWindow(frame, "Video Preferences", B_TITLED_WINDOW, B_ASYNCHRONOUS_CONTROLS)
{
	fView = controls;
	fNode = node;
		
	AddChild(fView);
}

//---------------------------------------------------------------

void
ControlWindow::MessageReceived(BMessage * message) 
{
	BParameterWeb * web = NULL;
	status_t err;
	
	switch (message->what)
	{
		case B_MEDIA_WEB_CHANGED:
		{
			// If this is a tab view, find out which tab 
			// is selected
			BTabView *tabView = dynamic_cast<BTabView*>(fView);
			int32 tabNum = -1;
			if (tabView)
				tabNum = tabView->Selection();

			RemoveChild(fView);
			delete fView;
			
			err = BMediaRoster::Roster()->GetParameterWebFor(fNode, &web);
			
			if ((err >= B_OK) &&
				(web != NULL))
			{
				fView = BMediaTheme::ViewFor(web);
				AddChild(fView);

				// Another tab view?  Restore previous selection
				if (tabNum > 0)
				{
					BTabView *newTabView = dynamic_cast<BTabView*>(fView);	
					if (newTabView)
						newTabView->Select(tabNum);
				}
			}
			break;
		}
		default:
			BWindow::MessageReceived(message);
	}
}

//---------------------------------------------------------------

bool 
ControlWindow::QuitRequested()
{
	be_app->PostMessage(msg_control_win);
	return true;
}



