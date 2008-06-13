#include "BitmapDrawingEngine.h"
#include "BitmapHWInterface.h"
#include "ServerBitmap.h"
#include <new>


BitmapDrawingEngine::BitmapDrawingEngine()
	:	DrawingEngine(),
		fHWInterface(NULL),
		fBitmap(NULL)
{
}


BitmapDrawingEngine::~BitmapDrawingEngine()
{
	SetSize(0, 0);
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
	if (fHWInterface) {
		fHWInterface->LockExclusiveAccess();
		fHWInterface->Shutdown();
		fHWInterface->UnlockExclusiveAccess();
		delete fHWInterface;
		fHWInterface = NULL;
	}

	delete fBitmap;
	fBitmap = NULL;

	if (newWidth <= 0 || newHeight <= 0)
		return B_OK;

	fBitmap = new(std::nothrow) UtilityBitmap(BRect(0, 0, newWidth - 1,
		newHeight - 1), B_RGB32, 0);
	if (fBitmap == NULL)
		return B_NO_MEMORY;

	fHWInterface = new(std::nothrow) BitmapHWInterface(fBitmap);
	if (fHWInterface == NULL)
		return B_NO_MEMORY;

	status_t result = fHWInterface->Initialize();
	if (result != B_OK)
		return result;

	// we have to set a valid clipping first
	fClipping.Set(fBitmap->Bounds());
	ConstrainClippingRegion(&fClipping);
	SetHWInterface(fHWInterface);
	return B_OK;
}


UtilityBitmap *
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
