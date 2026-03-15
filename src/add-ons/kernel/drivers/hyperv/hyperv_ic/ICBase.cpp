/*
 * Copyright 2026 John Davis. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include "ICBase.h"


//#define TRACE_HYPERV_IC
#ifdef TRACE_HYPERV_IC
#	define TRACE(x...) dprintf("\33[94mhyperv_ic:\33[0m " x)
#else
#	define TRACE(x...) ;
#endif
#define TRACE_ALWAYS(x...)	dprintf("\33[94mhyperv_ic:\33[0m " x)
#define ERROR(x...)			dprintf("\33[94mhyperv_ic:\33[0m " x)
#define CALLED(x...)		TRACE("CALLED %s\n", __PRETTY_FUNCTION__)


ICBase::ICBase(device_node* node, uint32 packetLength, uint16 messageType,
		const uint32* messageVersions, uint32 messageVersionCount)
	:
	fStatus(B_NO_INIT),
	fNode(node),
	fFrameworkVersion(0),
	fMessageVersion(0),
	fHyperV(NULL),
	fHyperVCookie(NULL),
	fPacket(NULL),
	fPacketLength(packetLength),
	fMessageType(messageType),
	fMessageVersions(messageVersions),
	fMessageVersionCount(messageVersionCount)
{
	CALLED();

	device_node* parent = gDeviceManager->get_parent_node(node);
	gDeviceManager->get_driver(parent, (driver_module_info**)&fHyperV, (void**)&fHyperVCookie);
	gDeviceManager->put_node(parent);

	fPacket = static_cast<uint8*>(malloc(fPacketLength));
	if (fPacket == NULL) {
		fStatus = B_NO_MEMORY;
		return;
	}

	fStatus = B_OK;
}


ICBase::~ICBase()
{
	CALLED();

	free(fPacket);
}


status_t
ICBase::Connect(uint32 txLength, uint32 rxLength)
{
	CALLED();

	status_t status = fHyperV->open(fHyperVCookie, txLength, rxLength, _CallbackHandler, this);
	if (status != B_OK) {
		ERROR("Failed to open channel");
		return status;
	}

	return B_OK;
}


void
ICBase::Disconnect()
{
	CALLED();
	fHyperV->close(fHyperVCookie);
}


/*virtual*/ void
ICBase::OnProtocolNegotiated()
{
	// Default implementation
}


/*virtual*/ void
ICBase::OnMessageSent(hv_ic_msg* icMessage)
{
	// Default implementation
}


status_t
ICBase::_NegotiateProtocol(hv_ic_msg_negotiate* message)
{
	CALLED();

	if (message->header.data_length < offsetof(hv_ic_msg_negotiate, versions[2])
			- sizeof(message->header)) {
		ERROR("IC[%u] invalid negotiate msg length 0x%X\n", fMessageType,
			message->header.data_length);
		return B_BAD_VALUE;
	}

	if (message->framework_version_count == 0 || message->message_version_count == 0) {
		ERROR("IC[%u] invalid negotiate msg version count\n", fMessageType);
		return B_BAD_VALUE;
	}

	uint32 versionCount = message->framework_version_count + message->message_version_count;
	if (versionCount < 2) {
		ERROR("IC[%u] invalid negotiate msg version count\n", fMessageType);
		return B_BAD_VALUE;
	}

	// Match the highest supported framework version
	bool foundFrameworkVersion = false;
	for (uint32 i = 0; i < hv_framework_version_count; i++) {
		for (uint32 j = 0; j < message->framework_version_count; j++) {
			TRACE("IC[%u] checking fw version %u.%u against %u.%u\n", fMessageType,
				GET_IC_VERSION_MAJOR(hv_framework_versions[i]),
				GET_IC_VERSION_MINOR(hv_framework_versions[i]),
				GET_IC_VERSION_MAJOR(message->versions[j]),
				GET_IC_VERSION_MINOR(message->versions[j]));

			if (hv_framework_versions[i] == message->versions[j]) {
				fFrameworkVersion = hv_framework_versions[i];
				foundFrameworkVersion = true;
				break;
			}
		}

		if (foundFrameworkVersion)
			break;
	}

	// Match the highest supported message version
	bool foundMessageVersion = false;
	for (uint32 i = 0; i < fMessageVersionCount; i++) {
		for (uint32 j = message->message_version_count; j < versionCount; j++) {
			TRACE("IC[%u] checking msg version %u.%u against %u.%u\n", fMessageType,
				GET_IC_VERSION_MAJOR(fMessageVersions[i]),
				GET_IC_VERSION_MINOR(fMessageVersions[i]),
				GET_IC_VERSION_MAJOR(message->versions[j]),
				GET_IC_VERSION_MINOR(message->versions[j]));

			if (fMessageVersions[i] == message->versions[j]) {
				fMessageVersion = fMessageVersions[i];
				foundMessageVersion = true;
				break;
			}
		}

		if (foundMessageVersion)
			break;
	}

	if (!foundFrameworkVersion || !foundMessageVersion) {
		ERROR("IC%u unsupported versions\n", fMessageType);
		message->framework_version_count = 0;
		message->message_version_count = 0;
		return B_UNSUPPORTED;
	}

	TRACE("IC[%u] found supported fw version %u.%u msg version %u.%u\n", fMessageType,
		GET_IC_VERSION_MAJOR(fFrameworkVersion), GET_IC_VERSION_MINOR(fFrameworkVersion),
		GET_IC_VERSION_MAJOR(fMessageVersion), GET_IC_VERSION_MINOR(fMessageVersion));

	message->framework_version_count = 1;
	message->message_version_count = 1;
	message->versions[0] = fFrameworkVersion;
	message->versions[1] = fMessageVersion;

	return B_OK;
}


/*static*/ void
ICBase::_CallbackHandler(void* arg)
{
	ICBase* icBaseDevice = reinterpret_cast<ICBase*>(arg);
	icBaseDevice->_Callback();
}


void
ICBase::_Callback()
{
	while (true) {
		uint32 length = fPacketLength;
		uint32 headerLength;
		uint32 dataLength;

		status_t status = fHyperV->read_packet(fHyperVCookie, fPacket, &length,
			&headerLength, &dataLength);
		if (status == B_DEV_NOT_READY) {
			break;
		} else if (status != B_OK) {
			ERROR("IC[%u] failed to read packet (%s)\n", fMessageType, strerror(status));
			break;
		}

		if (dataLength < sizeof(hv_ic_msg_header)) {
			ERROR("IC[%u] invalid packet\n", fMessageType);
			continue;
		}

		vmbus_pkt_header* header = reinterpret_cast<vmbus_pkt_header*>(fPacket);
		hv_ic_msg* message = reinterpret_cast<hv_ic_msg*>(fPacket + headerLength);
		if (message->header.data_length <= dataLength - sizeof(message->header)) {
			if (message->header.type == HV_IC_MSGTYPE_NEGOTIATE) {
				// IC protocol negotiation
				status = _NegotiateProtocol(&message->negotiate);
				if (status == B_OK) {
					OnProtocolNegotiated();
				} else {
					ERROR("IC[%u] protocol negotiation failed (%s)\n", fMessageType,
						strerror(status));
					message->header.status = HV_IC_STATUS_FAILED;
				}
			} else if (message->header.type == fMessageType) {
				// IC device-specific message
				OnMessageReceived(message);
			} else {
				ERROR("IC[%u] unknown message type %u\n", fMessageType, message->header.type);
				message->header.status = HV_IC_STATUS_FAILED;
			}
		} else {
			ERROR("IC[%u] invalid msg data length 0x%X pkt length 0x%X\n", fMessageType,
				message->header.data_length, dataLength);
			message->header.status = HV_IC_STATUS_FAILED;
		}

		// Always respond to Hyper-V with the same packet that was originally received
		message->header.flags = HV_IC_FLAG_TRANSACTION | HV_IC_FLAG_RESPONSE;
		status = fHyperV->write_packet(fHyperVCookie, VMBUS_PKTTYPE_DATA_INBAND, message,
			sizeof(hv_ic_msg_header) + message->header.data_length, false, header->transaction_id);

		if (status == B_OK)
			OnMessageSent(message);
		else
			ERROR("IC[%u] failed to send message (%s)\n", fMessageType, strerror(status));
	}
}
