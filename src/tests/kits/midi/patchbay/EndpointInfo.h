// EndpointInfo.h
// --------------
// A simple structure that describes a MIDI object.
// Currently, it only contains icon data associated with the object.
//
// Copyright 1999, Be Incorporated.   All Rights Reserved.
// This file may be used under the terms of the Be Sample Code License.

#ifndef _EndpointInfo_h
#define _EndpointInfo_h

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
	
	int32 ID() const { return m_id; }
	const BBitmap* Icon() const { return m_icon; }
	void UpdateProperties(const BMessage* props);
	
private:
	int32 m_id;
	BBitmap* m_icon;
};

#endif /* _EndpointInfo_h */
