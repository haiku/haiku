#include "BitmapDrawingEngine.h"
#include "BitmapHWInterface.h"
#include "ServerBitmap.h"
#include <new>


BitmapDrawingEngine::BitmapDrawingEngine(color_space colorSpace)
	:	DrawingEngine(),
		fColorSpace(colorSpace),
		fHWInterface(NULL),
		fBitmap(NULL)
{
}


BitmapDrawingEngine::~BitmapDrawingEngine()
{
	SetSize(0, 0);
}


#if DEBUG
bool
BitmapDrawingEngine::IsParallelAccessLocked() const
{
	// We don't share the HWInterface instance that the Painter is
	// attached to, so we never need to be locked.
	return true;
}
#endif


bool
BitmapDrawingEngine::IsExclusiveAccessLocked() const
{
	// See IsParallelAccessLocked().
	return true;
}


status_t
BitmapDrawingEngine::SetSize(int32 newWidth, int32 newHeight)
{
	if (fBitmap != NULL && newWidth > 0 && newHeight > 0
		&& fBitmap->Bounds().IntegerWidth() >= newWidth
		&& fBitmap->Bounds().IntegerHeight() >= newHeight) {
		return B_OK;
	}

	SetHWInterface(NULL);
	if (fHWInterface.Get() != NULL) {
		fHWInterface->LockExclusiveAccess();
		fHWInterface->Shutdown();
		fHWInterface->UnlockExclusiveAccess();
		fHWInterface.Unset();
	}

	if (newWidth <= 0 || newHeight <= 0)
		return B_OK;

	fBitmap.SetTo(new(std::nothrow) UtilityBitmap(BRect(0, 0, newWidth - 1,
		newHeight - 1), fColorSpace, 0));
	if (fBitmap.Get() == NULL)
		return B_NO_MEMORY;

	fHWInterface.SetTo(new(std::nothrow) BitmapHWInterface(fBitmap));
	if (fHWInterface.Get() == NULL)
		return B_NO_MEMORY;

	status_t result = fHWInterface->Initialize();
	if (result != B_OK)
		return result;

	// we have to set a valid clipping first
	fClipping.Set(fBitmap->Bounds());
	ConstrainClippingRegion(&fClipping);
	SetHWInterface(fHWInterface.Get());
	return B_OK;
}


UtilityBitmap*
BitmapDrawingEngine::ExportToBitmap(int32 width, int32 height,
	color_space space)
{
	if (width <= 0 || height <= 0)
		return NULL;

	UtilityBitmap *result = new(std::nothrow) UtilityBitmap(BRect(0, 0,
		width - 1, height - 1), space, 0);
	if (result == NULL)
		return NULL;

	if (result->ImportBits(fBitmap->Bits(), fBitmap->BitsLength(),
		fBitmap->BytesPerRow(), fBitmap->ColorSpace()) != B_OK) {
		delete result;
		return NULL;
	}

	return result;
}
