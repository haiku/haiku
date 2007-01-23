#include "SonixCamDevice.h"
#include "CamDebug.h"
#include "CamSensor.h"
#include "CamBufferingDeframer.h"
#include "CamStreamingDeframer.h"

#include <interface/Bitmap.h>
#include <media/Buffer.h>

const usb_named_support_descriptor kSupportedDevices[] = {
{{ 0, 0, 0, 0x0c45, 0x6005 }, "Sonix", "Sonix"},
{{ 0, 0, 0, 0x0c45, 0x6009 }, "Trust", "spacec@m 120" },
{{ 0, 0, 0, 0x0c45, 0x600D }, "Trust", "spacec@m 120" },
{{ 0, 0, 0, 0, 0}, NULL, NULL }
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

// -----------------------------------------------------------------------------
SonixCamDevice::SonixCamDevice(CamDeviceAddon &_addon, BUSBDevice* _device)
          :CamDevice(_addon, _device)
{
	uchar data[8]; /* store bytes returned from sonix commands */
	status_t err;
	fFrameTagState = 0;
	
	memset(fCachedRegs, 0, SN9C102_REG_COUNT);
	fChipVersion = 2;
	if ((GetDevice()->ProductID() & ~0x3F) == 0x6080) {
		fChipVersion = 3; // says V4L2
	}
	switch (GetDevice()->ProductID()) {
	case 0x6001:
	case 0x6005:
	case 0x60ab:
		fSensor = CreateSensor("tas5110c1b");
		break;
	default:
		break;
	}
	
//	fDeframer = new CamBufferingDeframer(this);
	fDeframer = new CamStreamingDeframer(this);
	fDeframer->RegisterSOFTags(sof_marks, 2, sizeof(sof_mark_1), 12);
	fDeframer->RegisterEOFTags(eof_marks, 4, sizeof(eof_mark_1), sizeof(eof_mark_1));
	SetDataInput(fDeframer);
	
	/* init hw */
	
	const BUSBConfiguration *config = GetDevice()->ConfigurationAt(0);
	if (config) {
		const BUSBInterface *inter = config->InterfaceAt(0);
		int i;
		
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
	
	
	if (Sensor()) {
		PRINT((CH ": CamSensor: %s" CT, Sensor()->Name()));
		fInitStatus = Sensor()->Setup();
//		SetVideoFrame(BRect(0, 0, Sensor()->MaxWidth()-1, Sensor()->MaxHeight()-1));
//		SetVideoFrame(BRect(0, 0, 320-1, 240-1));
	}
	//SetScale(1);
}

// -----------------------------------------------------------------------------
SonixCamDevice::~SonixCamDevice()
{
	if (Sensor())
		delete fSensor;
	fSensor = NULL;
}

// -----------------------------------------------------------------------------
bool
SonixCamDevice::SupportsBulk()
{
	return true;
}

// -----------------------------------------------------------------------------
bool
SonixCamDevice::SupportsIsochronous()
{
	return true;
}

// -----------------------------------------------------------------------------
status_t
SonixCamDevice::StartTransfer()
{
	status_t err;
	uint8 r;
	
	SetScale(1);
	if (Sensor())
		SetVideoFrame(BRect(0, 0, Sensor()->MaxWidth()-1, Sensor()->MaxHeight()-1));
	
	SetVideoFrame(BRect(0, 0, 320-1, 240-1));

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

// -----------------------------------------------------------------------------
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

// -----------------------------------------------------------------------------
ssize_t
SonixCamDevice::WriteReg(uint16 address, uint8 *data, size_t count)
{
	PRINT((CH "(%u, @%p, %u)" CT, address, data, count));
	if (address + count > SN9C102_REG_COUNT) {
		PRINT((CH ": Invalid register range [%u;%u]" CT, address, address+count));
		return EINVAL;
	}
	memcpy(&fCachedRegs[address], data, count);
	return SendCommand(USB_REQTYPE_DEVICE_OUT, 0x08, address, 0, count, data);
}

// -----------------------------------------------------------------------------
ssize_t
SonixCamDevice::ReadReg(uint16 address, uint8 *data, size_t count, bool cached)
{
	PRINT((CH "(%u, @%p, %u, %d)" CT, address, data, count, cached));
	if (address + count > SN9C102_REG_COUNT) {
		PRINT((CH ": Invalid register range [%u;%u]" CT, address, address+count));
		return EINVAL;
	}
	if (cached) {
		memcpy(data, &fCachedRegs[address], count);
		return count;
	}
	return SendCommand(USB_REQTYPE_DEVICE_IN, 0x00, address, 0, count, data);
}

// -----------------------------------------------------------------------------
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

// -----------------------------------------------------------------------------
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

// -----------------------------------------------------------------------------
ssize_t
SonixCamDevice::WriteIIC(uint8 address, uint8 *data, size_t count)
{
	status_t err;
	uint8 status;
	uint8 buffer[8];
	if (!Sensor())
		return B_NO_INIT;
	//dprintf(ID "sonix_i2c_write_multi(, %02x, %d, {%02x, %02x, %02x, %02x, %02x})\n", slave, count, d0, d1, d2, d3, d4);
	count++; // includes address
	if (count > 5)
		return EINVAL;
	buffer[0] = (count << 4) | Sensor()->Use400kHz()?0x01:0
							 | Sensor()->UseRealIIC()?0x80:0;
	buffer[1] = Sensor()->IICWriteAddress();
	buffer[2] = address;
	memset(&buffer[3], 0, 5);
	memcpy(&buffer[3], data, count);
	buffer[7] = 0x14; /* absolutely no idea why V4L2 driver use that value */
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

// -----------------------------------------------------------------------------
ssize_t
SonixCamDevice::ReadIIC(uint8 address, uint8 *data)
{
	status_t err, lasterr = B_OK;
	uint8 status;
	uint8 buffer[8];
	if (!Sensor())
		return B_NO_INIT;
	//dprintf(ID "sonix_i2c_write_multi(, %02x, %d, {%02x, %02x, %02x, %02x, %02x})\n", slave, count, d0, d1, d2, d3, d4);
	buffer[0] = (1 << 4) | Sensor()->Use400kHz()?0x01:0
						 | Sensor()->UseRealIIC()?0x80:0;
	buffer[1] = Sensor()->IICWriteAddress();
	buffer[2] = address;
	buffer[7] = 0x10; /* absolutely no idea why V4L2 driver use that value */
	err = WriteReg(SN9C102_I2C_SETUP, buffer, 8);
	//dprintf(ID "sonix_i2c_write_multi: set_regs error 0x%08lx\n", err);
	if (err) return err;
	err = WaitReadyIIC();
	//dprintf(ID "sonix_i2c_write_multi: sonix_i2c_wait_ready error 0x%08lx\n", err);
	//if (err) return err;
	
	
	//dprintf(ID "sonix_i2c_write_multi(, %02x, %d, {%02x, %02x, %02x, %02x, %02x})\n", slave, count, d0, d1, d2, d3, d4);
	buffer[0] = (1 << 4) | Sensor()->Use400kHz()?0x01:0
				  | 0x02 | Sensor()->UseRealIIC()?0x80:0; /* read 1 byte */
	buffer[1] = Sensor()->IICReadAddress();//IICWriteAddress
	buffer[7] = 0x10; /* absolutely no idea why V4L2 driver use that value */
	err = WriteReg(SN9C102_I2C_SETUP, buffer, 8);
	//dprintf(ID "sonix_i2c_write_multi: set_regs error 0x%08lx\n", err);
	if (err) return err;
	err = WaitReadyIIC();
	//dprintf(ID "sonix_i2c_write_multi: sonix_i2c_wait_ready error 0x%08lx\n", err);
	if (err) return err;
	
	err = ReadReg(SN9C102_I2C_DATA0, buffer, 5);
	if (err) lasterr = err;

	err = GetStatusIIC();
	//dprintf(ID "sonix_i2c_write_multi: sonix_i2c_status error 0x%08lx\n", err);
	if (err) return err;
	//dprintf(ID "sonix_i2c_write_multi: succeeded\n");
	if (lasterr) return err;

	/* we should get what we want in buffer[4] according to the V4L2 driver...
	 * probably because the 5 bytes are a bit shift register?
	 */
	*data = buffer[4];

	return 1;
}

// -----------------------------------------------------------------------------
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

// -----------------------------------------------------------------------------
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

// -----------------------------------------------------------------------------
status_t
SonixCamDevice::SetVideoParams(float brightness, float contrast, float hue, float red, float green, float blue)
{
	return B_OK;
}

// -----------------------------------------------------------------------------
size_t
SonixCamDevice::MinRawFrameSize()
{
	// if (fCompressionEnabled) { ... return ; }
	BRect vf(VideoFrame());
	int w = vf.IntegerWidth()+1;
	int h = vf.IntegerHeight()+1;
	// 1 byte/pixel
	return (size_t)(w*h);
}

// -----------------------------------------------------------------------------
size_t
SonixCamDevice::MaxRawFrameSize()
{
	// if (fCompressionEnabled) { ... return ; }
	return MinRawFrameSize()+1024*0; // fixed size frame (uncompressed)
}

// -----------------------------------------------------------------------------
bool
SonixCamDevice::ValidateStartOfFrameTag(const uint8 *tag, size_t taglen)
{
	// SOF come with an 00, 40, 80, C0 sequence,
	// supposedly corresponding with an equal byte in the end tag
	fFrameTagState = tag[7] & 0xC0;
	PRINT((CH "(, %d) state %x" CT, taglen, fFrameTagState));
	
	// which seems to be the same as of the EOF tag
	return true;
}

// -----------------------------------------------------------------------------
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

// -----------------------------------------------------------------------------
status_t
SonixCamDevice::GetFrameBitmap(BBitmap **bm)
{
	BBitmap *b;
	CamFrame *f;
	bigtime_t stamp;
	status_t err;
	PRINT((CH "()" CT));
	err = fDeframer->WaitFrame(200000);
	if (err < B_OK) { PRINT((CH ": WaitFrame: %s" CT, strerror(err))); }
	if (err < B_OK)
		return err;
	err = fDeframer->GetFrame(&f, &stamp);
	if (err < B_OK) { PRINT((CH ": GetFrame: %s" CT, strerror(err))); }
	if (err < B_OK)
		return err;
	PRINT((CH ": VideoFrame = %fx%f,%fx%f" CT, VideoFrame().left, VideoFrame().top, VideoFrame().right, VideoFrame().bottom));
	
	long int w = VideoFrame().right - VideoFrame().left + 1;
	long int h = VideoFrame().bottom - VideoFrame().top + 1;
	b = new BBitmap(VideoFrame().OffsetToSelf(0,0), 0, B_RGB32, w*4);
	PRINT((CH ": Frame: %dx%d" CT, w, h));
	
	bayer2rgb24((unsigned char *)b->Bits(), (unsigned char *)f->Buffer(), w, h);
	
	PRINT((CH ": got 1 frame (len %d)" CT, b->BitsLength()));
	*bm = b;
	return B_OK;
}

// -----------------------------------------------------------------------------
status_t
SonixCamDevice::FillFrameBuffer(BBuffer *buffer)
{
	CamFrame *f;
	bigtime_t stamp;
	status_t err;
	PRINT((CH "()" CT));

	memset(buffer->Data(), 0, buffer->SizeAvailable());
	err = fDeframer->WaitFrame(2000000);
	if (err < B_OK) { PRINT((CH ": WaitFrame: %s" CT, strerror(err))); }
	if (err < B_OK)
		return err;

	err = fDeframer->GetFrame(&f, &stamp);
	if (err < B_OK) { PRINT((CH ": GetFrame: %s" CT, strerror(err))); }
	if (err < B_OK)
		return err;

	long int w = VideoFrame().right - VideoFrame().left + 1;
	long int h = VideoFrame().bottom - VideoFrame().top + 1;
	PRINT((CH ": VideoFrame = %fx%f,%fx%f Frame: %dx%d" CT, VideoFrame().left, VideoFrame().top, VideoFrame().right, VideoFrame().bottom, w, h));
	
	if (buffer->SizeAvailable() >= w*h*4)
		bayer2rgb32le((unsigned char *)buffer->Data(), (unsigned char *)f->Buffer(), w, h);
	
	delete f;
	
	PRINT((CH ": available %d, required %d" CT, buffer->SizeAvailable(), w*h*4));
	if (buffer->SizeAvailable() < w*h*4)
		return E2BIG;
	PRINT((CH ": got 1 frame (len %d)" CT, buffer->SizeUsed()));
	return B_OK;
}

// -----------------------------------------------------------------------------
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

// -----------------------------------------------------------------------------
status_t
SonixCamDevice::SendCommand(uint8 dir, uint8 request, uint16 value,
							uint16 index, uint16 length, void* data)
{
	size_t ret;
	if (length > 64)
		return EINVAL;
	if (!GetDevice())
		return ENODEV;
	ret = GetDevice()->ControlTransfer(
				USB_REQTYPE_VENDOR | USB_REQTYPE_INTERFACE_OUT | dir, 
				request, value, index, length, data);
	return ret;
}

// -----------------------------------------------------------------------------
SonixCamDeviceAddon::SonixCamDeviceAddon(WebCamMediaAddOn* webcam)
	: CamDeviceAddon(webcam)
{
	SetSupportedDevices(kSupportedDevices);
}

// -----------------------------------------------------------------------------
SonixCamDeviceAddon::~SonixCamDeviceAddon()
{
}

// -----------------------------------------------------------------------------
const char *
SonixCamDeviceAddon::BrandName()
{
	return "Sonix";
}

// -----------------------------------------------------------------------------
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


