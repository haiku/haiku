#ifndef __RESOURCEUTILS_H__
#define __RESOURCEUTILS_H__

#include <Resources.h>
#include <Bitmap.h>
#include <SupportDefs.h>
#include <Mime.h>
#include <Errors.h>
#include <TypeConstants.h>
#include <Application.h>
#include <Locker.h>

class ResourceUtils :public BLocker {
public:	
			ResourceUtils(BResources *rsrc = NULL);
			ResourceUtils(const char* path);
			~ResourceUtils();

	void	SetResource(BResources *rsrc);
	void	SetResource(const char* path);
	
	void	Preload(type_code type);
	void	FreeResource();

status_t	GetIconResource(int32 id, icon_size size, BBitmap *dest);
status_t	GetIconResource(const char* name, icon_size size, BBitmap *dest);
status_t	GetBitmapResource(type_code type, int32 id, BBitmap **out);
status_t	GetBitmapResource(type_code type, const char* name, BBitmap **out);
BBitmap*	GetBitmapResource(type_code type, const char* name);
const char* GetString(const char* name);
status_t	GetString(const char* name,BString &outStr);

	BResources* Resources() {return fResource;}
protected:
	BResources *fResource;
};
#endif
