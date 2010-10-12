/*
 * Copyright (c) 2008 Stephan Aßmus <superstippi@gmx.de>.
 * Copyright (c) 2009 Philippe Saint-Pierre, stpere@gmail.com
 * All rights reserved. Distributed under the terms of the MIT license.
 *
 * Copyright (c) 1999 Mike Steed. You are free to use and distribute this software
 * as long as it is accompanied by it's documentation and this copyright notice.
 * The software comes with no warranty, etc.
 */
#ifndef PIE_VIEW_H
#define PIE_VIEW_H


#include <View.h>

#include <map>
#include <vector>


class AppMenuItem;
class BEntry;
class BMenu;
class BPath;
class BPopUpMenu;
class BVolume;
struct entry_ref;
struct FileInfo;
class Scanner;
class MainWindow;

using std::map;
using std::vector;


class PieView: public BView {
public:
								PieView(BVolume* volume);
	virtual						~PieView();

	virtual	void				AttachedToWindow();
	virtual	void				MessageReceived(BMessage* message);
	virtual	void				MouseDown(BPoint where);
	virtual	void				MouseUp(BPoint where);
	virtual	void				MouseMoved(BPoint where, uint32 transit,
									const BMessage* dragMessage);
	virtual	void				Draw(BRect updateRect);
			void				SetPath(BPath path);

private:
			void				_ShowVolume(BVolume* volume);
			void				_DrawProgressBar(BRect updateRect);
			void				_DrawPieChart(BRect updateRect);
			float				_DrawDirectory(BRect b, FileInfo* info,
									float parentSpan, float beginAngle,
									int colorIdx, int level);
			FileInfo*			_FileAt(const BPoint& where);
			void				_AddAppToList(vector<AppMenuItem*>& list,
									const char* appSignature, int category);
			BMenu*				_BuildOpenWithMenu(FileInfo* info);
			void				_ShowContextMenu(FileInfo* info, BPoint where);
			void				_Launch(FileInfo* info,
									const entry_ref* ref = NULL);
			void				_OpenInfo(FileInfo* info, BPoint p);

private:
		struct Segment {
			Segment()
				: begin(0.0), end(0.0), info(NULL) { }
			Segment(float b, float e, FileInfo* i)
				: begin(b), end(e), info(i) { }
			Segment(const Segment &s)
				: begin(s.begin), end(s.end), info(s.info) { }
			~Segment() { }

			float				begin;
			float				end;
			FileInfo*			info;
		};
		typedef vector<Segment> SegmentList;
		typedef map<int, SegmentList> MouseOverInfo;

private:
			MainWindow*			fWindow;
			Scanner*			fScanner;
			BVolume*			fVolume;
			FileInfo*			fCurrentDir;
			MouseOverInfo		fMouseOverInfo;
			BPopUpMenu*			fMouseOverMenu;
			BPopUpMenu*			fFileUnavailableMenu;
			float				fFontHeight;
			bool				fClicked;
			bool				fDragging;
			BPoint				fDragStart;
			FileInfo*			fClickedFile;
			bool				fOutdated;			
};

#endif // PIE_VIEW_H
