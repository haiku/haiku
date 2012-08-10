/*
Open Tracker License

Terms and Conditions

Copyright (c) 1991-2000, Be Incorporated. All rights reserved.

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:

The above copyright notice and this permission notice applies to all licensees
and shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF TITLE, MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
BE INCORPORATED BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF, OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of Be Incorporated shall not be
used in advertising or otherwise to promote the sale, use or other dealings in
this Software without prior written authorization from Be Incorporated.

Tracker(TM), Be(R), BeOS(R), and BeIA(TM) are trademarks or registered trademarks
of Be Incorporated in the United States and other countries. Other brand product
names are registered trademarks or trademarks of their respective holders.
All rights reserved.
*/
#ifndef	_POSE_VIEW_H
#define _POSE_VIEW_H


// BPoseView is a container for poses, handling all of the interaction, drawing,
// etc. The three different view modes are handled here.
//
// this is by far the fattest Tracker class and over time will undergo a lot of
// trimming


#include "AttributeStream.h"
#include "ContainerWindow.h"
#include "Model.h"
#include "PendingNodeMonitorCache.h"
#include "PoseList.h"
#include "TitleView.h"
#include "Utilities.h"
#include "ViewState.h"

#include <Directory.h>
#include <FilePanel.h>
#include <MessageRunner.h>
#include <String.h>
#include <ScrollBar.h>
#include <View.h>
#include <hash_set>
#include <set>


class BRefFilter;
class BList;

#if __GNUC__ > 2
namespace __gnu_cxx {
template<>
struct hash<node_ref>
#else
template<>
struct std::hash<node_ref>
#endif
{
	size_t operator()(node_ref ref) const {
		return ref.node;
	}
};
#if __GNUC__ > 2
} // namespace __gnu_cxx
typedef __gnu_cxx::hash_set<node_ref, __gnu_cxx::hash<node_ref> > NodeSet;
#else
typedef std::hash_set<node_ref, std::hash<node_ref> > NodeSet;
#endif


namespace BPrivate {

class BCountView;
class BContainerWindow;
class BHScrollBar;
class EntryListBase;


const int32 kSmallStep = 10;
const int32 kListOffset = 20;

const uint32 kMiniIconMode = 'Tmic';
const uint32 kIconMode = 'Ticn';
const uint32 kListMode = 'Tlst';

const uint32 kCheckTypeahead = 'Tcty';

class BPoseView : public BView {
	public:
		BPoseView(Model*, BRect, uint32 viewMode,
			uint32 resizeMask = B_FOLLOW_ALL);
		virtual ~BPoseView();

		// setup, teardown
		virtual void Init(AttributeStreamNode*);
		virtual void Init(const BMessage&);
		void InitCommon();
		virtual	void DetachedFromWindow();

		// Returns true if for instance, node ref is a remote desktop
		// directory and this is a desktop pose view.
		virtual bool Represents(const node_ref*) const;
		virtual bool Represents(const entry_ref*) const;

		BContainerWindow* ContainerWindow() const;
		const char* ViewStateAttributeName() const;
		const char* ForeignViewStateAttributeName() const;
		Model* TargetModel() const;

		virtual bool IsFilePanel() const;
		bool IsDesktopWindow() const;
		virtual bool IsDesktopView() const;

		// state saving/restoring
		virtual	void SaveState(AttributeStreamNode* node);
		virtual void RestoreState(AttributeStreamNode*);
		virtual void RestoreColumnState(AttributeStreamNode*);
		void AddColumnList(BObjectList<BColumn>*list);
		virtual void SaveColumnState(AttributeStreamNode*);
		virtual void SavePoseLocations(BRect* frameIfDesktop = NULL);
		void DisableSaveLocation();

		virtual	void SaveState(BMessage&) const;
		virtual void RestoreState(const BMessage&);
		virtual void RestoreColumnState(const BMessage&);
		virtual void SaveColumnState(BMessage&) const;

		bool StateNeedsSaving();

		// switch between mini icon mode, icon mode and list mode
		virtual void SetViewMode(uint32 mode);
		uint32 ViewMode() const;

		// re-use the pose view for a new directory
	 	virtual void SwitchDir(const entry_ref*,
	 		AttributeStreamNode* node = NULL);

		// in the rare cases where a pose view needs to be explicitly
		// refreshed (for instance in a query window with a dynamic
		// date query), this is used
		virtual void Refresh();

		// callbacks
		virtual	void MessageReceived(BMessage*);
		virtual	void AttachedToWindow();
		virtual void WindowActivated(bool);
		virtual void MakeFocus(bool = true);
		virtual	void Draw(BRect update_rect);
		virtual	void DrawAfterChildren(BRect update_rect);
		virtual void MouseMoved(BPoint, uint32, const BMessage*);
		virtual	void MouseDown(BPoint where);
		virtual	void MouseUp(BPoint where);
		virtual	void MouseDragged(const BMessage*);
		virtual	void MouseLongDown(const BMessage*);
		virtual void MouseIdle(const BMessage*);
		virtual	void KeyDown(const char*, int32);
		virtual void Pulse();
		virtual void MoveBy(float, float);
		virtual void ScrollTo(BPoint point);

		// misc. mode setters
		void SetMultipleSelection(bool);
		void SetDragEnabled(bool);
		void SetDropEnabled(bool);
		void SetSelectionRectEnabled(bool);
		void SetAlwaysAutoPlace(bool);
		void SetSelectionChangedHook(bool);
		void SetShowHideSelection(bool);
		void SetEnsurePosesVisible(bool);
		void SetIconMapping(bool);
		void SetAutoScroll(bool);
		void SetPoseEditing(bool);

		void UpdateIcon(BPose* pose);

		// file change notification handler
		virtual bool FSNotification(const BMessage*);

		// scrollbars
		virtual void UpdateScrollRange();
		virtual	void SetScrollBarsTo(BPoint);
		virtual void AddScrollBars();
		BHScrollBar* HScrollBar() const;
		BScrollBar* VScrollBar() const ;
		BCountView* CountView() const;
		void DisableScrollBars();
		void EnableScrollBars();

		// sorting
		virtual void SortPoses();
		void SetPrimarySort(uint32 attrHash);
		void SetSecondarySort(uint32 attrHash);
		void SetReverseSort(bool reverse);
		uint32 PrimarySort() const;
		uint32 PrimarySortType() const;
		uint32 SecondarySort() const;
		uint32 SecondarySortType() const;
		bool ReverseSort() const;
		void CheckPoseSortOrder(BPose*, int32 index);
		void CheckPoseVisibility(BRect* = NULL);
			// make sure pose fits the screen and/or window bounds if needed

		// view metrics
		font_height	FontInfo() const;
			// returns height, descent, etc.
		float FontHeight() const;
		float ListElemHeight() const;

		void SetIconPoseHeight();
		float IconPoseHeight() const;
		uint32 IconSizeInt() const;
		icon_size IconSize() const;

		BRect Extent() const;
		void GetLayoutInfo(uint32 viewMode, BPoint* grid,
			BPoint* offset) const;

		int32 CountItems() const;
		void UpdateCount();

		rgb_color DeskTextColor() const;
		rgb_color DeskTextBackColor() const;

		bool WidgetTextOutline() const;
		void SetWidgetTextOutline(bool);
			// used to not erase when we have a background image and
			// invalidate instead

		// column handling
		void ColumnRedraw(BRect updateRect);
		bool AddColumn(BColumn*, const BColumn* after = NULL);
		bool RemoveColumn(BColumn* column, bool runAlert);
		void MoveColumnTo(BColumn* src, BColumn* dest);
		bool ResizeColumnToWidest(BColumn* column);
		BPoint ResizeColumn(BColumn*, float, float* lastLineDrawPos = NULL,
			void (*drawLineFunc)(BPoseView*, BPoint, BPoint) = 0,
			void (*undrawLineFunc)(BPoseView*, BPoint, BPoint) = 0);
			// returns the bottom right of the last pose drawn or
			// the bottom right of bounds

		BColumn* ColumnAt(int32 index) const;
		BColumn* ColumnFor(uint32 attribute_hash) const;
		BColumn* FirstColumn() const;
		BColumn* LastColumn() const;
		int32 IndexOfColumn(const BColumn*) const;
		int32 CountColumns() const;

		// pose access
		int32 IndexOfPose(const BPose*) const;
		BPose* PoseAtIndex(int32 index) const;

		BPose* FindPose(BPoint where, int32* index = NULL) const;
			// return pose at location h, v (search list starting from
			// bottom so drawing and hit detection reflect the same pose
			// ordering)
		BPose* FindPose(const Model*, int32* index = NULL) const;
		BPose* FindPose(const node_ref*, int32* index = NULL) const;
		BPose* FindPose(const entry_ref*, int32* index = NULL) const;
		BPose* FindPose(const entry_ref*, int32 specifierForm,
			int32* index) const;
			// special form of FindPose used for scripting,
			// <specifierForm> may ask for previous or next pose
		BPose* DeepFindPose(const node_ref* node, int32* index = NULL) const;
			// same as FindPose, node can be a target of the actual
			// pose if the pose is a symlink

		void OpenInfoWindows();
		void SetDefaultPrinter();

		void IdentifySelection();
		void UnmountSelectedVolumes();
		virtual void OpenParent();

		virtual void OpenSelection(BPose* clicked_pose = NULL,
			int32* index = NULL);
		void OpenSelectionUsing(BPose* clicked_pose = NULL,
			int32* index = NULL);
			// launches the open with window
		virtual void MoveSelectionTo(BPoint, BPoint, BContainerWindow*);
		void DuplicateSelection(BPoint* dropStart = NULL,
			BPoint* dropEnd = NULL);

		// Move to trash calls try to select the next pose in the view
		// when they are dones
		virtual void MoveSelectionToTrash(bool selectNext = true);
		virtual void DeleteSelection(bool selectNext = true,
			bool askUser = true);
		virtual void MoveEntryToTrash(const entry_ref*,
			bool selectNext = true);

		void RestoreSelectionFromTrash(bool selectNext = true);

		// selection
		PoseList* SelectionList() const;
		void SelectAll();
		void InvertSelection();
		int32 SelectMatchingEntries(const BMessage*);
		void ShowSelectionWindow();
		void ClearSelection();
		void ShowSelection(bool);
		void AddRemovePoseFromSelection(BPose* pose, int32 index,
			bool select);

		BLooper* SelectionHandler();
		void SetSelectionHandler(BLooper*);

		BObjectList<BString>*MimeTypesInSelection();

		// pose selection
		void SelectPose(BPose*, int32 index, bool scrollIntoView = true);
		void AddPoseToSelection(BPose*, int32 index,
			bool scrollIntoView = true);
		void RemovePoseFromSelection(BPose*);
		void SelectPoseAtLocation(BPoint);
		void SelectPoses(int32 start, int32 end);

		// pose handling
		void ScrollIntoView(BPose* pose, int32 index);
		void ScrollIntoView(BRect poseRect);
		void SetActivePose(BPose*);
		BPose* ActivePose() const;
		void CommitActivePose(bool saveChanges = true);
		static bool PoseVisible(const Model*, const PoseInfo*);
		bool FrameForPose(BPose* targetpose, bool convert, BRect* poseRect);
		bool CreateSymlinkPoseTarget(Model* symlink);
			// used to complete a symlink pose; returns true if
			// target symlink should not be shown
		void ResetPosePlacementHint();
		void PlaceFolder(const entry_ref*, const BMessage*);

		// clipboard handling for poses
		inline bool HasPosesInClipboard();
		inline void SetHasPosesInClipboard(bool hasPoses);
		void SetPosesClipboardMode(uint32 clipboardMode);
		void UpdatePosesClipboardModeFromClipboard(
			BMessage* clipboardReport = NULL);

		// filtering
		void SetRefFilter(BRefFilter*);
		BRefFilter* RefFilter() const;

		// access for mime types represented in the pose view
		void AddMimeType(const char* mimeType);
		const char* MimeTypeAt(int32 index);
		int32 CountMimeTypes();
	 	void RefreshMimeTypeList();

		// drag&drop handling
		virtual bool HandleMessageDropped(BMessage*);
		static bool HandleDropCommon(BMessage* dragMessage, Model* target,
			BPose*, BView* view, BPoint dropPt);
			// used by pose views and info windows
		static bool CanHandleDragSelection(const Model* target,
			const BMessage* dragMessage, bool ignoreTypes);
		virtual	void DragSelectedPoses(const BPose* clickedPose, BPoint);

		void MoveSelectionInto(Model* destFolder, BContainerWindow* srcWindow,
			bool forceCopy, bool forceMove = false, bool createLink = false,
			bool relativeLink = false);
		static void MoveSelectionInto(Model* destFolder,
			BContainerWindow* srcWindow, BContainerWindow* destWindow,
			uint32 buttons, BPoint loc, bool forceCopy,
			bool forceMove = false, bool createLink = false,
			bool relativeLink = false, BPoint clickPt = BPoint(0, 0),
			bool pinToGrid = false);

		bool UpdateDropTarget(BPoint, const BMessage*,
			bool trackingContextMenu);
			// return true if drop target changed
		void HiliteDropTarget(bool hiliteState);

		void DragStop();
			// throw away cached up structures

		static bool MenuTrackingHook(BMenu* menu, void* castToThis);
			// hook for spring loaded nav-menus

		// scripting
		virtual BHandler* ResolveSpecifier(BMessage* message, int32 index,
			BMessage* specifier, int32 form, const char* property);
		virtual status_t GetSupportedSuites(BMessage*);

		// string width calls that use local width caches, faster than using
		// the general purpose BView::StringWidth
		float StringWidth(const char*) const;
		float StringWidth(const char*, int32) const;
			// deliberately hide the BView StringWidth here - this makes it
			// easy to have the right StringWidth picked up by
			// template instantiation, as used by WidgetAttributeText

		// show/hide barberpole while a background task is filling
		// up the view, etc.
		void ShowBarberPole();
		void HideBarberPole();

		bool fShowSelectionWhenInactive;
		bool fTransparentSelection;
		bool fIsDrawingSelectionRect;

		bool IsWatchingDateFormatChange();
		void StartWatchDateFormatChange();
		void StopWatchDateFormatChange();

		// type ahead filtering
		bool IsFiltering() const;

		void UpdateDateColumns(BMessage*);
		virtual void AdaptToVolumeChange(BMessage*);
		virtual void AdaptToDesktopIntegrationChange(BMessage*);

		void SetTextWidgetToCheck(BTextWidget*, BTextWidget* = NULL);

	protected:
		// view setup
		virtual void SetUpDefaultColumnsIfNeeded();

		virtual EntryListBase* InitDirentIterator(const entry_ref*);
			// sets up an entry iterator for _add_poses_
			// overriden by QueryPoseView, etc. to provide different iteration

		void Cleanup(bool doAll = false);
			// clean up poses
		void NewFolder(const BMessage*);
			// create a new folder, optionally specify a location

		void NewFileFromTemplate(const BMessage*);
			// create a new file based on a template, optionally specify
			// a location

		void ShowContextMenu(BPoint);

		// scripting handlers
		virtual bool HandleScriptingMessage(BMessage* message);
		bool SetProperty(BMessage* message, BMessage* specifier, int32 form,
			const char* property, BMessage* reply);
		bool GetProperty(BMessage*, int32, const char*, BMessage*);
		bool CreateProperty(BMessage* message, BMessage* specifier, int32,
			const char*, BMessage* reply);
		bool ExecuteProperty(BMessage* specifier, int32, const char*,
			BMessage* reply);
		bool CountProperty(BMessage*, int32, const char*, BMessage*);
		bool DeleteProperty(BMessage*, int32, const char*, BMessage*);

		void ClearPoses();
			// remove all the current poses from the view

		// pose info read/write calls
		void ReadPoseInfo(Model*, PoseInfo*);
		ExtendedPoseInfo* ReadExtendedPoseInfo(Model*);

		void _CheckPoseSortOrder(PoseList* list, BPose*, int32 index);

		// pose creation
		BPose* EntryCreated(const node_ref*, const node_ref*, const char*,
			int32* index = 0);

		void AddPoseToList(PoseList* list, bool visibleList,
			bool insertionSort, BPose* pose, BRect&viewBounds,
			float& listViewScrollBy, bool forceDraw, int32* indexPtr = NULL);
		BPose* CreatePose(Model*, PoseInfo*, bool insertionSort = true,
			int32* index = 0, BRect* boundsPtr = 0, bool forceDraw = true);
		virtual void CreatePoses(Model**models, PoseInfo* poseInfoArray,
			int32 count, BPose** resultingPoses, bool insertionSort = true,
			int32* lastPoseIndexPtr = 0, BRect* boundsPtr = 0,
			bool forceDraw = false);
		virtual bool ShouldShowPose(const Model*, const PoseInfo*);
			// filter, subclasses override to control which poses show up
			// subclasses should always call inherited
		void CreateVolumePose(BVolume*, bool watchIndividually);

		void CreateTrashPose();

		virtual bool AddPosesThreadValid(const entry_ref*) const;
			// verifies whether or not the current set of AddPoses threads
			// are valid and allowed to be adding poses -- returns false
			// in the case where the directory has been switched while
			// populating the view

		virtual void AddPoses(Model* model = NULL);
			// if <model> is zero, PoseView has other means of iterating
			// through all the entries thaat it adds

		virtual void AddRootPoses(bool watchIndividually, bool mountShared);
			// watchIndividually is used when placing a volume pose onto
			// the Desktop where unlike in the Root window it will not be
			// watched by the folder representing root. If set, each volume
			// will therefore be watched individually
		virtual void RemoveRootPoses();
		virtual void AddTrashPoses();

		virtual bool DeletePose(const node_ref*, BPose* pose = NULL,
			int32 index = 0);
		virtual void DeleteSymLinkPoseTarget(const node_ref* itemNode,
			BPose* pose, int32 index);
			// the pose itself wasn't deleted but it's target node was - the
			// pose must be a symlink
		static void PoseHandleDeviceUnmounted(BPose* pose, Model* model,
			int32 index, BPoseView* poseView, dev_t device);
		static void RemoveNonBootDesktopModels(BPose*, Model* model, int32,
			BPoseView* poseView, dev_t);

		// pose placement
		void CheckAutoPlacedPoses();
			// find poses that need placing and place them in a new spot
		void PlacePose(BPose*, BRect&);
			// find a new place for a pose, starting at fHintLocation
			// and place it
		bool IsValidLocation(const BPose* pose);
		bool IsValidLocation(const BRect& rect);
		status_t GetDeskbarFrame(BRect* frame);
		bool SlotOccupied(BRect poseRect, BRect viewBounds) const;
		void NextSlot(BPose*, BRect&poseRect, BRect viewBounds);
		void TrySettingPoseLocation(BNode* node, BPoint point);
		BPoint PinToGrid(BPoint, BPoint grid, BPoint offset) const;

		// zombie pose handling
		Model* FindZombie(const node_ref*, int32* index = 0);
		BPose* ConvertZombieToPose(Model* zombie, int32 index);

		// pose handling
		BRect CalcPoseRect(const BPose*, int32 index,
			bool firstColumnOnly = false) const;
		BRect CalcPoseRectIcon(const BPose*) const;
		BRect CalcPoseRectList(const BPose*, int32 index,
			bool firstColumnOnly = false) const;
		void DrawPose(BPose*, int32 index, bool fullDraw = true);
		void DrawViewCommon(const BRect&updateRect);

		// pose list handling
		int32 BSearchList(PoseList* poseList, const BPose*, int32* index,
			int32 oldIndex);
		void InsertPoseAfter(BPose* pose, int32* index, int32 orientation,
			BRect* invalidRect);
			// does a CopyBits to scroll poses making room for a new pose,
			// returns rectangle that needs invalidating
		void CloseGapInList(BRect* invalidRect);
		int32 FirstIndexAtOrBelow(int32 y, bool constrainIndex = true) const;
		void AddToVSList(BPose*);
		int32 RemoveFromVSList(const BPose*);
		BPose* FindNearbyPose(char arrow, int32* index);
		BPose* FindBestMatch(int32* index);
		BPose* FindNextMatch(int32* index, bool reverse = false);

		// node monitoring calls
		virtual void StartWatching();
		virtual void StopWatching();

		status_t WatchNewNode(const node_ref* item);
			// the above would ideally be the only call of these three and
			// it would be a virtual, overriding the specific watch mask in
			// query pose view, etc. however we need to call WatchNewNode
			// from inside AddPosesTask while the window is unlocked - we
			// have to use the static and a cached messenger and masks.
		static status_t WatchNewNode(const node_ref*, uint32, BMessenger);
		virtual uint32 WatchNewNodeMask();
			// override to change different watch modes for query pose
			// view, etc.

		// drag&drop handling
		static bool EachItemInDraggedSelection(const BMessage* message,
			bool (*)(BPose*, BPoseView*, void*), BPoseView* poseView,
			void* = NULL);
			// iterates through each pose in current selectiond in the source
			// window of the current drag message; locks the window
			// add const version
		BRect GetDragRect(int32 clickedPoseIndex);
		BBitmap* MakeDragBitmap(BRect dragRect, BPoint clickedPoint,
			int32 clickedPoseIndex, BPoint&offset);
		static bool FindDragNDropAction(const BMessage* dragMessage,
			bool&canCopy, bool&canMove, bool&canLink, bool&canErase);

		static bool CanTrashForeignDrag(const Model*);
		static bool CanCopyOrMoveForeignDrag(const Model*, const BMessage*);
		static bool DragSelectionContains(const BPose* target,
			const BMessage* dragMessage);
		static status_t CreateClippingFile(BPoseView* poseView, BFile&result,
			char* resultingName, BDirectory* dir, BMessage* message,
			const char* fallbackName, bool setLocation = false,
			BPoint dropPoint = BPoint(0, 0));

		// opening files, lanunching
		void OpenSelectionCommon(BPose*, int32*, bool);
			// used by OpenSelection and OpenSelectionUsing
		static void LaunchAppWithSelection(Model*, const BMessage*,
			bool checkTypes = true);

		// node monitoring calls
		virtual bool EntryMoved(const BMessage*);
		virtual bool AttributeChanged(const BMessage*);
		virtual bool NoticeMetaMimeChanged(const BMessage*);
		virtual void MetaMimeChanged(const char*, const char*);

		// click handling
		bool WasDoubleClick(const BPose*, BPoint);
		bool WasClickInPath(const BPose*, int32 index, BPoint) const;

		// selection
		void SelectPosesListMode(BRect, BList**);
		void SelectPosesIconMode(BRect, BList**);
		void AddRemoveSelectionRange(BPoint where, bool extendSelection,
			BPose*);

		void _BeginSelectionRect(const BPoint& point, bool extendSelection);
		void _UpdateSelectionRect(const BPoint& point);
		void _EndSelectionRect();

		// view drawing
		void SynchronousUpdate(BRect, bool clip = false);

		// scrolling
		void HandleAutoScroll();
		bool CheckAutoScroll(BPoint mouseLoc, bool shouldScroll);

		// view extent handling
		void RecalcExtent();
		void AddToExtent(const BRect&);
		void ClearExtent();
		void RemoveFromExtent(const BRect&);

		virtual void EditQueries();
		virtual void AddCountView();

		void HandleAttrMenuItemSelected(BMessage*);
		void TryUpdatingBrokenLinks();
			// ran a little after a volume gets mounted

		void MapToNewIconMode(BPose*, BPoint oldGrid, BPoint oldOffset);
		void ResetOrigin();
		void PinPointToValidRange(BPoint&);
			// used to ensure pose locations make sense after getting them
			// in pose info from attributes, etc.

		void FinishPendingScroll(float&listViewScrollBy, BRect bounds);
			// utility call for CreatePoses

		// background AddPoses task calls
		static status_t AddPosesTask(void*);
		virtual void AddPosesCompleted();
		bool IsValidAddPosesThread(thread_id) const;

		// typeahead filtering
		void EnsurePoseUnselected(BPose* pose);
		void RemoveFilteredPose(BPose* pose, int32 index);
		void FilterChanged();
		void UpdateAfterFilterChange();
		bool FilterPose(BPose* pose);
		void StartFiltering();
		void StopFiltering();
		void ClearFilter();
		PoseList* CurrentPoseList() const;

		// misc
		BList* GetDropPointList(BPoint dropPoint, BPoint startPoint,
			const PoseList*, bool sourceInListMode, bool dropOnGrid) const;
		void SendSelectionAsRefs(uint32 what, bool onlyQueries = false);
		void MoveListToTrash(BObjectList<entry_ref>*, bool selectNext,
			bool deleteDirectly);
		void Delete(BObjectList<entry_ref>*, bool selectNext, bool askUser);
		void Delete(const entry_ref&ref, bool selectNext, bool askUser);
		void RestoreItemsFromTrash(BObjectList<entry_ref>*, bool selectNext);

		void WatchParentOf(const entry_ref*);
		void StopWatchingParentsOf(const entry_ref*);

	private:
		void DrawOpenAnimation(BRect);

		void MoveSelectionOrEntryToTrash(const entry_ref* ref, bool selectNext);

	protected:
		BHScrollBar* fHScrollBar;
		BScrollBar* fVScrollBar;
		Model* fModel;
		BPose* fActivePose;
		BRect fExtent;
		// the following should probably be just member lists, not pointers
		PoseList* fPoseList;
		PoseList* fFilteredPoseList;
		PoseList* fVSPoseList;
		PoseList* fSelectionList;
		NodeSet fInsertedNodes;
		BObjectList<BString> fMimeTypesInSelectionCache;
			// used for mime string based icon highliting during a drag
		BObjectList<Model>* fZombieList;
		PendingNodeMonitorCache pendingNodeMonitorCache;
		BObjectList<BColumn>* fColumnList;
		BObjectList<BString>* fMimeTypeList;
		BObjectList<Model>* fBrokenLinks;
	  	bool fMimeTypeListIsDirty;
		BViewState* fViewState;
		bool fStateNeedsSaving;
		BCountView* fCountView;
		float fListElemHeight;
		float fIconPoseHeight;
		BPose* fDropTarget;
		BPose* fAlreadySelectedDropTarget;
		BLooper* fSelectionHandler;
		BPoint fLastClickPt;
		bigtime_t fLastClickTime;
		const BPose* fLastClickedPose;
		BPoint fLastLeftTop;
		BRect fLastExtent;
		BTitleView* fTitleView;
		BRefFilter* fRefFilter;
		BPoint fGrid;
		BPoint fOffset;
		BPoint fHintLocation;
		float fAutoScrollInc;
		int32 fAutoScrollState;
		std::set<thread_id> fAddPosesThreads;
		bool fWidgetTextOutline;
		const BPose* fSelectionPivotPose;
		const BPose* fRealPivotPose;
		BMessageRunner* fKeyRunner;
		bool fTrackRightMouseUp;

		struct SelectionRectInfo {
					SelectionRectInfo()
						:	isDragging(false), selection(NULL) {};
			bool	isDragging;
			BRect	rect;
			BRect	lastRect;
			BPoint	startPoint;
			BPoint	lastPoint;
			BList*	selection;
		};
		SelectionRectInfo fSelectionRectInfo;

		bool fSelectionVisible : 1;
		bool fMultipleSelection : 1;
		bool fDragEnabled : 1;
		bool fDropEnabled : 1;
		bool fSelectionRectEnabled : 1;
		bool fAlwaysAutoPlace : 1;
		bool fAllowPoseEditing : 1;
		bool fSelectionChangedHook : 1;	// get rid of this
		bool fSavePoseLocations : 1;
		bool fShowHideSelection : 1;
		bool fOkToMapIcons : 1;
		bool fEnsurePosesVisible : 1;
		bool fShouldAutoScroll : 1;
		bool fIsDesktopWindow : 1;
		bool fIsWatchingDateFormatChange : 1;
		bool fHasPosesInClipboard : 1;
		bool fCursorCheck : 1;
		bool fFiltering : 1;

		BObjectList<BString> fFilterStrings;
		int32 fLastFilterStringCount;
		int32 fLastFilterStringLength;

		BRect fStartFrame;

		static float sFontHeight;
		static font_height sFontInfo;
		static BFont sCurrentFont;
		static BString sMatchString;
			// used for typeahead - should be replaced by a typeahead state

		bigtime_t fLastKeyTime;
		bigtime_t fLastDeskbarFrameCheckTime;
		BRect fDeskbarFrame;

		static OffscreenBitmap* sOffscreen;

		BTextWidget* fTextWidgetToCheck;

		typedef BView _inherited;
};


class BHScrollBar : public BScrollBar {
	public:
		BHScrollBar(BRect, const char*, BView*);
		void SetTitleView(BView*);

		// BScrollBar overrides
		virtual	void ValueChanged(float);

	private:
		BView* fTitleView;

		typedef BScrollBar _inherited;
};


class TPoseViewFilter : public BMessageFilter {
	public:
		TPoseViewFilter(BPoseView* pose);
		~TPoseViewFilter();

		filter_result Filter(BMessage*, BHandler**);

	private:
		filter_result ObjectDropFilter(BMessage*, BHandler**);

		BPoseView* fPoseView;
};


extern bool
ClearViewOriginOne(const char* name, uint32 type, off_t size, void* data,
	void* params);


// inlines follow


inline BContainerWindow*
BPoseView::ContainerWindow() const
{
	return dynamic_cast<BContainerWindow*>(Window());
}


inline Model*
BPoseView::TargetModel() const
{
	return fModel;
}


inline float
BPoseView::ListElemHeight() const
{
	return fListElemHeight;
}


inline float
BPoseView::IconPoseHeight() const
{
	return fIconPoseHeight;
}


inline uint32
BPoseView::IconSizeInt() const
{
	return fViewState->IconSize();
}


inline icon_size
BPoseView::IconSize() const
{
	return (icon_size)fViewState->IconSize();
}


inline PoseList*
BPoseView::SelectionList() const
{
	return fSelectionList;
}


inline BObjectList<BString>*
BPoseView::MimeTypesInSelection()
{
	return&fMimeTypesInSelectionCache;
}


inline BHScrollBar*
BPoseView::HScrollBar() const
{
	return fHScrollBar;
}


inline BScrollBar*
BPoseView::VScrollBar() const
{
	return fVScrollBar;
}


inline BCountView*
BPoseView::CountView() const
{
	return fCountView;
}


inline bool
BPoseView::StateNeedsSaving()
{
	return fStateNeedsSaving || fViewState->StateNeedsSaving();
}


inline uint32
BPoseView::ViewMode() const
{
	return fViewState->ViewMode();
}


inline font_height
BPoseView::FontInfo() const
{
	return sFontInfo;
}


inline float
BPoseView::FontHeight() const
{
	return sFontHeight;
}


inline BPose*
BPoseView::ActivePose() const
{
	return fActivePose;
}


inline void
BPoseView::DisableSaveLocation()
{
	fSavePoseLocations = false;
}


inline bool
BPoseView::IsFilePanel() const
{
	return false;
}


inline bool
BPoseView::IsDesktopWindow() const
{
	return fIsDesktopWindow;
}


inline bool
BPoseView::IsDesktopView() const
{
	return false;
}


inline uint32
BPoseView::PrimarySort() const
{
	return fViewState->PrimarySort();
}


inline uint32
BPoseView::PrimarySortType() const
{
	return fViewState->PrimarySortType();
}


inline uint32
BPoseView::SecondarySort() const
{
	return fViewState->SecondarySort();
}


inline uint32
BPoseView::SecondarySortType() const
{
	return fViewState->SecondarySortType();
}


inline bool
BPoseView::ReverseSort() const
{
	return fViewState->ReverseSort();
}


inline void
BPoseView::SetShowHideSelection(bool on)
{
	fShowHideSelection = on;
}


inline void
BPoseView::SetIconMapping(bool on)
{
	fOkToMapIcons = on;
}


inline void
BPoseView::AddToExtent(const BRect&rect)
{
	fExtent = fExtent | rect;
}


inline void
BPoseView::ClearExtent()
{
	fExtent.Set(LONG_MAX, LONG_MAX, LONG_MIN, LONG_MIN);
}


inline int32
BPoseView::CountColumns() const
{
	return fColumnList->CountItems();
}


inline int32
BPoseView::IndexOfColumn(const BColumn* column) const
{
	return fColumnList->IndexOf(const_cast<BColumn*>(column));
}


inline int32
BPoseView::IndexOfPose(const BPose* pose) const
{
	return CurrentPoseList()->IndexOf(pose);
}


inline BPose*
BPoseView::PoseAtIndex(int32 index) const
{
	return CurrentPoseList()->ItemAt(index);
}


inline BColumn*
BPoseView::ColumnAt(int32 index) const
{
	return fColumnList->ItemAt(index);
}


inline BColumn*
BPoseView::FirstColumn() const
{
	return fColumnList->FirstItem();
}


inline BColumn*
BPoseView::LastColumn() const
{
	return fColumnList->LastItem();
}


inline int32
BPoseView::CountItems() const
{
	return CurrentPoseList()->CountItems();
}


inline void
BPoseView::SetMultipleSelection(bool state)
{
	fMultipleSelection = state;
}


inline void
BPoseView::SetSelectionChangedHook(bool state)
{
	fSelectionChangedHook = state;
}


inline void
BPoseView::SetAutoScroll(bool state)
{
	fShouldAutoScroll = state;
}


inline void
BPoseView::SetPoseEditing(bool state)
{
	fAllowPoseEditing = state;
}


inline void
BPoseView::SetDragEnabled(bool state)
{
	fDragEnabled = state;
}


inline void
BPoseView::SetDropEnabled(bool state)
{
	fDropEnabled = state;
}


inline void
BPoseView::SetSelectionRectEnabled(bool state)
{
	fSelectionRectEnabled = state;
}


inline void
BPoseView::SetAlwaysAutoPlace(bool state)
{
	fAlwaysAutoPlace = state;
}


inline void
BPoseView::SetEnsurePosesVisible(bool state)
{
	fEnsurePosesVisible = state;
}


inline void
BPoseView::SetSelectionHandler(BLooper* looper)
{
	fSelectionHandler = looper;
}


inline void
BPoseView::SetRefFilter(BRefFilter* filter)
{
	fRefFilter = filter;
	if (filter != NULL)
		FilterChanged();
}


inline BRefFilter*
BPoseView::RefFilter() const
{
	return fRefFilter;
}


inline void
BHScrollBar::SetTitleView(BView* view)
{
	fTitleView = view;
}


inline BPose*
BPoseView::FindPose(const Model* model, int32* index) const
{
	return CurrentPoseList()->FindPose(model, index);
}


inline BPose*
BPoseView::FindPose(const node_ref* node, int32* index) const
{
	return CurrentPoseList()->FindPose(node, index);
}


inline BPose*
BPoseView::FindPose(const entry_ref* entry, int32* index) const
{
	return CurrentPoseList()->FindPose(entry, index);
}


inline bool
BPoseView::HasPosesInClipboard()
{
	return fHasPosesInClipboard;
}


inline void
BPoseView::SetHasPosesInClipboard(bool hasPoses)
{
	fHasPosesInClipboard = hasPoses;
}


inline PoseList*
BPoseView::CurrentPoseList() const
{
	return fFiltering ? fFilteredPoseList : fPoseList;
}


template<class Param1>
void
EachTextWidget(BPose* pose, BPoseView* poseView,
	void (*func)(BTextWidget*, BPose*, BPoseView*, BColumn*, Param1), Param1 p1)
{
	for (int32 index = 0; ;index++) {
		BColumn* column = poseView->ColumnAt(index);
		if (!column)
			break;

		BTextWidget* widget = pose->WidgetFor(column->AttrHash());
		if (widget)
			(func)(widget, pose, poseView, column, p1);
	}
}


template<class Param1, class Param2>
void
EachTextWidget(BPose* pose, BPoseView* poseView,
	void (*func)(BTextWidget*, BPose*, BPoseView*, BColumn*,
	Param1, Param2), Param1 p1, Param2 p2)
{
	for (int32 index = 0; ;index++) {
		BColumn* column = poseView->ColumnAt(index);
		if (!column)
			break;

		BTextWidget* widget = pose->WidgetFor(column->AttrHash());
		if (widget)
			(func)(widget, pose, poseView, column, p1, p2);
	}
}


template<class Result, class Param1, class Param2>
Result
WhileEachTextWidget(BPose* pose, BPoseView* poseView,
	Result (*func)(BTextWidget*, BPose*, BPoseView*, BColumn*,
	Param1, Param2), Param1 p1, Param2 p2)
{
	for (int32 index = 0; ;index++) {
		BColumn* column = poseView->ColumnAt(index);
		if (!column)
			break;

		BTextWidget* widget = pose->WidgetFor(column->AttrHash());
		if (widget) {
			Result result = (func)(widget, pose, poseView, column, p1, p2);
			if (result)
				return result;
		}
	}
	return 0;
}


} // namespace BPrivate

using namespace BPrivate;

#endif	// _POSE_VIEW_H
