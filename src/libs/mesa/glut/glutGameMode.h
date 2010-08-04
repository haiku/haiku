/*
 * Copyright 2010, Haiku Inc.
 * Authors:
 *		Philippe Houdoin <phoudoin %at% haiku-os %dot% org>
 *
 * Distributed under the terms of the MIT License.
 */
#ifndef GLUT_GAME_MODE_H
#define GLUT_GAME_MODE_H

#include <GL/glut.h>
#include <Screen.h>


/*!	All information needed for game mode and
	subwindows (handled as similarly as possible).
*/
class GlutGameMode {
public:
				GlutGameMode();
	virtual		~GlutGameMode();

	status_t	Set(const char* modeString);

	status_t	Enter();
	status_t	Leave();

	bool		IsActive() const 				{ return fActive; }
	bool		IsPossible();
	bool		HasDisplayChanged() const		{ return fDisplayChanged; }

	int			Width() const					{ return fWidth; }
	int			Height() const					{ return fHeight; }
	int			PixelDepth() const				{ return fPixelDepth; }
	int			RefreshRate() const				{ return fRefreshRate; }

private:
	display_mode*	_FindMatchingMode();

	static int		_CompareModes(const void* mode1, const void* mode2);
	static int		_GetModeRefreshRate(const display_mode* mode);
	static int		_GetModePixelDepth(const display_mode* mode);

	bool			fActive;
	bool			fDisplayChanged;
	int				fWidth;
	int				fHeight;
	int				fPixelDepth;
	int				fRefreshRate;
	display_mode*	fModesList;
	uint32			fModesCount;
	display_mode	fOriginalMode;
	int				fGameModeWorkspace;
	display_mode	fCurrentMode;
	int				fGameModeWindow;
	int				fPreviousWindow;

};

#endif	// GLUT_GAME_MODE_H
