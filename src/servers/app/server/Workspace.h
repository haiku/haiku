#ifndef _WORKSPACE_H_
#define _WORKSPACE_H_

#include <SupportDefs.h>

#include "RGBColor.h"

class WinBorder;
class RBGColor;

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
											const RGBColor& BGColor);
								~Workspace();

			bool				AddLayerPtr(WinBorder* layer);
			bool				RemoveLayerPtr(WinBorder* layer);
			bool				SetFocusLayer(WinBorder* layer);
			WinBorder*			FocusLayer() const;
			bool				SetFrontLayer(WinBorder* layer);
			WinBorder*			FrontLayer() const;

			void				MoveToBack(WinBorder* newLast);

			WinBorder*			GoToBottomItem(); // the one that's visible.
			WinBorder*			GoToUpperItem();
			WinBorder*			GoToTopItem();
			WinBorder*			GoToLowerItem();
			bool				GoToItem(WinBorder* layer);

			void				SetLocalSpace(const uint32 colorspace);
			uint32				LocalSpace() const;

			void				SetBGColor(const RGBColor &c);
			RGBColor			BGColor(void) const;
			
			int32				ID() const { return fID; }

//			void				SetFlags(const uint32 flags);
//			uint32				Flags(void) const;
	// debug methods
			void				PrintToStream() const;

private:

			void				InsertItem(ListData* item, ListData* before);
			void				RemoveItem(ListData* item);
			ListData*			HasItem(ListData* item);
			ListData*			HasItem(WinBorder* layer);

			ListData*			FindPlace(ListData* pref);
			void				SearchAndSetNewFront(WinBorder* preferred);
			void				SearchAndSetNewFocus(WinBorder* preferred);

			int32				fID;
			uint32				fSpace;
//			uint32				fFlags;
			RGBColor			fBGColor;
			
			ListData			*fBottomItem, // first visible onscreen
								*fTopItem, // the last visible(or covered by other Layers)
								*fCurrentItem, // pointer to the currect element in the list
								*fFocusItem, // the focus WinBorder - for keyboard events
								*fFrontItem; // the one the mouse can bring in front as possible(in its set)
};

#endif
