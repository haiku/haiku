/*
 * Copyright (c) 2004-2007 Marcus Overhagen <marcus@overhagen.de>
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without restriction, 
 * including without limitation the rights to use, copy, modify, 
 * merge, publish, distribute, sublicense, and/or sell copies of 
 * the Software, and to permit persons to whom the Software is 
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be 
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES 
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND 
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT 
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, 
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR 
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef _DVB_MEDIA_ADDON_H_
#define _DVB_MEDIA_ADDON_H_

#include <MediaAddOn.h>

class DVBCard;

class DVBMediaAddon : public BMediaAddOn
{
public:
				DVBMediaAddon(image_id id);
				~DVBMediaAddon();

	status_t	InitCheck(const char **out_failure_text);
	
	int32		CountFlavors();
	
	status_t	GetFlavorAt(int32 n, const flavor_info **out_info);
	
	BMediaNode *InstantiateNodeFor(const flavor_info *info, BMessage *config, status_t *out_error);
	
	bool		WantsAutoStart();
	status_t	AutoStart(int index, BMediaNode **outNode, int32 *outInternalID, bool *outHasMore);

protected:
	void		ScanFolder(const char *path);

	void		AddDevice(DVBCard *card, const char *path);
	void		FreeDeviceList();

protected:
	BList		fDeviceList;
};

#endif
