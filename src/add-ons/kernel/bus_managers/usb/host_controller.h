#ifndef HOST_CONTROLLER_INFO_H
#define HOST_CONTROLLER_INFO_H

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