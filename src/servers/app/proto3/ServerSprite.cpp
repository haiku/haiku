#include "ServerCursor.h"
#include "ServerBitmap.h"

ServerCursor::ServerCursor(ServerBitmap *bmp)
{
}

ServerCursor::~ServerCursor(void)
{
}

ServerCursor::SetCursor(int8 *data)
{
	// 68-byte array used in R5 for holding cursors.
	// This API has serious problems and should be deprecated(but supported)
	// in a future release, perhaps the first one(?)
}

void ServerCursor::SaveClientData(void)
{
}

void ServerCursor::MoveTo(int32 x, int32 y)
{
}

void ServerCursor::SetClient(ServerBitmap *bmp)
{
}