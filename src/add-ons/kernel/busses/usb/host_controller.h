#ifndef HOST_CONTROLLER_INFO_H
#define HOST_CONTROLLER_INFO_H

/* +++++++++
USB Spec definitions (not interesting for the outside world
+++++++++ */
typedef struct
{
	uint8 bDescLength;
	uint8 bDescriptorType;
	uint8 bNbrPorts;
	uint16 wHubCharacteristics;
	uint8 bPwrOn2PwrGood;
	uint8 bHucContrCurrent;
	uint8 DeviceRemovable; //Should be variable!!!
	uint8 PortPwrCtrlMask;  //Deprecated
} usb_hub_descriptor;

#define USB_DESCRIPTOR_HUB 0x29

// USB Spec 1.1 page 273
typedef struct
{
	uint16 status;
	uint16 change;
} usb_port_status;

//The bits in the usb_port_status struct
// USB 1.1 spec page 274
#define PORT_STATUS_CONNECTION    0x1
#define PORT_STATUS_ENABLE        0x2
#define PORT_STATUS_SUSPEND       0x4
#define PORT_STATUS_OVER_CURRENT  0x8
#define PORT_STATUS_RESET         0x10
#define PORT_STATUS_POWER         0x100
#define PORT_STATUS_LOW_SPEED     0x200

//The feature requests with ports
// USB 1.1 spec page 268
#define PORT_CONNECTION    0
#define PORT_ENABLE        1
#define PORT_SUSPEND       2
#define PORT_OVER_CURRENT  3
#define PORT_RESET         4
#define PORT_POWER         8
#define PORT_LOW_SPEED     9
#define C_PORT_CONNECTION  16
#define C_PORT_ENABLE      17
#define C_PORT_SUSPEND     18
#define C_PORT_OVER_CURRENT 19
#define C_PORT_RESET       20

/* ++++++++++
Internally used types
++++++++++ */

//This embodies a request that can be send
typedef struct
{
	uint8   RequestType; 
	uint8   Request; 
	uint16  Value; 
	uint16  Index; 
	uint16  Length; 
} usb_request_data;

// Describes the internal organs of an usb packet
typedef struct usb_packet_t
{
	uint16 pipe;
	uint8 *requestdata;
	uint8 *buffer;
	int16 bufferlength;
	size_t *actual_length;
	int16 address;		//!!!! TEMPORARY ... or is it?
} usb_packet_t;

typedef struct host_controller_info 
{
	module_info info;
	status_t (*hwstart)(void);
	status_t (*SubmitPacket)(usb_packet_t *); 
} host_controller_info;

#endif //HOST_CONTROLLER_INFO_H
