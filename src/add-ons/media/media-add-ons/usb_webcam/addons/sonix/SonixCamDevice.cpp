/*
 * Copyright 2004-2008, François Revol, <revol@free.fr>.
 * Distributed under the terms of the MIT License.
 */

#include "SonixCamDevice.h"
#include "CamDebug.h"
#include "CamSensor.h"
#include "CamBufferingDeframer.h"
#include "CamStreamingDeframer.h"

#include <ParameterWeb.h>
#include <interface/Bitmap.h>
#include <media/Buffer.h>

const usb_webcam_support_descriptor kSupportedDevices[] = {
{{ 0, 0, 0, 0x0c45, 0x6005 }, "Sonix", "Sonix", "tas5110c1b" }, // mine
{{ 0, 0, 0, 0x0c45, 0x6007 }, "Sonix", "macally ICECAM", "tas5110c1b" }, // Rajah's cam - SN9C101R
{{ 0, 0, 0, 0x0c45, 0x6009 }, "Trust", "spacec@m 120", NULL },
{{ 0, 0, 0, 0x0c45, 0x600d }, "Trust", "spacec@m 120", NULL },

/* other devices that should be supported,
 * cf. sn9c102-1.15 linux driver, sn9c102_sensor.h
 * for IDs and sensors
 */
{{ 0, 0, 0, 0x0c45, 0x6001 }, "Sonix", "Sonix generic", "tas5110c1b" },
{{ 0, 0, 0, 0x0c45, 0x6024 }, "Sonix", "Sonix generic", NULL },
{{ 0, 0, 0, 0x0c45, 0x6025 }, "Sonix", "Sonix generic", "tas5110c1b,XXX" },
{{ 0, 0, 0, 0x0c45, 0x6028 }, "Sonix", "Sonix generic", NULL },
{{ 0, 0, 0, 0x0c45, 0x6029 }, "Sonix", "Sonix generic", NULL },
{{ 0, 0, 0, 0x0c45, 0x602a }, "Sonix", "Sonix generic", NULL },
{{ 0, 0, 0, 0x0c45, 0x602b }, "Sonix", "Sonix generic", NULL },
{{ 0, 0, 0, 0x0c45, 0x602c }, "Sonix", "Sonix generic", NULL },
{{ 0, 0, 0, 0x0c45, 0x6030 }, "Sonix", "Sonix generic", NULL },
{{ 0, 0, 0, 0x0c45, 0x6080 }, "Sonix", "Sonix generic", NULL },
{{ 0, 0, 0, 0x0c45, 0x6082 }, "Sonix", "Sonix generic", NULL },
{{ 0, 0, 0, 0x0c45, 0x6083 }, "Sonix", "Sonix generic", NULL },
{{ 0, 0, 0, 0x0c45, 0x6088 }, "Sonix", "Sonix generic", NULL },
{{ 0, 0, 0, 0x0c45, 0x608a }, "Sonix", "Sonix generic", NULL },
{{ 0, 0, 0, 0x0c45, 0x608b }, "Sonix", "Sonix generic", NULL },
{{ 0, 0, 0, 0x0c45, 0x608c }, "Sonix", "Sonix generic", NULL },
{{ 0, 0, 0, 0x0c45, 0x608e }, "Sonix", "Sonix generic", NULL },
{{ 0, 0, 0, 0x0c45, 0x608f }, "Sonix", "Sonix generic", NULL },
{{ 0, 0, 0, 0x0c45, 0x60a0 }, "Sonix", "Sonix generic", NULL },
{{ 0, 0, 0, 0x0c45, 0x60a2 }, "Sonix", "Sonix generic", NULL },
{{ 0, 0, 0, 0x0c45, 0x60a3 }, "Sonix", "Sonix generic", NULL },
{{ 0, 0, 0, 0x0c45, 0x60a8 }, "Sonix", "Sonix generic", NULL },
{{ 0, 0, 0, 0x0c45, 0x60aa }, "Sonix", "Sonix generic", NULL },
{{ 0, 0, 0, 0x0c45, 0x60ab }, "Sonix", "Sonix generic", "tas5110c1b" },
{{ 0, 0, 0, 0x0c45, 0x60ac }, "Sonix", "Sonix generic", NULL },
{{ 0, 0, 0, 0x0c45, 0x60ae }, "Sonix", "Sonix generic", NULL },
{{ 0, 0, 0, 0x0c45, 0x60af }, "Sonix", "Sonix generic", NULL },
{{ 0, 0, 0, 0x0c45, 0x60b0 }, "Sonix", "Sonix generic", NULL },
{{ 0, 0, 0, 0x0c45, 0x60b2 }, "Sonix", "Sonix generic", NULL },
{{ 0, 0, 0, 0x0c45, 0x60b3 }, "Sonix", "Sonix generic", NULL },
{{ 0, 0, 0, 0x0c45, 0x60b8 }, "Sonix", "Sonix generic", NULL },
{{ 0, 0, 0, 0x0c45, 0x60ba }, "Sonix", "Sonix generic", NULL },
{{ 0, 0, 0, 0x0c45, 0x60bb }, "Sonix", "Sonix generic", NULL },
{{ 0, 0, 0, 0x0c45, 0x60bc }, "Sonix", "Sonix generic", NULL },
{{ 0, 0, 0, 0x0c45, 0x60be }, "Sonix", "Sonix generic", NULL },
{{ 0, 0, 0, 0, 0}, NULL, NULL, NULL }
};

// 12 bytes actually
static const uint8 sof_mark_1[] = { 0xff, 0xff, 0x00, 0xc4, 0xc4, 0x96, 0x00 };
static const uint8 sof_mark_2[] = { 0xff, 0xff, 0x00, 0xc4, 0xc4, 0x96, 0x01 };
static const uint8 *sof_marks[] = { sof_mark_1, sof_mark_2 };

static const uint8 eof_mark_1[] = { 0x00, 0x00, 0x00, 0x00 };
static const uint8 eof_mark_2[] = { 0x40, 0x00, 0x00, 0x00 };
static const uint8 eof_mark_3[] = { 0x80, 0x00, 0x00, 0x00 };
static const uint8 eof_mark_4[] = { 0xc0, 0x00, 0x00, 0x00 };
static const uint8 *eof_marks[] = { eof_mark_1, eof_mark_2, eof_mark_3, eof_mark_4 };

void bayer2rgb24(unsigned char *dst, unsigned char *src, long int WIDTH, long int HEIGHT);
void bayer2rgb32le(unsigned char *dst, unsigned char *src, long int WIDTH, long int HEIGHT);


SonixCamDevice::SonixCamDevice(CamDeviceAddon &_addon, BUSBDevice* _device)
          :CamDevice(_addon, _device)
{
	uchar data[8]; /* store bytes returned from sonix commands */
	status_t err;
	fFrameTagState = 0;

	fRGain = fGGain = fBGain = 0;
	// unknown
	fBrightness = 0.5;

	memset(fCachedRegs, 0, SN9C102_REG_COUNT);
	fChipVersion = 2;
	if ((GetDevice()->ProductID() & ~0x3F) == 0x6080) {
		fChipVersion = 3; // says V4L2
	}
	err = ProbeSensor();

//	fDeframer = new CamBufferingDeframer(this);
	fDeframer = new CamStreamingDeframer(this);
	fDeframer->RegisterSOFTags(sof_marks, 2, sizeof(sof_mark_1), 12);
	fDeframer->RegisterEOFTags(eof_marks, 4, sizeof(eof_mark_1), sizeof(eof_mark_1));
	SetDataInput(fDeframer);

	/* init hw */

	const BUSBConfiguration *config = GetDevice()->ConfigurationAt(0);
	if (config) {
		const BUSBInterface *inter = config->InterfaceAt(0);
		uint32 i;

		GetDevice()->SetConfiguration(config);

		for (i = 0; inter && (i < inter->CountEndpoints()); i++) {
			const BUSBEndpoint *e = inter->EndpointAt(i);
			if (e && e->IsBulk() && e->IsInput()) {
				fBulkIn = e;
				PRINT((CH ": Using inter[0].endpoint[%d]; maxpktsz: %d" CT, i, e->MaxPacketSize()));
				break;
			}
		}

	}

	/* sanity check */
	err = ReadReg(SN9C102_ASIC_ID, data);
	if (err < 0 || data[0] != 0x10) {
		PRINT((CH ": BAD ASIC signature! (%u != %u)" CT, data[0], 0x10));
		return;
	}

		//XXX: the XP driver sends this to the ICECAM... need to investigate.
#if 1
	uint8 tmp_3[] = {
		0x44, 0x44, 0x00, 0x00, 0x00, 0x04, 0x00, 0xa0,
		0x61, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40,
		0x00, 0x41, 0x09, 0x00, 0x16, 0x12, 0x60, 0x86,
		0x3b, 0x0f, 0x0e, 0x06, 0x00, 0x00, 0x03 };
	WriteReg(SN9C102_CHIP_CTRL, tmp_3, 0x1f);
#endif
		//XXX:DEBUG

#if 0
		//XXX: the XP driver sends all this... investigate.
	uint8 tmp_1[] = {0x09, 0x44};
	WriteReg(SN9C102_CHIP_CTRL, tmp_1, sizeof(tmp_1));
	WriteReg8(SN9C102_CHIP_CTRL, 0x44);

	WriteReg8(SN9C102_CLOCK_SEL /*0x17*/, 0x29);

	uint8 tmp_2[] = {0x44, 0x44};
	WriteReg(SN9C102_CHIP_CTRL, tmp_2, 2);
		//URB_FUNCTION_VENDOR_INTERFACE:
		//(USBD_TRANSFER_DIRECTION_OUT, ~USBD_SHORT_TRANSFER_OK)

	uint8 tmp_3[] = {
		0x44, 0x44, 0x00, 0x00, 0x00, 0x04, 0x00, 0xa0,
		0x61, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40,
		0x00, 0x41, 0x09, 0x00, 0x16, 0x12, 0x60, 0x86,
		0x3b, 0x0f, 0x0e, 0x06, 0x00, 0x00, 0x03 };
	WriteReg(SN9C102_CHIP_CTRL, tmp_3, 0x1f);

	uint8 tmp_4[] = {0x01, 0x01, 0x07, 0x06};
	//WriteReg(SN9C102_AE_STRX, tmp_4, 4);

	uint8 tmp_5[] = {0x14, 0x0f};
	//WriteReg(SN9C102_H_SIZE, tmp_5, 2);

	WriteReg8(SN9C102_SYNC_N_SCALE, 0x86);

	WriteReg8(SN9C102_CHIP_CTRL, 0x44);	// again ??

	uint8 tmp_6[] = { 0x60, 0x86 };
	WriteReg(SN9C102_CLOCK_SEL /*0x17*/, tmp_6, 2);

	WriteReg8(SN9C102_PIX_CLK, 0x2b);

	// some IIC stuff for the sensor
	uint8 tmp_7[] = { 0xb0, 0x61, 0x1c, 0xf8, 0x10, 0x00, 0x00, 0x16 };
	//WriteReg(SN9C102_I2C_SETUP, tmp_7, 8);

	WriteReg8(SN9C102_PIX_CLK, 0x4b);

	uint8 tmp_8[] = { 0xa0, 0x61, 0x1c, 0x0f, 0x10, 0x00, 0x00, 0x16 };
	//WriteReg(SN9C102_I2C_SETUP, tmp_8, 8);

#endif
#if 0
	// some IIC stuff for the sensor
	uint8 tmp_7[] = { 0xb0, 0x61, 0x1c, 0xf8, 0x10, 0x00, 0x00, 0x16 };
	WriteReg(SN9C102_I2C_SETUP, tmp_7, 8);

	WriteReg8(SN9C102_PIX_CLK, 0x4b);

	uint8 tmp_8[] = { 0xa0, 0x61, 0x1c, 0x0f, 0x10, 0x00, 0x00, 0x16};
	WriteReg(SN9C102_I2C_SETUP, tmp_8, 8);
#endif

	//WriteReg8(SN9C102_PIX_CLK, 0x4b);

//###############

	if (Sensor()) {
		PRINT((CH ": CamSensor: %s" CT, Sensor()->Name()));
		fInitStatus = Sensor()->Setup();

		fVideoFrame = BRect(0, 0, Sensor()->MaxWidth()-1, Sensor()->MaxHeight()-1);
//		SetVideoFrame(BRect(0, 0, Sensor()->MaxWidth()-1, Sensor()->MaxHeight()-1));
//		SetVideoFrame(BRect(0, 0, 320-1, 240-1));
	}
	//SetScale(1);
}


SonixCamDevice::~SonixCamDevice()
{
	if (Sensor())
		delete fSensor;
	fSensor = NULL;
}


bool
SonixCamDevice::SupportsBulk()
{
	return true;
}


bool
SonixCamDevice::SupportsIsochronous()
{
	return true;
}


status_t
SonixCamDevice::StartTransfer()
{
	status_t err;
	uint8 r;

	SetScale(1);
	if (Sensor())
		SetVideoFrame(fVideoFrame);

	//SetVideoFrame(BRect(0, 0, 320-1, 240-1));

DumpRegs();
	err = ReadReg(SN9C102_CHIP_CTRL, &r, 1, true);
	if (err < 0)
		return err;
	r |= 0x04;
	err = WriteReg8(SN9C102_CHIP_CTRL, r);
	if (err < 0)
		return err;
	return CamDevice::StartTransfer();
}


status_t
SonixCamDevice::StopTransfer()
{
	status_t err;
	uint8 r;

DumpRegs();
	err = CamDevice::StopTransfer();
//	if (err < 0)
//		return err;
	err = ReadReg(SN9C102_CHIP_CTRL, &r, 1, true);
	if (err < 0)
		return err;
	r &= ~0x04;
	err = WriteReg8(SN9C102_CHIP_CTRL, r);
	if (err < 0)
		return err;
	return err;
}


status_t
SonixCamDevice::PowerOnSensor(bool on)
{
	if (OrReg8(SN9C102_CHIP_CTRL, on ? 0x01 : 0x00) < 0)
		return EIO;
	return B_OK;
}


ssize_t
SonixCamDevice::WriteReg(uint16 address, uint8 *data, size_t count)
{
	PRINT((CH "(%u, @%p, %" B_PRIuSIZE ")" CT, address, data, count));
	status_t err;
	if (address + count > SN9C102_REG_COUNT) {
		PRINT((CH ": Invalid register range [%u;%" B_PRIuSIZE "]" CT, address,
			address + count));
		return EINVAL;
	}
	memcpy(&fCachedRegs[address], data, count);
	err = SendCommand(USB_REQTYPE_DEVICE_OUT, 0x08, address, 0, count, data);
	if (err < B_OK)
		return err;
	return count;
}


ssize_t
SonixCamDevice::ReadReg(uint16 address, uint8 *data, size_t count, bool cached)
{
	PRINT((CH "(%u, @%p, %" B_PRIuSIZE ", %d)" CT, address, data, count,
		cached));
	status_t err;
	if (address + count > SN9C102_REG_COUNT) {
		PRINT((CH ": Invalid register range [%u;%" B_PRIuSIZE "]" CT, address,
			address + count));
		return EINVAL;
	}
	if (cached) {
		memcpy(data, &fCachedRegs[address], count);
		return count;
	}
	err = SendCommand(USB_REQTYPE_DEVICE_IN, 0x00, address, 0, count, data);
	if (err < B_OK)
		return err;
	return count;
}


status_t
SonixCamDevice::GetStatusIIC()
{
	status_t err;
	uint8 status = 0;
	err = ReadReg(SN9C102_I2C_SETUP, &status);
	//dprintf(ID "i2c_status: error 0x%08lx, status = %02x\n", err, status);
	if (err < 0)
		return err;
	return (status&0x08)?EIO:0;
}


status_t
SonixCamDevice::WaitReadyIIC()
{
	status_t err;
	uint8 status = 0;
	int tries = 5;
	if (!Sensor())
		return B_NO_INIT;
	while (tries--) {
		err = ReadReg(SN9C102_I2C_SETUP, &status);
		//dprintf(ID "i2c_wait_ready: error 0x%08lx, status = %02x\n", err, status);
		if (err < 0) return err;
		if (status & 0x04) return B_OK;
		//XXX:FIXME:spin((1+5+11*dev->sensor->use_400kHz)*8);
		snooze((1+5+11*Sensor()->Use400kHz())*8);
	}
	return EBUSY;
}


ssize_t
SonixCamDevice::WriteIIC(uint8 address, uint8 *data, size_t count)
{
	status_t err;
	uint8 buffer[8];
	PRINT((CH "(%u, @%p, %" B_PRIuSIZE ")" CT, address, data, count));

	if (!Sensor())
		return B_NO_INIT;
	//dprintf(ID "sonix_i2c_write_multi(, %02x, %d, {%02x, %02x, %02x, %02x, %02x})\n", slave, count, d0, d1, d2, d3, d4);
	count++; // includes address
	if (count > 5)
		return EINVAL;
	buffer[0] = ((count) << 4) | (Sensor()->Use400kHz()?0x01:0)
							 | (Sensor()->UseRealIIC()?0x80:0);
	buffer[1] = Sensor()->IICWriteAddress();
	buffer[2] = address;
	memset(&buffer[3], 0, 5);
	memcpy(&buffer[3], data, count-1);
	buffer[7] = 0x16; /* V4L2 driver uses 0x10, XP driver uses 0x16 ? */
	for (int i = 0; i < 8; i++) {
		PRINT(("[%d] = %02x\n", i, buffer[i]));
	}
	err = WriteReg(SN9C102_I2C_SETUP, buffer, 8);
	//dprintf(ID "sonix_i2c_write_multi: set_regs error 0x%08lx\n", err);
	//PRINT((CH ": WriteReg: %s" CT, strerror(err)));
	if (err < 0) return err;
	err = WaitReadyIIC();
	//dprintf(ID "sonix_i2c_write_multi: sonix_i2c_wait_ready error 0x%08lx\n", err);
	//PRINT((CH ": Wait: %s" CT, strerror(err)));
	if (err) return err;
	err = GetStatusIIC();
	//dprintf(ID "sonix_i2c_write_multi: sonix_i2c_status error 0x%08lx\n", err);
	//PRINT((CH ": Status: %s" CT, strerror(err)));
	if (err) return err;
	//dprintf(ID "sonix_i2c_write_multi: succeeded\n");
	PRINT((CH ": success" CT));
	return B_OK;
}


ssize_t
SonixCamDevice::ReadIIC(uint8 address, uint8 *data)
{
	status_t err, lasterr = B_OK;
	uint8 buffer[8];
	PRINT((CH "(%u, @%p)" CT, address, data));

	if (!Sensor())
		return B_NO_INIT;
	//dprintf(ID "sonix_i2c_write_multi(, %02x, %d, {%02x, %02x, %02x, %02x, %02x})\n", slave, count, d0, d1, d2, d3, d4);
	buffer[0] = (1 << 4) | (Sensor()->Use400kHz()?0x01:0)
						| (Sensor()->UseRealIIC()?0x80:0);
	buffer[1] = Sensor()->IICWriteAddress();
	buffer[2] = address;
	buffer[7] = 0x10; /* absolutely no idea why V4L2 driver use that value */
	err = WriteReg(SN9C102_I2C_SETUP, buffer, 8);
	//dprintf(ID "sonix_i2c_write_multi: set_regs error 0x%08lx\n", err);
	if (err < 8) return EIO;
	err = WaitReadyIIC();
	//dprintf(ID "sonix_i2c_write_multi: sonix_i2c_wait_ready error 0x%08lx\n", err);
	//if (err) return err;


	//dprintf(ID "sonix_i2c_write_multi(, %02x, %d, {%02x, %02x, %02x, %02x, %02x})\n", slave, count, d0, d1, d2, d3, d4);
	buffer[0] = (1 << 4) | (Sensor()->Use400kHz()?0x01:0)
				  | 0x02 | (Sensor()->UseRealIIC()?0x80:0); /* read 1 byte */
	buffer[1] = Sensor()->IICReadAddress();//IICWriteAddress
	buffer[7] = 0x10; /* absolutely no idea why V4L2 driver use that value */
	err = WriteReg(SN9C102_I2C_SETUP, buffer, 8);
	//dprintf(ID "sonix_i2c_write_multi: set_regs error 0x%08lx\n", err);
	if (err < 8) return EIO;
	err = WaitReadyIIC();
	//dprintf(ID "sonix_i2c_write_multi: sonix_i2c_wait_ready error 0x%08lx\n", err);
	if (err < B_OK) return err;

	err = ReadReg(SN9C102_I2C_DATA0, buffer, 5);
	if (err < 5) return EIO;

	err = GetStatusIIC();
	//dprintf(ID "sonix_i2c_write_multi: sonix_i2c_status error 0x%08lx\n", err);
	if (err < B_OK) return err;
	//dprintf(ID "sonix_i2c_write_multi: succeeded\n");
	if (lasterr) return err;

	/* we should get what we want in buffer[4] according to the V4L2 driver...
	 * probably because the 5 bytes are a bit shift register?
	 */
	*data = buffer[4];

	return 1;
}


status_t
SonixCamDevice::SetVideoFrame(BRect frame)
{
	uint16 x, y, width, height;
	x = (uint16)frame.left;
	y = (uint16)frame.top;
	width = (uint16)(frame.right - frame.left + 1) / 16;
	height = (uint16)(frame.bottom - frame.top + 1) / 16;
	PRINT((CH "(%u, %u, %u, %u)" CT, x, y, width, height));

	WriteReg8(SN9C102_H_START, x);
	WriteReg8(SN9C102_V_START, y);
	WriteReg8(SN9C102_H_SIZE, width);
	WriteReg8(SN9C102_V_SIZE, height);
	if (Sensor()) {
		Sensor()->SetVideoFrame(frame);
	}
	return CamDevice::SetVideoFrame(frame);
}


status_t
SonixCamDevice::SetScale(float scale)
{
	status_t err;
	uint8 r;
	int iscale = (int)scale;

	PRINT((CH "(%u)" CT, iscale));
	err = ReadReg(SN9C102_SYNC_N_SCALE, &r, 1, true);
	if (err < 0)
		return err;
	r &= ~0x30;
	switch (iscale) {
	case 1:
	case 2:
	case 4:
		r |= ((iscale-1) << 4);
		break;
	default:
		return EINVAL;
	}
	err = WriteReg8(SN9C102_SYNC_N_SCALE, r);
	return err;
}


status_t
SonixCamDevice::SetVideoParams(float brightness, float contrast, float hue, float red, float green, float blue)
{
	return B_OK;
}

void
SonixCamDevice::AddParameters(BParameterGroup *group, int32 &index)
{
	BParameterGroup *g;
	BContinuousParameter *p;
	CamDevice::AddParameters(group, index);

	// R,G,B gains
	g = group->MakeGroup("RGB gain");
	p = g->MakeContinuousParameter(index++,
		B_MEDIA_RAW_VIDEO, "RGB gain",
		B_GAIN, "", 1.0, 1.0+(float)(SN9C102_RGB_GAIN_MAX)/8, (float)1.0/8);

	p->SetChannelCount(3);
#if 0
	// Contrast - NON FUNCTIONAL
	g = group->MakeGroup("Contrast");
	p = g->MakeContinuousParameter(index++,
		B_MEDIA_RAW_VIDEO, "Contrast",
		B_GAIN, "", 0.0, 1.0, 1.0/256);

	// Brightness - NON FUNCTIONAL
	g = group->MakeGroup("Brightness");
	p = g->MakeContinuousParameter(index++,
		B_MEDIA_RAW_VIDEO, "Brightness",
		B_GAIN, "", 0.0, 1.0, 1.0/256);

#endif
}

status_t
SonixCamDevice::GetParameterValue(int32 id, bigtime_t *last_change, void *value, size_t *size)
{
	float *gains;
	switch (id - fFirstParameterID) {
		case 0:
			*size = 3 * sizeof(float);
			gains = ((float *)value);
			gains[0] = 1.0 + (float)fRGain / 8;
			gains[1] = 1.0 + (float)fGGain / 8;
			gains[2] = 1.0 + (float)fBGain / 8;
			*last_change = fLastParameterChanges;
			return B_OK;
#if 0
		case 1:
			*size = sizeof(float);
			gains = ((float *)value);
			gains[0] = fContrast;
			*last_change = fLastParameterChanges;
			return B_OK;
		case 2:
			*size = sizeof(float);
			gains = ((float *)value);
			gains[0] = fBrightness;
			*last_change = fLastParameterChanges;
			return B_OK;
#endif
	}
	return B_BAD_VALUE;
}

status_t
SonixCamDevice::SetParameterValue(int32 id, bigtime_t when, const void *value, size_t size)
{
	float *gains;
	switch (id - fFirstParameterID) {
		case 0:
			if (!value || (size != 3 * sizeof(float)))
				return B_BAD_VALUE;
			gains = ((float *)value);
			if ((gains[0] == 1.0 + (float)fRGain / 8)
				&& (gains[1] == 1.0 + (float)fGGain / 8)
				&& (gains[2] == 1.0 + (float)fBGain / 8))
				return B_OK;

			fRGain = (int)(8 * (gains[0] - 1.0)) & SN9C102_RGB_GAIN_MAX;
			fGGain = (int)(8 * (gains[1] - 1.0)) & SN9C102_RGB_GAIN_MAX;
			fBGain = (int)(8 * (gains[2] - 1.0)) & SN9C102_RGB_GAIN_MAX;
			fLastParameterChanges = when;
			PRINT((CH ": gain: %d,%d,%d" CT, fRGain, fGGain, fBGain));
			//WriteReg8(SN9C102_R_B_GAIN, (fBGain << 4) | fRGain);	/* red, blue gain = 1+0/8 = 1 */
			/* Datasheet says:
			 * reg 0x10 [0:3] is rgain, [4:7] is bgain and 0x11 [0:3] is ggain
			 * according to sn9c102-1.15 linux driver it's wrong for reg 0x10,
			 * but it doesn't seem to work any better for a rev 2 chip.
			 * XXX
			 */
#if 1
			WriteReg8(SN9C102_R_GAIN, fRGain);
			WriteReg8(SN9C102_B_GAIN, fBGain);
			if (fChipVersion >= 3)
				WriteReg8(SN9C103_G_GAIN, fGGain);
			else
				WriteReg8(SN9C102_G_GAIN, (fGGain / 16));
#endif
#if 0
			uint8 buf[2];
			buf[0] = (fBGain << 4) | fRGain;
			buf[1] = fGGain;
			WriteReg(SN9C102_R_B_GAIN, buf, 2);
#endif
			return B_OK;
#if 0
		case 1:
			if (!value || (size != sizeof(float)))
				return B_BAD_VALUE;
			gains = ((float *)value);
			fContrast = gains[0];
			WriteReg8(SN9C10x_CONTRAST, ((uint8)(fContrast * 256)));
			return B_OK;
		case 2:
			if (!value || (size != sizeof(float)))
				return B_BAD_VALUE;
			gains = ((float *)value);
			fBrightness = gains[0];
			// it actually ends up writing to SN9C102_V_SIZE...
			WriteReg8(SN9C10x_BRIGHTNESS, ((uint8)(fBrightness * 256)));

			return B_OK;
#endif
	}
	return B_BAD_VALUE;
}



size_t
SonixCamDevice::MinRawFrameSize()
{
	// if (fCompressionEnabled) { ... return ; }
	BRect vf(VideoFrame());
	size_t w = vf.IntegerWidth()+1;
	size_t h = vf.IntegerHeight()+1;
	// 1 byte/pixel
	return w * h;
}


size_t
SonixCamDevice::MaxRawFrameSize()
{
	// if (fCompressionEnabled) { ... return ; }
	return MinRawFrameSize()+1024*0; // fixed size frame (uncompressed)
}


bool
SonixCamDevice::ValidateStartOfFrameTag(const uint8 *tag, size_t taglen)
{
	// SOF come with an 00, 40, 80, C0 sequence,
	// supposedly corresponding with an equal byte in the end tag
	fFrameTagState = tag[7] & 0xC0;
	PRINT((CH "(, %" B_PRIuSIZE ") state %x" CT, taglen, fFrameTagState));

	// which seems to be the same as of the EOF tag
	return true;
}


bool
SonixCamDevice::ValidateEndOfFrameTag(const uint8 *tag, size_t taglen, size_t datalen)
{
	//PRINT((CH "(, %d) %x == %x" CT, taglen, (tag[0] & 0xC0), fFrameTagState));
	// make sure the tag corresponds to the SOF we refer to
	if ((tag[0] & 0xC0) != fFrameTagState) {
		PRINT((CH ": discarded EOF %x != %x" CT, fFrameTagState, tag[0] & 0xC0));
		return false;
	}
	//PRINT((CH ": validated EOF %x, len %d" CT, fFrameTagState, datalen));
	return true;
}


status_t
SonixCamDevice::GetFrameBitmap(BBitmap **bm, bigtime_t *stamp /* = NULL */)
{
	BBitmap *b;
	CamFrame *f;
	status_t err;
	PRINT((CH "()" CT));
	err = fDeframer->WaitFrame(200000);
	if (err < B_OK) { PRINT((CH ": WaitFrame: %s" CT, strerror(err))); }
	if (err < B_OK)
		return err;
	err = fDeframer->GetFrame(&f, stamp);
	if (err < B_OK) { PRINT((CH ": GetFrame: %s" CT, strerror(err))); }
	if (err < B_OK)
		return err;
	PRINT((CH ": VideoFrame = %fx%f,%fx%f" CT, VideoFrame().left, VideoFrame().top, VideoFrame().right, VideoFrame().bottom));

	long int w = (long)(VideoFrame().right - VideoFrame().left + 1);
	long int h = (long)(VideoFrame().bottom - VideoFrame().top + 1);
	b = new BBitmap(VideoFrame().OffsetToSelf(0,0), 0, B_RGB32, w*4);
	PRINT((CH ": Frame: %ldx%ld" CT, w, h));

	bayer2rgb24((unsigned char *)b->Bits(), (unsigned char *)f->Buffer(), w, h);

	PRINT((CH ": got 1 frame (len %d)" CT, b->BitsLength()));
	*bm = b;
	return B_OK;
}


status_t
SonixCamDevice::FillFrameBuffer(BBuffer *buffer, bigtime_t *stamp)
{
	CamFrame *f;
	status_t err;
	PRINT((CH "()" CT));

	memset(buffer->Data(), 0, buffer->SizeAvailable());
	err = fDeframer->WaitFrame(2000000);
	if (err < B_OK) { PRINT((CH ": WaitFrame: %s" CT, strerror(err))); }
	if (err < B_OK)
		return err;

	err = fDeframer->GetFrame(&f, stamp);
	if (err < B_OK) { PRINT((CH ": GetFrame: %s" CT, strerror(err))); }
	if (err < B_OK)
		return err;

	long int w = (long)(VideoFrame().right - VideoFrame().left + 1);
	long int h = (long)(VideoFrame().bottom - VideoFrame().top + 1);
	PRINT((CH ": VideoFrame = %fx%f,%fx%f Frame: %ldx%ld" CT,
		VideoFrame().left, VideoFrame().top, VideoFrame().right,
		VideoFrame().bottom, w, h));

	if (buffer->SizeAvailable() >= (size_t)w*h*4)
		bayer2rgb32le((unsigned char *)buffer->Data(), (unsigned char *)f->Buffer(), w, h);

	delete f;

	PRINT((CH ": available %" B_PRIuSIZE ", required %ld" CT,
		buffer->SizeAvailable(), w*h*4));
	if (buffer->SizeAvailable() < (size_t)w*h*4)
		return E2BIG;
	PRINT((CH ": got 1 frame (len %" B_PRIuSIZE ")" CT, buffer->SizeUsed()));
	return B_OK;
}


void
/* DEBUG: dump the SN regs */
SonixCamDevice::DumpRegs()
{
	uint8 regs[SN9C102_REG_COUNT];
	status_t err;

	//err = sonix_get_regs(dev, SN_ASIC_ID, regs, SN_REG_COUNT);
	err = ReadReg(0, regs, SN9C102_REG_COUNT);
	if (err < 0)
		return;
	printf("REG1: %02x %02x %02x %02x  %02x %02x %02x %02x\n",
			regs[0], regs[1], regs[2], regs[3], regs[4], regs[5], regs[6], regs[7]);
	printf("   2: %02x %02x %02x %02x  %02x %02x %02x %02x\n",
			regs[8], regs[9], regs[10], regs[11], regs[12], regs[13], regs[14], regs[15]);
	printf("   3: %02x %02x %02x %02x  %02x %02x %02x %02x\n",
			regs[16], regs[17], regs[18], regs[19], regs[20], regs[21], regs[22], regs[23]);
	printf("   4: %02x %02x %02x %02x  %02x %02x %02x %02x\n",
			regs[24], regs[25], regs[26], regs[27], regs[28], regs[29], regs[30], regs[31]);
}

#if 0

status_t
SonixCamDevice::SendCommand(uint8 dir, uint8 request, uint16 value,
							uint16 index, uint16 length, void* data)
{
	size_t ret;
	if (!GetDevice())
		return ENODEV;
	if (length > GetDevice()->MaxEndpoint0PacketSize())
		return EINVAL;
	ret = GetDevice()->ControlTransfer(
				USB_REQTYPE_VENDOR | USB_REQTYPE_INTERFACE_OUT | dir,
				request, value, index, length, data);
	return ret;
}
#endif


SonixCamDeviceAddon::SonixCamDeviceAddon(WebCamMediaAddOn* webcam)
	: CamDeviceAddon(webcam)
{
	SetSupportedDevices(kSupportedDevices);
}


SonixCamDeviceAddon::~SonixCamDeviceAddon()
{
}


const char *
SonixCamDeviceAddon::BrandName()
{
	return "Sonix";
}


SonixCamDevice *
SonixCamDeviceAddon::Instantiate(CamRoster &roster, BUSBDevice *from)
{
	return new SonixCamDevice(*this, from);
}

extern "C" status_t
B_WEBCAM_MKINTFUNC(sonix)
(WebCamMediaAddOn* webcam, CamDeviceAddon **addon)
{
	*addon = new SonixCamDeviceAddon(webcam);
	return B_OK;
}

// XXX: REMOVE ME



/*
 * BAYER2RGB24 ROUTINE TAKEN FROM:
 *
 * Sonix SN9C101 based webcam basic I/F routines
 * Copyright (C) 2004 Takafumi Mizuno <taka-qce@ls-a.jp>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

void bayer2rgb24(unsigned char *dst, unsigned char *src, long int WIDTH, long int HEIGHT)
{
    long int i;
    unsigned char *rawpt, *scanpt;
    long int size;

    rawpt = src;
    scanpt = dst;
    size = WIDTH*HEIGHT;

    for ( i = 0; i < size; i++ ) {
	if ( (i/WIDTH) % 2 == 0 ) {
	    if ( (i % 2) == 0 ) {
		/* B */
		if ( (i > WIDTH) && ((i % WIDTH) > 0) ) {
		    *scanpt++ = (*(rawpt-WIDTH-1)+*(rawpt-WIDTH+1)+
				 *(rawpt+WIDTH-1)+*(rawpt+WIDTH+1))/4;	/* R */
		    *scanpt++ = (*(rawpt-1)+*(rawpt+1)+
				 *(rawpt+WIDTH)+*(rawpt-WIDTH))/4;	/* G */
		    *scanpt++ = *rawpt;					/* B */
		} else {
		    /* first line or left column */
		    *scanpt++ = *(rawpt+WIDTH+1);		/* R */
		    *scanpt++ = (*(rawpt+1)+*(rawpt+WIDTH))/2;	/* G */
		    *scanpt++ = *rawpt;				/* B */
		}
	    } else {
		/* (B)G */
		if ( (i > WIDTH) && ((i % WIDTH) < (WIDTH-1)) ) {
		    *scanpt++ = (*(rawpt+WIDTH)+*(rawpt-WIDTH))/2;	/* R */
		    *scanpt++ = *rawpt;					/* G */
		    *scanpt++ = (*(rawpt-1)+*(rawpt+1))/2;		/* B */
		} else {
		    /* first line or right column */
		    *scanpt++ = *(rawpt+WIDTH);	/* R */
		    *scanpt++ = *rawpt;		/* G */
		    *scanpt++ = *(rawpt-1);	/* B */
		}
	    }
	} else {
	    if ( (i % 2) == 0 ) {
		/* G(R) */
		if ( (i < (WIDTH*(HEIGHT-1))) && ((i % WIDTH) > 0) ) {
		    *scanpt++ = (*(rawpt-1)+*(rawpt+1))/2;		/* R */
		    *scanpt++ = *rawpt;					/* G */
		    *scanpt++ = (*(rawpt+WIDTH)+*(rawpt-WIDTH))/2;	/* B */
		} else {
		    /* bottom line or left column */
		    *scanpt++ = *(rawpt+1);		/* R */
		    *scanpt++ = *rawpt;			/* G */
		    *scanpt++ = *(rawpt-WIDTH);		/* B */
		}
	    } else {
		/* R */
		if ( i < (WIDTH*(HEIGHT-1)) && ((i % WIDTH) < (WIDTH-1)) ) {
		    *scanpt++ = *rawpt;					/* R */
		    *scanpt++ = (*(rawpt-1)+*(rawpt+1)+
				 *(rawpt-WIDTH)+*(rawpt+WIDTH))/4;	/* G */
		    *scanpt++ = (*(rawpt-WIDTH-1)+*(rawpt-WIDTH+1)+
				 *(rawpt+WIDTH-1)+*(rawpt+WIDTH+1))/4;	/* B */
		} else {
		    /* bottom line or right column */
		    *scanpt++ = *rawpt;				/* R */
		    *scanpt++ = (*(rawpt-1)+*(rawpt-WIDTH))/2;	/* G */
		    *scanpt++ = *(rawpt-WIDTH-1);		/* B */
		}
	    }
	}
	rawpt++;
    }

}

/* modified bayer2rgb24 to output rgb-32 little endian (B_RGB32)
 * François Revol
 */

void bayer2rgb32le(unsigned char *dst, unsigned char *src, long int WIDTH, long int HEIGHT)
{
    long int i;
    unsigned char *rawpt, *scanpt;
    long int size;

    rawpt = src;
    scanpt = dst;
    size = WIDTH*HEIGHT;

    for ( i = 0; i < size; i++ ) {
	if ( (i/WIDTH) % 2 == 0 ) {
	    if ( (i % 2) == 0 ) {
		/* B */
		if ( (i > WIDTH) && ((i % WIDTH) > 0) ) {
		    *scanpt++ = *rawpt;					/* B */
		    *scanpt++ = (*(rawpt-1)+*(rawpt+1)+
				 *(rawpt+WIDTH)+*(rawpt-WIDTH))/4;	/* G */
		    *scanpt++ = (*(rawpt-WIDTH-1)+*(rawpt-WIDTH+1)+
				 *(rawpt+WIDTH-1)+*(rawpt+WIDTH+1))/4;	/* R */
		} else {
		    /* first line or left column */
		    *scanpt++ = *rawpt;				/* B */
		    *scanpt++ = (*(rawpt+1)+*(rawpt+WIDTH))/2;	/* G */
		    *scanpt++ = *(rawpt+WIDTH+1);		/* R */
		}
	    } else {
		/* (B)G */
		if ( (i > WIDTH) && ((i % WIDTH) < (WIDTH-1)) ) {
		    *scanpt++ = (*(rawpt-1)+*(rawpt+1))/2;		/* B */
		    *scanpt++ = *rawpt;					/* G */
		    *scanpt++ = (*(rawpt+WIDTH)+*(rawpt-WIDTH))/2;	/* R */
		} else {
		    /* first line or right column */
		    *scanpt++ = *(rawpt-1);	/* B */
		    *scanpt++ = *rawpt;		/* G */
		    *scanpt++ = *(rawpt+WIDTH);	/* R */
		}
	    }
	} else {
	    if ( (i % 2) == 0 ) {
		/* G(R) */
		if ( (i < (WIDTH*(HEIGHT-1))) && ((i % WIDTH) > 0) ) {
		    *scanpt++ = (*(rawpt+WIDTH)+*(rawpt-WIDTH))/2;	/* B */
		    *scanpt++ = *rawpt;					/* G */
		    *scanpt++ = (*(rawpt-1)+*(rawpt+1))/2;		/* R */
		} else {
		    /* bottom line or left column */
		    *scanpt++ = *(rawpt-WIDTH);		/* B */
		    *scanpt++ = *rawpt;			/* G */
		    *scanpt++ = *(rawpt+1);		/* R */
		}
	    } else {
		/* R */
		if ( i < (WIDTH*(HEIGHT-1)) && ((i % WIDTH) < (WIDTH-1)) ) {
		    *scanpt++ = (*(rawpt-WIDTH-1)+*(rawpt-WIDTH+1)+
				 *(rawpt+WIDTH-1)+*(rawpt+WIDTH+1))/4;	/* B */
		    *scanpt++ = (*(rawpt-1)+*(rawpt+1)+
				 *(rawpt-WIDTH)+*(rawpt+WIDTH))/4;	/* G */
		    *scanpt++ = *rawpt;					/* R */
		} else {
		    /* bottom line or right column */
		    *scanpt++ = *(rawpt-WIDTH-1);		/* B */
		    *scanpt++ = (*(rawpt-1)+*(rawpt-WIDTH))/2;	/* G */
		    *scanpt++ = *rawpt;				/* R */
		}
	    }
	}
	rawpt++;
	scanpt++;
    }

}
