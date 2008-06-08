/*
 * Copyright 2007 Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Oliver Ruiz Dorantes, oliver.ruiz.dorantes_at_gmail.com
 *		Ryan Leavengood, leavengood@gmail.com
 */
#ifndef _JOYSTICKS_H
#define _JOYSTICKS_H


#include <Application.h>

class JoyWin;
class BWindow;


class Joysticks : public BApplication
{
	public:
		Joysticks(const char *signature);
		~Joysticks();

		virtual void	ReadyToRun();
		virtual bool	QuitRequested();

	protected:
		JoyWin*			fJoywin;

};


#endif	/* _JOYSTICKS_H */
