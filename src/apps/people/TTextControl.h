//--------------------------------------------------------------------
//	
//	TTextControl.h
//
//	Written by: Robert Polic
//	
//--------------------------------------------------------------------
/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#ifndef TEXTCONTROL_H
#define TEXTCONTROL_H

#include <String.h>
#include <TextControl.h>

class TTextControl : public BTextControl {
	public:
							TTextControl(const char* label, const char* text);
		virtual				~TTextControl();

		virtual	void AttachedToWindow();

				bool		Changed();
				void		Revert();
				void		Update();

	private:
				BString		fOriginal;
};

#endif /* TEXTCONTROL_H */
