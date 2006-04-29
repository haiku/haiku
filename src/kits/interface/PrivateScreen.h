/*
 * Copyright 2002-2005, Haiku Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stefano Ceccherini (burton666@libero.it)
 *		Axel Dörfler, axeld@pinc-software.de
 */
#ifndef _PRIVATE_SCREEN_H_
#define _PRIVATE_SCREEN_H_


#include <Accelerant.h>
#include <GraphicsDefs.h>
#include <ObjectList.h>
#include <Rect.h>

struct color_map;
class BBitmap;
class BApplication;
class BWindow;


class BPrivateScreen {
	// Constructor and destructor are private. Use the static methods
	// CheckOut() and Return() instead.
	public:
		static BPrivateScreen* Get(BWindow *window);
		static BPrivateScreen* Get(screen_id id);
		static void Put(BPrivateScreen *screen);

		static BPrivateScreen* GetNext(BPrivateScreen* screen);

		bool IsValid() const;
		color_space ColorSpace();
		BRect Frame();
		screen_id ID() const { return fID; }
		status_t GetNextID(screen_id& id);

		status_t WaitForRetrace(bigtime_t timeout);

		uint8 IndexForColor(uint8 red, uint8 green, uint8 blue, uint8 alpha);
		rgb_color ColorForIndex(const uint8 index);
		uint8 InvertIndex(uint8 index);

		const color_map *ColorMap();

		status_t GetBitmap(BBitmap **bitmap, bool drawCursor, BRect *bounds);
		status_t ReadBitmap(BBitmap *bitmap, bool drawCursor, BRect *bounds);

		rgb_color DesktopColor(uint32 index);
		void SetDesktopColor(rgb_color, uint32, bool);

		status_t ProposeMode(display_mode *target, const display_mode *low,
			const display_mode *high);

		status_t GetModeList(display_mode **_modeList, uint32 *_count);
		status_t GetMode(uint32 workspace, display_mode *mode);
		status_t SetMode(uint32 workspace, display_mode *mode, bool makeDefault);

		status_t GetDeviceInfo(accelerant_device_info *info);
		status_t GetPixelClockLimits(display_mode *mode, uint32 *low, uint32 *high);
		status_t GetTimingConstraints(display_timing_constraints *constraints);

		status_t SetDPMS(uint32 dpmsState);
		uint32 DPMSState();
		uint32 DPMSCapabilites();

		void* BaseAddress();
		uint32 BytesPerRow();

	private:
		friend class BObjectList<BPrivateScreen>;

		BPrivateScreen(screen_id id);
		~BPrivateScreen();

		void _Acquire() { fRefCount++; }
		bool _Release() { return --fRefCount == 0; }

		sem_id _RetraceSemaphore();
		status_t _GetFrameBufferConfig(frame_buffer_config& config);

		static BPrivateScreen* _Get(screen_id id, bool check);
		static bool _IsValid(screen_id id);

	private:
		screen_id	fID;
		int32		fRefCount;
		color_map*	fColorMap;
		sem_id		fRetraceSem;
		bool		fRetraceSemValid;
		bool		fOwnsColorMap;
		BRect		fFrame;
		bigtime_t	fLastUpdate;
};

#endif // _PRIVATE_SCREEN_H_
