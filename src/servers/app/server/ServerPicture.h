#ifndef SERVER_PICTURE_H
#define SERVER_PICTURE_H

#include <OS.h>

class AreaLink;

class ServerPicture
{
public:
	ServerPicture(void);
	~ServerPicture(void);

	bool InitCheck(void) { return _initialized; }
	area_id Area(void) { return _area; }
	int32 GetToken(void) { return _token; }
private:
	
	AreaLink *arealink;
	bool _initialized;
	area_id _area;
	int32 _token;
};

#endif
