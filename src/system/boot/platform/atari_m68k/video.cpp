/*
 * Copyright 2008-2010, François Revol, revol@free.fr. All rights reserved.
 * Copyright 2004-2007, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include "toscalls.h"
#include "video.h"
#include "mmu.h"
//#include "images.h"

#include <arch/cpu.h>
#include <boot/stage2.h>
#include <boot/platform.h>
#include <boot/menu.h>
#include <boot/kernel_args.h>
#include <boot/platform/generic/video.h>
#include <util/list.h>
#include <drivers/driver_settings.h>
#include <GraphicsDefs.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#define TRACE_VIDEO
#ifdef TRACE_VIDEO
#	define TRACE(x) dprintf x
#else
#	define TRACE(x) ;
#endif


static void
dump_vars() {
	dprintf("v_bas_ad: %d\n", *TOSVAR_v_bas_ad);
	dprintf("Physbase %p\n", Physbase());
	dprintf("Logbase %p\n", Logbase());

}

// There are several API available to set video modes on atari platforms,
// cf. http://toshyp.atari.org/en/004.html

class ModeOps {
public:
	ModeOps(const char *name) { fName = name; fInitStatus = B_NO_INIT; };
	~ModeOps() {};
	const char *Name() const { return fName; };
	virtual status_t	Init() { fInitStatus = B_OK; };
	status_t	InitStatus() const { return fInitStatus; };
	struct video_mode	*AllocMode();

	// mode handling
	virtual status_t	Enumerate() = 0;
	virtual status_t	Decode(int16 id, struct video_mode *mode);
	virtual status_t	Get(struct video_mode *mode) = 0;
	virtual status_t	Set(const struct video_mode *mode) = 0;
	virtual status_t	Unset(const struct video_mode *mode) { return B_OK; };

	// current settings
	virtual status_t	SetPalette(const struct video_mode *mode,
							const uint8 *palette) { return B_OK; };
	virtual addr_t		Framebuffer() { return NULL; };

	virtual int16	Width(const struct video_mode *mode=NULL);
	virtual int16	Height(const struct video_mode *mode=NULL);
	virtual int16	Depth(const struct video_mode *mode=NULL);
	virtual int16	BytesPerRow(const struct video_mode *mode=NULL);
	
	virtual void	MakeLabel(const struct video_mode *mode, char *label,
						size_t len);

private:
	const char *fName;
protected:
	status_t fInitStatus;
};

struct video_mode {
	list_link	link;
	ModeOps		*ops;
	color_space	space;
	uint16		mode;
	uint16		width, height, bits_per_pixel;
	uint32		bytes_per_row;
status_t	Set() { ops->Set(this); };
status_t	Unset() { ops->Unset(this); };
};

static struct list sModeList;
static video_mode *sMode, *sDefaultMode;
static uint32 sModeCount;
static addr_t sFrameBuffer;
static bool sModeChosen;


static int
compare_video_modes(video_mode *a, video_mode *b)
{
	int compare = a->width - b->width;
	if (compare != 0)
		return compare;

	compare = a->height - b->height;
	if (compare != 0)
		return compare;

	compare = a->bits_per_pixel - b->bits_per_pixel;
	if (compare != 0)
		return compare;

	compare = a->mode - b->mode;
	if (compare != 0)
		return compare;

	return 0;
}


/*!	Insert the video mode into the list, sorted by resolution and bit depth.
	Higher resolutions/depths come first.
*/
static void
add_video_mode(video_mode *videoMode)
{
	video_mode *mode = NULL;
	while ((mode = (video_mode *)list_get_next_item(&sModeList, mode))
			!= NULL) {
		int compare = compare_video_modes(videoMode, mode);
		if (compare == 0) {
			// mode already exists
			return;
		}

		if (compare > 0)
			break;
	}

	list_insert_item_before(&sModeList, mode, videoMode);
	sModeCount++;
}

//	#pragma mark - 


struct video_mode *
ModeOps::AllocMode()
{
	
	video_mode *videoMode = (video_mode *)malloc(sizeof(struct video_mode));
	if (videoMode == NULL)
		return NULL;

	videoMode->ops = this;
	return videoMode;
}

status_t
ModeOps::Decode(int16 id, struct video_mode *mode)
{
	mode->ops = this;
	mode->mode = id;
	return B_OK;
}


int16
ModeOps::Width(const struct video_mode *mode)
{
	return mode ? mode->width : 0;
}


int16
ModeOps::Height(const struct video_mode *mode)
{
	return mode ? mode->height : 0;
}


int16
ModeOps::Depth(const struct video_mode *mode)
{
	return mode ? mode->bits_per_pixel : 0;
}


int16
ModeOps::BytesPerRow(const struct video_mode *mode)
{
	return mode ? mode->bytes_per_row : 0;
}


void
ModeOps::MakeLabel(const struct video_mode *mode, char *label, size_t len)
{
	sprintf(label, "%ux%u %u bit (%s)", mode->width, mode->height,
			mode->bits_per_pixel, mode->ops->Name());

}


//	#pragma mark - ST/TT XBIOS API

class STModeOps : public ModeOps {
public:
	STModeOps() : ModeOps("ST/TT") {};
	~STModeOps() {};
	virtual status_t	Init();

	virtual status_t	Enumerate();
	virtual status_t	Decode(int16 id, struct video_mode *mode);
	virtual status_t	Get(struct video_mode *mode);
	virtual status_t	Set(const struct video_mode *mode);
	virtual status_t	Unset(const struct video_mode *mode);

	virtual status_t	SetPalette(const struct video_mode *mode,
							const uint8 *palette);
	virtual addr_t		Framebuffer();
	virtual void		MakeLabel(const struct video_mode *mode,
							char *label, size_t len);
private:
	static int16		fPreviousMode;
	static bool			fIsTT;
};


int16 STModeOps::fPreviousMode = -1;
bool STModeOps::fIsTT = false;


status_t
STModeOps::Init()
{
	const tos_cookie *c = tos_find_cookie('_VDO');
	if (c == NULL)
		return ENODEV;
	if (c->ivalue >> 16 < 1)
		return ENODEV;
	if (c->ivalue >= 2)
		fIsTT = true;
	fInitStatus = B_OK;
	return fInitStatus;
}



status_t
STModeOps::Enumerate()
{
	if (fInitStatus < B_OK)
		return fInitStatus;

	static int16 modes[] = { 0, /*TT:*/ 4, 7 };
	for (int i = 0; i < sizeof(modes) / sizeof(int16); i++) {
		if (!fIsTT && i > 0)
			break;

		video_mode *videoMode = AllocMode();
		if (videoMode == NULL)
			continue;

		if (Decode(modes[i], videoMode) != B_OK)
			continue;
		add_video_mode(videoMode);

	}
	return B_OK;

#if 0
	// TODO: use TT video monitor detection and build possible mode list there...
	return ENODEV;
#endif
}


status_t
STModeOps::Decode(int16 id, struct video_mode *mode)
{
	mode->ops = this;
	mode->mode = id;

	switch (id) {
		case 0:
			mode->width = 320;
			mode->height = 200;
			mode->bits_per_pixel = 4;
			break;
		case 4:
			mode->width = 640;
			mode->height = 480;
			mode->bits_per_pixel = 4;
			break;
		case 7:
			mode->width = 320;
			mode->height = 480;
			mode->bits_per_pixel = 8;
			break;
		default:
			mode->bits_per_pixel = 0;
			break;
	}

	mode->bytes_per_row = mode->width * mode->bits_per_pixel / 8;
	return B_OK;
}


status_t
STModeOps::Get(struct video_mode *mode)
{
	if (fInitStatus < B_OK)
		return fInitStatus;

	int16 m = Getrez();
	return Decode(m, mode);
}


status_t
STModeOps::Set(const struct video_mode *mode)
{
	if (fInitStatus < B_OK)
		return fInitStatus;
	if (mode == NULL)
		return B_BAD_VALUE;

	fPreviousMode = Getrez();

#warning M68K: FIXME: allocate framebuffer
	dprintf("Switching to mode 0x%04x\n", mode->mode);
	//VsetScreen(((uint32)0x00d00000), ((uint32)0x00d00000), 3, mode->mode);
	Setscreen(-1, -1, mode->mode, 0);
	if (Getrez() != mode->mode) {
		dprintf("failed to set mode %d. Current is %d\n", mode->mode, fPreviousMode);
		fPreviousMode = -1;
	}

	return B_OK;
}


status_t
STModeOps::Unset(const struct video_mode *mode)
{
	if (fInitStatus < B_OK)
		return fInitStatus;

	if (fPreviousMode != -1) {
		dprintf("Reverting to mode 0x%04x\n", fPreviousMode);
		Setscreen(-1, -1, fPreviousMode, 0);
		fPreviousMode = -1;
	}

	return B_OK;
}


status_t
STModeOps::SetPalette(const struct video_mode *mode, const uint8 *palette)
{
	switch (mode->bits_per_pixel) {
		case 4:
			//VsetRGB(0, 16, palette);
			//XXX: Use ESet*
			break;
		case 8:
			//VsetRGB(0, 256, palette);
			//XXX: Use ESet*
			break;
		default:
			break;
	}
}


addr_t
STModeOps::Framebuffer()
{
	addr_t fb = (addr_t)Physbase();
	return fb;
}


void
STModeOps::MakeLabel(const struct video_mode *mode, char *label,
	size_t len)
{
	ModeOps::MakeLabel(mode, label, len);
	label += strlen(label);
	// XXX no len check
	sprintf(label, " 0x%04x", mode->mode);
}


static STModeOps sSTModeOps;


//	#pragma mark - Falcon XBIOS API

class FalconModeOps : public ModeOps {
public:
	FalconModeOps() : ModeOps("Falcon") {};
	~FalconModeOps() {};
	virtual status_t	Init();

	virtual status_t	Enumerate();
	virtual status_t	Decode(int16 id, struct video_mode *mode);
	virtual status_t	Get(struct video_mode *mode);
	virtual status_t	Set(const struct video_mode *mode);
	virtual status_t	Unset(const struct video_mode *mode);

	virtual status_t	SetPalette(const struct video_mode *mode,
							const uint8 *palette);
	virtual addr_t		Framebuffer();
	virtual void		MakeLabel(const struct video_mode *mode,
							char *label, size_t len);
private:
	static int16		fPreviousMode;
};


int16 FalconModeOps::fPreviousMode = -1;


status_t
FalconModeOps::Init()
{
	const tos_cookie *c = tos_find_cookie('_VDO');
	if (c == NULL)
		return ENODEV;
	if (c->ivalue < 0x00030000)
		return ENODEV;
	fInitStatus = B_OK;
	return fInitStatus;
}



status_t
FalconModeOps::Enumerate()
{
	if (fInitStatus < B_OK)
		return fInitStatus;

	static int16 modes[] = {
		0x001b, 0x001c, 0x002b, 0x002c, 
		0x003a, 0x003b, 0x003c, 0x000c, 
		0x0034, 0x0004
		/*0x003a, 0x003b, 0x0003, 0x000c, 
		0x000b, 0x0033, 0x000c, 0x001c*/ };
	for (int i = 0; i < sizeof(modes) / sizeof(int16); i++) {
		video_mode *videoMode = AllocMode();
		if (videoMode == NULL)
			continue;

		if (Decode(modes[i], videoMode) != B_OK)
			continue;
		add_video_mode(videoMode);

	}
	return B_OK;

#if 0
	// TODO: use falcon video monitor detection and build possible mode list there...
	int16 monitor;
	bool vga = false;
	bool tv = false;
	monitor = VgetMonitor();
	switch (monitor) {
		case 0:
			panic("Monochrome ?\n");
			break;
		case 2:
			vga = true;
			break;
		case 3:
			tv = true;
			break;
		//case 4 & 5: check for CT60
		case 1:
		default:
			dprintf("monitor type %d\n", monitor);
			break;
	}
	return ENODEV;
#endif
}


status_t
FalconModeOps::Decode(int16 id, struct video_mode *mode)
{
	bool vga = (id & 0x0010) != 0;
	//bool pal = (id & 0x0020) != 0;
	bool overscan = (id & 0x0040) != 0;
	bool st = (id & 0x0080) != 0;
	bool interlace = (id & 0x0100) != 0;
	bool dbl = interlace;

	mode->ops = this;
	mode->mode = id;
	// cf. F30.TXT
	mode->width = (id & 0x0008) ? 640 : 320;
	mode->height = (vga ? (interlace ? 400 : 200) : (dbl ? 240 : 480));
	if (overscan) {
		// *= 1.2
		mode->width = (mode->width * 12) / 10;
		mode->height = (mode->width * 12) / 10;
	}
	mode->bits_per_pixel = 1 << (id & 0x0007);
	mode->bytes_per_row = mode->width * mode->bits_per_pixel / 8;
	return B_OK;
}


status_t
FalconModeOps::Get(struct video_mode *mode)
{
	if (fInitStatus < B_OK)
		return fInitStatus;

	int16 m = VsetMode(VM_INQUIRE);
	return Decode(m, mode);
}


status_t
FalconModeOps::Set(const struct video_mode *mode)
{
	if (fInitStatus < B_OK)
		return fInitStatus;
	if (mode == NULL)
		return B_BAD_VALUE;

	fPreviousMode = VsetMode(VM_INQUIRE);

#warning M68K: FIXME: allocate framebuffer
	dprintf("Switching to mode 0x%04x\n", mode->mode);
	//VsetScreen(((uint32)0x00d00000), ((uint32)0x00d00000), 3, mode->mode);
	VsetScreen(((uint32)0x00c00000), ((uint32)0x00c00000), 3, mode->mode);
	//VsetScreen(((uint32)-1), ((uint32)-1), 3, mode->mode);

	return B_OK;
}


status_t
FalconModeOps::Unset(const struct video_mode *mode)
{
	if (fInitStatus < B_OK)
		return fInitStatus;

	if (fPreviousMode != -1) {
		dprintf("Reverting to mode 0x%04x\n", fPreviousMode);
		VsetScreen(-1, -1, 3, fPreviousMode);
		fPreviousMode = -1;
	}

	return B_OK;
}


status_t
FalconModeOps::SetPalette(const struct video_mode *mode, const uint8 *palette)
{
	switch (mode->bits_per_pixel) {
		case 4:
			VsetRGB(0, 16, palette);
			break;
		case 8:
			VsetRGB(0, 256, palette);
			break;
		default:
			break;
	}
}


addr_t
FalconModeOps::Framebuffer()
{
	addr_t fb = (addr_t)Physbase();
	return fb;
}


void
FalconModeOps::MakeLabel(const struct video_mode *mode, char *label,
	size_t len)
{
	ModeOps::MakeLabel(mode, label, len);
	label += strlen(label);
	// XXX no len check
	int16 m = mode->mode;
	sprintf(label, " 0x%04x", mode->mode);
	/*sprintf(label, "%s%s%s%s",
		m & 0x0010 ? " vga" : " tv",
		m & 0x0020 ? " pal" : "",
		m & 0x0040 ? " oscan" : "",
		//m & 0x0080 ? " tv" : "",
		m & 0x0100 ? " ilace" : "");*/
}


static FalconModeOps sFalconModeOps;


//	#pragma mark - Milan XBIOS API

class MilanModeOps : public ModeOps {
public:
	MilanModeOps() : ModeOps("Milan") {};
	~MilanModeOps() {};
	virtual status_t	Init();

	virtual status_t	Enumerate();
	virtual status_t	Decode(int16 id, struct video_mode *mode);
	virtual status_t	Get(struct video_mode *mode);
	virtual status_t	Set(const struct video_mode *mode);
	virtual status_t	Unset(const struct video_mode *mode);

	virtual status_t	SetPalette(const struct video_mode *mode,
							const uint8 *palette);
	virtual addr_t		Framebuffer();
	virtual void		MakeLabel(const struct video_mode *mode,
							char *label, size_t len);
private:
	static int16		fPreviousMode;
};


int16 MilanModeOps::fPreviousMode = -1;


status_t
MilanModeOps::Init()
{
	const tos_cookie *c = tos_find_cookie('_MIL');
	if (c == NULL)
		return ENODEV;
	fInitStatus = B_OK;
	return fInitStatus;
}



status_t
MilanModeOps::Enumerate()
{
	if (fInitStatus < B_OK)
		return fInitStatus;

	SCREENINFO info;
	info.size = sizeof(info);
	

	static int16 modes[] = {
		0x001b, 0x001c, 0x002b, 0x002c, 
		0x003a, 0x003b, 0x003c, 0x000c, 
		0x0034, 0x0004
		/*0x003a, 0x003b, 0x0003, 0x000c, 
		0x000b, 0x0033, 0x000c, 0x001c*/ };
	for (int i = 0; i < sizeof(modes) / sizeof(int16); i++) {
		video_mode *videoMode = AllocMode();
		if (videoMode == NULL)
			continue;

		if (Decode(modes[i], videoMode) != B_OK)
			continue;
		add_video_mode(videoMode);

	}
	return B_OK;

#if 0
	// TODO: use Milan video monitor detection and build possible mode list there...
	int16 monitor;
	bool vga = false;
	bool tv = false;
	monitor = VgetMonitor();
	switch (monitor) {
		case 0:
			panic("Monochrome ?\n");
			break;
		case 2:
			vga = true;
			break;
		case 3:
			tv = true;
			break;
		//case 4 & 5: check for CT60
		case 1:
		default:
			dprintf("monitor type %d\n", monitor);
			break;
	}
	return ENODEV;
#endif
}


status_t
MilanModeOps::Decode(int16 id, struct video_mode *mode)
{
	SCREENINFO info;
	info.size = sizeof(info);
	info.devID = mode->mode;
	info.scrFlags = 0;

	mode->ops = this;
	mode->mode = id;

	Setscreen(-1,&info,MI_MAGIC,CMD_GETINFO);
	
	if (info.scrFlags & SCRINFO_OK == 0)
		return B_ERROR;

	// cf. F30.TXT
	mode->width = info.scrWidth;
	mode->height = info.scrHeight;
	mode->bits_per_pixel = info.scrPlanes;
	mode->bytes_per_row = mode->width * mode->bits_per_pixel / 8;
	return B_OK;
}


status_t
MilanModeOps::Get(struct video_mode *mode)
{
	if (fInitStatus < B_OK)
		return fInitStatus;

	int16 m = -1;
	Setscreen(-1,&m,MI_MAGIC,CMD_GETMODE);
	if (m == -1)
		return B_ERROR;
	return Decode(m, mode);
}


status_t
MilanModeOps::Set(const struct video_mode *mode)
{
	if (fInitStatus < B_OK)
		return fInitStatus;
	if (mode == NULL)
		return B_BAD_VALUE;

	Setscreen(-1,&fPreviousMode,MI_MAGIC,CMD_GETMODE);

#warning M68K: FIXME: allocate framebuffer
	dprintf("Switching to mode 0x%04x\n", mode->mode);
	//VsetScreen(((uint32)0x00d00000), ((uint32)0x00d00000), 3, mode->mode);
	//VsetScreen(((uint32)-1), ((uint32)-1), 3, mode->mode);
	Setscreen(-1,mode->mode,MI_MAGIC,CMD_SETMODE);

	return B_OK;
}


status_t
MilanModeOps::Unset(const struct video_mode *mode)
{
	if (fInitStatus < B_OK)
		return fInitStatus;

	if (fPreviousMode != -1) {
		dprintf("Reverting to mode 0x%04x\n", fPreviousMode);
		Setscreen(-1,fPreviousMode,MI_MAGIC,CMD_SETMODE);
		fPreviousMode = -1;
	}

	return B_OK;
}


status_t
MilanModeOps::SetPalette(const struct video_mode *mode, const uint8 *palette)
{
	switch (mode->bits_per_pixel) {
		case 4:
			//VsetRGB(0, 16, palette);
			break;
		case 8:
			//VsetRGB(0, 256, palette);
			break;
		default:
			break;
	}
}


addr_t
MilanModeOps::Framebuffer()
{
	//XXX
	addr_t fb = (addr_t)Physbase();
	return fb;
}


void
MilanModeOps::MakeLabel(const struct video_mode *mode, char *label,
	size_t len)
{
	ModeOps::MakeLabel(mode, label, len);
	label += strlen(label);
	// XXX no len check
	int16 m = mode->mode;
	sprintf(label, " 0x%04x", mode->mode);
	/*sprintf(label, "%s%s%s%s",
		m & 0x0010 ? " vga" : " tv",
		m & 0x0020 ? " pal" : "",
		m & 0x0040 ? " oscan" : "",
		//m & 0x0080 ? " tv" : "",
		m & 0x0100 ? " ilace" : "");*/
}


static MilanModeOps sMilanModeOps;


//	#pragma mark - ARAnyM NFVDI API

/* NatFeat VDI */
#define FVDIDRV_NFAPI_VERSION	0x14000960L
#define FVDI_GET_VERSION	0
#define FVDI_GET_FBADDR	11
#define FVDI_SET_RESOLUTION	12
#define FVDI_GET_WIDTH	13
#define FVDI_GET_HEIGHT	14
#define FVDI_OPENWK 15
#define FVDI_CLOSEWK 16
#define FVDI_GETBPP	17


class NFVDIModeOps : public ModeOps {
public:
	NFVDIModeOps() : ModeOps("NFVDI") {};
	~NFVDIModeOps() {};
	virtual status_t	Init();
	virtual status_t	Enumerate();
	virtual status_t	Get(struct video_mode *mode);
	virtual status_t	Set(const struct video_mode *mode);
	virtual status_t	Unset(const struct video_mode *mode);
	virtual addr_t		Framebuffer();

	virtual int16	Width(const struct video_mode *mode=NULL);
	virtual int16	Height(const struct video_mode *mode=NULL);
	virtual int16	Depth(const struct video_mode *mode=NULL);

private:
	int32 fNatFeatId;
};


status_t
NFVDIModeOps::Init()
{
	// NF calls not available when the ctor is called
	fNatFeatId = nat_feat_getid("fVDI");
	if (fNatFeatId == 0)
		return B_ERROR;
	dprintf("fVDI natfeat id 0x%08lx\n", fNatFeatId);
	
	int32 version = nat_feat_call(fNatFeatId, FVDI_GET_VERSION);
	dprintf("fVDI NF version %lx\n", version);
	if (version < FVDIDRV_NFAPI_VERSION)
		return B_ERROR;
	fInitStatus = B_OK;
	return fInitStatus;
}


status_t
NFVDIModeOps::Enumerate()
{
	if (fNatFeatId == 0)
		return B_NO_INIT;

	video_mode * mode;

	mode = AllocMode();
	if (mode == NULL)
		return B_ERROR;

	Get(mode);
	//mode->space = ;
	mode->mode = 0;
	mode->width = 800;
	mode->height = 600;
	mode->bits_per_pixel = 8;
	mode->bytes_per_row = mode->width * mode->bits_per_pixel / 8;

	add_video_mode(mode);


	mode = AllocMode();
	if (mode == NULL)
		return B_ERROR;

	Get(mode);
	//mode->space = ;
	mode->mode = 0;
	mode->width = 1024;
	mode->height = 768;
	mode->bits_per_pixel = 16;
	mode->bytes_per_row = mode->width * mode->bits_per_pixel / 8;

	add_video_mode(mode);


	return B_OK;
}


status_t
NFVDIModeOps::Get(struct video_mode *mode)
{
	if (mode == NULL)
		return B_BAD_VALUE;
	if (fNatFeatId == 0)
		return B_NOT_SUPPORTED;
	mode->width = Width();
	mode->height = Height();
	mode->bits_per_pixel = Depth();
	mode->bytes_per_row = mode->width * mode->bits_per_pixel / 8;
	dprintf("Get: %dx%d\n", mode->width, mode->height);
	return B_OK;
}


status_t
NFVDIModeOps::Set(const struct video_mode *mode)
{
	if (mode == NULL)
		return B_BAD_VALUE;
	if (fNatFeatId == 0)
		return B_NOT_SUPPORTED;
	status_t err;
	dprintf("fVDI::Set(%ldx%ld %ld)\n",
		(int32)Width(mode), (int32)Height(mode), (int32)Depth(mode));
	err = nat_feat_call(fNatFeatId, FVDI_SET_RESOLUTION,
		(int32)Width(mode), (int32)Height(mode), (int32)Depth(mode));
	err = toserror(err);
	err = nat_feat_call(fNatFeatId, FVDI_OPENWK);

	return B_OK;
}


status_t
NFVDIModeOps::Unset(const struct video_mode *mode)
{
	if (mode == NULL)
		return B_BAD_VALUE;
	if (fNatFeatId == 0)
		return B_NOT_SUPPORTED;
	nat_feat_call(fNatFeatId, FVDI_CLOSEWK);
	return B_OK;
}


addr_t
NFVDIModeOps::Framebuffer()
{
	addr_t fb;
	if (fNatFeatId == 0)
		return (addr_t)NULL;
	fb = (addr_t)nat_feat_call(fNatFeatId, FVDI_GET_FBADDR);
	dprintf("fb 0x%08lx\n", fb);
	return fb;
}


int16
NFVDIModeOps::Width(const struct video_mode *mode)
{
	if (mode)
		return ModeOps::Width(mode);
	if (fNatFeatId == 0)
		return 0;
	return (int16)nat_feat_call(fNatFeatId, FVDI_GET_WIDTH);
}


int16
NFVDIModeOps::Height(const struct video_mode *mode)
{
	if (mode)
		return ModeOps::Height(mode);
	if (fNatFeatId == 0)
		return 0;
	return (int16)nat_feat_call(fNatFeatId, FVDI_GET_HEIGHT);
}


int16
NFVDIModeOps::Depth(const struct video_mode *mode)
{
	if (mode)
		return ModeOps::Depth(mode);
	if (fNatFeatId == 0)
		return 0;
	return (int16)nat_feat_call(fNatFeatId, FVDI_GETBPP);
}


static NFVDIModeOps sNFVDIModeOps;


//	#pragma mark -


bool
video_mode_hook(Menu *menu, MenuItem *item)
{
	// find selected mode
	video_mode *mode = NULL;

	menu = item->Submenu();
	item = menu->FindMarked();
	if (item != NULL) {
		switch (menu->IndexOf(item)) {
			case 0:
				// "Default" mode special
				sMode = sDefaultMode;
				sModeChosen = false;
				return true;
			//case 1:
				// "Standard VGA" mode special
				// sets sMode to NULL which triggers VGA mode
				//break;
			default:
				mode = (video_mode *)item->Data();
				break;
		}
	}

	if (mode != sMode) {
		// update standard mode
		// ToDo: update fb settings!
		sMode = mode;
platform_switch_to_logo();
	}

	sModeChosen = true;
	return true;
}


Menu *
video_mode_menu()
{
	Menu *menu = new(nothrow) Menu(CHOICE_MENU, "Select Video Mode");
	MenuItem *item;

	menu->AddItem(item = new(nothrow) MenuItem("Default"));
	item->SetMarked(true);
	item->Select(true);
	item->SetHelpText("The Default video mode is the one currently configured "
		"in the system. If there is no mode configured yet, a viable mode will "
		"be chosen automatically.");

	//menu->AddItem(new(nothrow) MenuItem("Standard VGA"));

	video_mode *mode = NULL;
	while ((mode = (video_mode *)list_get_next_item(&sModeList, mode)) != NULL) {
		char label[64];
		mode->ops->MakeLabel(mode, label, sizeof(label));

		menu->AddItem(item = new(nothrow) MenuItem(label));
		item->SetData(mode);
	}

	menu->AddSeparatorItem();
	menu->AddItem(item = new(nothrow) MenuItem("Return to main menu"));
	item->SetType(MENU_ITEM_NO_CHOICE);

	return menu;
}


void
platform_blit4(addr_t frameBuffer, const uint8 *data,
	uint16 width, uint16 height, uint16 imageWidth, uint16 left, uint16 top)
{
	if (!data)
		return;
}


extern "C" void
platform_set_palette(const uint8 *palette)
{
	if (sMode)
		sMode->ops->SetPalette(sMode, palette);
}


//	#pragma mark -


extern "C" void
platform_switch_to_logo(void)
{
	// in debug mode, we'll never show the logo
	if ((platform_boot_options() & BOOT_OPTION_DEBUG_OUTPUT) != 0)
		return;

	addr_t lastBase = gKernelArgs.frame_buffer.physical_buffer.start;
	size_t lastSize = gKernelArgs.frame_buffer.physical_buffer.size;

	if (sMode != NULL) {
		sMode->Set();

		gKernelArgs.frame_buffer.width = sMode->ops->Width(sMode);
		gKernelArgs.frame_buffer.height = sMode->ops->Height(sMode);
		gKernelArgs.frame_buffer.bytes_per_row = sMode->ops->BytesPerRow(sMode);
		gKernelArgs.frame_buffer.depth = sMode->ops->Depth(sMode);
		gKernelArgs.frame_buffer.physical_buffer.size = 
			gKernelArgs.frame_buffer.height
			* gKernelArgs.frame_buffer.bytes_per_row;
		gKernelArgs.frame_buffer.physical_buffer.start =
			sMode->ops->Framebuffer();
		//XXX: FIXME: this is just for testing...
		sFrameBuffer = sMode->ops->Framebuffer();
	} else {
		gKernelArgs.frame_buffer.enabled = false;
		return;
	}
	gKernelArgs.frame_buffer.enabled = true;

#if 1
	// If the new frame buffer is either larger than the old one or located at
	// a different address, we need to remap it, so we first have to throw
	// away its previous mapping
	if (lastBase != 0
		&& (lastBase != gKernelArgs.frame_buffer.physical_buffer.start
			|| lastSize < gKernelArgs.frame_buffer.physical_buffer.size)) {
		mmu_free((void *)sFrameBuffer, lastSize);
		lastBase = 0;
	}
	if (lastBase == 0) {
		// the graphics memory has not been mapped yet!
		sFrameBuffer = mmu_map_physical_memory(
			gKernelArgs.frame_buffer.physical_buffer.start,
			gKernelArgs.frame_buffer.physical_buffer.size, kDefaultPageFlags);
	}
#endif
	video_display_splash(sFrameBuffer);
	dump_vars();
	spin(10000000);
	platform_switch_to_text_mode();
	dprintf("splash done\n");
	dump_vars();
}


extern "C" void
platform_switch_to_text_mode(void)
{
	if (!gKernelArgs.frame_buffer.enabled) {
		return;
	}

	if (sMode)
		sMode->Unset();

	gKernelArgs.frame_buffer.enabled = 0;
}


extern "C" status_t
platform_init_video(void)
{
	gKernelArgs.frame_buffer.enabled = 0;
	list_init(&sModeList);

	dprintf("current video mode: \n");
	dprintf("Vsetmode(-1): 0x%08x\n", VsetMode(VM_INQUIRE));
	dump_vars();

	// NF VDI does not implement FVDI_GET_FBADDR :(
	//sNFVDIModeOps.Init();
	//sNFVDIModeOps.Enumerate();

	if (sMilanModeOps.Init() == B_OK) {
		sMilanModeOps.Enumerate();
	} else if (sSTModeOps.Init() == B_OK) {
		sSTModeOps.Enumerate();
	} else if (sFalconModeOps.Init() == B_OK) {
		sFalconModeOps.Enumerate();
	} else {
		dprintf("No usable video API found\n");
	}
	
	return B_OK;
}

