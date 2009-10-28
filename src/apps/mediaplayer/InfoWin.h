/*
 * MainWin.h - Media Player for the Haiku Operating System
 *
 * Copyright (C) 2006 Marcus Overhagen <marcus@overhagen.de>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 */
#ifndef __FILE_INFO_WIN_H
#define __FILE_INFO_WIN_H


#include <Window.h>


class BStringView;
class BTextView;
class Controller;
class ControllerObserver;
class InfoView;


#define INFO_STATS		0x00000001
#define INFO_TRANSPORT	0x00000002
#define INFO_FILE		0x00000004
#define INFO_AUDIO		0x00000008
#define INFO_VIDEO		0x00000010
#define INFO_COPYRIGHT	0x00000020

#define INFO_ALL		0xffffffff


class InfoWin : public BWindow {
public:
								InfoWin(BPoint leftTop,
									Controller* controller);
	virtual						~InfoWin();

	virtual	void				FrameResized(float newWidth, float newHeight);
	virtual	void				MessageReceived(BMessage* message);
	virtual	bool				QuitRequested();
	virtual void				Pulse();

			void				ResizeToPreferred();
			void				Update(uint32 which = INFO_ALL);

private:
			Controller*			fController;
			ControllerObserver*	fControllerObserver;

			InfoView*			fInfoView;
			BStringView*		fFilenameView;
			BTextView*			fLabelsView;
			BTextView*			fContentsView;
};

#endif // __FILE_INFO_WIN_H
