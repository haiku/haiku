/* EndpointInfo.h
 * --------------
 * A simple structure that describes a MIDI object.
 * Currently, it only contains icon data associated with the object.
 *
 * Copyright 2013, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Revisions by Pete Goodeve
 *
 * Copyright 1999, Be Incorporated.   All Rights Reserved.
 * This file may be used under the terms of the Be Sample Code License.
 */
#ifndef ENDPOINTINFO_H
#define ENDPOINTINFO_H

#include <Mime.h> /* for icon_size */
#include <GraphicsDefs.h> /* for color_space */

class BMidiEndpoint;

extern const uint8 LARGE_ICON_SIZE;
extern const uint8 MINI_ICON_SIZE;
extern const icon_size DISPLAY_ICON_SIZE;
extern const color_space ICON_COLOR_SPACE;

class EndpointInfo
{
public:
	EndpointInfo();
	EndpointInfo(int32 id);
	EndpointInfo(const EndpointInfo& info);
	EndpointInfo& operator=(const EndpointInfo& info);
	~EndpointInfo();
	
	int32 ID() const
	{
		return fId;
	}
	const BBitmap* Icon() const
	{
		return fIcon;
	}
	void UpdateProperties(const BMessage* props);
	
private:
	int32 fId;
	BBitmap* fIcon;
};

#endif /* ENDPOINTINFO_H */
