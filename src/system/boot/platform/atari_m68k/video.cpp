/*
 * Copyright 2004-2007, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include "toscalls.h"
#include "video.h"
//#include "mmu.h"
//#include "images.h"

#include <arch/cpu.h>
#include <boot/stage2.h>
#include <boot/platform.h>
#include <boot/menu.h>
#include <boot/kernel_args.h>
#include <util/list.h>
#include <drivers/driver_settings.h>
#include <GraphicsDefs.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


//#define TRACE_VIDEO
#ifdef TRACE_VIDEO
#	define TRACE(x) dprintf x
#else
#	define TRACE(x) ;
#endif


// XXX: use falcon video monitor detection and build possible mode list there...

// which API to use to handle this mode
// cf. http://toshyp.atari.org/004.htm
enum {
	MODETYPE_XBIOS_ST,
	MODETYPE_XBIOS_TT,
	MODETYPE_XBIOS_FALCON,
	MODETYPE_CENTSCREEN,
	MODETYPE_CRAZYDOTS,
	MODETYPE_CT60,
	MODETYPE_NATFEAT
};

class ModeAPI {
public:
	ModeAPI(const char *name) { fName = name; };
	~ModeAPI() {};
	const char *Name() const { return fName; };
	virtual status_t Enumerate() = 0;
	virtual status_t Get(struct video_mode *mode) = 0;
	virtual status_t Set(const struct video_mode *mode) = 0;
private:
	const char *fName;
};

struct video_mode {
	list_link	link;
	ModeAPI		*ops;
	color_space	space;
	uint16		mode;
	uint16		width, height, bits_per_pixel;
	uint32		bytes_per_row;
};

static struct list sModeList;
static video_mode *sMode, *sDefaultMode;


//	#pragma mark - Falcon XBIOS API

class FalconModeAPI : public ModeAPI {
public:
	FalconModeAPI() : ModeAPI("Falcon XBIOS") {};
	~FalconModeAPI() {};
	virtual status_t Enumerate();
	virtual status_t Get(struct video_mode *mode);
	virtual status_t Set(const struct video_mode *mode);
};


status_t
FalconModeAPI::Enumerate()
{
	int16 monitor;
	monitor = VgetMonitor();
	switch (monitor) {
		case 0:
			panic("Monochrome ??");
			break;
		//case 4 & 5: check for CT60
		case 1:
		default:
			dprintf("monitor type %d\n", monitor);
			break;
	}
	return ENODEV;
}


status_t
FalconModeAPI::Get(struct video_mode *mode)
{
	int16 m = VsetMode(VM_INQUIRE);
	int bpp;
	int width = 320;
	if (m < 0)
		return B_ERROR;
	bpp = 1 << (m & 0x0007);
	if (m & 0x0008)
		width *= 2;
	bool vga = (m & 0x0010) != 0;
	bool pal = (m & 0x0020) != 0;
	bool overscan = (m & 0x0040) != 0;
	bool st = (m & 0x0080) != 0;
	bool interlace = (m & 0x0100) != 0;
	return ENODEV;
}


status_t
FalconModeAPI::Set(const struct video_mode *mode)
{
	return ENODEV;
}


static FalconModeAPI sFalconModeAPI;


//	#pragma mark -


bool
video_mode_hook(Menu *menu, MenuItem *item)
{
	// nothing yet
#if 0
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
			case 1:
				// "Standard VGA" mode special
				// sets sMode to NULL which triggers VGA mode
				break;
			default:
				mode = (video_mode *)item->Data();
				break;
		}
	}

	if (mode != sMode) {
		// update standard mode
		// ToDo: update fb settings!
		sMode = mode;
	}

	sModeChosen = true;
#endif
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

#if 0
	menu->AddItem(new(nothrow) MenuItem("Standard VGA"));

	video_mode *mode = NULL;
	while ((mode = (video_mode *)list_get_next_item(&sModeList, mode)) != NULL) {
		char label[64];
		sprintf(label, "%ux%u %u bit", mode->width, mode->height,
			mode->bits_per_pixel);

		menu->AddItem(item = new(nothrow) MenuItem(label));
		item->SetData(mode);
	}
#endif

	menu->AddSeparatorItem();
	menu->AddItem(item = new(nothrow) MenuItem("Return to main menu"));
	item->SetType(MENU_ITEM_NO_CHOICE);

	return menu;
}


//	#pragma mark -


extern "C" void
platform_switch_to_logo(void)
{
	// ToDo: implement me
}


extern "C" void
platform_switch_to_text_mode(void)
{
	// ToDo: implement me
}


extern "C" status_t
platform_init_video(void)
{
	// ToDo: implement me
	dprintf("current video mode: \n");
	dprintf("Vsetmode(-1): 0x%08x\n", VsetMode(VM_INQUIRE));
	sFalconModeAPI.Enumerate();
	return B_OK;
}

