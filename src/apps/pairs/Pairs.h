/*
 * Copyright 2008 Ralf Schülke, ralf.schuelke@googlemail.com.
 * Copyright 2014 Haiku, Inc. All rights reserved.
 *
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Ralf Schülke, ralf.schuelke@googlemail.com
 *		John Scipione, jscipione@gmail.com
 */
#ifndef PAIRS_H
#define PAIRS_H


#include <map>

#include <Application.h>


extern const char* kSignature;


struct vector_icon {
	uint8* data;
	size_t size;
};


class BBitmap;
class BMessage;
class PairsWindow;


typedef std::map<size_t, vector_icon*> IconMap;


class Pairs : public BApplication {
public:
								Pairs();
	virtual						~Pairs();

	virtual	void				ReadyToRun();
	virtual	void				RefsReceived(BMessage* message);
	virtual	void				MessageReceived(BMessage* message);
	virtual	bool				QuitRequested();

			IconMap				GetIconMap() const { return fIconMap; };

private:
			void				_GetVectorIcons();

			PairsWindow*		fWindow;
			IconMap				fIconMap;
};


#endif	// PAIRS_H
