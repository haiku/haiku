/*
 * Copyright 2003-2005, Haiku Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stefano Ceccherini (burton666@libero.it)
 *		Axel Dörfler, axeld@pinc-software.de
 */

/**	BScreen lets you retrieve and change the display settings. */


#include "PrivateScreen.h"

#include <Screen.h>
#include <Window.h>


/*!	\brief Creates a BScreen object which represents the display with the given screen_id
	\param id The screen_id of the screen to get.

	In the current implementation, there is only one display (B_MAIN_SCREEN_ID).
	To be sure that the object was correctly constructed, call IsValid().
*/
BScreen::BScreen(screen_id id)
{
	fScreen = BPrivateScreen::Get(id);
}


/*!	\brief Creates a BScreen object which represents the display which contains
	the given BWindow.
	\param window A BWindow.
*/
BScreen::BScreen(BWindow *window)
{
	fScreen = BPrivateScreen::Get(window);
}


/*!	\brief Releases the resources allocated by the constructor.
*/ 
BScreen::~BScreen()
{
	BPrivateScreen::Put(fScreen);
}


/*! \brief Checks if the BScreen object represents a real screen connected to the computer.
	\return \c true if the BScreen object is valid, \c false if not.
*/
bool
BScreen::IsValid()
{
	return fScreen != NULL && fScreen->IsValid();
}


/*!	\brief In the current implementation, this function always returns B_ERROR.
	\return Always \c B_ERROR.
*/
status_t
BScreen::SetToNext()
{
	if (fScreen != NULL) {
		BPrivateScreen* screen = BPrivateScreen::GetNext(fScreen);
		if (screen != NULL) {
			fScreen = screen;
			return B_OK;
		}
	}
	return B_ERROR;
}


/*!	\brief Returns the color space of the screen display.
	\return \c B_CMAP8, \c B_RGB15, or \c B_RGB32, or \c B_NO_COLOR_SPACE
		if the screen object is invalid.
*/
color_space
BScreen::ColorSpace()
{
	if (fScreen != NULL)
		return fScreen->ColorSpace();
	return B_NO_COLOR_SPACE;
}


/*!	\brief Returns the rectangle that locates the screen in the screen coordinate system.
	\return a BRect that locates the screen in the screen coordinate system.
*/
BRect
BScreen::Frame()
{
	if (fScreen != NULL)
		return fScreen->Frame();
	return BRect(0, 0, 0, 0);	
}


/*!	\brief Returns the identifier for the screen.
	\return A screen_id struct that identifies the screen.

	In the current implementation, this function always returns \c B_MAIN_SCREEN_ID,
	even if the object is invalid.
*/
screen_id
BScreen::ID()
{
	if (fScreen != NULL)
		return fScreen->ID();
	return B_MAIN_SCREEN_ID;
}


/*!	\brief Blocks until the monitor has finished the current vertical retrace.
	\return \c B_OK, or \c B_ERROR if the screen object is invalid.
*/
status_t
BScreen::WaitForRetrace()
{
	return WaitForRetrace(B_INFINITE_TIMEOUT);
}


/*!	\brief Blocks until the monitor has finished the current vertical retrace,
	or until the given timeout has passed.
	\param timeout A bigtime_t which indicates the time to wait before returning.
	\return \c B_OK if the monitor has retraced in the given amount of time,
		\c B_ERROR otherwise.
*/
status_t
BScreen::WaitForRetrace(bigtime_t timeout)
{
	if (fScreen != NULL)
		return fScreen->WaitForRetrace(timeout);
	return B_ERROR;
}


/*!	\brief Returns the index of the 8-bit color that,
		most closely matches the given 32-bit color.
	\param r The red value for a 32-bit color.
	\param g The green value for a 32-bit color.
	\param b The blue value for a 32-bit color.
	\param a The alpha value for a 32-bit color.
	\return An index for a 8-bit color in the screen's color map.
*/
uint8
BScreen::IndexForColor(uint8 r, uint8 g, uint8 b, uint8 a)
{
	if (fScreen != NULL)
		return fScreen->IndexForColor(r, g, b, a);
	return 0;
}


/*!	\brief Returns the 32-bit color representation of a given 8-bit color index.
	\param index The 8-bit color index to convert.
	\return A rgb_color struct which represents the given 8-bit color index.
*/
rgb_color
BScreen::ColorForIndex(const uint8 index)
{
	if (fScreen != NULL)
		return fScreen->ColorForIndex(index);
	return rgb_color();
}


/*!	\brief Returns the "inversion" ov the given 8-bit color.
	\param index An 8-bit color index.
	\return An 8-bit color index that represents the "inversion" of the given color.
*/
uint8
BScreen::InvertIndex(uint8 index)
{
	if (fScreen != NULL)
		return fScreen->InvertIndex(index);
	return 0;
}


/*!	\brief Returns the color map of the current display.
	\return A pointer to the object's color_map.
*/
const color_map*
BScreen::ColorMap()
{
	if (fScreen != NULL)
		return fScreen->ColorMap();
	return NULL;
}


/*!	\brief Copies the screen's contents into the first argument BBitmap.
	\param screen_shot A pointer to a BBitmap pointer, where the function will allocate a BBitmap for you.
	\param draw_cursor Specifies if you want the cursor to be drawn.
	\param bound Let you specify the area you want copied. If it's NULL, the entire screen is copied.
	\return \c B_OK if the operation was succesful, \c B_ERROR on failure.
*/
status_t
BScreen::GetBitmap(BBitmap **screen_shot, bool draw_cursor, BRect *bound)
{
	if (fScreen != NULL)
		return fScreen->GetBitmap(screen_shot, draw_cursor, bound);
	return B_ERROR;
}


/*!	\brief Copies the screen's contents into the first argument BBitmap.
	\param screen_shot A pointer to an allocated BBitmap, where the function will store the screen's content.
	\param draw_cursor Specifies if you want the cursor to be drawn.
	\param bound Let you specify the area you want copied. If it's NULL, the entire screen is copied.
	\return \c B_OK if the operation was succesful, \c B_ERROR on failure.

	The only difference between this method and GetBitmap() is that ReadBitmap requires you
	to allocate a BBitmap, while the latter will allocate a BBitmap for you.
*/
status_t
BScreen::ReadBitmap(BBitmap *buffer, bool draw_cursor, BRect *bound)
{
	if (fScreen != NULL)
		return fScreen->ReadBitmap(buffer, draw_cursor, bound);
	return B_ERROR;
}


/*!	\brief Returns the color of the desktop.
	\return An rgb_color structure which is the color of the desktop.
*/
rgb_color
BScreen::DesktopColor()
{
	if (fScreen != NULL)
		return fScreen->DesktopColor(B_ALL_WORKSPACES);

	return rgb_color();
}


/*!	\brief Returns the color of the desktop in the given workspace.
	\param workspace The workspace of which you want to have the color.
	\return An rgb_color structure which is the color of the desktop in the given workspace.
*/
rgb_color
BScreen::DesktopColor(uint32 workspace)
{
	if (fScreen != NULL)
		return fScreen->DesktopColor(workspace);

	return rgb_color();
}


/*!	\brief Set the color of the desktop.
	\param rgb The color you want to paint the desktop background.
	\param stick If you pass \c true here, the color will be maintained across boots.
*/
void
BScreen::SetDesktopColor(rgb_color rgb, bool stick)
{
	if (fScreen != NULL)
		fScreen->SetDesktopColor(rgb, B_ALL_WORKSPACES, stick);
}


/*!	\brief Set the color of the desktop in the given workspace.
	\param rgb The color you want to paint the desktop background.
	\param index The workspace you want to change the color.
	\param stick If you pass \c true here, the color will be maintained across boots.
*/
void
BScreen::SetDesktopColor(rgb_color rgb, uint32 index, bool stick)
{
	if (fScreen != NULL)
		fScreen->SetDesktopColor(rgb, index, stick);
}


/*!	\brief Attempts to adjust the supplied mode so that it's a supported mode.
	\param target The mode you want to be adjusted.
	\param low The lower limit you want target to be adjusted.
	\param high The higher limit you want target to be adjusted.
	\return
		- \c B_OK if target (as returned) is supported and falls into the limits.
		- \c B_BAD_VALUE if target (as returned) is supported but doesn't fall into the limits.
		- \c B_ERROR if target isn't supported.
*/
status_t
BScreen::ProposeMode(display_mode *target, const display_mode *low, const display_mode *high)
{
	if (fScreen != NULL)
		return fScreen->ProposeMode(target, low, high);
	return B_ERROR;
}


/*!	\brief allocates and returns a list of the display_modes 
		that the graphics card supports.
	\param mode_list A pointer to a mode_list pointer, where the function will
		allocate an array of display_mode structures.
	\param count A pointer to an integer, where the function will store the amount of
		available display modes.
	\return \c B_OK.
*/
status_t
BScreen::GetModeList(display_mode **mode_list, uint32 *count)
{
	if (fScreen != NULL)
		return fScreen->GetModeList(mode_list, count);
	return B_ERROR;
}


/*!	\brief Copies the current display_mode into mode.
	\param mode A pointer to a display_mode structure, 
		where the current display_mode will be copied.
	\return \c B_OK if the operation was succesful.
*/
status_t
BScreen::GetMode(display_mode *mode)
{
	if (fScreen != NULL)
		return fScreen->GetMode(B_ALL_WORKSPACES, mode);
	return B_ERROR;
}


/*!	\brief Copies the current display_mode of the given workspace into mode.
	\param mode A pointer to a display_mode structure, 
		where the current display_mode will be copied.
	\return \c B_OK if the operation was succesful.
*/
status_t
BScreen::GetMode(uint32 workspace, display_mode *mode)
{
	if (fScreen != NULL)
		return fScreen->GetMode(workspace, mode);
	return B_ERROR;
}


/*!	\brief Set the screen to the given mode.
	\param mode A pointer to a display_mode.
	\param makeDefault If true, the mode becomes the default for the screen.
	\return \c B_OK.
*/
status_t
BScreen::SetMode(display_mode *mode, bool makeDefault)
{
	if (fScreen != NULL)
		return fScreen->SetMode(B_ALL_WORKSPACES, mode, makeDefault);
	return B_ERROR;
}


/*!	\brief Set the given workspace to the given mode.
	\param workspace The workspace that you want to change.
	\param mode A pointer to a display_mode.
	\param makeDefault If true, the mode becomes the default for the workspace.
	\return \c B_OK.
*/
status_t
BScreen::SetMode(uint32 workspace, display_mode *mode, bool makeDefault)
{
	if (fScreen != NULL)
		return fScreen->SetMode(workspace, mode, makeDefault);
	return B_ERROR;
}


/*!	\brief Returns information about the graphics card.
	\param info An accelerant_device_info struct where to store the retrieved info.
	\return \c B_OK if the operation went fine, otherwise an error code.
*/
status_t
BScreen::GetDeviceInfo(accelerant_device_info *info)
{
	if (fScreen != NULL)
		return fScreen->GetDeviceInfo(info);
	return B_ERROR;
}


/*!	\brief Returns, in low and high, the minimum and maximum pixel clock rates 
		that are possible for the given mode.
	\param mode A pointer to a display_mode.
	\param low A pointer to an int where the function will store the lowest available pixel clock.
	\param high A pointer to an int where the function wills tore the highest available pixel clock.
	\return \c B_OK if the operation went fine, otherwise an error code.
*/
status_t
BScreen::GetPixelClockLimits(display_mode *mode, uint32 *low, uint32 *high)
{
	if (fScreen != NULL)
		return fScreen->GetPixelClockLimits(mode, low, high);
	return B_ERROR;
}


/*!	\brief Fills out the dtc structure with the timing constraints of the current display mode.
	\param dtc A pointer to a display_timing_constraints structure where the function will store
		the timing constraints of the current mode.
	\return \c B_OK if the operation went fine, otherwise an error code.
*/
status_t
BScreen::GetTimingConstraints(display_timing_constraints *dtc)
{
	if (fScreen != NULL)
		return fScreen->GetTimingConstraints(dtc);
	return B_ERROR;
}


/*!	\brief Lets you set the VESA Display Power Management Signaling state for the screen.
	\param dpms_state The DPMS state you want to be set.
		valid values are:
		- \c B_DPMS_ON
		- \c B_DPMS_STAND_BY
		- \c B_DPMS_SUSPEND
		- \c B_DPMS_OFF
	\return \c B_OK if the operation went fine, otherwise an error code.
*/
status_t
BScreen::SetDPMS(uint32 dpms_state)
{
	if (fScreen != NULL)
		return fScreen->SetDPMS(dpms_state);
	return B_ERROR;
}


/*!	\brief Returns the current DPMS state of the screen.
*/
uint32
BScreen::DPMSState()
{
	if (fScreen != NULL)
		return fScreen->DPMSState();
	return B_ERROR;
}


/*!	\brief Indicates which DPMS modes the monitor supports.
*/
uint32
BScreen::DPMSCapabilites()
{
	if (fScreen != NULL)
		return fScreen->DPMSCapabilites();
	return B_ERROR;
}


/*!	\brief Returns the BPrivateScreen used by the BScreen object.
	\return A pointer to the BPrivateScreen class internally used by the BScreen object.
*/
BPrivateScreen*
BScreen::private_screen()
{
	return fScreen;
}


/*----- Private or reserved -----------------------------------------*/
/*!	\brief Deprecated, use ProposeMode() instead.
*/
status_t
BScreen::ProposeDisplayMode(display_mode *target, const display_mode *low, const display_mode *high)
{
	return ProposeMode(target, low, high);
}


/*!	\brief Copy not allowed.
*/
BScreen&
BScreen::operator=(const BScreen&)
{
	return *this;
}


/*!	\brief Copy not allowed.
*/
BScreen::BScreen(const BScreen &screen)
{
}


/*!	\brief Returns the base address of the framebuffer.
*/
void*
BScreen::BaseAddress()
{
	if (fScreen != NULL)
		return fScreen->BaseAddress();
	return NULL;
}


/*!	\brief Returns the amount of bytes per row of the framebuffer.
*/
uint32
BScreen::BytesPerRow()
{
	if (fScreen != NULL)
		return fScreen->BytesPerRow();
	return 0;
}
