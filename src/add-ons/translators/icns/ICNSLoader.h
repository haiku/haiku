/*
 * Copyright 2012, Gerasim Troeglazov, 3dEyes@gmail.com. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#ifndef ICNS_LOADER_H
#define ICNS_LOADER_H

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <Translator.h>
#include <TranslatorFormats.h>
#include <TranslationDefs.h>
#include <GraphicsDefs.h>
#include <InterfaceDefs.h>
#include <DataIO.h>
#include <File.h>
#include <ByteOrder.h>
#include <List.h>

extern "C" {
#include "icns.h"
}

#define IS_SPUPPORTED_TYPE(type) ((type) == ICNS_1024x1024_32BIT_ARGB_DATA || \
								  (type) == ICNS_512x512_32BIT_ARGB_DATA || \
								  (type) == ICNS_256x256_32BIT_ARGB_DATA || \
								  (type) == ICNS_128X128_32BIT_DATA || \
								  (type) == ICNS_48x48_32BIT_DATA || \
								  (type) == ICNS_32x32_32BIT_DATA || \
								  (type) == ICNS_16x16_32BIT_DATA)

icns_type_t ICNSFormat(float width, float height, color_space colors);

class ICNSLoader {
public:
					ICNSLoader(BPositionIO *stream);
					~ICNSLoader();
		
	int				IconsCount(void);
	int 			GetIcon(BPositionIO *target, int index);
	bool			IsLoaded(void);
private:	
	icns_family_t*	fIconFamily;
	int				fIconsCount;
	size_t			fStreamSize;
	BList			fFormatList;
	bool			fLoaded;	
};

class ICNSSaver {
public:
					ICNSSaver(BPositionIO *stream, uint32 rowBytes,
						icns_type_t type);
					~ICNSSaver();
		
	int 			SaveData(BPositionIO *target);
	bool			IsCreated(void);
private:	
	icns_family_t*	fIconFamily;
	bool			fCreated;
	
};


#endif	/* ICNS_LOADER_H */
