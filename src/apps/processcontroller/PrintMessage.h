/*
	
	print_message.h
	
	ProcessController
	(c) 2000, Georges-Edouard Berenger, All Rights Reserved.
	Copyright (C) 2004 beunited.org 

	This library is free software; you can redistribute it and/or 
	modify it under the terms of the GNU Lesser General Public 
	License as published by the Free Software Foundation; either 
	version 2.1 of the License, or (at your option) any later version. 

	This library is distributed in the hope that it will be useful, 
	but WITHOUT ANY WARRANTY; without even the implied warranty of 
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU 
	Lesser General Public License for more details. 

	You should have received a copy of the GNU Lesser General Public 
	License along with this library; if not, write to the Free Software 
	Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA	
	
*/

#ifndef	_PRINT_MESSAGE_H_
#define	_PRINT_MESSAGE_H_

#include <SupportDefs.h>

class BMessage;
typedef char* pchar;

void print_message(pchar & texte, BMessage* message, bool dig=false);

extern	int32	gHexalinesPref;

//	texte=new char[5000000];
//	print_message(texte,message);
//	...
//	delete[] texte;

#endif // _PRINT_MESSAGE_H_
