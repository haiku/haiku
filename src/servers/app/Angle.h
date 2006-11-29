//------------------------------------------------------------------------------
//	Copyright (c) 2001-2002, Haiku, Inc.
//
//	Permission is hereby granted, free of charge, to any person obtaining a
//	copy of this software and associated documentation files (the "Software"),
//	to deal in the Software without restriction, including without limitation
//	the rights to use, copy, modify, merge, publish, distribute, sublicense,
//	and/or sell copies of the Software, and to permit persons to whom the
//	Software is furnished to do so, subject to the following conditions:
//
//	The above copyright notice and this permission notice shall be included in
//	all copies or substantial portions of the Software.
//
//	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
//	FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
//	DEALINGS IN THE SOFTWARE.
//
//	File Name:		Angle.h
//	Author:			DarkWyrm <bpmagic@columbus.rr.com>
//	Description:	Angle class for speeding up trig functions
//  
//------------------------------------------------------------------------------
#ifndef _ANGLE_H_
#define _ANGLE_H_

#include <GraphicsDefs.h>

/*!
	\class Angle Angle.h
	\brief Class for speeding up trig functions. Works in degrees only.
*/
class Angle {
public:
						Angle(float angle);
						Angle();
virtual					~Angle();

		void			Normalize();

		float			Sine(void);
		Angle			InvSine(float value);

		float			Cosine(void);
		Angle			InvCosine(float value);

		float			Tangent(int *status=NULL);
		Angle			InvTangent(float value);

		uint8			Quadrant(void);
		Angle			Constrain180(void);
		Angle			Constrain90(void);

		void			SetValue(float angle);
		float			Value(void) const;

		Angle			&operator=(const Angle &from);
		Angle			&operator=(const float &from);
		Angle			&operator=(const long &from);
		Angle			&operator=(const int &from);

		bool			operator==(const Angle &from);
		bool			operator!=(const Angle &from);
		bool			operator<(const Angle &from);
		bool			operator>(const Angle &from);
		bool			operator>=(const Angle &from);
		bool			operator<=(const Angle &from);

protected:	
		void			_InitTrigTables(void);

		float			fAngleValue;
};

#endif
