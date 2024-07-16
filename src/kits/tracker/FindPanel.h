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
#ifndef _FIND_PANEL_H
#define _FIND_PANEL_H


#include <ByteOrder.h>
#include <FilePanel.h>
#include <ObjectList.h>
#include <View.h>
#include <Window.h>

#include "DialogPane.h"
#include "MimeTypeList.h"
#include "Utilities.h"
#include "NodeWalker.h"


class BFilePanel;
class BQuery;
class BBox;
class BTextControl;
class BCheckBox;
class BMenuBar;
class BMenuField;
class BFile;
class BPopUpMenu;
class BGroupView;
class BGridLayout;

namespace BPrivate {

class FindPanel;
class Model;
class DraggableIcon;
class TAttrView;

const uint32 kVolumeItem = 'Fvol';
const uint32 kAttributeItemMain = 'Fatr';
const uint32 kByNameItem = 'Fbyn';
const uint32 kByAttributeItem = 'Fbya';
const uint32 kByFormulaItem = 'Fbyq';
const uint32 kAddItem = 'Fadd';
const uint32 kRemoveItem = 'Frem';

const uint32 kClearHistory = 'FClH';
const uint32 kClearTemplates = 'FClT';
const uint32 kSaveQueryOrTemplate = 'FSaQ';
const uint32 kOpenSaveAsPanel = 'Fosv';
const uint32 kOpenLoadQueryPanel = 'Folo';
const uint32 kTemporaryOptionClicked = 'FTCl';
const uint32 kSearchInTrashOptionClicked = 'FSCl';

const uint32 kSelectDirectoryFilter = 'FSeD';
const uint32 kAddDirectoryFilters = 'FAdF';
const uint32 kRemoveDirectoryFilter = 'FRmD';

#ifdef _IMPEXP_TRACKER
_IMPEXP_TRACKER
#endif
BMenu* TrackerBuildRecentFindItemsMenu(const char* title);

struct MoreOptionsStruct {
			// Some options used to be in a collapsable part of the window, but
			// this was removed. Now the options are always visible.
			bool 				showMoreOptions;
			bool 				searchTrash;
			// reserve a bunch of fields so that we can add stuff later but not
			// make old queries incompatible. Reserved fields are set to 0 when
			// saved
			int32 				reserved1;
			bool 				temporary;
			bool 				reserved9;
			bool 				reserved10;
			bool 				reserved11;
			int32 				reserved3;
			int32 				reserved4;
			int32 				reserved5;
			int32 				reserved6;
			int32 				reserved7;
			int32 				reserved8;

								MoreOptionsStruct()
									:
									showMoreOptions(true),
									searchTrash(false),
									reserved1(0),
									temporary(true),
									reserved9(false),
									reserved10(false),
									reserved11(false),
									reserved3(0),
									reserved4(0),
									reserved5(0),
									reserved6(0),
									reserved7(0),
									reserved8(0)
								{}

	static	void				EndianSwap(void* castToThis);

	static	void 				SetQueryTemporary(BNode*, bool on);
	static	bool 				QueryTemporary(const BNode*);
};


class FindWindow : public BWindow {
public:
								FindWindow(const entry_ref* ref = NULL,
									bool editIfTemplateOnly = false);
	virtual 					~FindWindow();

			FindPanel* 			BackgroundView() const { return fBackground; }

			BNode* 				QueryNode() const { return fFile; }

	// reads in the query name from either a saved name in a template
	// or form a saved query name
	const 	char* 				QueryName() const;

	static 	bool 				IsQueryTemplate(BNode* file);
			void				SetOptions(bool searchInTrash);
			void				AddIconToMenuBar(BView*);

protected:
	virtual	void 				MessageReceived(BMessage* message);

private:
	static 	BFile* 				TryOpening(const entry_ref* ref);
	// when opening an empty panel, use the default query to set the panel up
	static 	void 				GetDefaultQuery(BEntry& entry);
			void 				SaveQueryAttributes(BNode* file, bool templateQuery);

			// retrieve the results
			void 				Find();
			// save the contents of the find window into the query file
			void 				Save();

			void 				SwitchToTemplate(const entry_ref*);
			bool 				FindSaveCommon(bool find);

			status_t 			SaveQueryAsAttributes(BNode*, BEntry*, bool queryTemplate,
									const BMessage* oldAttributes = 0,
									const BPoint* oldLocation = 0, bool temporary = true);
			status_t			GetQueryLastChangeTimeFromFile(BMessage* message);

			void 				GetDefaultName(BString&);
			// dynamic date is a date such as 'today'
			void 				GetPredicateString(BString&, bool& dynamicDate);

			void				BuildMenuBar();
			void				PopulateTemplatesMenu();
			void				UpdateFileReferences(const entry_ref*);
			void				ClearHistoryOrTemplates(bool clearTemplates, bool temporaryOnly);
			status_t			DeleteQueryOrTemplate(BEntry*);

private:
			BFile* 				fFile;
			entry_ref 			fRef;
			bool 				fFromTemplate;
			bool 				fEditTemplateOnly;
			FindPanel* 			fBackground;
	mutable BString 			fQueryNameFromTemplate;
			BFilePanel* 		fSaveAsPanel;
			BFilePanel*			fOpenQueryPanel;

			// Menu Bar For New Panel
			BGroupView*			fMenuBarContainer;
			BMenuBar*			fMenuBar;
			BMenu*				fQueryMenu;
			BMenuItem*			fSaveQueryOrTemplateItem;
			BMenu*				fOptionsMenu;
			BMenu*				fTemplatesMenu;
			BMenu*				fHistoryMenu;
			BMenu*				fSaveAsMenu;
			BMenuItem*			fSearchInTrash;

	typedef BWindow _inherited;
};


class FindPanel : public BView {
public:
								FindPanel(BFile*, FindWindow* parent, bool fromTemplate,
									bool editTemplateOnly);
	virtual 					~FindPanel();

	virtual	void 				AttachedToWindow();
	virtual	void 				Draw(BRect updateRect);
	virtual	void 				MessageReceived(BMessage*);

			void 				BuildAttrQuery(BQuery*, bool& dynamicDate) const;
			BPopUpMenu* 		MimeTypeMenu() const { return fMimeTypeMenu; }
			BMenuItem* 			CurrentMimeType(const char** type = NULL) const;
			status_t 			SetCurrentMimeType(BMenuItem* item);
			status_t 			SetCurrentMimeType(const char* label);

			BPopUpMenu* 		VolMenu() const { return fVolMenu; }
			uint32 				Mode() const { return fMode; }

	static 	uint32 				InitialMode(const BNode* entry);
			void 				SaveWindowState(BNode*, bool editTemplate);

			void 				SwitchToTemplate(const BNode*);

			// build up a query from by-attribute items
			void 				GetByAttrPredicate(BQuery*, bool& dynamicDate) const;
			// build up a simple query from the name we are searching for
			void 				GetByNamePredicate(BQuery*) const;

			void 				GetDefaultName(BString&) const;
			void 				GetDefaultAttrName(BString&, int32) const;
	// name filled out in the query name text field
	const 	char* 				UserSpecifiedName() const;

	// populate the recent query menu with query templates and recent queries
	static 	void 				AddRecentQueries(BMenu*, bool addSaveAsItem,
									const BMessenger* target, uint32 what,
									bool includeTemplates = true,
									bool includeTemporaryQueries = true,
									bool includePersistedQueries = true);

			status_t			SaveDirectoryFiltersToFile(BNode*);
			void				LoadDirectoryFiltersFromFile(const BNode*);

private:
	// populates the type menu
	void 						AddMimeTypesToMenu();
	static 	bool 				AddOneMimeTypeToMenu(const ShortMimeInfo*, void* castToMenu);

			// populates the volume menu
			void 				AddVolumes(BMenu*);

			void 				ShowVolumeMenuLabel();

			// add one more attribute item to the attr view
			void 				AddAttrRow();
			// remove the last attribute item
			void 				RemoveAttrRow();
			void 				AddFirstAttr();

			// panel building/restoring calls
			void 				RestoreWindowState(const BNode*);
			void 				RestoreMimeTypeMenuSelection(const BNode*);
			void 				AddByAttributeItems(const BNode*);
			void 				RemoveByAttributeItems();
			void 				RemoveAttrViewItems(bool removeGrid = true);
			// MimeTypeWindow is only shown in kByNameItem and kByAttributeItem modes
			void 				ShowOrHideMimeTypeMenu();

			void 				AddAttributeControls(int32);

			// fMode gets set by this and the call relies on it being up-to-date
			void 				ShowOrHideMoreOptions(bool show);
	static 	int32 				InitialAttrCount(const BNode*);
			void 				FillCurrentQueryName(BTextControl*, FindWindow*);
			void 				AddByNameOrFormulaItems();
			void 				SetupAddRemoveButtons();

			// go from search by name to search by attribute, etc.
			void 				SwitchMode(uint32);

			void 				PushMimeType(BQuery* query) const;

			void 				SaveAsQueryOrTemplate(const entry_ref*,
									const char*, bool queryTemplate);

			BView* 				FindAttrView(const char*, int row) const;

			void 				AddAttributes(BMenu* menu, const BMimeType& type);
			void 				AddMimeTypeAttrs(BMenu* menu);
			void 				RestoreAttrState(const BMessage&, int32);
			void 				SaveAttrState(BMessage*, int32);
			void 				AddLogicMenu(int32, bool selectAnd = true);
			void 				RemoveLogicMenu(int32);

			void 				ResizeMenuField(BMenuField*);

			status_t			AddDirectoryFiltersToMenu(BMenu*, BHandler* target);
			status_t			AddDirectoryFilter(const entry_ref* ref, bool addToMenu = true);
			void				RemoveDirectoryFilter(const entry_ref* ref);
	static	status_t			AddDirectoryFilterItemToMenu(BMenu*, const entry_ref* ref,
									BHandler* target, int32 index = -1);
			void				UnmarkDisks();
			void				MarkDiskAccordingToDirectoryFilter(entry_ref*);

public:
			BObjectList<entry_ref>	fDirectoryFilters;

private:
			uint32 				fMode;
			BGridLayout*		fAttrGrid;
			BPopUpMenu* 		fMimeTypeMenu;
			BMenuField* 		fMimeTypeField;
			BPopUpMenu* 		fVolMenu;
			BPopUpMenu*			fSearchModeMenu;
			BPopUpMenu* 		fRecentQueries;
			BBox* 				fMoreOptions;
			BTextControl* 		fQueryName;
			BString 			fInitialQueryName;

			DraggableIcon* 		fDraggableIcon;

			BFilePanel*			fDirectorySelectPanel;

			bool				fAddSeparatorItemState;

	typedef BView _inherited;

	friend class RecentQueriesPopUp;
};


// transient queries get deleted if they didn't get used in a while;
// this is the task that takes care of it
class DeleteTransientQueriesTask {
public:
	virtual 					~DeleteTransientQueriesTask();

	static 	void 				StartUpTransientQueryCleaner();

			// returns true when done
			bool 				DoSomeWork();

protected:
								DeleteTransientQueriesTask();

			void 				Initialize();
			bool 				GetSome();

			bool 				ProcessOneRef(Model*);

protected:
			enum State {
				kInitial,
				kAllocatedWalker,
				kTraversing,
				kError
			};

protected:
			State 				state;

private:
			BTrackerPrivate::TNodeWalker* fWalker;
};


class RecentFindItemsMenu : public BMenu {
public:
								RecentFindItemsMenu(const char* title,
									const BMessenger* target, uint32 what);

protected:
	virtual void 				AttachedToWindow();

private:
			BMessenger 			fTarget;
			uint32 				fWhat;
};


// query/query template drag&drop helper
class DraggableQueryIcon : public DraggableIcon {
public:
								DraggableQueryIcon(BRect frame, const char* name,
									const BMessage* message, BMessenger target,
									uint32 resizeFlags = B_FOLLOW_LEFT | B_FOLLOW_TOP,
									uint32 flags = B_WILL_DRAW);

protected:
	virtual bool 				DragStarted(BMessage*);
	virtual	void				Draw(BRect);
};


class FolderFilter : public BRefFilter {
public:
	virtual	bool				Filter(const entry_ref*, BNode*, struct stat_beos*, const char*);
};

} // namespace BPrivate

using namespace BPrivate;

#endif	// _FIND_PANEL_H
