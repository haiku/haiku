/*
 * Copyright 2008 Oliver Ruiz Dorantes, oliver.ruiz.dorantes_at_gmail.com
 * Copyright 2008 Mika Lindqvist
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include "CommandManager.h"


#include <bluetooth/bluetooth_error.h>

inline void* buildCommand(uint8 ogf, uint8 ocf, void** param, size_t psize,
	size_t* outsize)
{
	struct hci_command_header* header;

#ifdef BT_IOCTLS_PASS_SIZE
	header = (struct hci_command_header*) malloc(psize
		+ sizeof(struct hci_command_header));
	*outsize = psize + sizeof(struct hci_command_header);
#else
	size_t* size = (size_t*)malloc(psize + sizeof(struct hci_command_header)
		+ sizeof(size_t));
	*outsize = psize + sizeof(struct hci_command_header) + sizeof(size_t);

	*size = psize + sizeof(struct hci_command_header);
	header = (struct hci_command_header*) (((uint8*)size)+4);
#endif


	if (header != NULL) {

		header->opcode = B_HOST_TO_LENDIAN_INT16(PACK_OPCODE(ogf, ocf));
		header->clen = psize;

		if (param != NULL && psize != 0) {
			*param = ((uint8*)header) + sizeof(struct hci_command_header);
		}
	}
#ifdef BT_IOCTLS_PASS_SIZE
	return header;
#else
	return (void*)size;
#endif
}


// This is for request that only require a Command complete in reply.

// Propagate to ReadBufferSize => reply stored in server side
// ReadLocalVersion => reply stored in server side
// Reset => no reply

// Request that do not need any input parameter
// Output reply can be fit in 32 bits field without talking status into account
status_t
NonParameterCommandRequest(uint8 ofg, uint8 ocf, int32* result, hci_id hId,
	BMessenger* messenger)
{
	int8 bt_status = BT_ERROR;

	BluetoothCommand<> simpleCommand(ofg, ocf);

	BMessage request(BT_MSG_HANDLE_SIMPLE_REQUEST);
	BMessage reply;

	request.AddInt32("hci_id", hId);
	request.AddData("raw command", B_ANY_TYPE,
		simpleCommand.Data(), simpleCommand.Size());
	request.AddInt16("eventExpected",  HCI_EVENT_CMD_COMPLETE);
	request.AddInt16("opcodeExpected", PACK_OPCODE(ofg, ocf));

	if (messenger->SendMessage(&request, &reply) == B_OK) {
		reply.FindInt8("status", &bt_status);
		if (result != NULL)
			reply.FindInt32("result", result);
	}

	return bt_status;
}


#if 0
#pragma mark - CONTROL BASEBAND -
#endif


void* buildReset(size_t* outsize)
{
	return buildCommand(OGF_CONTROL_BASEBAND, OCF_RESET,
		NULL, 0, outsize);
}


void* buildReadLocalName(size_t* outsize)
{
	return buildCommand(OGF_CONTROL_BASEBAND, OCF_READ_LOCAL_NAME,
		NULL, 0, outsize);
}


void* buildReadClassOfDevice(size_t* outsize)
{
	return buildCommand(OGF_CONTROL_BASEBAND, OCF_READ_CLASS_OF_DEV,
	NULL, 0, outsize);
}


void* buildReadScan(size_t* outsize)
{
	return buildCommand(OGF_CONTROL_BASEBAND, OCF_READ_SCAN_ENABLE,
	NULL, 0, outsize);
}


void* buildWriteScan(uint8 scanmode, size_t* outsize)
{
	struct hci_write_scan_enable* param;
	void* command = buildCommand(OGF_CONTROL_BASEBAND, OCF_WRITE_SCAN_ENABLE,
		(void**) &param, sizeof(struct hci_write_scan_enable), outsize);


	if (command != NULL) {
		param->scan = scanmode;
	}

	return command;
}


#if 0
#pragma mark - LINK CONTROL -
#endif


void* buildRemoteNameRequest(bdaddr_t bdaddr, uint8 pscan_rep_mode,
	uint16 clock_offset, size_t* outsize)
{

	struct hci_remote_name_request* param;
	void* command = buildCommand(OGF_LINK_CONTROL, OCF_REMOTE_NAME_REQUEST,
		(void**)&param, sizeof(struct hci_remote_name_request), outsize);

	if (command != NULL) {
		param->bdaddr = bdaddr;
		param->pscan_rep_mode = pscan_rep_mode;
		param->clock_offset = clock_offset;
	}

	return command;
}


void* buildInquiry(uint32 lap, uint8 length, uint8 num_rsp, size_t* outsize)
{

	struct hci_cp_inquiry* param;
	void* command = buildCommand(OGF_LINK_CONTROL, OCF_INQUIRY,
		(void**) &param, sizeof(struct hci_cp_inquiry), outsize);

	if (command != NULL) {

		param->lap[2] = (lap >> 16) & 0xFF;
		param->lap[1] = (lap >>  8) & 0xFF;
		param->lap[0] = (lap >>  0) & 0xFF;
		param->length = length;
		param->num_rsp = num_rsp;
	}

	return command;
}


void* buildInquiryCancel(size_t* outsize)
{

	return buildCommand(OGF_LINK_CONTROL, OCF_INQUIRY_CANCEL, NULL, 0, outsize);

}


void* buildPinCodeRequestReply(bdaddr_t bdaddr, uint8 length, char pincode[16],
	size_t* outsize)
{
	struct hci_cp_pin_code_reply* param;

	if (length > HCI_PIN_SIZE)  // PinCode cannot be longer than 16
		return NULL;

	void* command = buildCommand(OGF_LINK_CONTROL, OCF_PIN_CODE_REPLY,
		(void**)&param, sizeof(struct hci_cp_pin_code_reply), outsize);

	if (command != NULL) {
		param->bdaddr = bdaddr;
		param->pin_len = length;
		memcpy(&param->pin_code, pincode, length);
	}

	return command;
}


void* buildPinCodeRequestNegativeReply(bdaddr_t bdaddr, size_t* outsize)
{

	struct hci_cp_pin_code_neg_reply* param;

	void* command = buildCommand(OGF_LINK_CONTROL, OCF_PIN_CODE_NEG_REPLY,
		(void**) &param, sizeof(struct hci_cp_pin_code_neg_reply), outsize);

	if (command != NULL) {

		param->bdaddr = bdaddr;

	}

	return command;
}


void* buildAcceptConnectionRequest(bdaddr_t bdaddr, uint8 role, size_t* outsize)
{
	struct hci_cp_accept_conn_req* param;

	void* command = buildCommand(OGF_LINK_CONTROL, OCF_ACCEPT_CONN_REQ,
					(void**) &param, sizeof(struct hci_cp_accept_conn_req), outsize);

	if (command != NULL) {
		param->bdaddr = bdaddr;
		param->role = role;
	}

	return command;
}


void* buildRejectConnectionRequest(bdaddr_t bdaddr, size_t* outsize)
{
	struct hci_cp_reject_conn_req* param;

	void* command = buildCommand(OGF_LINK_CONTROL, OCF_REJECT_CONN_REQ,
					(void**)&param, sizeof(struct hci_cp_reject_conn_req),
					outsize);

	if (command != NULL) {
		param->bdaddr = bdaddr;
	}

	return command;
}


#if 0
#pragma mark - INFORMATIONAL_PARAM -
#endif

void* buildReadLocalVersionInformation(size_t* outsize)
{
	return buildCommand(OGF_INFORMATIONAL_PARAM, OCF_READ_LOCAL_VERSION,
		NULL, 0, outsize);
}


void* buildReadBufferSize(size_t* outsize)
{
	return buildCommand(OGF_INFORMATIONAL_PARAM, OCF_READ_BUFFER_SIZE,
		NULL, 0, outsize);
}


void* buildReadBdAddr(size_t* outsize)
{
	return buildCommand(OGF_INFORMATIONAL_PARAM, OCF_READ_BD_ADDR,
		NULL, 0, outsize);
}


const char* bluetoothManufacturers[] = {
	"Ericsson Technology Licensing",
	"Nokia Mobile Phones",
	"Intel Corp.",
	"IBM Corp.",
	"Toshiba Corp.",
	"3Com",
	"Microsoft",
	"Lucent",
	"Motorola",
	"Infineon Technologies AG",
	"Cambridge Silicon Radio",
	"Silicon Wave",
	"Digianswer A/S",
	"Texas Instruments Inc.",
	"Parthus Technologies Inc.",
	"Broadcom Corporation",
	"Mitel Semiconductor",
	"Widcomm, Inc.",
	"Zeevo, Inc.",
	"Atmel Corporation",
	"Mitsubishi Electric Corporation",
	"RTX Telecom A/S",
	"KC Technology Inc.",
	"Newlogic",
	"Transilica, Inc.",
	"Rohde & Schwartz GmbH & Co. KG",
	"TTPCom Limited",
	"Signia Technologies, Inc.",
	"Conexant Systems Inc.",
	"Qualcomm",
	"Inventel",
	"AVM Berlin",
	"BandSpeed, Inc.",
	"Mansella Ltd",
	"NEC Corporation",
	"WavePlus Technology Co., Ltd.",
	"Alcatel",
	"Philips Semiconductors",
	"C Technologies",
	"Open Interface",
	"R F Micro Devices",
	"Hitachi Ltd",
	"Symbol Technologies, Inc.",
	"Tenovis",
	"Macronix International Co. Ltd.",
	"GCT Semiconductor",
	"Norwood Systems",
	"MewTel Technology Inc.",
	"ST Microelectronics",
	"Synopsys",
	"Red-M (Communications) Ltd",
	"Commil Ltd",
	"Computer Access Technology Corporation (CATC)",
	"Eclipse (HQ España) S.L.",
	"Renesas Technology Corp.",
	"Mobilian Corporation",
	"Terax",
	"Integrated System Solution Corp.",
	"Matsushita Electric Industrial Co., Ltd.",
	"Gennum Corporation",
	"Research In Motion",
	"IPextreme, Inc.",
	"Systems and Chips, Inc",
	"Bluetooth SIG, Inc",
	"Seiko Epson Corporation",
	"Integrated Silicon Solution Taiwain, Inc.",
	"CONWISE Technology Corporation Ltd",
	"PARROT SA",
	"Socket Communications",
	"Atheros Communications, Inc.",
	"MediaTek, Inc.",
	"Bluegiga",	/* (tentative) */
	"Marvell Technology Group Ltd.",
	"3DSP Corporation",
	"Accel Semiconductor Ltd.",
	"Continental Automotive Systems",
	"Apple, Inc.",
	"Staccato Communications, Inc."
};


const char* linkControlCommands[] = {
	"Inquiry",
	"Inquiry Cancel",
	"Periodic Inquiry Mode",
	"Exit Periodic Inquiry Mode",
	"Create Connection",
	"Disconnect",
	"Add SCO Connection", // not on 2.1
	"Cancel Create Connection",
	"Accept Connection Request",
	"Reject Connection Request",
	"Link Key Request Reply",
	"Link Key Request Negative Reply",
	"PIN Code Request Reply",
	"PIN Code Request Negative Reply",
	"Change Connection Packet Type",
	"Reserved", // not on 2.1",
	"Authentication Requested",
	"Reserved", // not on 2.1",
	"Set Connection Encryption",
	"Reserved", // not on 2.1",
	"Change Connection Link Key",
	"Reserved", // not on 2.1",
	"Master Link Key",
	"Reserved", // not on 2.1",
	"Remote Name Request",
	"Cancel Remote Name Request",
	"Read Remote Supported Features",
	"Read Remote Extended Features",
	"Read Remote Version Information",
	"Reserved", // not on 2.1",
	"Read Clock Offset",
	"Read LMP Handle",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Setup Synchronous Connection",
	"Accept Synchronous Connection",
	"Reject Synchronous Connection",
	"IO Capability Request Reply",
	"User Confirmation Request Reply",
	"User Confirmation Request Negative Reply",
	"User Passkey Request Reply",
	"User Passkey Request Negative Reply",
	"Remote OOB Data Request Reply",
	"Reserved",
	"Reserved",
	"Remote OOB Data Request Negative Reply",
	"IO Capabilities Response Negative Reply"
};


const char* linkPolicyCommands[] = {
	"Hold Mode",
	"Reserved",
	"Sniff Mode",
	"Exit Sniff Mode",
	"Park State",
	"Exit Park State",
	"QoS Setup",
	"Reserved",
	"Role Discovery",
	"Reserved",
	"Switch Role",
	"Read Link Policy Settings",
	"Write Link Policy Settings",
	"Read Default Link Policy Settings",
	"Write Default Link Policy Settings",
	"Flow Specification",
	"Sniff Subrating"
};


const char* controllerBasebandCommands[] = {
	"Set Event Mask",
	"Reserved",
	"Reset",
	"Reserved",
	"Set Event Filter",
	"Reserved",
	"Reserved",
	"Flush",
	"Read PIN Type",
	"Write PIN Type",
	"Create New Unit Key",
	"Reserved",
	"Read Stored Link Key",
	"Reserved",
	"Reserved",
	"Reserved",
	"Write Stored Link Key",
	"Delete Stored Link Key",
	"Write Local Name",
	"Read Local Name",
	"Read Connection Accept Timeout",
	"Write Connection Accept Timeout",
	"Read Page Timeout",
	"Write Page Timeout",
	"Read Scan Enable",
	"Write Scan Enable",
	"Read Page Scan Activity",
	"Write Page Scan Activity",
	"Read Inquiry Scan Activity",
	"Write Inquiry Scan Activity",
	"Read Authentication Enable",
	"Write Authentication Enable",
	"Read Encryption Mode", // not 2.1
	"Write Encryption Mode",// not 2.1
	"Read Class Of Device",
	"Write Class Of Device",
	"Read Voice Setting",
	"Write Voice Setting",
	"Read Automatic Flush Timeout",
	"Write Automatic Flush Timeout",
	"Read Num Broadcast Retransmissions",
	"Write Num Broadcast Retransmissions",
	"Read Hold Mode Activity",
	"Write Hold Mode Activity",
	"Read Transmit Power Level",
	"Read Synchronous Flow Control Enable",
	"Write Synchronous Flow Control Enable",
	"Reserved",
	"Set Host Controller To Host Flow Control",
	"Reserved",
	"Host Buffer Size",
	"Reserved",
	"Host Number Of Completed Packets",
	"Read Link Supervision Timeout",
	"Write Link Supervision Timeout",
	"Read Number of Supported IAC",
	"Read Current IAC LAP",
	"Write Current IAC LAP",
	"Read Page Scan Period Mode", // not 2.1
	"Write Page Scan Period Mode", // not 2.1
	"Read Page Scan Mode",		// not 2.1
	"Write Page Scan Mode",		// not 2.1
	"Set AFH Channel Classification",
	"Reserved",
	"Reserved",
	"Read Inquiry Scan Type",
	"Write Inquiry Scan Type",
	"Read Inquiry Mode",
	"Write Inquiry Mode",
	"Read Page Scan Type",
	"Write Page Scan Type",
	"Read AFH Channel Assessment Mode",
	"Write AFH Channel Assessment Mode",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Read Extended Inquiry Response",
	"Write Extended Inquiry Response",
	"Refresh Encryption Key",
	"Reserved",
	"Read Simple Pairing Mode",
	"Write Simple Pairing Mode",
	"Read Local OOB Data",
	"Read Inquiry Transmit Power Level",
	"Write Inquiry Transmit Power Level",
	"Read Default Erroneous Data Reporting",
	"Write Default Erroneous Data Reporting",
	"Reserved",
	"Reserved",
	"Reserved",
	"Enhanced Flush",
	"Send Keypress Notification"
};


const char* informationalParametersCommands[] = {
	"Read Local Version Information",
	"Read Local Supported Commands",
	"Read Local Supported Features",
	"Read Local Extended Features",
	"Read Buffer Size",
	"Reserved",
	"Read Country Code", // not 2.1
	"Reserved",
	"Read BD ADDR"
};


const char* statusParametersCommands[] = {
	"Read Failed Contact Counter",
	"Reset Failed Contact Counter",
	"Read Link Quality",
	"Reserved",
	"Read RSSI",
	"Read AFH Channel Map",
	"Read Clock",
};


const char* testingCommands[] = {
	"Read Loopback Mode",
	"Write Loopback Mode",
	"Enable Device Under Test Mode",
	"Write Simple Pairing Debug Mode",
};


const char* bluetoothEvents[] = {
	"Inquiry Complete",
	"Inquiry Result",
	"Conn Complete",
	"Conn Request",
	"Disconnection Complete",
	"Auth Complete",
	"Remote Name Request Complete",
	"Encrypt Change",
	"Change Conn Link Key Complete",
	"Master Link Key Compl",
	"Rmt Features",
	"Rmt Version",
	"Qos Setup Complete",
	"Command Complete",
	"Command Status",
	"Hardware Error",
	"Flush Occur",
	"Role Change",
	"Num Comp Pkts",
	"Mode Change",
	"Return Link Keys",
	"Pin Code Req",
	"Link Key Req",
	"Link Key Notify",
	"Loopback Command",
	"Data Buffer Overflow",
	"Max Slot Change",
	"Read Clock Offset Compl",
	"Con Pkt Type Changed",
	"Qos Violation",
	"Reserved",
	"Page Scan Rep Mode Change",
	"Flow Specification",
	"Inquiry Result With Rssi",
	"Remote Extended Features",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Synchronous Connection Completed",
	"Synchronous Connection Changed",
	"Reserved",
	"Extended Inquiry Result",
	"Encryption Key Refresh Complete",
	"Io Capability Request",
	"Io Capability Response",
	"User Confirmation Request",
	"User Passkey Request",
	"Oob Data Request",
	"Simple Pairing Complete",
	"Reserved",
	"Link Supervision Timeout Changed",
	"Enhanced Flush Complete",
	"Reserved",
	"Reserved",
	"Keypress Notification",
	"Remote Host Supported Features Notification"
};


const char* bluetoothErrors[] = {
	"No Error",
	"Unknown Command",
	"No Connection",
	"Hardware Failure",
	"Page Timeout",
	"Authentication Failure",
	"Pin Or Key Missing",
	"Memory Full",
	"Connection Timeout",
	"Max Number Of Connections",
	"Max Number Of Sco Connections",
	"Acl Connection Exists",
	"Command Disallowed",
	"Rejected Limited Resources",
	"Rejected Security",
	"Rejected Personal",
	"Host Timeout",
	"Unsupported Feature",
	"Invalid Parameters",
	"Remote User Ended Connection",
	"Remote Low Resources",
	"Remote Power Off",
	"Connection Terminated",
	"Repeated Attempts",
	"Pairing Not Allowed",
	"Unknown Lmp Pdu",
	"Unsupported Remote Feature",
	"Sco Offset Rejected",
	"Sco Interval Rejected",
	"Air Mode Rejected",
	"Invalid Lmp Parameters",
	"Unspecified Error",
	"Unsupported Lmp Parameter Value",
	"Role Change Not Allowed",
	"Lmp Response Timeout",
	"Lmp Error Transaction Collision",
	"Lmp Pdu Not Allowed",
	"Encryption Mode Not Accepted",
	"Unit Link Key Used",
	"Qos Not Supported",
	"Instant Passed",
	"Pairing With Unit Key Not Supported",
	"Different Transaction Collision",
	"Qos Unacceptable Parameter",
	"Qos Rejected",
	"Classification Not Supported",
	"Insufficient Security",
	"Parameter Out Of Range",
	"Reserved",
	"Role Switch Pending",
	"Reserved",
	"Slot Violation",
	"Role Switch Failed",
	"Extended Inquiry Response Too Large",
	"Simple Pairing Not Supported By Host",
	"Host Busy Pairing"
};


const char* hciVersion[] = { "1.0B" , "1.1 " , "1.2 " , "2.0 " , "2.1 "};
const char* lmpVersion[] = { "1.0 " , "1.1 " , "1.2 " , "2.0 " , "2.1 "};


#if 0
#pragma mark -
#endif


const char*
BluetoothHciVersion(uint16 ver)
{
	return hciVersion[ver];
}


const char*
BluetoothLmpVersion(uint16 ver)
{
	return lmpVersion[ver];
}


const char*
BluetoothCommandOpcode(uint16 opcode)
{

	// NOTE: BT implementations beyond 2.1
	// could specify new commands with OCF numbers
	// beyond the boundaries of the arrays and crash.
	// But only our stack could issue them so its under
	// our control.
	switch (GET_OPCODE_OGF(opcode)) {
		case OGF_LINK_CONTROL:
			return linkControlCommands[GET_OPCODE_OCF(opcode) - 1];
			break;

		case OGF_LINK_POLICY:
			return linkPolicyCommands[GET_OPCODE_OCF(opcode) - 1];
			break;

		case OGF_CONTROL_BASEBAND:
			return controllerBasebandCommands[GET_OPCODE_OCF(opcode) - 1];
			break;

		case OGF_INFORMATIONAL_PARAM:
			return informationalParametersCommands[GET_OPCODE_OCF(opcode) - 1];
			break;

		case OGF_STATUS_PARAM:
			return statusParametersCommands[GET_OPCODE_OCF(opcode) - 1];
			break;

		case OGF_TESTING_CMD:
			return testingCommands[GET_OPCODE_OCF(opcode) - 1];
			break;
		case OGF_VENDOR_CMD:
			return "Vendor specific command";
			break;
		default:
			return "Unknown command";
			break;
	}

}


const char*
BluetoothEvent(uint8 event)
{
	if (event < sizeof(bluetoothEvents) / sizeof(const char*))
		return bluetoothEvents[event - 1];
	else
		return "Event out of Range!";
}


const char*
BluetoothManufacturer(uint16 manufacturer)
{
	if (manufacturer < sizeof(bluetoothManufacturers) / sizeof(const char*))
		return bluetoothManufacturers[manufacturer];
	else if (manufacturer == 0xFFFF)
		return "internal use";
	else
		return "not assigned";
}


const char*
BluetoothError(uint8 error)
{
	if (error < sizeof(bluetoothErrors) / sizeof(const char*))
		return bluetoothErrors[error];
	else
		return "not specified";
}
