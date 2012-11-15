/*
 * Copyright (c) 2010 Philippe St-Pierre <stpere@gmail.com>. All rights reserved.
 * Copyright (c) 2008 Stephan Aßmus <superstippi@gmx.de>. All rights reserved.
 * Distributed under the terms of the MIT/X11 license.
 *
 * Copyright (c) 1999 Mike Steed. You are free to use and distribute this software
 * as long as it is accompanied by it's documentation and this copyright notice.
 * The software comes with no warranty, etc.
 */
#ifndef STATUS_VIEW_H
#define STATUS_VIEW_H

#include <Button.h>
#include <View.h>
#include <StringView.h>
#include <Rect.h>


struct FileInfo;

class StatusView: public BView {
public:
								StatusView();
	virtual						~StatusView();

			void				ShowInfo(const FileInfo* info);
			void				EnableRescan();
			void				EnableCancel();

private:
			BStringView*		fPathView;
			BStringView*		fSizeView;
			BStringView*		fCountView;
			const FileInfo*		fCurrentFileInfo;
	
			BButton*			fRefreshBtn;
};


#endif // STATUS_VIEW_H

