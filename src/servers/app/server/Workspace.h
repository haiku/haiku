#ifndef _WORKSPACE_H_
#define _WORKSPACE_H_

#include <SupportDefs.h>
#include <Locker.h>

#include "RGBColor.h"

class WinBorder;
class RBGColor;
class RootLayer;

struct ListData{
	WinBorder	*layerPtr;
	ListData	*upperItem;
	ListData	*lowerItem;
};

class Workspace
{
public:
								Workspace(const uint32 colorspace,
											int32 ID,
											const RGBColor& BGColor,
											RootLayer* owner);
								~Workspace();

			bool				AddLayerPtr(WinBorder* layer);
			bool				RemoveLayerPtr(WinBorder* layer);
			bool				HideSubsetWindows(WinBorder* layer);
			WinBorder*			SetFocusLayer(WinBorder* layer);
			WinBorder*			FocusLayer() const;
			WinBorder*			SetFrontLayer(WinBorder* layer);
			WinBorder*			FrontLayer() const;

			void				MoveToBack(WinBorder* newLast);

			WinBorder*			GoToBottomItem(); // the one that's visible.
			WinBorder*			GoToUpperItem();
			WinBorder*			GoToTopItem();
			WinBorder*			GoToLowerItem();
			bool				GoToItem(WinBorder* layer);

			WinBorder*			SearchLayerUnderPoint(BPoint pt);
			void				Invalidate();

			void				SetLocalSpace(const uint32 colorspace);
			uint32				LocalSpace() const;

			void				SetBGColor(const RGBColor &c);
			RGBColor			BGColor(void) const;
			
			int32				ID() const { return fID; }

//			void				SetFlags(const uint32 flags);
//			uint32				Flags(void) const;
	// debug methods
			void				PrintToStream() const;
			void				PrintItem(ListData *item) const;

// .... private :-) - do not use!
			void				SearchAndSetNewFront(WinBorder* preferred);
			void				SearchAndSetNewFocus(WinBorder* preferred);
			void				BringToFrontANormalWindow(WinBorder* layer);

			ListData*			HasItem(ListData* item);
			ListData*			HasItem(WinBorder* layer);

private:

			void				InsertItem(ListData* item, ListData* before);
			void				RemoveItem(ListData* item);

			ListData*			FindPlace(ListData* pref);

			int32				fID;
			uint32				fSpace;
//			uint32				fFlags;
			RGBColor			fBGColor;
			
			BLocker				opLock;

			RootLayer			*fOwner;
			
			ListData			*fBottomItem, // first visible onscreen
								*fTopItem, // the last visible(or covered by other Layers)
								*fCurrentItem, // pointer to the currect element in the list
								*fFocusItem, // the focus WinBorder - for keyboard events
								*fFrontItem; // the one the mouse can bring in front as possible(in its set)
};

#endif
