/*
 * Copyright 2010, Andreas Färber <andreas.faerber@web.de>
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#include <boot/net/iSCSITarget.h>

#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include <KernelExport.h>

#include <boot/net/ChainBuffer.h>
#include <boot/net/TCP.h>
#include <iscsi_cmds.h>
#include <scsi_cmds.h>

#include "real_time_clock.h"


//#define TRACE_ISCSI

#ifndef TRACE_ISCSI
#define TRACE(fmt, ...) ;
#else
#define TRACE(fmt, ...) printf("iSCSI: %s(): " fmt, __func__, ## __VA_ARGS__)
#endif
#define PANIC(fmt, ...)  panic("iSCSI: %s(): " fmt, __func__, ## __VA_ARGS__)


#define ISCSI_ALIGNMENT 4
#define ISCSI_ALIGN(x) (((x) + ISCSI_ALIGNMENT - 1) & ~(ISCSI_ALIGNMENT - 1))
#define ISCSI_PADDING(x) ((((x) % ISCSI_ALIGNMENT) == 0) ? 0 \
	: (ISCSI_ALIGNMENT - ((x) % ISCSI_ALIGNMENT)))

// derived from the schedulers
static int
_rand(void)
{
	static int next = 0;
	if (next == 0)
		next = real_time_clock_usecs() / 1000;

	next = next * 1103515245 + 12345;
	return next;
}


bool
iSCSITarget::DiscoverTargets(ip_addr_t address, uint16 port, NodeList* devicesList)
{
	iSCSISession* session = new(nothrow) iSCSISession();
	if (session == NULL)
		return false;
	status_t error = session->Init(address, port);
	if (error != B_OK) {
		delete session;
		return false;
	}
	const char* request = "SendTargets=All";
	char* sendTargets = NULL;
	size_t sendTargetsLength = 0;
	error = session->Connection()->GetText(request, strlen(request) + 1,
		&sendTargets, &sendTargetsLength);
	session->Close();
	delete session;
	if (error != B_OK) {
		return false;
	}

	bool addedDisk = false;
	char* targetName = NULL;
	bool seenAddress = false;
	for (char* pair = sendTargets; pair < sendTargets + sendTargetsLength;
			pair += strlen(pair) + 1) {
		printf("%s\n", pair);
		if (strncmp(pair, "TargetName=", strlen("TargetName=")) == 0) {
			if (targetName != NULL && !seenAddress) {
				if (_AddDevice(address, port, targetName, devicesList))
					addedDisk = true;
			}
			seenAddress = false;
			targetName = pair + strlen("TargetName=");
		} else if (strncmp(pair, "TargetAddress=", strlen("TargetAddress="))
				== 0) {
			seenAddress = true;
			char* targetAddress = pair + strlen("TargetAddress=");
			char* comma = strrchr(targetAddress, ',');
			int addressLength = comma - targetAddress;
			targetAddress = strndup(targetAddress, addressLength);
			uint16 targetPort = ISCSI_PORT;
			char* colon = strrchr(targetAddress, ':');
			if (colon != NULL) {
				// colon could be part of an IPv6 address, e.g. [::1.2.3.4]
				char* bracket = strrchr(targetAddress, ']');
				if (bracket == NULL || bracket < colon) {
					targetPort = strtol(colon + 1, NULL, 10);
					colon[0] = '\0';
				}
			}
			if (targetName != NULL && isdigit(targetAddress[0])) {
				if (_AddDevice(ip_parse_address(targetAddress), targetPort,
						targetName, devicesList))
					addedDisk = true;
			}
			free(targetAddress);
		}
	}
	if (targetName != NULL && !seenAddress) {
		if (_AddDevice(address, port, targetName, devicesList))
			addedDisk = true;
	}
	free(sendTargets);
	return addedDisk;
}


bool
iSCSITarget::_AddDevice(ip_addr_t address, uint16 port,
	const char* targetName, NodeList* devicesList)
{
	TRACE("=> %s @ %08lx:%u\n", targetName, address, port);
	iSCSITarget* disk = new(nothrow) iSCSITarget();
	if (disk == NULL)
		return false;
	status_t error = disk->Init(address, port, targetName);
	if (error != B_OK) {
		delete disk;
		return false;
	}
	TRACE("disk size = %llu\n", disk->Size());
	devicesList->Add(disk);
	return true;
}


iSCSITarget::iSCSITarget()
	:
	fSession(NULL),
	fTargetName(NULL),
	fTargetAlias(NULL)
{
}


iSCSITarget::~iSCSITarget()
{
	free(fTargetName);
	free(fTargetAlias);
	delete fSession;
}


status_t
iSCSITarget::Init(ip_addr_t address, uint16 port, const char* targetName)
{
	fTargetName = strdup(targetName);
	if (fTargetName == NULL)
		return B_NO_MEMORY;

	fSession = new(nothrow) iSCSISession(fTargetName);
	if (fSession == NULL)
		return B_NO_MEMORY;
	status_t error = fSession->Init(address, port, &fTargetAlias);
	if (error != B_OK)
		return error;
	if (targetName == NULL)
		return B_OK;

	error = _GetSize();
	if (error != B_OK)
		return error;

	return B_OK;
}


status_t
iSCSITarget::_GetSize()
{
	scsi_cmd_read_capacity cmd;
	memset(&cmd, 0, sizeof(cmd));
	cmd.opcode = SCSI_OP_READ_CAPACITY;
	cmd.relative_address = 0;
	cmd.lun = 0;
	cmd.lba = 0;
	cmd.pmi = 0;
	cmd.control = 0;
	scsi_res_read_capacity resp;
	status_t error = fSession->Connection()->SendCommand(&cmd, sizeof(cmd),
		true, false, sizeof(resp),
		&resp, 0, sizeof(resp));
	if (error != B_OK) {
		TRACE("error %lx sending command!\n", error);
		return error;
	}
	TRACE("lba = %lu, block size = %lu\n", resp.lba, resp.block_size);
	fLastLBA = resp.lba;
	fBlockSize = resp.block_size;

	return B_OK;
}


ssize_t
iSCSITarget::ReadAt(void* /*cookie*/, off_t pos, void* buffer, size_t bufferSize)
{
	TRACE("pos=%llu, size = %lu\n", pos, bufferSize);

	if (fSession == NULL)
		return B_NO_INIT;

	if (buffer == NULL || pos < 0 || pos >= Size())
		return B_BAD_VALUE;
	if (bufferSize == 0)
		return 0;

	uint32 blockOffset = pos % fBlockSize;

	scsi_cmd_rw_16 cmd;
	memset(&cmd, 0, sizeof(cmd));
	cmd.opcode = SCSI_OP_READ_16;
	cmd.force_unit_access = 0;
	cmd.disable_page_out = 0;
	cmd.lba = pos / fBlockSize;
	cmd.length = (blockOffset + bufferSize + fBlockSize - 1) / fBlockSize;
	cmd.control = 0;
	status_t error = fSession->Connection()->SendCommand(&cmd, sizeof(cmd),
		true, false, cmd.length * fBlockSize, buffer, blockOffset, bufferSize);
	if (error != B_OK) {
		TRACE("error %lx sending command!\n", error);
		return error;
	}

	return bufferSize;
}


ssize_t
iSCSITarget::WriteAt(void* /*cookie*/, off_t pos, const void* buffer, size_t bufferSize)
{
	return B_PERMISSION_DENIED;
}


status_t
iSCSITarget::GetName(char* buffer, size_t bufferSize) const
{
	if (buffer == NULL)
		return B_BAD_VALUE;

	snprintf(buffer, bufferSize, "iSCSI:%s", fTargetName);

	return B_OK;
}


off_t
iSCSITarget::Size() const
{
	return (fLastLBA + 1) * fBlockSize;
}


iSCSISession::iSCSISession(const char* targetName)
	:
	fDiscovery(targetName == NULL),
	fTargetName(targetName),
	fConnection(NULL)
{
	fCommandSequenceNumber = _rand();
}


iSCSISession::~iSCSISession()
{
	if (Active())
		Close();
	delete fConnection;
}


status_t
iSCSISession::Init(ip_addr_t address, uint16 port, char** targetAlias)
{
	fConnection = new(nothrow) iSCSIConnection();
	if (fConnection == NULL)
		return B_NO_MEMORY;
	status_t error = fConnection->Init(this);
	if (error != B_OK)
		return error;
	error = fConnection->Open(address, port);
	if (error != B_OK)
		return error;
	error = fConnection->Login(fTargetName, targetAlias);
	if (error != B_OK)
		return error;
	return B_OK;
}


status_t
iSCSISession::Close()
{
	if (fConnection == NULL)
		return B_NO_INIT;

	status_t error = fConnection->Logout(true);
	if (error != B_OK)
		return error;
	return B_OK;
}


iSCSIConnection::iSCSIConnection()
	:
	fSession(NULL),
	fSocket(NULL),
	fConnected(false),
	fConnectionID(0)
{
}


iSCSIConnection::~iSCSIConnection()
{
	if (fSocket != NULL && fConnected) {
		if (Logout() != B_OK)
			fSocket->Close();
	}
	delete fSocket;
}


status_t
iSCSIConnection::Init(iSCSISession* session)
{
	fSession = session;
	return B_OK;
}

status_t
iSCSIConnection::Open(ip_addr_t address, uint16 port)
{
	fSocket = new(nothrow) TCPSocket();
	if (fSocket == NULL)
		return B_NO_MEMORY;
	TRACE("connecting to target...\n");
	status_t error = fSocket->Connect(address, port);
	if (error != B_OK)
		return error;
	fConnected = true;
	TRACE("connected.\n");
	fConnectionID = _rand();
	return B_OK;
}


status_t
iSCSIConnection::Login(const char* targetName, char** targetAlias)
{
	char data[256];
	size_t dataLength = 0;
	const char* keyValue;
	keyValue = "InitiatorName=iqn.2002-10.org.haiku-os:haiku.bootloader.hostX";
	strcpy(data + dataLength, keyValue);
	dataLength += strlen(keyValue) + 1;
	if (targetName != NULL) {
		dataLength += sprintf(data + dataLength, "TargetName=%s", targetName) + 1;
	} else {
		keyValue = "SessionType=Discovery";
		strcpy(data + dataLength, keyValue);
		dataLength += strlen(keyValue) + 1;
	}

	iscsi_login_request req;
	req.transit = true;
	req.c = false;
	req.currentStage = ISCSI_SESSION_STAGE_LOGIN_OPERATIONAL_NEGOTIATION;
	req.nextStage    = ISCSI_SESSION_STAGE_FULL_FEATURE_PHASE;
	req.versionMax = ISCSI_VERSION;
	req.versionMin = ISCSI_VERSION;
	req.totalAHSLength = 0;
	req.dataSegmentLength = dataLength;
	TRACE("data segment length = %lu\n", req.dataSegmentLength);
	req.isid.t = ISCSI_ISID_RANDOM;
	req.isid.a = 0;
	uint32 random = _rand();
	req.isid.b = random >> 8;
	req.isid.c = random & 0xff;
	req.isid.d = _rand();
	req.tsih = 0;
	req.initiatorTaskTag = _rand();
	req.cid = fConnectionID;
	req.cmdSN = fSession->CommandSequenceNumber();
	req.expStatSN = 0;
	status_t error = fSocket->Write(&req, sizeof(req));
	if (error != B_OK) {
		return error;
	}
	for (unsigned int i = dataLength; i < ISCSI_ALIGN(dataLength); i++)
		data[i] = '\0';
	dataLength = ISCSI_ALIGN(dataLength);
	TRACE("data length = %lu\n", dataLength);
	error = fSocket->Write(data, dataLength);
	if (error != B_OK) {
		TRACE("write error\n");
		return error;
	}
	TRACE("request sent.\n");

	iscsi_login_response resp;
	size_t bytesRead = 0;
	error = fSocket->Read(&resp, sizeof(resp), &bytesRead, 10000000LL);
	if (error != B_OK) {
		TRACE("response read error\n");
		return error;
	}
	TRACE("bytesRead = %lu\n", bytesRead);
	TRACE("response: opcode = %x, status class = %u, status detail = %u, transit = %u, data length = %lu\n",
		resp.opcode, resp.statusClass, resp.statusDetail, resp.transit, resp.dataSegmentLength);
	if (resp.dataSegmentLength > 0) {
		error = fSocket->Read(data, resp.dataSegmentLength, &bytesRead, 1000000LL);
		if (error != B_OK) {
			TRACE("response data read error\n");
			return error;
		}
		TRACE("bytesRead = %lu\n", bytesRead);
		if (resp.dataSegmentLength % ISCSI_ALIGNMENT) {
			error = fSocket->Read(NULL, ISCSI_PADDING(resp.dataSegmentLength), &bytesRead, 100000LL);
			if (error != B_OK) {
				TRACE("response padding read error\n");
				return error;
			}
			TRACE("bytesRead = %lu\n", bytesRead);
		}
		for (char* pair = data; pair < data + resp.dataSegmentLength; pair += strlen(pair) + 1) {
			TRACE("%s\n", pair);
			if (targetAlias != NULL && strncmp(pair, "TargetAlias=", strlen("TargetAlias=")) == 0) {
				*targetAlias = strdup(pair + strlen("TargetAlias="));
			}
		}
	}
	fStatusSequenceNumber = resp.statSN;
	if (resp.statusClass != 0) {
		TRACE("response indicates failure.\n");
		return B_BAD_VALUE;
	}
	return B_OK;
}


status_t
iSCSIConnection::Logout(bool closeSession)
{
	iscsi_logout_request req;
	TRACE("req size = %lu\n", sizeof(req));
	req.immediateDelivery = true;
	req.reasonCode = closeSession ? ISCSI_LOGOUT_REASON_CLOSE_SESSION
		: ISCSI_LOGOUT_REASON_CLOSE_CONNECTION;
	req.totalAHSLength = 0;
	req.dataSegmentLength = 0;
	req.initiatorTaskTag = _rand();
	req.cid = closeSession ? 0 : fConnectionID;
	req.cmdSN = fSession->CommandSequenceNumber();
	req.expStatSN = fStatusSequenceNumber;
	status_t error = fSocket->Write(&req, sizeof(req));
	if (error != B_OK) {
		TRACE("request write error\n");
		return error;
	}

	iscsi_logout_response resp;
	TRACE("resp size = %lu\n", sizeof(resp));
	size_t bytesRead = 0;
	error = fSocket->Read(&resp, sizeof(resp), &bytesRead, 10000000LL);
	if (error != B_OK) {
		TRACE("response read error\n");
		return error;
	}
	TRACE("bytesRead = %lu\n", bytesRead);
	TRACE("response: opcode = %x, response = %u\n",
		resp.opcode, resp.response);
	fStatusSequenceNumber = resp.statSN;
	if (resp.response != 0) {
		TRACE("response indicates failure.\n");
		return B_ERROR;
	}

	fSocket->Close();
	fConnected = false;
	return B_OK;
}


status_t
iSCSIConnection::GetText(const char* request, size_t requestLength, char** response, size_t* responseLength)
{
	iscsi_text_request req;
	req.immediateDelivery = true;
	req.final = true;
	req.c = false;
	req.totalAHSLength = 0;
	req.dataSegmentLength = requestLength;
	req.lun = 0;
	req.initiatorTaskTag = _rand();
	req.targetTransferTag = 0xffffffffL;
	req.cmdSN = fSession->NextCommandSequenceNumber();
	req.expStatSN = fStatusSequenceNumber;
	status_t error = fSocket->Write(&req, sizeof(req));
	if (error != B_OK) {
		TRACE("iSCSI text request write error\n");
		return error;
	}
	error = fSocket->Write(request, requestLength);
	if (error != B_OK) {
		TRACE("iSCSI text request data write error\n");
		return error;
	}
	if (requestLength % ISCSI_ALIGNMENT != 0) {
		char buffer[3];
		memset(buffer, 0, 3);
		error = fSocket->Write(buffer, ISCSI_ALIGNMENT - (requestLength % ISCSI_ALIGNMENT));
		if (error != B_OK) {
			TRACE("iSCSI text request padding write error\n");
			return error;
		}
	}

	iscsi_logout_response resp;
	size_t bytesRead = 0;
	error = fSocket->Read(&resp, sizeof(resp), &bytesRead, 10000000LL);
	if (error != B_OK) {
		TRACE("response read error\n");
		return error;
	}
	TRACE("bytesRead = %lu\n", bytesRead);
	TRACE("response: opcode = %x\n",
		resp.opcode);
	*response = (char*)malloc(resp.dataSegmentLength);
	if (*response == NULL)
		return B_NO_MEMORY;
	error = fSocket->Read(*response, resp.dataSegmentLength, &bytesRead, 10000000LL);
	if (error != B_OK) {
		TRACE("response read error\n");
		return error;
	}
	TRACE("bytesRead = %lu\n", bytesRead);
	*responseLength = resp.dataSegmentLength;
	if ((resp.dataSegmentLength % ISCSI_ALIGNMENT) != 0) {
		error = fSocket->Read(NULL, ISCSI_PADDING(resp.dataSegmentLength), &bytesRead, 1000000LL);
		if (error != B_OK) {
			TRACE("response padding read error\n");
			return error;
		}
	}
	fStatusSequenceNumber = resp.statSN;
	if (resp.opcode != ISCSI_OPCODE_TEXT_RESPONSE) {
		PANIC("response opcode unexpected!");
		return B_ERROR;
	}
	
	return B_OK;
}


status_t
iSCSIConnection::SendCommand(const void* command, size_t commandSize,
	bool r, bool w, uint32 expectedDataTransferLength,
	void* response, uint32 responseOffset, size_t responseLength)
{
	TRACE("command size = %lu, offset = %lu, length = %lu\n", commandSize, responseOffset, responseLength);
	iscsi_scsi_command req;
	req.immediateDelivery = true;
	req.final = true;
	req.r = r;
	req.w = w;
	req.attr = 0;
	req.totalAHSLength = 0;
	req.dataSegmentLength = (commandSize > 16) ? commandSize - 16 : 0;
	req.lun = 0;
	req.initiatorTaskTag = _rand();
	req.expectedDataTransferLength = expectedDataTransferLength;
	req.cmdSN = fSession->NextCommandSequenceNumber();
	req.expStatSN = fStatusSequenceNumber;
	if (commandSize < 16)
		memset(req.cdb, 0, 16);
	memcpy(req.cdb, command, commandSize <= 16 ? commandSize : 16);
	status_t error = fSocket->Write(&req, sizeof(req));
	if (error != B_OK) {
		TRACE("write error\n");
		return error;
	}
	if (commandSize > 16) {
		error = fSocket->Write((uint8*)command + 16, commandSize - 16);
		if (error != B_OK) {
			TRACE("write error\n");
			return error;
		}
		if ((req.dataSegmentLength % ISCSI_ALIGNMENT) != 0) {
			char buffer[3];
			memset(buffer, 0, 3);
			error = fSocket->Write(buffer, ISCSI_ALIGNMENT - (req.dataSegmentLength % ISCSI_ALIGNMENT));
			if (error != B_OK) {
				TRACE("request padding write error\n");
				return error;
			}
		}
	}

	struct iscsi_basic_header_segment resp;
	error = _ReadResponse(&resp);
	if (error != B_OK) {
		TRACE("response read error\n");
		return error;
	}
	while (r && resp.opcode != ISCSI_OPCODE_SCSI_RESPONSE) {
		if (resp.opcode != ISCSI_OPCODE_SCSI_DATA_IN)
			PANIC("response opcode %x unexpected\n", resp.opcode);
		iscsi_scsi_data_in* dataIn = (iscsi_scsi_data_in*)&resp;
		TRACE("A=%u, S=%u, bufferOffset=%lu, length = %lu\n", dataIn->acknowledge, dataIn->S, dataIn->bufferOffset, resp.dataSegmentLength);
		if (resp.dataSegmentLength > 0) {
			size_t toRead = resp.dataSegmentLength;
			size_t toSkip = 0;
			if (dataIn->bufferOffset < responseOffset) {
				toSkip = MIN(responseOffset - dataIn->bufferOffset, resp.dataSegmentLength);
				error = _Read(NULL, toSkip);
				if (error != B_OK) {
					TRACE("response skip error\n");
					return error;
				}
				TRACE("skipped %lu response bytes\n", toSkip);
				toRead -= toSkip;
			}
			if (toRead > 0) {
				if (dataIn->bufferOffset + resp.dataSegmentLength > responseOffset + responseLength) {
					size_t cutOff = expectedDataTransferLength - (responseOffset + responseLength);
					toRead = MIN(expectedDataTransferLength - cutOff - dataIn->bufferOffset, resp.dataSegmentLength) - toSkip;
				}
				error = _Read((uint8*)response + dataIn->bufferOffset - responseOffset, toRead);
				if (error != B_OK) {
					TRACE("response read error: %lx\n", error);
					return error;
				}
			}
			size_t toCutOff = resp.dataSegmentLength - toSkip - toRead;
			if (toCutOff > 0) {
				error = _Read(NULL, toCutOff);
				if (error != B_OK) {
					TRACE("response cut-off error\n");
					return error;
				}
			}
			if (toSkip + toRead + toCutOff != resp.dataSegmentLength)
				PANIC("inconcistency while reading detected!\n");
		}
		if ((resp.dataSegmentLength % ISCSI_ALIGNMENT) != 0) {
			error = _Read(NULL, ISCSI_PADDING(resp.dataSegmentLength));
			if (error != B_OK) {
				TRACE("response padding read error\n");
				return error;
			}
		}
		fStatusSequenceNumber = dataIn->statSN;
		if (dataIn->S) {
			// TODO check status
			return B_OK;
		}

		error = _ReadResponse(&resp);
		if (error != B_OK) {
			TRACE("response read error\n");
			return error;
		}
	}

	TRACE("response: opcode = %x, AHS length = %x, data length = %lu\n",
		resp.opcode, resp.totalAHSLength, resp.dataSegmentLength);
	if (resp.opcode != ISCSI_OPCODE_SCSI_RESPONSE) {
		PANIC("response opcode unexpected!\n");
		return B_ERROR;
	}
	iscsi_scsi_response* scsiResp = (iscsi_scsi_response*)&resp;
	TRACE("response: response = %x, status = %x, SNACK=%x\n",
		scsiResp->response, scsiResp->status, scsiResp->snackTag);
	if (resp.dataSegmentLength > 0) {
		void* buffer = NULL;
		if (scsiResp->status != SCSI_STATUS_GOOD)
			buffer = malloc(resp.dataSegmentLength);
		error = _Read(buffer, resp.dataSegmentLength);
		if (error != B_OK) {
			TRACE("response read error\n");
			return error;
		}
#ifdef TRACE_ISCSI
		if (scsiResp->status != SCSI_STATUS_GOOD) {
			scsi_sense* sense = (scsi_sense*)((uint16*)buffer + 1);
			TRACE("error code = %u, sense key = %u, ILI = %u, asc = %x\n",
				sense->error_code, sense->sense_key, sense->ILI, (sense->asc << 8) | sense->ascq);
		}
#endif
		free(buffer);
	}
	if ((resp.dataSegmentLength % ISCSI_ALIGNMENT) != 0) {
		error = _Read(NULL, ISCSI_PADDING(resp.dataSegmentLength));
		if (error != B_OK) {
			TRACE("response padding read error\n");
			return error;
		}
	}
	fStatusSequenceNumber = scsiResp->statSN;
	if (scsiResp->response != 0x00 || scsiResp->status != SCSI_STATUS_GOOD)
		return B_BAD_VALUE;

	return B_OK;
}


status_t
iSCSIConnection::_ReadResponse(iscsi_basic_header_segment* resp, bigtime_t timeout)
{
	if (resp == NULL)
		return B_BAD_VALUE;

	status_t error = _Read(resp, sizeof(iscsi_basic_header_segment), timeout);
	if (error != B_OK) {
		TRACE("initial read failed\n");
		return error;
	}

	if (resp->reserved == true) {
		TRACE("reserved bit is 1!\n");
		return B_IO_ERROR;
	}
	// theoretically process header checksum here, if available

	// At any time a target can send a NOP-In, requiring us to answer with a NOP-Out.
	// TODO handle asynchronous event message
	while (resp->opcode == ISCSI_OPCODE_NOP_IN) {
		iscsi_nop_in* nopIn = (iscsi_nop_in*)resp;
		// A NOP-In can also be the answer to our own NOP-Out though.
		if (nopIn->targetTransferTag == 0xffffffff)
			return B_OK;
		TRACE("received NOP-In (ping data length = %lu)\n", nopIn->dataSegmentLength);
		void* pingData = NULL;
		if (nopIn->dataSegmentLength > 0) {
			pingData = malloc(nopIn->dataSegmentLength);
			if (pingData == NULL)
				return B_NO_MEMORY;
			error = _Read(pingData, nopIn->dataSegmentLength, timeout);
			if (error != B_OK) {
				TRACE("NOP-In ping data read error\n");
				free(pingData);
				return error;
			}
		}
		if ((nopIn->dataSegmentLength % ISCSI_ALIGNMENT) != 0) {
			error = _Read(NULL, ISCSI_PADDING(nopIn->dataSegmentLength), timeout);
			if (error != B_OK) {
				TRACE("NOP-In padding read error\n");
				free(pingData);
				return error;
			}
		}
		iscsi_nop_out nopOut;
		nopOut.totalAHSLength = 0;
		nopOut.dataSegmentLength = nopIn->dataSegmentLength;
		nopOut.lun = nopIn->lun;
		nopOut.initiatorTaskTag = 0xffffffff;
		nopOut.targetTransferTag = nopIn->targetTransferTag;
		nopOut.cmdSN = fSession->NextCommandSequenceNumber();
		nopOut.expStatSN = fStatusSequenceNumber;
		error = fSocket->Write(&nopOut, sizeof(nopOut));
		if (error != B_OK) {
			TRACE("NOP-Out write error\n");
			free(pingData);
			return error;
		}
		if (nopOut.dataSegmentLength > 0) {
			error = fSocket->Write(pingData, nopOut.dataSegmentLength);
			free(pingData);
			if (error != B_OK) {
				TRACE("NOP-Out ping data write error\n");
				return error;
			}
			if ((nopOut.dataSegmentLength % ISCSI_ALIGNMENT) != 0) {
				char buffer[3];
				memset(buffer, 0, 3);
				error = fSocket->Write(buffer, ISCSI_ALIGNMENT - (nopOut.dataSegmentLength % ISCSI_ALIGNMENT));
				if (error != B_OK) {
					TRACE("NOP-Out padding write error\n");
					return error;
				}
			}
		}

		error = _Read(resp, sizeof(iscsi_basic_header_segment), timeout);
		if (error != B_OK) {
			TRACE("read failed\n");
			return error;
		}
	}

	return B_OK;
}


status_t
iSCSIConnection::_Read(void* buffer, size_t bufferSize, bigtime_t timeout)
{
	size_t bytesRead;
	status_t error = fSocket->Read(buffer, bufferSize, &bytesRead, timeout);
	if (error != B_OK)
		return error;
	if (bytesRead < bufferSize) {
		dprintf("not enough data available: %lu vs. %lu\n", bytesRead, bufferSize);
		return B_ERROR;
	}
	return B_OK;
}
