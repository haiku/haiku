/*
	Copyright 2007-2011 Haiku, Inc.  All rights reserved.
	Distributed under the terms of the MIT license.

	Authors:
	Gerald Zajac
*/

#ifndef _ACCELERANT_H
#define _ACCELERANT_H

#include "DriverInterface.h"



#undef TRACE

#ifdef ENABLE_DEBUG_TRACE
extern "C" void  _sPrintf(const char* format, ...);
#	define TRACE(x...) _sPrintf("ati: " x)
#else
#	define TRACE(x...) ;
#endif


// Global data used by various source files of the accelerant.

struct AccelerantInfo {
	int			deviceFileDesc;		// file descriptor of kernel driver

	SharedInfo*	sharedInfo;			// address of info shared between accelerants & driver
	area_id		sharedInfoArea;		// shared info area ID

	uint8*		regs;				// base address of MMIO register area
	area_id		regsArea;			// MMIO register area ID

	display_mode* modeList;			// list of standard display modes
	area_id		modeListArea;		// mode list area ID

	bool		bAccelerantIsClone;	// true if this is a cloned accelerant

	// Pointers to various global accelerant functions.
	//-------------------------------------------------

	// Pointers to wait handlers.
	void	(*WaitForFifo)(uint32);
	void	(*WaitForIdle)();

	// Pointers to DPMS functions.
	uint32	(*DPMSCapabilities)(void);
	uint32	(*GetDPMSMode)(void);
	status_t (*SetDPMSMode)(uint32 dpms_flags);

	// Pointers to cursor functions.
	bool	(*LoadCursorImage)(int width, int height, uint8* and_mask,
				uint8* xor_mask);
	void	(*SetCursorPosition)(int x, int y);
	void	(*ShowCursor)(bool bShow);

	// Pointers to 2D acceleration functions.
	void	(*FillRectangle)(engine_token*, uint32 color, fill_rect_params*,
				uint32 count);
	void	(*FillSpan)(engine_token*, uint32 color, uint16* list,
				uint32 count);
	void	(*InvertRectangle)(engine_token*, fill_rect_params*, uint32 count);
	void	(*ScreenToScreenBlit)(engine_token*, blit_params*, uint32 count);

	// Pointers to other functions.
	void	 (*AdjustFrame)(const DisplayModeEx& mode);
	status_t (*ChipInit)(void);
	bool	 (*GetColorSpaceParams)(int colorSpace, uint8& bpp,
				uint32& maxPixelClk);
	status_t (*SetDisplayMode)(const DisplayModeEx& mode);
	void	 (*SetIndexedColors)(uint count, uint8 first, uint8* color_data,
				uint32 flags);
};

extern AccelerantInfo gInfo;


// Prototypes of the interface functions called by the app_server.  Note that
// the functions that are unique to a particular chip family, will be prefixed
// with the name of the family, and the functions that are applicable to all
// chips will have no prefix.
//================================================================

#if defined(__cplusplus)
extern "C" {
#endif

// General
status_t InitAccelerant(int fd);
ssize_t  AccelerantCloneInfoSize(void);
void	 GetAccelerantCloneInfo(void* data);
status_t CloneAccelerant(void* data);
void	 UninitAccelerant(void);
status_t GetAccelerantDeviceInfo(accelerant_device_info* adi);
sem_id	 AccelerantRetraceSemaphore(void);

// Mode Configuration
uint32	 AccelerantModeCount(void);
status_t GetModeList(display_mode* dm);
status_t ProposeDisplayMode(display_mode* target, const display_mode* low,
					const display_mode* high);
status_t SetDisplayMode(display_mode* mode_to_set);
status_t GetDisplayMode(display_mode* current_mode);
status_t GetFrameBufferConfig(frame_buffer_config* a_frame_buffer);
status_t GetPixelClockLimits(display_mode* dm, uint32* low, uint32* high);
status_t MoveDisplay(uint16 h_display_start, uint16 v_display_start);
status_t GetTimingConstraints(display_timing_constraints* dtc);
void	 Mach64_SetIndexedColors(uint count, uint8 first, uint8* color_data,
					uint32 flags);
void	 Rage128_SetIndexedColors(uint count, uint8 first, uint8* color_data,
					uint32 flags);
status_t GetPreferredDisplayMode(display_mode* preferredMode);
status_t GetEdidInfo(void* info, size_t size, uint32* _version);

// DPMS
uint32   Mach64_DPMSCapabilities(void);
uint32   Mach64_GetDPMSMode(void);
status_t Mach64_SetDPMSMode(uint32 dpms_flags);

uint32   Rage128_DPMSCapabilities(void);
uint32   Rage128_GetDPMSMode(void);
status_t Rage128_SetDPMSMode(uint32 dpms_flags);

// Cursor
status_t SetCursorShape(uint16 width, uint16 height, uint16 hot_x, uint16 hot_y,
					uint8* andMask, uint8* xorMask);
void	 MoveCursor(uint16 x, uint16 y);

// Engine Management
uint32   AccelerantEngineCount(void);
status_t AcquireEngine(uint32 capabilities, uint32 max_wait, sync_token* st,
					engine_token** et);
status_t ReleaseEngine(engine_token* et, sync_token* st);
void	 WaitEngineIdle(void);
status_t GetSyncToken(engine_token* et, sync_token* st);
status_t SyncToToken(sync_token* st);

// 2D acceleration
void	 Mach64_FillRectangle(engine_token* et, uint32 color,
					fill_rect_params* list, uint32 count);
void	 Mach64_FillSpan(engine_token* et, uint32 color, uint16* list,
					uint32 count);
void	 Mach64_InvertRectangle(engine_token* et, fill_rect_params* list,
					uint32 count);
void	 Mach64_ScreenToScreenBlit(engine_token* et, blit_params* list,
					uint32 count);

void	 Rage128_FillRectangle(engine_token* et, uint32 color,
					fill_rect_params* list, uint32 count);
void	 Rage128_FillSpan(engine_token* et, uint32 color, uint16* list,
					uint32 count);
void	 Rage128_InvertRectangle(engine_token* et, fill_rect_params* list,
					uint32 count);
void	 Rage128_ScreenToScreenBlit(engine_token* et, blit_params* list,
					uint32 count);

// Video_overlay
uint32		OverlayCount(const display_mode* dm);
const uint32* OverlaySupportedSpaces(const display_mode* dm);
uint32		OverlaySupportedFeatures(uint32 a_color_space);
const overlay_buffer* AllocateOverlayBuffer(color_space cs, uint16 width,
					uint16 height);
status_t	ReleaseOverlayBuffer(const overlay_buffer* ob);
status_t	GetOverlayConstraints(const display_mode* dm,
					const overlay_buffer* ob, overlay_constraints* oc);
overlay_token AllocateOverlay(void);
status_t	ReleaseOverlay(overlay_token ot);
status_t	ConfigureOverlay(overlay_token ot, const overlay_buffer* ob,
					const overlay_window* ow, const overlay_view* ov);

#if defined(__cplusplus)
}
#endif



// Prototypes for other functions that are called from source files other than
// where they are defined.
//============================================================================

status_t CreateModeList(bool (*checkMode)(const display_mode* mode));
uint16	 GetVesaModeNumber(const display_mode& mode, uint8 bitsPerPixel);
bool	 IsModeUsable(const display_mode* mode);

// Mach64 functions.

bool	 Mach64_DisplayOverlay(const overlay_window* window,
				const overlay_buffer* buffer);
void	 Mach64_StopOverlay(void);

void	 Mach64_EngineReset(void);
void	 Mach64_EngineInit(const DisplayModeEx& mode);

bool	 Mach64_LoadCursorImage(int width, int height, uint8* and_mask,
				uint8* xor_mask);
void	 Mach64_SetCursorPosition(int x, int y);
void	 Mach64_ShowCursor(bool bShow);

void	 Mach64_AdjustFrame(const DisplayModeEx& mode);
status_t Mach64_SetDisplayMode(const DisplayModeEx& mode);
void	 Mach64_SetFunctionPointers(void);

int		 Mach64_Divide(int numerator, int denom, int shift,
				const int roundingKind);
void	 Mach64_ReduceRatio(int* numerator, int* denominator);


// Rage128 functions.

bool	 Rage128_DisplayOverlay(const overlay_window* window,
				const overlay_buffer* buffer);
void	 Rage128_StopOverlay(void);

void	 Rage128_EngineFlush(void);
void	 Rage128_EngineReset(void);
void	 Rage128_EngineInit(const DisplayModeEx& mode);

bool	 Rage128_GetEdidInfo(void);

bool	 Rage128_LoadCursorImage(int width, int height, uint8* and_mask,
				uint8* xor_mask);
void	 Rage128_SetCursorPosition(int x, int y);
void	 Rage128_ShowCursor(bool bShow);

void	 Rage128_AdjustFrame(const DisplayModeEx& mode);
status_t Rage128_SetDisplayMode(const DisplayModeEx& mode);
void	 Rage128_SetFunctionPointers(void);


// Macros for memory mapped I/O for both Mach64 and Rage128 chips.
//================================================================

#define INREG8(addr)		*((vuint8*)(gInfo.regs + addr))
#define INREG16(addr)		*((vuint16*)(gInfo.regs + addr))
#define INREG(addr)			*((vuint32*)(gInfo.regs + addr))

#define OUTREG8(addr, val)	*((vuint8*)(gInfo.regs + addr)) = val
#define OUTREG16(addr, val)	*((vuint16*)(gInfo.regs + addr)) = val
#define OUTREG(addr, val)	*((vuint32*)(gInfo.regs + addr)) = val

// Write a value to an 32-bit reg using a mask.  The mask selects the
// bits to be modified.
#define OUTREGM(addr, value, mask)	\
	(OUTREG(addr, (INREG(addr) & ~(mask)) | ((value) & (mask))))


#endif	// _ACCELERANT_H
