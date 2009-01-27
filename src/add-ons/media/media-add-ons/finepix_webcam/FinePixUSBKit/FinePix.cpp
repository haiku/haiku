#include <stdio.h>
#include "FinePix.h"

/* Describes the hardware. */
struct camera_hw {
	unsigned long /*__u16*/ vendor;
	unsigned long /*__u16*/ product;
	/* Offical name of the camera. */
	char name[32];
};

static struct camera_hw cam_supp[23] = {
	{USB_FUJIFILM_VENDOR_ID, USB_FINEPIX_4800_PID, "Fujifilm FinePix 4800"},
	{USB_FUJIFILM_VENDOR_ID, USB_FINEPIX_A202_PID, "Fujifilm FinePix A202"},
	{USB_FUJIFILM_VENDOR_ID, USB_FINEPIX_A203_PID, "Fujifilm FinePix A203"},
	{USB_FUJIFILM_VENDOR_ID, USB_FINEPIX_A204_PID, "Fujifilm FinePix A204"},
	{USB_FUJIFILM_VENDOR_ID, USB_FINEPIX_A205_PID, "Fujifilm FinePix A205"},
	{USB_FUJIFILM_VENDOR_ID, USB_FINEPIX_A210_PID, "Fujifilm FinePix A210"},
	{USB_FUJIFILM_VENDOR_ID, USB_FINEPIX_A303_PID, "Fujifilm FinePix A303"},
	{USB_FUJIFILM_VENDOR_ID, USB_FINEPIX_A310_PID, "Fujifilm FinePix A310"},
	{USB_FUJIFILM_VENDOR_ID, USB_FINEPIX_F401_PID, "Fujifilm FinePix F401"},
	{USB_FUJIFILM_VENDOR_ID, USB_FINEPIX_F402_PID, "Fujifilm FinePix F402"},
	{USB_FUJIFILM_VENDOR_ID, USB_FINEPIX_F410_PID, "Fujifilm FinePix F410"},
	{USB_FUJIFILM_VENDOR_ID, USB_FINEPIX_F601_PID, "Fujifilm FinePix F601"},
	{USB_FUJIFILM_VENDOR_ID, USB_FINEPIX_F700_PID, "Fujifilm FinePix F700"},
	{USB_FUJIFILM_VENDOR_ID, USB_FINEPIX_M603_PID, "Fujifilm FinePix M603"},
	{USB_FUJIFILM_VENDOR_ID, USB_FINEPIX_S3000_PID,
	 "Fujifilm FinePix S3000"},
	{USB_FUJIFILM_VENDOR_ID, USB_FINEPIX_S304_PID, "Fujifilm FinePix S304"},
	{USB_FUJIFILM_VENDOR_ID, USB_FINEPIX_S5000_PID,
	 "Fujifilm FinePix S5000"},
	{USB_FUJIFILM_VENDOR_ID, USB_FINEPIX_S602_PID, "Fujifilm FinePix S602"},
	{USB_FUJIFILM_VENDOR_ID, USB_FINEPIX_S7000_PID,
	 "Fujifilm FinePix S7000"},

	{USB_FUJIFILM_VENDOR_ID, USB_FINEPIX_X1_PID,
	 "Fujifilm FinePix unknown model"},
	{USB_FUJIFILM_VENDOR_ID, USB_FINEPIX_X2_PID,
	 "Fujifilm FinePix unknown model"},
	{USB_FUJIFILM_VENDOR_ID, USB_FINEPIX_X3_PID,
	 "Fujifilm FinePix unknown model"},
	{USB_FUJIFILM_VENDOR_ID, USB_FINEPIX_X4_PID,
	 "Fujifilm FinePix unknown model"},
};

FinePix::FinePix()
{
	fprintf(stderr, "FinePix::FinePix()\n");

	// Initially we don't have a camera device opened.
	camera = NULL;

	// Start the roster that will wait for FinePix USB devices.
	BUSBRoster::Start();
}

FinePix::~FinePix()
{
	fprintf(stderr, "FinePix: ~FinePix()\n");

	BUSBRoster::Stop();
}

status_t FinePix::InitCheck()
{
	fprintf(stderr, "FinePix: InitCheck()\n");

	if (camera != NULL)
		return B_NO_ERROR;
	else
		return B_ERROR;
}

int FinePix::SetupCam()
{
	int ret; // Return value
	
	fprintf(stderr, "FinePix: SetupCam()\n");
	
	/* Reset bulk in endpoint */				
	camera->ControlTransfer (USB_REQTYPE_STANDARD | USB_REQTYPE_ENDPOINT_IN, 
						 USB_REQUEST_CLEAR_FEATURE, USB_FEATURE_ENDPOINT_HALT, 0, 0, NULL);
	
	/* Reset the camera *//* Init the device */

		unsigned char data[] = { 0xc6, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00 };
		
		ret = camera->ControlTransfer(USB_REQTYPE_INTERFACE_OUT |USB_REQTYPE_CLASS ,
										 USB_REQUEST_GET_STATUS, 0x00, 0x00, 12, data);

		fprintf(stderr,"data: %x, %x, %x, %x, %x, %x, %x, %x, %x, %x, %x, %x\n", 
					data[0], data[1],data[2], data[3],data[4], data[5],
					data[6], data[7],data[8], data[9],data[10], data[11]);

		if (ret != 12) {
			fprintf(stderr,"usb_control_msg failed (%d)\n", ret);
			return 1;
		}
		
		unsigned char data3[MAX_BUFFER_SIZE]; 
		ret = bulk_in->BulkTransfer(data3, MAX_BUFFER_SIZE); 
			fprintf(stderr,"BulkIn: %x, %x, %x, %x, %x, %x\n", 
					data3[0],data3[1],data3[2],data3[3],data3[4],data3[5]);
		
		if (ret < 0) {
			fprintf(stderr,"failed to read the result (%d)\n", ret);
			//return 1;
		}

	/* Again, reset bulk in endpoints */
	camera->ControlTransfer (USB_REQTYPE_STANDARD | USB_REQTYPE_ENDPOINT_IN, 
						 USB_REQUEST_CLEAR_FEATURE, USB_FEATURE_ENDPOINT_HALT, 0, 0, NULL);
	
	return 0;
}

int FinePix::GetPic(uint8 *frame, int &total_size)
{
	fprintf(stderr, "FinePix: GetPic()\n");
	int ret; // Return value
	
	/* Request a frame */
	fprintf(stderr,"request a frame\n");
	
	unsigned char data2[] = { 0xd3, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00 };

	ret = camera->ControlTransfer(USB_REQTYPE_INTERFACE_OUT |USB_REQTYPE_CLASS ,
									 USB_REQUEST_GET_STATUS, 0x00, 0x00, 12, data2);
		
	fprintf(stderr,"data: %x, %x, %x, %x, %x, %x, %x, %x, %x, %x, %x, %x\n", 
				data2[0], data2[1],data2[2], data2[3],data2[4], data2[5],
				data2[6], data2[7],data2[8], data2[9],data2[10], data2[11]);

	if (ret != 12) 
	{
		fprintf(stderr,"usb_control_msg failed (%d)\n", ret);
		return 1;
	}
	
	/* Read the frame */
	int offset = 0;
	total_size = 0;

	do 
	{
		fprintf(stderr,"reading part of the frame\n");

		ret = bulk_in->BulkTransfer(&frame[offset], MAX_BUFFER_SIZE); 
					
		fprintf(stderr,"frame: %x, %x, %x, %x, %x, %x, %x, %x, %x, %x, %x, %x\n", 
			frame[offset+0], frame[offset+1],frame[offset+2], frame[offset+3],
			frame[offset+4], frame[offset+5],frame[offset+6], frame[offset+7],
			frame[offset+8], frame[offset+9],frame[offset+10], frame[offset+11]);			
			
		if (ret < 0) { //this doesn't help, if we have an error we hang at the transfer
			fprintf(stderr,"failed to read (%d)\n", ret);

			return 1;
		}

		offset += ret;
		total_size += ret;

		if (ret != MAX_BUFFER_SIZE) // not a full buffer, must be end of frame
			break;			
	} while(1);

	fprintf(stderr,"this frame was %d bytes\n", total_size);
			
	return 0;
}

status_t FinePix::DeviceAdded(BUSBDevice *dev)
{
	fprintf(stderr, "FinePix: DeviceAdded()\n");

	// Waits for FinePix devices.  When one is attached, configure it,
	// so that we are ready to use it.

	if (camera != NULL)
		return B_ERROR;
		
	int myCam = -1;
	// Find the device in our hardware list
	if (dev->VendorID() == USB_FUJIFILM_VENDOR_ID) 
	{
		for (int j = 0; j < 23; j++) //TODO use a cleaner way to check j < cam_supp.count
		{ 
			if (cam_supp[j].product == dev->ProductID()) 
			{
				myCam = j;
				break;
			}
		}
	}
	
	if (myCam >= 0)
	{
		fprintf(stderr, "Found cam: ");
		fprintf(stderr, cam_supp[myCam].name);
		fprintf(stderr, "\n");
		
		if (dev->SetConfiguration(dev->ConfigurationAt(0)) == 0)
		{
			camera = dev;
			
			//set endpoint
			bulk_in = 0;

			int num_epoints = camera->ActiveConfiguration()->InterfaceAt(0)->CountEndpoints();
			
			fprintf(stderr, "CountEndpoints: %d\n", num_epoints);
			
			for (int i = 0; i < num_epoints; i++) 
			{
				if (camera->ActiveConfiguration()->InterfaceAt(0)->EndpointAt(i)->IsBulk())
					if (camera->ActiveConfiguration()->InterfaceAt(0)->EndpointAt(i)->IsInput())
						bulk_in = camera->ActiveConfiguration()->InterfaceAt(0)->EndpointAt(i);
			}

			if (bulk_in == 0) 
			{
				fprintf(stderr, "bad endpoint");
				return B_ERROR;
			}
			
			fprintf(stderr, "Successfully set configuration!\n");
			
			return B_OK;
		}
		else 
			return B_ERROR;
	}
	else
		return B_ERROR;
}

void FinePix::DeviceRemoved(BUSBDevice *dev)
{
	fprintf(stderr, "FinePix: DeviceRemoved()\n");

	// If they remove our device, then we can't use it anymore.
	if (dev == camera)
		camera = NULL;
}
