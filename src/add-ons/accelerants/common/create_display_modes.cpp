/*
 * Copyright 2007-2011, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include <create_display_modes.h>

#include <math.h>
#include <stdlib.h>
#include <string.h>

#include <compute_display_timing.h>
#include <video_overlay.h>


#define	POSITIVE_SYNC \
	(B_POSITIVE_HSYNC | B_POSITIVE_VSYNC)
#define MODE_FLAGS \
	(B_8_BIT_DAC | B_HARDWARE_CURSOR | B_PARALLEL_ACCESS | B_DPMS \
		| B_SUPPORTS_OVERLAYS)

// TODO: move this list into the app_server
static const display_mode kBaseModeList[] = {
	{{25175, 640, 656, 752, 800, 350, 387, 389, 449, B_POSITIVE_HSYNC}, B_CMAP8, 640, 350, 0, 0, MODE_FLAGS}, /* 640x350 - www.epanorama.net/documents/pc/vga_timing.html) */

	{{25175, 640, 656, 752, 800, 400, 412, 414, 449, B_POSITIVE_VSYNC}, B_CMAP8, 640, 400, 0, 0, MODE_FLAGS}, /* 640x400 - www.epanorama.net/documents/pc/vga_timing.html) */

	{{25175, 640, 656, 752, 800, 480, 490, 492, 525, 0}, B_CMAP8, 640, 480, 0, 0, MODE_FLAGS}, /* Vesa_Monitor_@60Hz_(640X480X8.Z1) */
	{{31500, 640, 664, 704, 832, 480, 489, 492, 520, 0}, B_CMAP8, 640, 480, 0, 0, MODE_FLAGS}, /* Vesa_Monitor_@70-72Hz_(640X480X8.Z1) */
	{{31500, 640, 656, 720, 840, 480, 481, 484, 500, 0}, B_CMAP8, 640, 480, 0, 0, MODE_FLAGS}, /* Vesa_Monitor_@75Hz_(640X480X8.Z1) */
	{{36000, 640, 696, 752, 832, 480, 481, 484, 509, 0}, B_CMAP8, 640, 480, 0, 0, MODE_FLAGS}, /* Vesa_Monitor_@85Hz_(640X480X8.Z1) */

	{{29580, 800, 816, 896, 992, 480, 481, 484, 497, B_POSITIVE_VSYNC}, B_CMAP8, 800, 480, 0, 0, MODE_FLAGS}, /* 800x480x60Hz */

	{{38100, 800, 832, 960, 1088, 600, 602, 606, 620, 0}, B_CMAP8, 800, 600, 0, 0, MODE_FLAGS}, /* SVGA_800X600X56HzNI */
	{{40000, 800, 840, 968, 1056, 600, 601, 605, 628, POSITIVE_SYNC}, B_CMAP8, 800, 600, 0, 0, MODE_FLAGS}, /* Vesa_Monitor_@60Hz_(800X600X8.Z1) */
	{{49500, 800, 816, 896, 1056, 600, 601, 604, 625, POSITIVE_SYNC}, B_CMAP8, 800, 600, 0, 0, MODE_FLAGS}, /* Vesa_Monitor_@75Hz_(800X600X8.Z1) */
	{{50000, 800, 856, 976, 1040, 600, 637, 643, 666, POSITIVE_SYNC}, B_CMAP8, 800, 600, 0, 0, MODE_FLAGS}, /* Vesa_Monitor_@70-72Hz_(800X600X8.Z1) */
	{{56250, 800, 832, 896, 1048, 600, 601, 604, 631, POSITIVE_SYNC}, B_CMAP8, 800, 600, 0, 0, MODE_FLAGS}, /* Vesa_Monitor_@85Hz_(800X600X8.Z1) */

	{{65000, 1024, 1048, 1184, 1344, 768, 771, 777, 806, 0}, B_CMAP8, 1024, 768, 0, 0, MODE_FLAGS}, /* Vesa_Monitor_@60Hz_(1024X768X8.Z1) */
	{{75000, 1024, 1048, 1184, 1328, 768, 771, 777, 806, 0}, B_CMAP8, 1024, 768, 0, 0, MODE_FLAGS}, /* Vesa_Monitor_@70-72Hz_(1024X768X8.Z1) */
	{{78750, 1024, 1040, 1136, 1312, 768, 769, 772, 800, POSITIVE_SYNC}, B_CMAP8, 1024, 768, 0, 0, MODE_FLAGS}, /* Vesa_Monitor_@75Hz_(1024X768X8.Z1) */
	{{94500, 1024, 1072, 1168, 1376, 768, 769, 772, 808, POSITIVE_SYNC}, B_CMAP8, 1024, 768, 0, 0, MODE_FLAGS}, /* Vesa_Monitor_@85Hz_(1024X768X8.Z1) */

	{{81640, 1152, 1216, 1336, 1520, 864, 865, 868, 895, POSITIVE_SYNC}, B_CMAP8, 1152, 864, 0, 0, MODE_FLAGS}, /* 1152x864x60Hz */
	{{94200, 1152, 1184, 1280, 1472, 864, 865, 868, 914, POSITIVE_SYNC}, B_CMAP8, 1152, 864, 0, 0, MODE_FLAGS}, /* Vesa_Monitor_@70Hz_(1152X864X8.Z1) */
	{{108000, 1152, 1216, 1344, 1600, 864, 865, 868, 900, POSITIVE_SYNC}, B_CMAP8, 1152, 864, 0, 0, MODE_FLAGS}, /* Vesa_Monitor_@75Hz_(1152X864X8.Z1) */
	{{121500, 1152, 1216, 1344, 1568, 864, 865, 868, 911, POSITIVE_SYNC}, B_CMAP8, 1152, 864, 0, 0, MODE_FLAGS}, /* Vesa_Monitor_@85Hz_(1152X864X8.Z1) */

	{{74520, 1280, 1368, 1424, 1656, 720, 724, 730, 750, POSITIVE_SYNC}, B_CMAP8, 1280, 720, 0, 0, MODE_FLAGS}, /* Vesa_Monitor_@60Hz_(1280X720) */

	{{83460, 1280, 1344, 1480, 1680, 800, 801, 804, 828, B_POSITIVE_VSYNC}, B_CMAP8, 1280, 800, 0, 0, MODE_FLAGS}, /* WXGA (1280x800x60) */

	{{108000, 1280, 1328, 1440, 1688, 1024, 1025, 1028, 1066, POSITIVE_SYNC}, B_CMAP8, 1280, 1024, 0, 0, MODE_FLAGS}, /* Vesa_Monitor_@60Hz_(1280X1024X8.Z1) */
	{{135000, 1280, 1296, 1440, 1688, 1024, 1025, 1028, 1066, POSITIVE_SYNC}, B_CMAP8, 1280, 1024, 0, 0, MODE_FLAGS}, /* Vesa_Monitor_@75Hz_(1280X1024X8.Z1) */
	{{157500, 1280, 1344, 1504, 1728, 1024, 1025, 1028, 1072, POSITIVE_SYNC}, B_CMAP8, 1280, 1024, 0, 0, MODE_FLAGS}, /* Vesa_Monitor_@85Hz_(1280X1024X8.Z1) */

	{{122600, 1400, 1488, 1640, 1880, 1050, 1051, 1054, 1087, POSITIVE_SYNC}, B_CMAP8, 1400, 1050, 0, 0, MODE_FLAGS}, /* Vesa_Monitor_@60Hz_(1400X1050) */
	{{155800, 1400, 1464, 1784, 1912, 1050, 1052, 1064, 1090, POSITIVE_SYNC}, B_CMAP8, 1400, 1050, 0, 0, MODE_FLAGS}, /* Vesa_Monitor_@75Hz_(1400X1050) */

	{{106500, 1440, 1520, 1672, 1904, 900, 901, 904, 932, POSITIVE_SYNC}, B_CMAP8, 1440, 900, 0, 0, MODE_FLAGS}, /* Vesa_Monitor_@60Hz_(1440X900) */

	{{162000, 1600, 1664, 1856, 2160, 1200, 1201, 1204, 1250, POSITIVE_SYNC}, B_CMAP8, 1600, 1200, 0, 0, MODE_FLAGS}, /* Vesa_Monitor_@60Hz_(1600X1200X8.Z1) */
	{{175500, 1600, 1664, 1856, 2160, 1200, 1201, 1204, 1250, POSITIVE_SYNC}, B_CMAP8, 1600, 1200, 0, 0, MODE_FLAGS}, /* Vesa_Monitor_@65Hz_(1600X1200X8.Z1) */
	{{189000, 1600, 1664, 1856, 2160, 1200, 1201, 1204, 1250, POSITIVE_SYNC}, B_CMAP8, 1600, 1200, 0, 0, MODE_FLAGS}, /* Vesa_Monitor_@70Hz_(1600X1200X8.Z1) */
	{{202500, 1600, 1664, 1856, 2160, 1200, 1201, 1204, 1250, POSITIVE_SYNC}, B_CMAP8, 1600, 1200, 0, 0, MODE_FLAGS}, /* Vesa_Monitor_@75Hz_(1600X1200X8.Z1) */
	{{216000, 1600, 1664, 1856, 2160, 1200, 1201, 1204, 1250, POSITIVE_SYNC}, B_CMAP8, 1600, 1200, 0, 0, MODE_FLAGS}, /* Vesa_Monitor_@80Hz_(1600X1200X8.Z1) */
	{{229500, 1600, 1664, 1856, 2160, 1200, 1201, 1204, 1250, POSITIVE_SYNC}, B_CMAP8, 1600, 1200, 0, 0, MODE_FLAGS}, /* Vesa_Monitor_@85Hz_(1600X1200X8.Z1) */

	{{147100, 1680, 1784, 1968, 2256, 1050, 1051, 1054, 1087, POSITIVE_SYNC}, B_CMAP8, 1680, 1050, 0, 0, MODE_FLAGS}, /* Vesa_Monitor_@60Hz_(1680X1050) */

	//{{160000, 1920, 2010, 2060, 2110, 1200, 1202, 1208, 1235, POSITIVE_SYNC}, B_CMAP8, 1920, 1200, 0, 0, MODE_FLAGS}, /* Vesa_Monitor_@60Hz_(1920X1200) */
	{{193160, 1920, 2048, 2256, 2592, 1200, 1201, 1204, 1242, POSITIVE_SYNC}, B_CMAP8, 1920, 1200, 0, 0, MODE_FLAGS}, /* Vesa_Monitor_@60Hz_(1920X1200) */
};
static const uint32 kNumBaseModes = sizeof(kBaseModeList) / sizeof(display_mode);


namespace BPrivate {

class ModeList {
public:
								ModeList();
								~ModeList();

			bool				AddModes(edid1_info* info);
			bool				AddModes(const display_mode* modes,
									uint32 count);

			bool				CreateColorSpaces(const color_space* spaces,
									uint32 count);
			void				Filter(check_display_mode_hook hook);
			void				Clean();

			const display_mode*	Modes() const { return fModes; }
			uint32				Count() const { return fCount; }

private:
			bool				_MakeSpace(uint32 count);
			bool				_AddMode(const display_mode& mode);
			void				_RemoveModeAt(uint32 index);
			void				_AddBaseMode(uint16 width, uint16 height,
									uint32 refresh);
			display_mode*		_FindMode(uint16 width, uint16 height) const;

private:
			display_mode*		fModes;
			uint32				fCount;
			uint32				fCapacity;
};

}	// namespace BPrivate

using namespace BPrivate;


static float
get_refresh_rate(const display_mode& mode)
{
	return float(mode.timing.pixel_clock * 1000)
		/ float(mode.timing.h_total * mode.timing.v_total);
}


static int
compare_mode(const void* _mode1, const void* _mode2)
{
	display_mode *mode1 = (display_mode *)_mode1;
	display_mode *mode2 = (display_mode *)_mode2;
	uint16 width1, width2, height1, height2;

	width1 = mode1->virtual_width;
	height1 = mode1->virtual_height;
	width2 = mode2->virtual_width;
	height2 = mode2->virtual_height;

	if (width1 != width2)
		return width1 - width2;

	if (height1 != height2)
		return height1 - height2;

	if (mode1->space != mode2->space)
		return mode1->space - mode2->space;

	return (int)(100 * (get_refresh_rate(*mode1) - get_refresh_rate(*mode2)));
}


//	#pragma mark -


ModeList::ModeList()
	:
	fModes(NULL),
	fCount(0),
	fCapacity(0)
{
}


ModeList::~ModeList()
{
	free(fModes);
}


bool
ModeList::AddModes(edid1_info* info)
{
	if (info->established_timing.res_720x400x70)
		_AddBaseMode(720, 400, 70);
	if (info->established_timing.res_720x400x88)
		_AddBaseMode(720, 400, 88);

	if (info->established_timing.res_640x480x60)
		_AddBaseMode(640, 480, 60);
	if (info->established_timing.res_640x480x67)
		_AddBaseMode(640, 480, 67);
	if (info->established_timing.res_640x480x72)
		_AddBaseMode(640, 480, 72);
	if (info->established_timing.res_640x480x75)
		_AddBaseMode(640, 480, 75);

	if (info->established_timing.res_800x600x56)
		_AddBaseMode(800, 600, 56);
	if (info->established_timing.res_800x600x60)
		_AddBaseMode(800, 600, 60);
	if (info->established_timing.res_800x600x72)
		_AddBaseMode(800, 600, 72);
	if (info->established_timing.res_800x600x75)
		_AddBaseMode(800, 600, 75);

#if 0
	if (info->established_timing.res_832x624x75)
		_AddBaseMode(832, 624, 75);

	if (info->established_timing.res_1024x768x87i)
		_AddBaseMode(1024, 768, 87);
#endif
	if (info->established_timing.res_1024x768x60)
		_AddBaseMode(1024, 768, 60);
	if (info->established_timing.res_1024x768x70)
		_AddBaseMode(1024, 768, 70);
	if (info->established_timing.res_1024x768x75)
		_AddBaseMode(1024, 768, 75);

	if (info->established_timing.res_1152x870x75)
		_AddBaseMode(1152, 870, 75);
	if (info->established_timing.res_1280x1024x75)
		_AddBaseMode(1280, 1024, 75);

	for (uint32 i = 0; i < EDID1_NUM_STD_TIMING; ++i) {
		if (info->std_timing[i].h_size <= 256)
			continue;

		_AddBaseMode(info->std_timing[i].h_size, info->std_timing[i].v_size,
			info->std_timing[i].refresh);
	}

	bool hasRanges = false;
	uint32 minHorizontalFrequency = 0;
	uint32 maxHorizontalFrequency = 0;
	uint32 minVerticalFrequency = 0;
	uint32 maxVerticalFrequency = 0;
	uint32 maxPixelClock = 0;

	for (uint32 i = 0; i < EDID1_NUM_DETAILED_MONITOR_DESC; ++i) {
		if (info->detailed_monitor[i].monitor_desc_type
				== EDID1_MONITOR_RANGES) {
			edid1_monitor_range& range
				= info->detailed_monitor[i].data.monitor_range;

			hasRanges = true;
			minHorizontalFrequency = range.min_h;
			maxHorizontalFrequency = range.max_h;
			minVerticalFrequency = range.min_v;
			maxVerticalFrequency = range.max_v;
			maxPixelClock = range.max_clock * 10000;
			continue;
		} else if (info->detailed_monitor[i].monitor_desc_type
				!= EDID1_IS_DETAILED_TIMING)
			continue;

		// TODO: handle flags correctly!
		const edid1_detailed_timing& timing
			= info->detailed_monitor[i].data.detailed_timing;
		display_mode mode;

		if (timing.pixel_clock <= 0/* || timing.sync != 3*/)
			continue;

		mode.timing.pixel_clock = timing.pixel_clock * 10;
		mode.timing.h_display = timing.h_active;
		mode.timing.h_sync_start = timing.h_active + timing.h_sync_off;
		mode.timing.h_sync_end = mode.timing.h_sync_start + timing.h_sync_width;
		mode.timing.h_total = timing.h_active + timing.h_blank;
		mode.timing.v_display = timing.v_active;
		mode.timing.v_sync_start = timing.v_active + timing.v_sync_off;
		mode.timing.v_sync_end = mode.timing.v_sync_start + timing.v_sync_width;
		mode.timing.v_total = timing.v_active + timing.v_blank;
		mode.timing.flags = 0;
		if (timing.sync == 3) {
			if (timing.misc & 1)
				mode.timing.flags |= B_POSITIVE_HSYNC;
			if (timing.misc & 2)
				mode.timing.flags |= B_POSITIVE_VSYNC;
		}
		if (timing.interlaced)
			mode.timing.flags |= B_TIMING_INTERLACED;
		mode.space = B_RGB32;
		mode.virtual_width = timing.h_active;
		mode.virtual_height = timing.v_active;
		mode.h_display_start = 0;
		mode.v_display_start = 0;
		mode.flags = MODE_FLAGS;

		_AddMode(mode);
	}

	// Add other modes from the base list that satisfy the display's
	// requirements

	for (uint32 i = 0; i < kNumBaseModes; i++) {
		const display_mode& mode = kBaseModeList[i];

		// Check if a mode with this resolution already exists

		if (_FindMode(mode.timing.h_display, mode.timing.v_display) != NULL)
			continue;

		// Check monitor limits

		if (hasRanges) {
			uint32 verticalFrequency = 1000 * mode.timing.pixel_clock
				/ (mode.timing.h_total * mode.timing.v_total);
			uint32 horizontalFrequency = mode.timing.h_total * verticalFrequency
				/ 1000;

			if (minHorizontalFrequency > horizontalFrequency
				|| maxHorizontalFrequency < horizontalFrequency
				|| minVerticalFrequency > verticalFrequency
				|| maxVerticalFrequency < verticalFrequency
				|| maxPixelClock < mode.timing.pixel_clock)
				continue;
		}

		_AddMode(mode);
	}

	return true;
}


bool
ModeList::AddModes(const display_mode* modes, uint32 count)
{
	if (!_MakeSpace(count))
		return false;

	for (uint32 i = 0; i < count; i++) {
		fModes[fCount++] = modes[i];
	}

	return true;
}


bool
ModeList::CreateColorSpaces(const color_space* spaces, uint32 count)
{
	uint32 modeCount = fCount;

	for (uint32 i = 0; i < count; i++) {
		if (i > 0 && !AddModes(fModes, modeCount))
			return false;

		for (uint32 j = 0; j < modeCount; j++) {
			fModes[j + fCount - modeCount].space = spaces[i];
		}
	}

	return true;
}


void
ModeList::Filter(check_display_mode_hook hook)
{
	if (hook == NULL)
		return;

	for (uint32 i = fCount; i-- > 0;) {
		if (!hook(&fModes[i]))
			_RemoveModeAt(i);
	}
}


void
ModeList::Clean()
{
	// sort mode list
	qsort(fModes, fCount, sizeof(display_mode), compare_mode);

	// remove duplicates
	for (uint32 i = fCount; i-- > 1;) {
		if (compare_mode(&fModes[i], &fModes[i - 1]) == 0)
			_RemoveModeAt(i);
	}
}


void
ModeList::_AddBaseMode(uint16 width, uint16 height, uint32 refresh)
{
	// Check the manually tweaked list first

	for (uint32 i = 0; i < kNumBaseModes; i++) {
		const display_mode& mode = kBaseModeList[i];

		// Add mode if width and height match, and the computed refresh rate of
		// the mode is within 1.2 percent of the refresh rate specified by the
		// caller.  Note that refresh rates computed from mode parameters is
		// not exact;  thus, the tolerance of 1.2% was obtained by testing the
		// various established modes that can be selected by the EDID info.

		if (mode.timing.h_display == width && mode.timing.v_display == height
			&& fabs(get_refresh_rate(mode) - refresh) < refresh * 0.012) {
			_AddMode(mode);
			return;
		}
	}

	// If that didn't have any entries, compute the entry
	display_mode mode;
	if (compute_display_timing(width, height, refresh, false, &mode.timing)
			!= B_OK)
		return;

	fill_display_mode(width, height, &mode);

	_AddMode(mode);
}


display_mode*
ModeList::_FindMode(uint16 width, uint16 height) const
{
	for (uint32 i = 0; i < fCount; i++) {
		const display_mode& mode = fModes[i];

		if (mode.timing.h_display == width && mode.timing.v_display == height)
			return &fModes[i];
	}

	return NULL;
}


bool
ModeList::_MakeSpace(uint32 count)
{
	if (fCount + count <= fCapacity)
		return true;

	uint32 capacity = (fCapacity + count + 0xf) & ~0xf;
	display_mode* modes = (display_mode*)realloc(fModes,
		capacity * sizeof(display_mode));
	if (modes == NULL)
		return false;

	fModes = modes;
	fCapacity = capacity;
	return true;
}


bool
ModeList::_AddMode(const display_mode& mode)
{
	// TODO: filter by monitor timing constraints!
	// TODO: remove double entries
	if (!_MakeSpace(1))
		return false;

	fModes[fCount++] = mode;
	return true;
}


void
ModeList::_RemoveModeAt(uint32 index)
{
	if (index < fCount - 1) {
		memmove(&fModes[index], &fModes[index + 1],
			(fCount - 1 - index) * sizeof(display_mode));
	}

	fCount--;
}


//	#pragma mark -


extern "C" area_id
create_display_modes(const char* name, edid1_info* edid,
	const display_mode* initialModes, uint32 initialModeCount,
	const color_space *spaces, uint32 spacesCount,
	check_display_mode_hook hook, display_mode** _modes, uint32* _count)
{
	if (_modes == NULL || _count == NULL)
		return B_BAD_VALUE;

	// compile initial mode list from the different sources

	ModeList modes;
	if (initialModes != NULL)
		modes.AddModes(initialModes, initialModeCount);

	if (edid != NULL)
		modes.AddModes(edid);
	else
		modes.AddModes(kBaseModeList, kNumBaseModes);

	// filter out modes the caller doesn't like, and multiply modes for
	// every color space

	if (spaces == NULL) {
		const color_space kDefaultSpaces[] = {B_RGB32_LITTLE, B_RGB16_LITTLE,
			B_RGB15_LITTLE, B_CMAP8};
		modes.CreateColorSpaces(kDefaultSpaces,
			sizeof(kDefaultSpaces) / sizeof(kDefaultSpaces[0]));
	} else
		modes.CreateColorSpaces(spaces, spacesCount);

	modes.Filter(hook);
	modes.Clean();

	// create area for output modes

	size_t size = (sizeof(display_mode) * modes.Count() + B_PAGE_SIZE - 1)
		& ~(B_PAGE_SIZE - 1);
	display_mode *list;
	area_id area = create_area(name, (void **)&list, B_ANY_ADDRESS,
		size, B_NO_LOCK, B_READ_AREA | B_WRITE_AREA);
	if (area < B_OK)
		return area;

	memcpy(list, modes.Modes(), sizeof(display_mode) * modes.Count());
	*_modes = list;
	*_count = modes.Count();

	return area;
}


void
fill_display_mode(uint32 width, uint32 height, display_mode* mode)
{
	mode->space = B_CMAP8;
	mode->virtual_width = width;
	mode->virtual_height = height;
	mode->h_display_start = 0;
	mode->v_display_start = 0;
	mode->flags = MODE_FLAGS;
}
