/*
 * Copyright 2008 Oliver Ruiz Dorantes, oliver.ruiz.dorantes_at_gmail.com
 * Copyright 2008 Mika Lindqvist
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include "CommandManager.h"

inline void* buildCommand(uint8 ogf, uint8 ocf, void** param, size_t psize, size_t* outsize)
{
	struct hci_command_header* header;
	
#ifdef BT_IOCTLS_PASS_SIZE
    header = (struct hci_command_header*) malloc(psize + sizeof(struct hci_command_header));
	*outsize = psize + sizeof(struct hci_command_header);
#else
	size_t* size = (size_t*)malloc(psize + sizeof(struct hci_command_header) + sizeof(size_t));
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


#if 0
#pragma mark - CONTROL BASEBAND -
#endif


void* buildReset(size_t* outsize)
{
    return buildCommand(OGF_CONTROL_BASEBAND, OCF_RESET, NULL, 0, outsize);
}


void* buildReadLocalName(size_t* outsize)
{
    return buildCommand(OGF_CONTROL_BASEBAND, OCF_READ_LOCAL_NAME, NULL, 0, outsize);
}


void* buildReadClassOfDevice(size_t* outsize)
{
    return buildCommand(OGF_CONTROL_BASEBAND, OCF_READ_CLASS_OF_DEV, NULL, 0, outsize);
}


void* buildWriteScan(uint8 scanmode, size_t* outsize)
{
    struct hci_write_scan_enable* param;
	void* command = buildCommand(OGF_CONTROL_BASEBAND, OCF_WRITE_SCAN_ENABLE, (void**) &param, sizeof(struct hci_write_scan_enable), outsize);


    if (command != NULL) {
        param->scan = scanmode;
    }

    return command;

}


void* buildAuthEnable(uint8 auth, size_t* outsize)
{
    struct hci_write_authentication_enable* param;
	void* command = buildCommand(OGF_CONTROL_BASEBAND, OCF_WRITE_AUTH_ENABLE, (void**) &param, sizeof(struct hci_write_authentication_enable), outsize);


    if (command != NULL) {
        param->authentication = auth;
    }

    return command;

}


#if 0
#pragma mark - LINK CONTROL -
#endif


void* buildCreateConnection(bdaddr_t bdaddr)
{
	/*
	cm4.size = sizeof(struct hci_command_header)+sizeof(struct hci_cp_create_conn);
	cm4.header.opcode = B_HOST_TO_LENDIAN_INT16(hci_opcode_pack(OGF_LINK_CONTROL, OCF_CREATE_CONN));
	cm4.body.bdaddr.b[0] = 0x92;
	cm4.body.bdaddr.b[1] = 0xd3;
	cm4.body.bdaddr.b[2] = 0xaf;
	cm4.body.bdaddr.b[3] = 0xd9;
	cm4.body.bdaddr.b[4] = 0x0a;
	cm4.body.bdaddr.b[5] = 0x00;
	cm4.body.pkt_type = 0xFFFF;
	cm4.body.pscan_rep_mode = 1;
	cm4.body.pscan_mode = 0;
	cm4.body.clock_offset = 0xc7;
	cm4.body.role_switch = 1;
	cm4.header.clen = 13;					
	ioctl(fd1, ISSUE_BT_COMMAND, &cm4, sizeof(cm4));
	*/

	return NULL;
}


void* buildRemoteNameRequest(bdaddr_t bdaddr,uint8 pscan_rep_mode, uint16 clock_offset, size_t* outsize)
{

    struct hci_remote_name_request* param;
    void* command = buildCommand(OGF_LINK_CONTROL, OCF_REMOTE_NAME_REQUEST, (void**) &param, sizeof(struct hci_remote_name_request), outsize);

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
    void* command = buildCommand(OGF_LINK_CONTROL, OCF_INQUIRY, (void**) &param, sizeof(struct hci_cp_inquiry), outsize);

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


void* buildPinCodeRequestReply(bdaddr_t bdaddr, uint8 length, char pincode[16], size_t* outsize)
{

    struct hci_cp_pin_code_reply* param;

    if (length > HCI_PIN_SIZE)  // PinCode cannot be longer than 16
	return NULL;

    void* command = buildCommand(OGF_LINK_CONTROL, OCF_PIN_CODE_REPLY, (void**) &param, sizeof(struct hci_cp_pin_code_reply), outsize);

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

	void *command = buildCommand(OGF_LINK_CONTROL, OCF_REJECT_CONN_REQ,
					(void**) &param, sizeof(struct hci_cp_reject_conn_req), outsize);

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
    return buildCommand(OGF_INFORMATIONAL_PARAM, OCF_READ_LOCAL_VERSION, NULL, 0, outsize);
}


void* buildReadBufferSize(size_t* outsize)
{
    return buildCommand(OGF_INFORMATIONAL_PARAM, OCF_READ_BUFFER_SIZE, NULL, 0, outsize);
}


void* buildReadBdAddr(size_t* outsize)
{
	return buildCommand(OGF_INFORMATIONAL_PARAM, OCF_READ_BD_ADDR, NULL, 0, outsize);
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
	"Inquiry cancel",
	"Periodic inquiry mode",
	"Exit periodic inquiry mode",
	"Create connection",
	"Disconnect",
	"Add SCO connection", // not on 2.1
	"Cancel create connection",
	"Accept connection request",
	"Reject connection request",
	"Link key request reply",
	"Link key request negative reply",
	"PIN code request reply",
	"PIN code request negative reply",
	"Change connection packet type",
	"Reserved", // not on 2.1",
	"Authentication requested",
	"Reserved", // not on 2.1",
	"Set connection encryption",
	"Reserved", // not on 2.1",
	"Change connection link key",
	"Reserved", // not on 2.1",
	"Master link key",
	"Reserved", // not on 2.1",
	"Remote name request",
	"Cancel remote name request",
	"Read remote supported features",
	"Read remote extended features",
	"Read remote version information",
	"Reserved", // not on 2.1",
	"Read clock offset",
	"Read LMP handle",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Setup synchronous connection",
	"Accept synchronous connection",
	"Reject synchronous connection",	
	"IO capability request reply",
	"User confirmation request reply",
	"User confirmation request negative reply",
	"User passkey request reply",
	"User passkey request negative reply",
	"Remote OOB data request reply",
	"Reserved",
	"Reserved",
	"Remote OOB data request negative reply",
	"IO capabilities response negative reply"
};

	
const char* linkPolicyCommands[] = {	
	"Hold mode",
	"Reserved",
	"Sniff mode",
	"Exit sniff mode",
	"Park state",
	"Exit park state",
	"QoS setup",
	"Reserved",
	"Role discovery",
	"Reserved",
	"Switch role",
	"Read link policy settings",
	"Write link policy settings",
	"Read default link policy settings",
	"Write default link policy settings",
	"Flow specification",
	"Sniff subrating"
};


const char* controllerBasebandCommands[] = {	
	"Set event mask",
	"Reserved",
	"Reset",
	"Reserved",	
	"Set event filter",
	"Reserved",
	"Reserved",
	"Flush",
	"Read PIN type",
	"Write PIN type",
	"Create new unit key",
	"Reserved",
	"Read stored link key",
	"Reserved",
	"Reserved",
	"Reserved",
	"Write stored link key",
	"Delete stored link key",
	"Write local name",
	"Read local name",
	"Read connection accept timeout",
	"Write connection accept timeout",
	"Read page timeout",
	"Write page timeout",
	"Read scan enable",
	"Write scan enable",
	"Read page scan activity",
	"Write page scan activity",
	"Read inquiry scan activity",
	"Write inquiry scan activity",
	"Read authentication enable",
	"Write authentication enable",
	"Read encryption mode", // not 2.1
	"Write encryption mode",// not 2.1
	"Read class of device",
	"Write class of device",
	"Read voice setting",
	"Write voice setting",
	"Read automatic flush timeout",
	"Write automatic flush timeout",
	"Read num broadcast retransmissions",
	"Write num broadcast retransmissions",	
	"Read hold mode activity",
	"Write hold mode activity",
	"Read transmit power level",
	"Read synchronous flow control enable",
	"Write synchronous flow control enable",
	"Reserved",
	"Set host controller to host flow control",
	"Reserved",
	"Host buffer size",
	"Reserved",
	"Host number of completed packets",
	"Read link supervision timeout",
	"Write link supervision timeout",
	"Read number of supported IAC",
	"Read current IAC LAP",
	"Write current IAC LAP",
	"Read page scan period mode", // not 2.1
	"Write page scan period mode", // not 2.1
	"Read page scan mode",		// not 2.1
	"Write page scan mode",		// not 2.1
	"Set AFH channel classification",
	"Reserved",
	"Reserved",
	"Read inquiry scan type",
	"Write inquiry scan type",
	"Read inquiry mode",
	"Write inquiry mode",
	"Read page scan type",
	"Write page scan type",
	"Read AFH channel assessment mode",
	"Write AFH channel assessment mode",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Read extended inquiry response",
	"Write extended inquiry response",
	"Refresh encryption key",
	"Reserved",
	"Read simple pairing mode",
	"Write simple pairing mode",
	"Read local OOB data",
	"Read inquiry transmit power level",
	"Write inquiry transmit power level",
	"Read default erroneous data reporting",
	"Write default erroneous data reporting",
	"Reserved",
	"Reserved",
	"Reserved",
	"Enhanced flush",
	"Send keypress notification"
};


const char* informationalParametersCommands[] = {	
	"Read local version information",
	"Read local supported commands",
	"Read local supported features",
	"Read local extended features",
	"Read buffer size",
	"Reserved",
	"Read country code", // not 2.1
	"Reserved",
	"Read BD ADDR"
};

	
const char* statusParametersCommands[] = {
	"Read failed contact counter",
	"Reset failed contact counter",
	"Read link quality",
	"Reserved",
	"Read RSSI",
	"Read AFH channel map",
	"Read clock",
};


const char* testingCommands[] = {
	"Read loopback mode",
	"Write loopback mode",
	"Enable device under test mode",
	"Write simple pairing debug mode",
};


const char* bluetoothEvents[] = {
	"Inquiry complete",
	"Inquiry result",
	"Conn complete",
	"Conn request",
	"Disconnection complete",
	"Auth complete",
	"Remote name request complete",
	"Encrypt change",
	"Change conn link key complete",
	"Master link key compl",
	"Rmt features",
	"Rmt version",
	"QoS setup complete",
	"Command complete",
	"Command status",
	"Hardware error",
	"Flush occur",
	"Role change",
	"Num comp Pkts",
	"Mode change",
	"Return link keys",
	"Pin code req",
	"Link key req",
	"Link key notify",
	"Loopback command",
	"Data buffer overflow",
	"Max slot change",
	"Read clock offset compl",
	"Con Pkt type changed",
	"QoS violation",
	"Reserved",
	"Page scan Rep mode change",
	"Flow specification",
	"Inquiry result with RSSI",
	"Remote extended features",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Synchronous connection completed",
	"Synchronous connection changed",
	"Reserved",
	"Extended inquiry result",
	"Encryption key refresh complete",
	"IO capability request",
	"IO capability response",
	"User confirmation request",
	"User passkey request",
	"OOB data request",
	"Simple pairing complete",
	"Reserved",
	"Link supervision timeout changed",
	"Enhanced flush complete",
	"Reserved",
	"Reserved",
	"Keypress notification",
	"Remote host supported features notification"
};


const char* bluetoothErrors[] = {
	"No error",
	"Unknown command",
	"No connection",
	"Hardware failure",
	"Page timeout",
	"Authentication failure",
	"Pin or key missing",
	"Memory full",
	"Connection timeout",
	"Max number of connections",
	"Max number of SCO connections",
	"Acl connection exists",
	"Command disallowed",
	"Rejected limited resources",
	"Rejected security",
	"Rejected personal",
	"Host timeout",
	"Unsupported feature",
	"Invalid parameters",
	"Remote user ended connection",
	"Remote low resources",
	"Remote power off",
	"Connection terminated",
	"Repeated attempts",
	"Pairing not allowed",
	"Unknown LMP Pdu",
	"Unsupported remote feature",
	"SCO offset rejected",
	"SCO interval rejected",
	"Air mode rejected",
	"Invalid LMP parameters",
	"Unspecified error",
	"Unsupported LMP parameter value",
	"Role change not allowed",
	"LMP response timeout",
	"LMP error transaction collision",
	"LMP Pdu not allowed",
	"Encryption mode not accepted",
	"Unit link key used",
	"QoS not supported",
	"Instant passed",
	"Pairing with unit key not supported",
	"Different transaction collision",
	"QoS unacceptable parameter",
	"QoS rejected",
	"Classification not supported",
	"Insufficient security",
	"Parameter out of range",
	"Reserved",
	"Role switch pending",
	"Reserved",
	"Slot violation",
	"Role switch failed",
	"Extended inquiry response too Large",
	"Simple pairing not supported by host",
	"Host busy pairing"
};


const char* hciVersion[] = { "1.0B" , "1.1 " , "1.2 " , "2.0 " , "2.1 "};
const char* lmpVersion[] = { "1.0 " , "1.1 " , "1.2 " , "2.0 " , "2.1 "};


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
			return linkControlCommands[GET_OPCODE_OCF(opcode)-1];
			break;

		case OGF_LINK_POLICY:
			return linkPolicyCommands[GET_OPCODE_OCF(opcode)-1];
			break;

		case OGF_CONTROL_BASEBAND:
			return controllerBasebandCommands[GET_OPCODE_OCF(opcode)-1];
			break;

		case OGF_INFORMATIONAL_PARAM:
			return informationalParametersCommands[GET_OPCODE_OCF(opcode)-1];
			break;

		case OGF_STATUS_PARAM:
			return statusParametersCommands[GET_OPCODE_OCF(opcode)-1];
			break;

		case OGF_TESTING_CMD:
			return testingCommands[GET_OPCODE_OCF(opcode)-1];
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
BluetoothEvent(uint8 event) {
	
	if (event < sizeof(bluetoothEvents) / sizeof(const char*)) {
		return bluetoothEvents[event-1];
	} else {
		return "Event out of range!";
	}
}


const char* 
BluetoothManufacturer(uint16 manufacturer) {
	
	if (manufacturer < sizeof(bluetoothManufacturers) / sizeof(const char*)) {
		return bluetoothManufacturers[manufacturer];
	} else if (manufacturer == 0xFFFF) {
		return "internal use";
	} else {
		return "not assigned";
	}
}


const char*
BluetoothError(uint8 error) {
	
	if (error < sizeof(bluetoothErrors) / sizeof(const char*)) {
		return bluetoothErrors[error];
	} else {
		return "not specified";
    }
}

