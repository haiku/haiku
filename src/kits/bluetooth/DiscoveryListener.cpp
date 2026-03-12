/*
 * Copyright 2007 Oliver Ruiz Dorantes, oliver.ruiz.dorantes_at_gmail.com
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#include <bluetooth/DiscoveryAgent.h>
#include <bluetooth/DiscoveryListener.h>
#include <bluetooth/RemoteDevice.h>
#include <bluetooth/DeviceClass.h>
#include <bluetooth/bdaddrUtils.h>
#include <bluetooth/debug.h>

#include <bluetooth/HCI/btHCI_event.h>

#include <bluetoothserver_p.h>

#include <Message.h>


namespace Bluetooth {


/* hooks */
void
DiscoveryListener::DeviceDiscovered(RemoteDevice* btDevice, DeviceClass cod)
{
	CALLED();
}


void
DiscoveryListener::InquiryStarted(status_t status)
{
	CALLED();
}


void
DiscoveryListener::InquiryCompleted(int discType)
{
	CALLED();
}


/* private */

/* A LocalDevice is always referenced in any request to the
 * Bluetooth server therefore is going to be needed in any
 */
void
DiscoveryListener::SetLocalDeviceOwner(LocalDevice* ld)
{
	CALLED();
	fLocalDevice = ld;
}


RemoteDevicesList
DiscoveryListener::GetRemoteDevicesList(void)
{
	CALLED();
	return fRemoteDevicesList;
}


void
DiscoveryListener::MessageReceived(BMessage* message)
{
	CALLED();
	int8 status;

	switch (message->what) {
		case BT_MSG_INQUIRY_DEVICE:
		{
			uint8 count = 0;
			if (message->FindUInt8("count", &count) != B_OK || count == 0)
				break;

			for (uint8 i = 0; i < count; i++) {
				ssize_t size;
				const bdaddr_t* bdaddr;
				const uint8* devClass;
				uint8 pageRepetitionMode = 0;
				uint8 scanPeriodMode = 0;
				// default value is 0 only, in newer specs this has been removed in such case it
				// should be set to zero
				uint8 scanMode = 0;
				uint16 clockOffset = 0;
				int8 rssi = HCI_RSSI_INVALID;
				BString friendlyName;
				bool friendlyNameIsComplete = false;


				if (message->FindData("bdaddr", B_ANY_TYPE, i, (const void**)&bdaddr, &size) != B_OK
					|| message->FindData("dev_class", B_ANY_TYPE, i, (const void**)&devClass, &size)
						!= B_OK) {
					continue;
				}

				message->FindUInt8("page_repetition_mode", i, &pageRepetitionMode);
				message->FindUInt8("scan_period_mode", i, &scanPeriodMode);

				// if not present, the default value of these fields will be used
				message->FindUInt8("scan_mode", i, &scanMode);
				message->FindUInt16("clock_offset", i, &clockOffset);
				message->FindInt8("rssi", i, &rssi);
				message->FindBool("friendly_name_is_complete", &friendlyNameIsComplete);

				message->FindString("friendly_name", i, &friendlyName);

				// Skip duplicated replies
				bool duplicatedFound = false;
				for (int32 index = 0; index < fRemoteDevicesList.CountItems(); index++) {
					RemoteDevice* existingDevice = fRemoteDevicesList.ItemAt(index);
					bdaddr_t b1 = existingDevice->GetBluetoothAddress();
					if (bdaddrUtils::Compare(*bdaddr, b1)) {
						// update these values
						existingDevice->fPageRepetitionMode = pageRepetitionMode;
						existingDevice->fScanPeriodMode = scanPeriodMode;
						existingDevice->fScanMode = scanMode;
						existingDevice->fClockOffset = clockOffset;
						existingDevice->fRSSI = rssi;
						if (!existingDevice->fFriendlyNameIsComplete && !friendlyName.IsEmpty()) {
							existingDevice->fFriendlyNameIsComplete = friendlyNameIsComplete;
							existingDevice->fFriendlyName = friendlyName;
						}
						duplicatedFound = true;
						break;
					}
				}

				if (!duplicatedFound) {
					RemoteDevice* rd = new RemoteDevice(*bdaddr, (uint8*)devClass);
					fRemoteDevicesList.AddItem(rd);
					// keep all inquiry reported data
					rd->SetLocalDeviceOwner(fLocalDevice);
					rd->fPageRepetitionMode = pageRepetitionMode;
					rd->fScanPeriodMode = scanPeriodMode;
					rd->fScanMode = scanMode;
					rd->fClockOffset = clockOffset;
					rd->fRSSI = rssi;
					rd->fFriendlyNameIsComplete = friendlyNameIsComplete;
					rd->fFriendlyName = friendlyName;
					DeviceDiscovered(rd, rd->GetDeviceClass());
				}
			}
			break;
		}

		case BT_MSG_INQUIRY_STARTED:
			if (message->FindInt8("status", &status) == B_OK) {
				fRemoteDevicesList.MakeEmpty();
				InquiryStarted(status);
			}
			break;

		case BT_MSG_INQUIRY_COMPLETED:
			InquiryCompleted(BT_INQUIRY_COMPLETED);
			break;

		case BT_MSG_INQUIRY_TERMINATED: /* inquiry was cancelled */
			InquiryCompleted(BT_INQUIRY_TERMINATED);
			break;

		case BT_MSG_INQUIRY_ERROR:
			InquiryCompleted(BT_INQUIRY_ERROR);
			break;

		default:
			BLooper::MessageReceived(message);
			break;
	}
}


DiscoveryListener::DiscoveryListener()
	:
	BLooper(),
	fRemoteDevicesList(BT_MAX_RESPONSES)
{
	CALLED();
	// TODO: Make a better handling of the running not running state
	Run();
}

} /* end namespace Bluetooth */
