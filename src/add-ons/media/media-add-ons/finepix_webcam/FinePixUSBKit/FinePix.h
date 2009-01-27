#include <USBKit.h>


#define MAX_BUFFER_SIZE 4096 //Size ot transfer buffer from camera

/* IDs of cameras the driver (hopefully) supports. Some different 
 * cameras have the same USB ids, so we just keep one here.  */
#define USB_FUJIFILM_VENDOR_ID  0x04cb

#define USB_FINEPIX_4800_PID	0x0104
#define USB_FINEPIX_F601_PID	0x0109
#define USB_FINEPIX_S602_PID	0x010b
#define USB_FINEPIX_F402_PID	0x010f
#define USB_FINEPIX_M603_PID	0x0111
#define USB_FINEPIX_A202_PID	0x0113
#define USB_FINEPIX_F401_PID	0x0115
#define USB_FINEPIX_A203_PID	0x0117
#define USB_FINEPIX_A303_PID	0x0119
#define USB_FINEPIX_S304_PID	0x011b
#define USB_FINEPIX_A204_PID	0x011d
#define USB_FINEPIX_F700_PID	0x0121
#define USB_FINEPIX_F410_PID	0x0123
#define USB_FINEPIX_A310_PID	0x0125
#define USB_FINEPIX_A210_PID	0x0127
#define USB_FINEPIX_A205_PID	0x0129
#define USB_FINEPIX_X1_PID		0x012B
#define USB_FINEPIX_S7000_PID	0x012d
#define USB_FINEPIX_X2_PID		0x012F
#define USB_FINEPIX_S5000_PID	0x0131
#define USB_FINEPIX_X3_PID		0x013B
#define USB_FINEPIX_S3000_PID	0x013d
#define USB_FINEPIX_X4_PID		0x013f


class FinePix : private BUSBRoster
{
public:
	FinePix();
	virtual ~FinePix();

	status_t InitCheck();			// check if any error occurs
	int SetupCam();					// ready camera for sending pictures
	int GetPic(uint8 *frame, int &total_size);			// get pictures!
	
	status_t DeviceAdded(BUSBDevice* dev);	//dev added
	void DeviceRemoved(BUSBDevice* dev);		//dev  removed

private:
	BUSBDevice* camera;
	const BUSBEndpoint* bulk_in;
};
