/*
 * Copyright 2008-09, Oliver Ruiz Dorantes, <oliver.ruiz.dorantes_at_gmail.com>
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include <Alert.h>
#include <Button.h>
#include <Catalog.h>
#include <LayoutBuilder.h>
#include <ListView.h>
#include <ListItem.h>
#include <MessageRunner.h>
#include <ScrollView.h>
#include <StatusBar.h>
#include <SpaceLayoutItem.h>
#include <TextView.h>
#include <TabView.h>

#include <bluetooth/bdaddrUtils.h>
#include <bluetooth/DiscoveryAgent.h>
#include <bluetooth/DiscoveryListener.h>
#include <bluetooth/LocalDevice.h>

#include "defs.h"
#include "DeviceListItem.h"
#include "InquiryPanel.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Inquiry panel"

using Bluetooth::DeviceListItem;

// private funcionaility provided by kit
extern uint8 GetInquiryTime();

static const uint32 kMsgStart = 'InSt';
static const uint32 kMsgFinish = 'InFn';
static const uint32 kMsgShowDebug = 'ShDG';

static const uint32 kMsgInquiry = 'iQbt';
static const uint32 kMsgAddListDevice = 'aDdv';

static const uint32 kMsgSelected = 'isLt';
static const uint32 kMsgSecond = 'sCMs';
static const uint32 kMsgRetrieve = 'IrEt';


class PanelDiscoveryListener : public DiscoveryListener {

public:

	PanelDiscoveryListener(InquiryPanel* iPanel)
		:
		DiscoveryListener(),
		fInquiryPanel(iPanel)
	{

	}


	void
	DeviceDiscovered(RemoteDevice* btDevice, DeviceClass cod)
	{
		BMessage* message = new BMessage(kMsgAddListDevice);

		message->AddPointer("remoteItem", new DeviceListItem(btDevice));

		fInquiryPanel->PostMessage(message);
	}


	void
	InquiryCompleted(int discType)
	{
		BMessage* message = new BMessage(kMsgFinish);
		fInquiryPanel->PostMessage(message);
	}


	void
	InquiryStarted(status_t status)
	{
		BMessage* message = new BMessage(kMsgStart);
		fInquiryPanel->PostMessage(message);
	}

private:
	InquiryPanel*	fInquiryPanel;

};


InquiryPanel::InquiryPanel(BRect frame, LocalDevice* lDevice)
	:
	BWindow(frame, B_TRANSLATE_SYSTEM_NAME("Bluetooth"), B_FLOATING_WINDOW,
	B_NOT_ZOOMABLE | B_AUTO_UPDATE_SIZE_LIMITS,	B_ALL_WORKSPACES ),
	fMessenger(this),
 	fScanning(false),
 	fRetrieving(false),
	fLocalDevice(lDevice)

{
	fScanProgress = new BStatusBar("status",
		B_TRANSLATE("Scanning progress"), "");
	activeColor = fScanProgress->BarColor();

	if (fLocalDevice == NULL)
		fLocalDevice = LocalDevice::GetLocalDevice();

	fMessage = new BTextView("description", B_WILL_DRAW);
	fMessage->SetViewUIColor(B_PANEL_BACKGROUND_COLOR);
	fMessage->SetLowColor(fMessage->ViewColor());
	fMessage->MakeEditable(false);
	fMessage->MakeSelectable(false);

	fInquiryButton = new BButton("Inquiry", B_TRANSLATE("Inquiry"),
		new BMessage(kMsgInquiry), B_WILL_DRAW);

	fAddButton = new BButton("add", B_TRANSLATE("Add device to list"),
		new BMessage(kMsgAddToRemoteList), B_WILL_DRAW);
	fAddButton->SetEnabled(false);

	fRemoteList = new BListView("AttributeList", B_SINGLE_SELECTION_LIST);
	fRemoteList->SetSelectionMessage(new BMessage(kMsgSelected));

	fScrollView = new BScrollView("ScrollView", fRemoteList, 0, false, true);
	fScrollView->SetViewUIColor(B_PANEL_BACKGROUND_COLOR);

	if (fLocalDevice != NULL) {
		fMessage->SetText(B_TRANSLATE(
			"Check that the Bluetooth capabilities of your"
			" remote device are activated. Press 'Inquiry' to start scanning."
			" The needed time for the retrieval of the names is unknown, "
			"although should not take more than 3 seconds per device. "
			"Afterwards you will be able to add them to your main list,"
			" where you will be able to pair with them."));
		fInquiryButton->SetEnabled(true);
		fDiscoveryAgent = fLocalDevice->GetDiscoveryAgent();
		fDiscoveryListener = new PanelDiscoveryListener(this);

		SetTitle((const char*)(fLocalDevice->GetFriendlyName().String()));
	} else {
		fMessage->SetText(B_TRANSLATE("There isn't any Bluetooth LocalDevice "
			"registered on the system."));
		fInquiryButton->SetEnabled(false);
	}

	fRetrieveMessage = new BMessage(kMsgRetrieve);
	fSecondsMessage = new BMessage(kMsgSecond);

	BLayoutBuilder::Group<>(this, B_VERTICAL, 10)
		.SetInsets(15)
		.Add(fMessage)
		.Add(fScanProgress, 10)
		.Add(fScrollView)
		.AddGroup(B_HORIZONTAL, 10)
			.Add(fAddButton)
			.AddGlue()
			.Add(fInquiryButton)
		.End()
	.End();
}


void
InquiryPanel::MessageReceived(BMessage* message)
{
	static float timer = 0; // expected time of the inquiry process
	static float scanningTime = 0;
	static int32 retrievalIndex = 0;
	static bool labelPlaced = false;

	switch (message->what) {
		case kMsgInquiry:

			fDiscoveryAgent->StartInquiry(BT_GIAC, fDiscoveryListener, GetInquiryTime());

			timer = BT_BASE_INQUIRY_TIME * GetInquiryTime() + 1;
			// does it works as expected?
			fScanProgress->SetMaxValue(timer);

		break;

		case kMsgAddListDevice:
		{
			DeviceListItem* listItem;

			message->FindPointer("remoteItem", (void **)&listItem);

			fRemoteList->AddItem(listItem);
		}
		break;

		case kMsgAddToRemoteList:
		{
			message->PrintToStream();
			int32 index = fRemoteList->CurrentSelection(0);
			DeviceListItem* item = (DeviceListItem*) fRemoteList->RemoveItem(index);;

			BMessage message(kMsgAddToRemoteList);
			message.AddPointer("device", item);

			be_app->PostMessage(&message);
			// TODO: all others listitems can be deleted
		}
		break;

		case kMsgSelected:
			UpdateListStatus();
		break;

		case kMsgStart:
			fRemoteList->MakeEmpty();
			fScanProgress->Reset();
			fScanProgress->SetTo(1);
			fScanProgress->SetTrailingText(B_TRANSLATE("Starting scan"
				B_UTF8_ELLIPSIS));
			fScanProgress->SetBarColor(activeColor);

			fAddButton->SetEnabled(false);
			fInquiryButton->SetEnabled(false);

			BMessageRunner::StartSending(fMessenger, fSecondsMessage, 1000000, timer);

			scanningTime = 1;
			fScanning = true;

		break;

		case kMsgFinish:

			retrievalIndex = 0;
			fScanning = false;
			fRetrieving = true;
			labelPlaced = false;
			fScanProgress->SetTo(100);
			fScanProgress->SetTrailingText(B_TRANSLATE("Retrieving names"
				B_UTF8_ELLIPSIS));
			BMessageRunner::StartSending(fMessenger, fRetrieveMessage, 1000000, 1);

		break;

		case kMsgSecond:
			if (fScanning && scanningTime < timer) {
				// TODO time formatting could use Locale Kit

				// TODO should not be needed if SetMaxValue works...
				fScanProgress->SetTo(scanningTime * 100 / timer);
				BString elapsedTime = B_TRANSLATE("Remaining %1 seconds");

				BString seconds("");
				seconds << (int)(timer - scanningTime);

				elapsedTime.ReplaceFirst("%1", seconds.String());
				fScanProgress->SetTrailingText(elapsedTime.String());

				scanningTime = scanningTime + 1;
			}
		break;

		case kMsgRetrieve:

			if (fRetrieving) {

				if (retrievalIndex < fDiscoveryAgent->RetrieveDevices(0).CountItems()) {

					if (!labelPlaced) {

						labelPlaced = true;
						BString progressText(B_TRANSLATE("Retrieving name of %1"));

						BString namestr;
						namestr << bdaddrUtils::ToString(fDiscoveryAgent
							->RetrieveDevices(0).ItemAt(retrievalIndex)
							->GetBluetoothAddress());
						progressText.ReplaceFirst("%1", namestr.String());
						fScanProgress->SetTrailingText(progressText.String());

					} else {
						// Really erally expensive operation should be done in a separate thread
						// once Haiku gets a BarberPole in API replacing the progress bar
						((DeviceListItem*)fRemoteList->ItemAt(retrievalIndex))
							->SetDevice((BluetoothDevice*) fDiscoveryAgent
							->RetrieveDevices(0).ItemAt(retrievalIndex));
						fRemoteList->InvalidateItem(retrievalIndex);

						retrievalIndex++;
						labelPlaced = false;
					}

					BMessageRunner::StartSending(fMessenger, fRetrieveMessage, 500000, 1);

				} else {

					fRetrieving = false;
					retrievalIndex = 0;

					fScanProgress->SetBarColor(
						ui_color(B_PANEL_BACKGROUND_COLOR));
					fScanProgress->SetTrailingText(
						B_TRANSLATE("Scanning completed."));
					fInquiryButton->SetEnabled(true);
					UpdateListStatus();
				}
			}

		break;

		default:
			BWindow::MessageReceived(message);
			break;
	}
}


void
InquiryPanel::UpdateListStatus(void)
{
	if (fRemoteList->CurrentSelection() < 0 || fScanning || fRetrieving)
		fAddButton->SetEnabled(false);
	else
		fAddButton->SetEnabled(true);
}


bool
InquiryPanel::QuitRequested(void)
{

	return true;
}
