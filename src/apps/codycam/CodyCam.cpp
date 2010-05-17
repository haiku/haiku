#include "CodyCam.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <Alert.h>
#include <Button.h>
#include <GridLayout.h>
#include <GroupLayout.h>
#include <GroupLayoutBuilder.h>
#include <MediaDefs.h>
#include <MediaNode.h>
#include <MediaRoster.h>
#include <MediaTheme.h>
#include <Menu.h>
#include <MenuBar.h>
#include <MenuItem.h>
#include <PopUpMenu.h>
#include <scheduler.h>
#include <SpaceLayoutItem.h>
#include <TabView.h>
#include <TextControl.h>
#include <TimeSource.h>
#include <TranslationKit.h>


#undef B_TRANSLATE_CONTEXT
#define B_TRANSLATE_CONTEXT "CodyCam"

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

static void ErrorAlert(const char* message, status_t err, BWindow *window);
static status_t AddTranslationItems(BMenu* intoMenu, uint32 fromType);

#define	CALL		printf
#define ERROR		printf
#define FTPINFO		printf
#define	INFO		printf


// Utility functions

static void
ErrorAlert(const char* message, status_t err, BWindow *window = NULL)
{
	BAlert *alert = new BAlert("", message, B_TRANSLATE("OK"));
	if (window != NULL)
		alert->CenterIn(window->Frame());
	alert->Go();

	printf("%s\n%s [%lx]", message, strerror(err), err);
//	be_app->PostMessage(B_QUIT_REQUESTED);
}


status_t
AddTranslationItems(BMenu* intoMenu, uint32 fromType)
{

	BTranslatorRoster* use;
	char* translatorTypeName;
	const char* translatorIdName;

	use = BTranslatorRoster::Default();
	translatorIdName = "be:translator";
	translatorTypeName = (char *)"be:type";
	translator_id* ids = NULL;
	int32 count = 0;

	status_t err = use->GetAllTranslators(&ids, &count);
	if (err < B_OK)
		return err;

	for (int tix = 0; tix < count; tix++) {
		const translation_format* formats = NULL;
		int32 num_formats = 0;
		bool ok = false;
		err = use->GetInputFormats(ids[tix], &formats, &num_formats);
		if (err == B_OK)
			for (int iix = 0; iix < num_formats; iix++) {
				if (formats[iix].type == fromType) {
					ok = true;
					break;
				}
			}

		if (!ok)
			continue;

		err = use->GetOutputFormats(ids[tix], &formats, &num_formats);
		if (err == B_OK)
			for (int oix = 0; oix < num_formats; oix++) {
 				if (formats[oix].type != fromType) {
					BMessage* itemmsg;
					itemmsg = new BMessage(msg_translate);
					itemmsg->AddInt32(translatorIdName, ids[tix]);
					itemmsg->AddInt32(translatorTypeName, formats[oix].type);
					intoMenu->AddItem(new BMenuItem(formats[oix].name, itemmsg));
				}
			}
	}
	delete[] ids;
	return B_OK;
}


//	#pragma mark -


CodyCam::CodyCam()
	: BApplication("application/x-vnd.Haiku.CodyCam"),
	fMediaRoster(NULL),
	fVideoConsumer(NULL),
	fWindow(NULL),
	fPort(0),
	fVideoControlWindow(NULL),
	fAppCatalog(NULL)
{
	be_locale->GetAppCatalog(&fAppCatalog);

	int32 index = 0;
	kCaptureRate[index++] = B_TRANSLATE("Every 15 seconds");
	kCaptureRate[index++] = B_TRANSLATE("Every 30 seconds");
	kCaptureRate[index++] = B_TRANSLATE("Every minute");
	kCaptureRate[index++] = B_TRANSLATE("Every 5 minutes");
	kCaptureRate[index++] = B_TRANSLATE("Every 10 minutes");
	kCaptureRate[index++] = B_TRANSLATE("Every 15 minutes");
	kCaptureRate[index++] = B_TRANSLATE("Every 30 minutes");
	kCaptureRate[index++] = B_TRANSLATE("Every hour");
	kCaptureRate[index++] = B_TRANSLATE("Every 2 hours");
	kCaptureRate[index++] = B_TRANSLATE("Every 4 hours");
	kCaptureRate[index++] = B_TRANSLATE("Every 8 hours");
	kCaptureRate[index++] = B_TRANSLATE("Every 24 hours");
	kCaptureRate[index++] = B_TRANSLATE("Never");

	index = 0;
	kUploadClient[index++] = B_TRANSLATE("FTP");
	kUploadClient[index++] = B_TRANSLATE("SFTP");
	kUploadClient[index++] = B_TRANSLATE("Local");

	chdir("/boot/home");
}


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


void
CodyCam::ReadyToRun()
{
	fWindow = new VideoWindow(BRect(28, 28, 28, 28),
		(const char*) B_TRANSLATE("CodyCam"), B_TITLED_WINDOW,
		B_NOT_ZOOMABLE | B_AUTO_UPDATE_SIZE_LIMITS, &fPort);

	_SetUpNodes();

	((VideoWindow*)fWindow)->ApplyControls();
}



bool
CodyCam::QuitRequested()
{
	_TearDownNodes();
	snooze(100000);

	return true;
}


void
CodyCam::MessageReceived(BMessage *message)
{
	switch (message->what) {
		case msg_start:
		{
			BTimeSource* timeSource = fMediaRoster->MakeTimeSourceFor(
				fTimeSourceNode);
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
			if (fVideoControlWindow) {
				fVideoControlWindow->Activate();
				break;
			}
			BParameterWeb* web = NULL;
			BView* view = NULL;
			media_node node = fProducerNode;
			status_t err = fMediaRoster->GetParameterWebFor(node, &web);
			if (err >= B_OK && web != NULL) {
				view = BMediaTheme::ViewFor(web);
				fVideoControlWindow = new ControlWindow(
					BRect(2 * WINDOW_OFFSET_X + WINDOW_SIZE_X, WINDOW_OFFSET_Y,
					2 * WINDOW_OFFSET_X + WINDOW_SIZE_X + view->Bounds().right,
					WINDOW_OFFSET_Y + view->Bounds().bottom), view, node);
				fMediaRoster->StartWatching(BMessenger(NULL, 
					fVideoControlWindow), node,	B_MEDIA_WEB_CHANGED);
				fVideoControlWindow->Show();
			}
			break;
		}

		case msg_about:
			(new BAlert(B_TRANSLATE("About CodyCam"), B_TRANSLATE("CodyCam\n\n"
				"The Original BeOS webcam"), B_TRANSLATE("Close")))->Go();
			break;

		case msg_control_win:
			// our control window is being asked to go away
			// set our pointer to NULL
			fVideoControlWindow = NULL;
			break;

		default:
			BApplication::MessageReceived(message);
			break;
	}
}


status_t
CodyCam::_SetUpNodes()
{
	status_t status = B_OK;

	/* find the media roster */
	fMediaRoster = BMediaRoster::Roster(&status);
	if (status != B_OK) {
		ErrorAlert(B_TRANSLATE("Cannot find the media roster"), status, 
			fWindow);
		return status;
	}

	/* find the time source */
	status = fMediaRoster->GetTimeSource(&fTimeSourceNode);
	if (status != B_OK) {
		ErrorAlert(B_TRANSLATE("Cannot get a time source"), status, fWindow);
		return status;
	}

	/* find a video producer node */
	INFO("CodyCam acquiring VideoInput node\n");
	status = fMediaRoster->GetVideoInput(&fProducerNode);
	if (status != B_OK) {
		ErrorAlert(B_TRANSLATE("Cannot find a video source. You need a webcam "
			"to use CodyCam."), status, fWindow);
		return status;
	}

	/* create the video consumer node */
	fVideoConsumer = new VideoConsumer("CodyCam", 
		((VideoWindow*)fWindow)->VideoView(),
		((VideoWindow*)fWindow)->StatusLine(), NULL, 0);
	if (!fVideoConsumer) {
		ErrorAlert(B_TRANSLATE("Cannot create a video window"), B_ERROR, 
			fWindow);
		return B_ERROR;
	}

	/* register the node */
	status = fMediaRoster->RegisterNode(fVideoConsumer);
	if (status != B_OK) {
		ErrorAlert(B_TRANSLATE("Cannot register the video window"), status, 
			fWindow);
		return status;
	}
	fPort = fVideoConsumer->ControlPort();

	/* find free producer output */
	int32 cnt = 0;
	status = fMediaRoster->GetFreeOutputsFor(fProducerNode, &fProducerOut, 1, 
		&cnt, B_MEDIA_RAW_VIDEO);
	if (status != B_OK || cnt < 1) {
		status = B_RESOURCE_UNAVAILABLE;
		ErrorAlert(B_TRANSLATE("Cannot find an available video stream"), 
			status,	fWindow);
		return status;
	}

	/* find free consumer input */
	cnt = 0;
	status = fMediaRoster->GetFreeInputsFor(fVideoConsumer->Node(), 
		&fConsumerIn, 1, &cnt, B_MEDIA_RAW_VIDEO);
	if (status != B_OK || cnt < 1) {
		status = B_RESOURCE_UNAVAILABLE;
		ErrorAlert(B_TRANSLATE("Can't find an available connection to the "
			"video window"), status, fWindow);
		return status;
	}

	/* Connect The Nodes!!! */
	media_format format;
	format.type = B_MEDIA_RAW_VIDEO;
	media_raw_video_format vid_format = {0, 1, 0, 239, B_VIDEO_TOP_LEFT_RIGHT,
		1, 1, {B_RGB32, VIDEO_SIZE_X, VIDEO_SIZE_Y, VIDEO_SIZE_X * 4, 0, 0}};
	format.u.raw_video = vid_format;

	/* connect producer to consumer */
	status = fMediaRoster->Connect(fProducerOut.source, 
		fConsumerIn.destination, &format, &fProducerOut, &fConsumerIn);
	if (status != B_OK) {
		ErrorAlert(B_TRANSLATE("Cannot connect the video source to the video "
			"window"), status);
		return status;
	}


	/* set time sources */
	status = fMediaRoster->SetTimeSourceFor(fProducerNode.node, 
		fTimeSourceNode.node);
	if (status != B_OK) {
		ErrorAlert(B_TRANSLATE("Cannot set the time source for the video "
			"source"), status);
		return status;
	}

	status = fMediaRoster->SetTimeSourceFor(fVideoConsumer->ID(), 
		fTimeSourceNode.node);
	if (status != B_OK) {
		ErrorAlert(B_TRANSLATE("Cannot set the time source for the video "
			"window"), status);
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
		ErrorAlert(B_TRANSLATE("Error getting initial latency for the capture "
			"node"), status);
		return status;
	}

	initLatency += estimate_max_scheduling_latency();

	BTimeSource* timeSource = fMediaRoster->MakeTimeSourceFor(fProducerNode);
	bool running = timeSource->IsRunning();

	/* workaround for people without sound cards */
	/* because the system time source won't be running */
	bigtime_t real = BTimeSource::RealTime();
	if (!running) {
		status = fMediaRoster->StartTimeSource(fTimeSourceNode, real);
		if (status != B_OK) {
			timeSource->Release();
			ErrorAlert(B_TRANSLATE("Cannot start time source!"), status);
			return status;
		}
		status = fMediaRoster->SeekTimeSource(fTimeSourceNode, 0, real);
		if (status != B_OK) {
			timeSource->Release();
			ErrorAlert(B_TRANSLATE("Cannot seek time source!"), status);
			return status;
		}
	}

	bigtime_t perf = timeSource->PerformanceTimeFor(real + latency 
		+ initLatency);
	timeSource->Release();

	/* start the nodes */
	status = fMediaRoster->StartNode(fProducerNode, perf);
	if (status != B_OK) {
		ErrorAlert(B_TRANSLATE("Cannot start the video source"), status);
		return status;
	}
	status = fMediaRoster->StartNode(fVideoConsumer->Node(), perf);
	if (status != B_OK) {
		ErrorAlert(B_TRANSLATE("Cannot start the video window"), status);
		return status;
	}

	return status;
}


void
CodyCam::_TearDownNodes()
{
	CALL("CodyCam::_TearDownNodes\n");
	if (!fMediaRoster)
		return;

	if (fVideoConsumer) {
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


//	#pragma mark - Video Window Class


VideoWindow::VideoWindow(BRect frame, const char* title, window_type type,
		uint32 flags, port_id* consumerPort)
	:
	BWindow(frame, title, type, flags),
	fPortPtr(consumerPort),
	fView(NULL),
	fVideoView(NULL)
{
	fFtpInfo.port = 0;
	fFtpInfo.rate = 0x7fffffff;
	fFtpInfo.imageFormat = 0;
	fFtpInfo.translator = 0;
	fFtpInfo.passiveFtp = true;
	fFtpInfo.uploadClient = 0;
	strcpy(fFtpInfo.fileNameText, "filename");
	strcpy(fFtpInfo.serverText, "server");
	strcpy(fFtpInfo.loginText, "login");
	strcpy(fFtpInfo.passwordText, "password");
	strcpy(fFtpInfo.directoryText, "directory");

	_SetUpSettings("codycam", "");

	BMenuBar* menuBar = new BMenuBar(BRect(0, 0, 0, 0), "menu bar");

	BMenuItem* menuItem;
	BMenu* menu = new BMenu(B_TRANSLATE("File"));

	menuItem = new BMenuItem(B_TRANSLATE("Video settings"), 
		new BMessage(msg_video), 'P');
	menuItem->SetTarget(be_app);
	menu->AddItem(menuItem);

	menu->AddSeparatorItem();

	menuItem = new BMenuItem(B_TRANSLATE("Start video"), 
		new BMessage(msg_start), 'A');
	menuItem->SetTarget(be_app);
	menu->AddItem(menuItem);

	menuItem = new BMenuItem(B_TRANSLATE("Stop video"), 
		new BMessage(msg_stop), 'O');
	menuItem->SetTarget(be_app);
	menu->AddItem(menuItem);

	menu->AddSeparatorItem();

	menuItem = new BMenuItem(B_TRANSLATE("About Codycam" B_UTF8_ELLIPSIS),
		new BMessage(msg_about), 'B');
	menuItem->SetTarget(be_app);
	menu->AddItem(menuItem);

	menu->AddSeparatorItem();

	menuItem = new BMenuItem(B_TRANSLATE("Quit"), 
		new BMessage(B_QUIT_REQUESTED), 'Q');
	menuItem->SetTarget(be_app);
	menu->AddItem(menuItem);

	menuBar->AddItem(menu);

	/* give it a gray background view */
	fView = new BView(B_TRANSLATE("Background View"), B_WILL_DRAW);
 	fView->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));

	/* add some controls */
	_BuildCaptureControls(fView);

	SetLayout(new BGroupLayout(B_VERTICAL));
	AddChild(menuBar);
	AddChild(fView);

	Show();
}



VideoWindow::~VideoWindow()
{
	_QuitSettings();
}



bool
VideoWindow::QuitRequested()
{
	be_app->PostMessage(B_QUIT_REQUESTED);
	return false;
}


void
VideoWindow::MessageReceived(BMessage* message)
{
	BControl* control = NULL;
	message->FindPointer((const char*)"source", (void **)&control);

	switch (message->what) {
		case msg_filename:
			if (control != NULL) {
				strncpy(fFtpInfo.fileNameText, ((BTextControl*)control)->Text(), 63);
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
			message->FindInt32("be:type", (int32*)&(fFtpInfo.imageFormat));
			message->FindInt32("be:translator", &(fFtpInfo.translator));
			break;

		case msg_upl_client:
			message->FindInt32("client", &(fFtpInfo.uploadClient));
			FTPINFO("upl client = %ld\n", fFtpInfo.uploadClient);
			_UploadClientChanged();
			break;

		case msg_server:
			if (control != NULL) {
				strncpy(fFtpInfo.serverText, ((BTextControl*)control)->Text(), 64);
				FTPINFO("server = '%s'\n", fFtpInfo.serverText);
			}
			break;

		case msg_login:
			if (control != NULL) {
				strncpy(fFtpInfo.loginText, ((BTextControl*)control)->Text(), 64);
				FTPINFO("login = '%s'\n", fFtpInfo.loginText);
			}
			break;

		case msg_password:
			if (control != NULL) {
				strncpy(fFtpInfo.passwordText, ((BTextControl*)control)->Text(), 64);
				FTPINFO("password = '%s'\n", fFtpInfo.passwordText);
			}
			break;

		case msg_directory:
			if (control != NULL) {
				strncpy(fFtpInfo.directoryText, ((BTextControl*)control)->Text(), 64);
				FTPINFO("directory = '%s'\n", fFtpInfo.directoryText);
			}
			break;

		case msg_passiveftp:
			if (control != NULL) {
				fFtpInfo.passiveFtp = ((BCheckBox*)control)->Value();
				if (fFtpInfo.passiveFtp)
					FTPINFO("using passive ftp\n");
			}
			break;

		default:
			BWindow::MessageReceived(message);
			return;
	}

	if (*fPortPtr)
		write_port(*fPortPtr, FTP_INFO, (void*)&fFtpInfo, sizeof(ftp_msg_info));

}


BView*
VideoWindow::VideoView()
{
	return fVideoView;
}


BStringView*
VideoWindow::StatusLine()
{
	return fStatusLine;
}


void
VideoWindow::_BuildCaptureControls(BView* theView)
{
	// a view to hold the video image
	fVideoView = new BView("Video View", B_WILL_DRAW);
	fVideoView->SetExplicitMinSize(BSize(VIDEO_SIZE_X, VIDEO_SIZE_Y));
	fVideoView->SetExplicitMaxSize(BSize(VIDEO_SIZE_X, VIDEO_SIZE_Y));

	// Capture controls
	fCaptureSetupBox = new BBox("Capture Controls", B_WILL_DRAW);
	fCaptureSetupBox->SetLabel(B_TRANSLATE("Capture controls"));

	BGridLayout *controlsLayout = new BGridLayout(kXBuffer, 0);
	controlsLayout->SetInsets(10, 15, 5, 5);
	fCaptureSetupBox->SetLayout(controlsLayout);

	fFileName = new BTextControl("File Name", B_TRANSLATE("File name:"),
		fFilenameSetting->Value(), new BMessage(msg_filename));
	fFileName->SetTarget(BMessenger(NULL, this));

	fImageFormatMenu = new BPopUpMenu(B_TRANSLATE("Image Format Menu"));
	AddTranslationItems(fImageFormatMenu, B_TRANSLATOR_BITMAP);
	fImageFormatMenu->SetTargetForItems(this);

	if (fImageFormatSettings->Value()
		&& fImageFormatMenu->FindItem(fImageFormatSettings->Value()) != NULL) {
		fImageFormatMenu->FindItem(
			fImageFormatSettings->Value())->SetMarked(true);
	} else if (fImageFormatMenu->FindItem("JPEG image") != NULL)
		fImageFormatMenu->FindItem("JPEG image")->SetMarked(true);
	else
		fImageFormatMenu->ItemAt(0)->SetMarked(true);

	fImageFormatSelector = new BMenuField("Format", B_TRANSLATE("Format:"),
		fImageFormatMenu, NULL);

	fCaptureRateMenu = new BPopUpMenu(B_TRANSLATE("Capture Rate Menu"));
	fCaptureRateMenu->AddItem(new BMenuItem(kCaptureRate[0],
		new BMessage(msg_rate_15s)));
	fCaptureRateMenu->AddItem(new BMenuItem(kCaptureRate[1],
		new BMessage(msg_rate_30s)));
	fCaptureRateMenu->AddItem(new BMenuItem(kCaptureRate[2],
		new BMessage(msg_rate_1m)));
	fCaptureRateMenu->AddItem(new BMenuItem(kCaptureRate[3],
		new BMessage(msg_rate_5m)));
	fCaptureRateMenu->AddItem(new BMenuItem(kCaptureRate[4],
		new BMessage(msg_rate_10m)));
	fCaptureRateMenu->AddItem(new BMenuItem(kCaptureRate[5],
		new BMessage(msg_rate_15m)));
	fCaptureRateMenu->AddItem(new BMenuItem(kCaptureRate[6],
		new BMessage(msg_rate_30m)));
	fCaptureRateMenu->AddItem(new BMenuItem(kCaptureRate[7],
		new BMessage(msg_rate_1h)));
	fCaptureRateMenu->AddItem(new BMenuItem(kCaptureRate[8],
		new BMessage(msg_rate_2h)));
	fCaptureRateMenu->AddItem(new BMenuItem(kCaptureRate[9],
		new BMessage(msg_rate_4h)));
	fCaptureRateMenu->AddItem(new BMenuItem(kCaptureRate[10],
		new BMessage(msg_rate_8h)));
	fCaptureRateMenu->AddItem(new BMenuItem(kCaptureRate[11],
		new BMessage(msg_rate_24h)));
	fCaptureRateMenu->AddItem(new BMenuItem(kCaptureRate[12],
		new BMessage(msg_rate_never)));
	fCaptureRateMenu->SetTargetForItems(this);
	fCaptureRateMenu->FindItem(fCaptureRateSetting->Value())->SetMarked(true);
	fCaptureRateSelector = new BMenuField("Rate", B_TRANSLATE("Rate:"),
		fCaptureRateMenu, NULL);

	controlsLayout->AddItem(fFileName->CreateLabelLayoutItem(), 0, 0);
	controlsLayout->AddItem(fFileName->CreateTextViewLayoutItem(), 1, 0);
	controlsLayout->AddItem(fImageFormatSelector->CreateLabelLayoutItem(), 0, 
		1);
	controlsLayout->AddItem(fImageFormatSelector->CreateMenuBarLayoutItem(), 1,
		1);
	controlsLayout->AddItem(fCaptureRateSelector->CreateLabelLayoutItem(), 0,
		2);
	controlsLayout->AddItem(fCaptureRateSelector->CreateMenuBarLayoutItem(), 1,
		2);
	controlsLayout->AddItem(BSpaceLayoutItem::CreateGlue(), 0, 3, 2);

	// FTP setup box
	fFtpSetupBox = new BBox(B_TRANSLATE("FTP Setup"), B_WILL_DRAW);

	fUploadClientMenu = new BPopUpMenu(B_TRANSLATE("Send to" B_UTF8_ELLIPSIS));
	for (int i = 0; kUploadClient[i]; i++) {
		BMessage *m = new BMessage(msg_upl_client);
		m->AddInt32("client", i);
		fUploadClientMenu->AddItem(new BMenuItem(kUploadClient[i], m));
	}
	fUploadClientMenu->SetTargetForItems(this);
	fUploadClientMenu->FindItem(fUploadClientSetting->Value())->SetMarked(true);
	fUploadClientSelector = new BMenuField("UploadClient", NULL,
		fUploadClientMenu, NULL);

	fFtpSetupBox->SetLabel(B_TRANSLATE("Output"));
	// this doesn't work with the layout manager
	// fFtpSetupBox->SetLabel(fUploadClientSelector);
	fUploadClientSelector->SetLabel(B_TRANSLATE("Type:"));

	BGridLayout *ftpLayout = new BGridLayout(kXBuffer, 0);
	ftpLayout->SetInsets(10, 15, 5, 5);
	fFtpSetupBox->SetLayout(ftpLayout);

	fServerName = new BTextControl("Server", B_TRANSLATE("Server:"),
		fServerSetting->Value(), new BMessage(msg_server));
	fServerName->SetTarget(this);

	fLoginId = new BTextControl("Login", B_TRANSLATE("Login:"),
		fLoginSetting->Value(), new BMessage(msg_login));
	fLoginId->SetTarget(this);

	fPassword = new BTextControl("Password", B_TRANSLATE("Password:"),
		fPasswordSetting->Value(), new BMessage(msg_password));
	fPassword->SetTarget(this);
	fPassword->TextView()->HideTyping(true);
	// BeOS HideTyping() seems broken, it empties the text
	fPassword->SetText(fPasswordSetting->Value());

	fDirectory = new BTextControl("Directory", B_TRANSLATE("Directory:"),
		fDirectorySetting->Value(), new BMessage(msg_directory));
	fDirectory->SetTarget(this);

	fPassiveFtp = new BCheckBox("Passive FTP", B_TRANSLATE("Passive FTP"),
		new BMessage(msg_passiveftp));
	fPassiveFtp->SetTarget(this);
	fPassiveFtp->SetValue(fPassiveFtpSetting->Value());

	ftpLayout->AddItem(fUploadClientSelector->CreateLabelLayoutItem(), 0, 0);
	ftpLayout->AddItem(fUploadClientSelector->CreateMenuBarLayoutItem(), 1, 0);
	ftpLayout->AddItem(fServerName->CreateLabelLayoutItem(), 0, 1);
	ftpLayout->AddItem(fServerName->CreateTextViewLayoutItem(), 1, 1);
	ftpLayout->AddItem(fLoginId->CreateLabelLayoutItem(), 0, 2);
	ftpLayout->AddItem(fLoginId->CreateTextViewLayoutItem(), 1, 2);
	ftpLayout->AddItem(fPassword->CreateLabelLayoutItem(), 0, 3);
	ftpLayout->AddItem(fPassword->CreateTextViewLayoutItem(), 1, 3);
	ftpLayout->AddItem(fDirectory->CreateLabelLayoutItem(), 0, 4);
	ftpLayout->AddItem(fDirectory->CreateTextViewLayoutItem(), 1, 4);
	ftpLayout->AddView(fPassiveFtp, 0, 5, 2);

	fStatusLine = new BStringView("Status Line", B_TRANSLATE("Waiting" B_UTF8_ELLIPSIS));

	BGroupLayout *groupLayout = new BGroupLayout(B_VERTICAL);
	groupLayout->SetInsets(kXBuffer, kYBuffer, kXBuffer, kYBuffer);

	theView->SetLayout(groupLayout);

	theView->AddChild(BSpaceLayoutItem::CreateVerticalStrut(kYBuffer));
	theView->AddChild(BGroupLayoutBuilder(B_HORIZONTAL)
		.Add(BSpaceLayoutItem::CreateHorizontalStrut(0.0))
		.Add(fVideoView)
		.Add(BSpaceLayoutItem::CreateHorizontalStrut(0.0))
	);
	theView->AddChild(BSpaceLayoutItem::CreateVerticalStrut(kYBuffer));
	theView->AddChild(BGroupLayoutBuilder(B_HORIZONTAL, kXBuffer)
		.Add(fCaptureSetupBox)
		.Add(fFtpSetupBox)
	);
	theView->AddChild(BSpaceLayoutItem::CreateVerticalStrut(kYBuffer));
	theView->AddChild(fStatusLine);
}


void
VideoWindow::ApplyControls()
{
	if (!Lock())
		return;

	// apply controls
	fFileName->Invoke();
	PostMessage(fImageFormatMenu->FindMarked()->Message());
	PostMessage(fCaptureRateMenu->FindMarked()->Message());
	PostMessage(fUploadClientMenu->FindMarked()->Message());
	fServerName->Invoke();
	fLoginId->Invoke();
	fPassword->Invoke();
	fDirectory->Invoke();
	fPassiveFtp->Invoke();

	Unlock();
}


void
VideoWindow::_SetUpSettings(const char* filename, const char* dirname)
{
	fSettings = new Settings(filename, dirname);

	fServerSetting = new StringValueSetting("Server", "ftp.my.server",
		B_TRANSLATE("server address expected"), "");
	fLoginSetting = new StringValueSetting("Login", "loginID",
		B_TRANSLATE("login ID expected"), "");
	fPasswordSetting = new StringValueSetting("Password", 
		B_TRANSLATE("password"), B_TRANSLATE("password expected"), "");
	fDirectorySetting = new StringValueSetting("Directory", "web/images",
		B_TRANSLATE("destination directory expected"), "");
	fPassiveFtpSetting = new BooleanValueSetting("PassiveFtp", 1);
	fFilenameSetting = new StringValueSetting("StillImageFilename",
		"codycam.jpg", B_TRANSLATE("still image filename expected"), "");
	fImageFormatSettings = new StringValueSetting("ImageFileFormat",
		B_TRANSLATE("JPEG image"), B_TRANSLATE("image file format expected"), 
		"");
	fCaptureRateSetting = new EnumeratedStringValueSetting("CaptureRate",
		kCaptureRate[3], kCaptureRate, B_TRANSLATE("capture rate expected"),
		"unrecognized capture rate specified");
	fUploadClientSetting = new EnumeratedStringValueSetting("UploadClient",
		B_TRANSLATE("FTP"), kUploadClient, 
		B_TRANSLATE("upload client name expected"),
		B_TRANSLATE("unrecognized upload client specified"));

	fSettings->Add(fServerSetting);
	fSettings->Add(fLoginSetting);
	fSettings->Add(fPasswordSetting);
	fSettings->Add(fDirectorySetting);
	fSettings->Add(fPassiveFtpSetting);
	fSettings->Add(fFilenameSetting);
	fSettings->Add(fImageFormatSettings);
	fSettings->Add(fCaptureRateSetting);
	fSettings->Add(fUploadClientSetting);

	fSettings->TryReadingSettings();
}


void
VideoWindow::_UploadClientChanged()
{
	bool enableServerControls = fFtpInfo.uploadClient < 2;
	fServerName->SetEnabled(enableServerControls);
	fLoginId->SetEnabled(enableServerControls);
	fPassword->SetEnabled(enableServerControls);
	fDirectory->SetEnabled(enableServerControls);
	fPassiveFtp->SetEnabled(enableServerControls);
}


void
VideoWindow::_QuitSettings()
{
	fServerSetting->ValueChanged(fServerName->Text());
	fLoginSetting->ValueChanged(fLoginId->Text());
	fPasswordSetting->ValueChanged(fFtpInfo.passwordText);
	fDirectorySetting->ValueChanged(fDirectory->Text());
	fPassiveFtpSetting->ValueChanged(fPassiveFtp->Value());
	fFilenameSetting->ValueChanged(fFileName->Text());
	fImageFormatSettings->ValueChanged(fImageFormatMenu->FindMarked()->Label());
	fCaptureRateSetting->ValueChanged(fCaptureRateMenu->FindMarked()->Label());
	fUploadClientSetting->ValueChanged(fUploadClientMenu->FindMarked()->Label());

	fSettings->SaveSettings();
	delete fSettings;
}


//	#pragma mark -


ControlWindow::ControlWindow(const BRect& frame, BView* controls, 
	media_node node)
	: BWindow(frame, B_TRANSLATE("Video settings"), B_TITLED_WINDOW, 
	B_ASYNCHRONOUS_CONTROLS)
{
	fView = controls;
	fNode = node;

	AddChild(fView);
}


void
ControlWindow::MessageReceived(BMessage* message)
{
	BParameterWeb* web = NULL;
	status_t err;

	switch (message->what) {
		case B_MEDIA_WEB_CHANGED:
		{
			// If this is a tab view, find out which tab
			// is selected
			BTabView* tabView = dynamic_cast<BTabView*>(fView);
			int32 tabNum = -1;
			if (tabView)
				tabNum = tabView->Selection();

			RemoveChild(fView);
			delete fView;

			err = BMediaRoster::Roster()->GetParameterWebFor(fNode, &web);

			if (err >= B_OK && web != NULL) {
				fView = BMediaTheme::ViewFor(web);
				AddChild(fView);

				// Another tab view?  Restore previous selection
				if (tabNum > 0) {
					BTabView* newTabView = dynamic_cast<BTabView*>(fView);
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


bool
ControlWindow::QuitRequested()
{
	be_app->PostMessage(msg_control_win);
	return true;
}


//	#pragma mark -


int main() {
	CodyCam app;
	app.Run();
	return 0;
}

