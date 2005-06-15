//--------------------------------------------------------------------
//	
//	TextControl.h
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

#include <TextControl.h>

class TTextControl : public BTextControl {
	public:
		TTextControl(BRect rect, const char* label, int32 length, const char* text,
			int32 modificationMessage, int32 invokationMessage);
		~TTextControl();

		virtual void AttachedToWindow(void);

		bool Changed(void);
		void Revert(void);
		void Update(void);

	private:
		char	*fOriginal;
};

#endif /* TEXTCONTROL_H */
