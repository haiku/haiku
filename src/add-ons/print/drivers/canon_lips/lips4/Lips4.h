/*
 * Lips4.h
 * Copyright 1999-2000 Y.Takagi. All Rights Reserved.
 */
#ifndef __LIPS4_H
#define __LIPS4_H


#include "GraphicsDriver.h"


class Halftone;


class LIPS4Driver : public GraphicsDriver {
public:
						LIPS4Driver(BMessage* message,
							PrinterData* printerData,
							const PrinterCap* printerCap);

protected:
	virtual	bool		StartDocument();
	virtual	bool		StartPage(int page);
	virtual	bool		NextBand(BBitmap* bitmap, BPoint* offset);
	virtual	bool		EndPage(int page);
	virtual	bool		EndDocument(bool success);

private:
			void		_Move(int x, int y);
			void		_BeginTextMode();
			void		_JobStart();
			void		_ColorModeDeclaration();
			void		_SoftReset();
			void		_SizeUnitMode();
			void		_SelectSizeUnit();
			void		_PaperFeedMode();
			void		_SelectPageFormat();
			void		_DisableAutoFF();
			void		_SetNumberOfCopies();
			void		_SidePrintingControl();
			void		_SetBindingMargin();
			void		_MemorizedPosition();
			void		_MoveAbsoluteHorizontal(int x);
			void		_CarriageReturn();
			void		_MoveDown(int dy);
			void		_RasterGraphics(int size, int widthbyte, int height,
							int compressionMethod, const uchar* buffer);
			void		_FormFeed();
			void		_JobEnd();

			int			fCurrentX;
			int			fCurrentY;
			Halftone*	fHalftone;
};

#endif // __LIPS4_H
