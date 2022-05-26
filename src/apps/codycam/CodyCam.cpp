/*
 * Copyright 1998-1999 Be, Inc. All Rights Reserved.
 * Copyright 2003-2019 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include "CodyCam.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <Alert.h>
#include <Button.h>
#include <Catalog.h>
#include <FindDirectory.h>
#include <LayoutBuilder.h>
#include <MediaDefs.h>
#include <MediaNode.h>
#include <MediaRoster.h>
#include <MediaTheme.h>
#include <Menu.h>
#include <MenuBar.h>
#include <MenuItem.h>
#include <Path.h>
#include <PopUpMenu.h>
#include <scheduler.h>
#include <TabView.h>
#include <TextControl.h>
#include <TimeSource.h>
#include <TranslationUtils.h>
#include <TranslatorFormats.h>

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "CodyCam"

#define VIDEO_SIZE_X 320
#define VIDEO_SIZE_Y 240

#define WINDOW_SIZE_X (VIDEO_SIZE_X + 80)
#define WINDOW_SIZE_Y (VIDEO_SIZE_Y + 230)

#define WINDOW_OFFSET_X 28
#define WINDOW_OFFSET_Y 28

#define	CALL		printf
#define ERROR		printf
#define FTPINFO		printf
#define	INFO		printf


// Utility functions

namespace {

// functions for EnumeratedStringValueSettings

const char*
CaptureRateAt(int32 i)
{
	return (i >= 0 && i < kCaptureRatesCount) ? kCaptureRates[i].name : NULL;
}


const char*
UploadClientAt(int32 i)
{
	return (i >= 0 && i < kUploadClientsCount) ? kUploadClients[i] : NULL;
}


}; // end anonymous namespace



//	#pragma mark -


CodyCam::CodyCam()
	:
	BApplication("application/x-vnd.Haiku-CodyCam"),
	fMediaRoster(NULL),
	fVideoConsumer(NULL),
	fWindow(NULL),
	fPort(0),
	fVideoControlWindow(NULL)
{
	int32 index = 0;
	kCaptureRates[index++].name = B_TRANSLATE("Every 15 seconds");
	kCaptureRates[index++].name = B_TRANSLATE("Every 30 seconds");
	kCaptureRates[index++].name = B_TRANSLATE("Every minute");
	kCaptureRates[index++].name = B_TRANSLATE("Every 5 minutes");
	kCaptureRates[index++].name = B_TRANSLATE("Every 10 minutes");
	kCaptureRates[index++].name = B_TRANSLATE("Every 15 minutes");
	kCaptureRates[index++].name = B_TRANSLATE("Every 30 minutes");
	kCaptureRates[index++].name = B_TRANSLATE("Every hour");
	kCaptureRates[index++].name = B_TRANSLATE("Every 2 hours");
	kCaptureRates[index++].name = B_TRANSLATE("Every 4 hours");
	kCaptureRates[index++].name = B_TRANSLATE("Every 8 hours");
	kCaptureRates[index++].name = B_TRANSLATE("Every 24 hours");
	kCaptureRates[index++].name = B_TRANSLATE("Never");

	index = 0;
	kUploadClients[index++] = B_TRANSLATE("FTP");
	kUploadClients[index++] = B_TRANSLATE("SFTP");
	kUploadClients[index++] = B_TRANSLATE("Local");

	BPath homeDir;
	if (find_directory(B_USER_DIRECTORY, &homeDir) != B_OK)
		homeDir.SetTo("/boot/home");

	chdir(homeDir.Path());
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
	fWindow = new VideoWindow(
		(const char*) B_TRANSLATE_SYSTEM_NAME("CodyCam"), B_TITLED_WINDOW,
		B_NOT_ZOOMABLE | B_NOT_V_RESIZABLE
		| B_AUTO_UPDATE_SIZE_LIMITS, &fPort);

	if (_SetUpNodes() != B_OK)
		fWindow->ToggleMenuOnOff();

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
CodyCam::MessageReceived(BMessage* message)
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
				view->SetResizingMode(B_FOLLOW_ALL_SIDES);
				fVideoControlWindow = new ControlWindow(view, node);
				fMediaRoster->StartWatching(BMessenger(NULL,
					fVideoControlWindow), node,	B_MEDIA_WEB_CHANGED);
				fVideoControlWindow->Show();
			}
			break;
		}

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
		fWindow->ErrorAlert(B_TRANSLATE("Cannot find the media roster"),
			status);
		return status;
	}

	/* find the time source */
	status = fMediaRoster->GetTimeSource(&fTimeSourceNode);
	if (status != B_OK) {
		fWindow->ErrorAlert(B_TRANSLATE("Cannot get a time source"), status);
		return status;
	}

	/* find a video producer node */
	INFO("CodyCam acquiring VideoInput node\n");
	status = fMediaRoster->GetVideoInput(&fProducerNode);
	if (status != B_OK) {
		fWindow->ErrorAlert(B_TRANSLATE("Cannot find a video source.\n"
			"You need a webcam to use CodyCam."), status);
		return status;
	}

	/* create the video consumer node */
	fVideoConsumer = new VideoConsumer("CodyCam",
		((VideoWindow*)fWindow)->VideoView(),
		((VideoWindow*)fWindow)->StatusLine(), NULL, 0);
	if (fVideoConsumer == NULL) {
		fWindow->ErrorAlert(B_TRANSLATE("Cannot create a video window"),
			B_ERROR);
		return B_ERROR;
	}

	/* register the node */
	status = fMediaRoster->RegisterNode(fVideoConsumer);
	if (status != B_OK) {
		fWindow->ErrorAlert(B_TRANSLATE("Cannot register the video window"),
			status);
		return status;
	}
	fPort = fVideoConsumer->ControlPort();

	/* find free producer output */
	int32 cnt = 0;
	status = fMediaRoster->GetFreeOutputsFor(fProducerNode, &fProducerOut, 1,
		&cnt, B_MEDIA_RAW_VIDEO);
	if (status != B_OK || cnt < 1) {
		status = B_RESOURCE_UNAVAILABLE;
		fWindow->ErrorAlert(
			B_TRANSLATE("Cannot find an available video stream"), status);
		return status;
	}

	/* find free consumer input */
	cnt = 0;
	status = fMediaRoster->GetFreeInputsFor(fVideoConsumer->Node(),
		&fConsumerIn, 1, &cnt, B_MEDIA_RAW_VIDEO);
	if (status != B_OK || cnt < 1) {
		status = B_RESOURCE_UNAVAILABLE;
		fWindow->ErrorAlert(B_TRANSLATE("Can't find an available connection to "
			"the video window"), status);
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
		fWindow->ErrorAlert(B_TRANSLATE("Cannot connect the video source to "
			"the video window"), status);
		return status;
	}


	/* set time sources */
	status = fMediaRoster->SetTimeSourceFor(fProducerNode.node,
		fTimeSourceNode.node);
	if (status != B_OK) {
		fWindow->ErrorAlert(B_TRANSLATE("Cannot set the time source for the "
			"video source"), status);
		return status;
	}

	status = fMediaRoster->SetTimeSourceFor(fVideoConsumer->ID(),
		fTimeSourceNode.node);
	if (status != B_OK) {
		fWindow->ErrorAlert(B_TRANSLATE("Cannot set the time source for the "
			"video window"), status);
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
		fWindow->ErrorAlert(B_TRANSLATE("Error getting initial latency for the "
			"capture node"), status);
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
			fWindow->ErrorAlert(B_TRANSLATE("Cannot start time source!"),
				status);
			return status;
		}
		status = fMediaRoster->SeekTimeSource(fTimeSourceNode, 0, real);
		if (status != B_OK) {
			timeSource->Release();
			fWindow->ErrorAlert(B_TRANSLATE("Cannot seek time source!"),
				status);
			return status;
		}
	}

	bigtime_t perf = timeSource->PerformanceTimeFor(real + latency
		+ initLatency);
	timeSource->Release();

	/* start the nodes */
	status = fMediaRoster->StartNode(fProducerNode, perf);
	if (status != B_OK) {
		fWindow->ErrorAlert(B_TRANSLATE("Cannot start the video source"),
			status);
		return status;
	}
	status = fMediaRoster->StartNode(fVideoConsumer->Node(), perf);
	if (status != B_OK) {
		fWindow->ErrorAlert(B_TRANSLATE("Cannot start the video window"),
			status);
		return status;
	}

	return status;
}


void
CodyCam::_TearDownNodes()
{
	CALL("CodyCam::_TearDownNodes\n");
	if (fMediaRoster == NULL)
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


VideoWindow::VideoWindow(const char* title, window_type type,
		uint32 flags, port_id* consumerPort)
	:
	BWindow(BRect(50, 50, 50, 50), title, type, flags),
	fPortPtr(consumerPort),
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

	BMenuBar* menuBar = new BMenuBar("menu bar");

	BMenuItem* menuItem;
	fMenu = new BMenu(B_TRANSLATE("File"));

	menuItem = new BMenuItem(B_TRANSLATE("Video settings"),
		new BMessage(msg_video), 'P');
	menuItem->SetTarget(be_app);
	fMenu->AddItem(menuItem);

	fMenu->AddSeparatorItem();

	menuItem = new BMenuItem(B_TRANSLATE("Start video"),
		new BMessage(msg_start), 'A');
	menuItem->SetTarget(be_app);
	fMenu->AddItem(menuItem);

	menuItem = new BMenuItem(B_TRANSLATE("Stop video"),
		new BMessage(msg_stop), 'O');
	menuItem->SetTarget(be_app);
	fMenu->AddItem(menuItem);

	fMenu->AddSeparatorItem();

	menuItem = new BMenuItem(B_TRANSLATE("Quit"),
		new BMessage(B_QUIT_REQUESTED), 'Q');
	menuItem->SetTarget(be_app);
	fMenu->AddItem(menuItem);

	menuBar->AddItem(fMenu);

	/* add some controls */
	_BuildCaptureControls();

	BBox* box = new BBox("box");
	BGroupLayout* layout = new BGroupLayout(B_VERTICAL);
	box->SetLayout(layout);
	layout->SetInsets(2, 2, 2, 2);
	box->AddChild(fVideoView);
	box->AddChild(fErrorView);

	BLayoutBuilder::Group<>(this, B_VERTICAL, 0)
		.Add(menuBar)
		.AddGroup(B_VERTICAL)
			.SetInsets(B_USE_WINDOW_SPACING)
			.AddGroup(B_HORIZONTAL)
				.AddGlue()
				.Add(box)
				.AddGlue()
			.End()
			.AddGroup(B_HORIZONTAL, B_USE_DEFAULT_SPACING)
				.Add(fCaptureSetupBox)
				.Add(fFtpSetupBox)
				.End()
			.Add(fStatusLine)
			.End()
		.AddGlue();

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
	message->FindPointer((const char*)"source", (void**)&control);

	switch (message->what) {
		case msg_filename:
			if (control != NULL) {
				strlcpy(fFtpInfo.fileNameText,
					((BTextControl*)control)->Text(), 64);
				FTPINFO("file is '%s'\n", fFtpInfo.fileNameText);
			}
			break;

		case msg_rate_changed: {
			int32 seconds;
			message->FindInt32("seconds", &seconds);
			if (seconds == 0) {
				FTPINFO("never\n");
				fFtpInfo.rate = (bigtime_t)(B_INFINITE_TIMEOUT);
			} else {
				FTPINFO("%" B_PRId32 " seconds\n", seconds);
				fFtpInfo.rate = (bigtime_t)(seconds * 1000000LL);
			}
			break;
		}

		case msg_translate:
			message->FindInt32("be:type", (int32*)&(fFtpInfo.imageFormat));
			message->FindInt32("be:translator", &(fFtpInfo.translator));
			break;

		case msg_upl_client:
			message->FindInt32("client", &(fFtpInfo.uploadClient));
			FTPINFO("upl client = %" B_PRId32 "\n", fFtpInfo.uploadClient);
			_UploadClientChanged();
			break;

		case msg_server:
			if (control != NULL) {
				strlcpy(fFtpInfo.serverText,
					((BTextControl*)control)->Text(), 64);
				FTPINFO("server = '%s'\n", fFtpInfo.serverText);
			}
			break;

		case msg_login:
			if (control != NULL) {
				strlcpy(fFtpInfo.loginText,
					((BTextControl*)control)->Text(), 64);
				FTPINFO("login = '%s'\n", fFtpInfo.loginText);
			}
			break;

		case msg_password:
			if (control != NULL) {
				strlcpy(fFtpInfo.passwordText,
					((BTextControl*)control)->Text(), 64);
				FTPINFO("password = '%s'\n", fFtpInfo.passwordText);
			}
			break;

		case msg_directory:
			if (control != NULL) {
				strlcpy(fFtpInfo.directoryText,
					((BTextControl*)control)->Text(), 64);
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
VideoWindow::_BuildCaptureControls()
{
	// a view to hold the video image
	fVideoView = new BView("Video preview", B_WILL_DRAW);
	fVideoView->SetExplicitMinSize(BSize(VIDEO_SIZE_X, VIDEO_SIZE_Y));
	fVideoView->SetExplicitMaxSize(BSize(VIDEO_SIZE_X, VIDEO_SIZE_Y));

	fErrorView = new BTextView("error");
	fErrorView->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));

	// Capture controls
	BGridLayout* controlsLayout = new BGridLayout(B_USE_DEFAULT_SPACING,
		B_USE_SMALL_SPACING);
	controlsLayout->SetInsets(B_USE_SMALL_SPACING);

	BView* controlView = new BView("Controls", B_SUPPORTS_LAYOUT, NULL);
	controlView->SetLayout(controlsLayout);

	fCaptureSetupBox = new BBox("Capture Controls", B_WILL_DRAW);
	fCaptureSetupBox->SetLabel(B_TRANSLATE("Capture controls"));
	fCaptureSetupBox->AddChild(controlView);

	// file name
	fFileName = new BTextControl("File Name", B_TRANSLATE("File name:"),
		fFilenameSetting->Value(), new BMessage(msg_filename));
	fFileName->SetTarget(BMessenger(NULL, this));

	// format menu
	fImageFormatMenu = new BPopUpMenu(B_TRANSLATE("Image Format Menu"));
	BTranslationUtils::AddTranslationItems(fImageFormatMenu,
		B_TRANSLATOR_BITMAP);
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
		fImageFormatMenu);

	// capture rate
	fCaptureRateMenu = new BPopUpMenu(B_TRANSLATE("Capture Rate Menu"));
	for (int32 i = 0; i < kCaptureRatesCount; i++) {
		BMessage* itemMessage = new BMessage(msg_rate_changed);
		itemMessage->AddInt32("seconds", kCaptureRates[i].seconds);
		fCaptureRateMenu->AddItem(new BMenuItem(kCaptureRates[i].name,
			itemMessage));
	}
	fCaptureRateMenu->SetTargetForItems(this);
	fCaptureRateMenu->FindItem(fCaptureRateSetting->Value())->SetMarked(true);
	fCaptureRateSelector = new BMenuField("Rate", B_TRANSLATE("Rate:"),
		fCaptureRateMenu);

	BLayoutBuilder::Grid<>(controlsLayout)
		.AddTextControl(fFileName, 0, 0)
		.AddMenuField(fImageFormatSelector, 0, 1)
		.AddMenuField(fCaptureRateSelector, 0, 2)
		.Add(BSpaceLayoutItem::CreateGlue(), 0, 3, 2, 1);

	// FTP setup box
	BGridLayout* ftpLayout = new BGridLayout(B_USE_DEFAULT_SPACING,
		B_USE_SMALL_SPACING);
	ftpLayout->SetInsets(B_USE_SMALL_SPACING);

	BView* outputView = new BView("Output", B_SUPPORTS_LAYOUT, NULL);
	outputView->SetLayout(ftpLayout);

	fFtpSetupBox = new BBox("FTP Setup", B_WILL_DRAW);
	fFtpSetupBox->SetLabel(B_TRANSLATE("Output"));
	fFtpSetupBox->AddChild(outputView);
	float minWidth = be_plain_font->StringWidth(
		"The server label plus ftp.reasonably.com");
	fFtpSetupBox->SetExplicitMinSize(BSize(minWidth, B_SIZE_UNSET));
	fFtpSetupBox->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, B_SIZE_UNSET));

	fUploadClientMenu = new BPopUpMenu(B_TRANSLATE("Send to" B_UTF8_ELLIPSIS));
	for (int i = 0; i < kUploadClientsCount; i++) {
		BMessage* m = new BMessage(msg_upl_client);
		m->AddInt32("client", i);
		fUploadClientMenu->AddItem(new BMenuItem(kUploadClients[i], m));
	}

	fUploadClientMenu->SetTargetForItems(this);
	fUploadClientMenu->FindItem(fUploadClientSetting->Value())->SetMarked(true);

	fUploadClientSelector = new BMenuField("UploadClient", NULL,
		fUploadClientMenu);

	fUploadClientSelector->SetLabel(B_TRANSLATE("Type:"));

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
	fPassiveFtp->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, B_SIZE_UNSET));

	BLayoutBuilder::Grid<>(ftpLayout)
		.AddMenuField(fUploadClientSelector, 0, 0)
		.AddTextControl(fServerName, 0, 1)
		.AddTextControl(fLoginId, 0, 2)
		.AddTextControl(fPassword, 0, 3)
		.AddTextControl(fDirectory, 0, 4)
		.Add(fPassiveFtp, 0, 5, 2, 1);

	fStatusLine = new BStringView("Status Line",
		B_TRANSLATE("Waiting" B_UTF8_ELLIPSIS));
	fStatusLine->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, B_SIZE_UNSET));
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
VideoWindow::ErrorAlert(const char* message, status_t err)
{
	Lock();
	fErrorView->SetText(message);
	fErrorView->MakeEditable(false);
	fErrorView->MakeSelectable(false);
	fErrorView->SetWordWrap(true);
	fErrorView->SetAlignment(B_ALIGN_CENTER);
	fErrorView->SetExplicitMinSize(BSize(VIDEO_SIZE_X, VIDEO_SIZE_Y));
	fErrorView->SetExplicitMaxSize(BSize(VIDEO_SIZE_X, VIDEO_SIZE_Y));
	fErrorView->Show();
	fVideoView->Hide();
	Unlock();

	printf("%s\n%s [%" B_PRIx32 "]", message, strerror(err), err);
}


void
VideoWindow::_SetUpSettings(const char* filename, const char* dirname)
{
	fSettings = new Settings(filename, dirname);

	fServerSetting = new StringValueSetting("Server", "ftp.my.server",
		B_TRANSLATE("server address expected"));

	fLoginSetting = new StringValueSetting("Login", "loginID",
		B_TRANSLATE("login ID expected"));

	fPasswordSetting = new StringValueSetting("Password",
		B_TRANSLATE("password"), B_TRANSLATE("password expected"));

	fDirectorySetting = new StringValueSetting("Directory", "web/images",
		B_TRANSLATE("destination directory expected"));

	fPassiveFtpSetting = new BooleanValueSetting("PassiveFtp", 1);

	fFilenameSetting = new StringValueSetting("StillImageFilename",
		"codycam.jpg", B_TRANSLATE("still image filename expected"));

	fImageFormatSettings = new StringValueSetting("ImageFileFormat",
		B_TRANSLATE("JPEG image"), B_TRANSLATE("image file format expected"));

	fCaptureRateSetting = new EnumeratedStringValueSetting("CaptureRate",
		kCaptureRates[3].name, &CaptureRateAt,
		B_TRANSLATE("capture rate expected"),
		"unrecognized capture rate specified");

	fUploadClientSetting = new EnumeratedStringValueSetting("UploadClient",
		B_TRANSLATE("FTP"), &UploadClientAt,
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
	fUploadClientSetting->ValueChanged(
		fUploadClientMenu->FindMarked()->Label());

	fSettings->SaveSettings();
	delete fSettings;
}


void
VideoWindow::ToggleMenuOnOff()
{
	BMenuItem* item = fMenu->FindItem(msg_video);
	item->SetEnabled(!item->IsEnabled());

	item = fMenu->FindItem(msg_start);
	item->SetEnabled(!item->IsEnabled());

	item = fMenu->FindItem(msg_stop);
	item->SetEnabled(!item->IsEnabled());
}


//	#pragma mark -


ControlWindow::ControlWindow(BView* controls,
	media_node node)
	:
	BWindow(controls->Bounds().OffsetToSelf(100, 100),
		B_TRANSLATE("Video settings"), B_TITLED_WINDOW,
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
			break;
	}
}


bool
ControlWindow::QuitRequested()
{
	be_app->PostMessage(msg_control_win);
	return true;
}


//	#pragma mark -


int main()
{
	CodyCam app;
	app.Run();
	return 0;
}
