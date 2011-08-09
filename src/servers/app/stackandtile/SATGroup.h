/*
 * Copyright 2010, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Clemens Zeidler <haiku@clemens-zeidler.de>
 */
#ifndef SAT_GROUP_H
#define SAT_GROUP_H


#include <Rect.h>

#include "ObjectList.h"
#include "Referenceable.h"

#include "MagneticBorder.h"

#include "LinearSpec.h"


class SATWindow;
class Tab;
class WindowArea;

typedef BObjectList<SATWindow> SATWindowList;


class Corner {
public:
		enum info_t
		{
			kFree,
			kUsed,
			kNotDockable
		};

		enum position_t
		{
			kLeftTop = 0,
			kRightTop = 1,
			kLeftBottom = 2,
			kRightBottom = 3
		};

						Corner();
		void			Trace() const;

		info_t			status;
		WindowArea*		windowArea;
};


class Crossing : public BReferenceable {
public:
								Crossing(Tab* vertical, Tab* horizontal);
								~Crossing();

			Corner*				GetCorner(Corner::position_t corner) const;
			Corner*				GetOppositeCorner(
									Corner::position_t corner) const;

			Corner*				LeftTopCorner()
									{ return &fCorners[Corner::kLeftTop]; }
			Corner*				RightTopCorner()
									{ return &fCorners[Corner::kRightTop]; }
			Corner*				LeftBottomCorner()
									{ return &fCorners[Corner::kLeftBottom]; }
			Corner*				RightBottomCorner()
									{ return &fCorners[Corner::kRightBottom]; }

			Tab*				VerticalTab() const;
			Tab*				HorizontalTab() const;

			void				Trace() const;
private:
			Corner				fCorners[4];

			BReference<Tab>		fVerticalTab;
			BReference<Tab>		fHorizontalTab;
};


typedef BObjectList<Constraint> ConstraintList;
class SATGroup;

typedef BObjectList<Crossing> CrossingList;


// make all coordinates positive needed for the solver
const float kMakePositiveOffset = 5000;


class Tab : public BReferenceable {
public:
		enum orientation_t
		{
			kVertical,
			kHorizontal
		};

								Tab(SATGroup* group, Variable* variable,
									orientation_t orientation);
								~Tab();

			float				Position() const;
			void				SetPosition(float position);
			orientation_t		Orientation() const;
			Variable*			Var() {	return fVariable;	}

			//! Caller takes ownership of the constraint.
			Constraint*			Connect(Variable* variable);

			BReference<Crossing>	AddCrossing(Tab* tab);
			bool				RemoveCrossing(Crossing* crossing);
			int32				FindCrossingIndex(Tab* tab);
			int32				FindCrossingIndex(float tabPosition);
			Crossing*			FindCrossing(Tab* tab);
			Crossing*			FindCrossing(float tabPosition);

			const CrossingList*	GetCrossingList() const;

	static	int					CompareFunction(const Tab* tab1,
									const Tab* tab2);

private:
			SATGroup*			fGroup;
			Variable*			fVariable;
			orientation_t		fOrientation;

			CrossingList		fCrossingList;
};


class WindowArea : public BReferenceable {
public:
								WindowArea(Crossing* leftTop,
									Crossing* rightTop, Crossing* leftBottom,
									Crossing* rightBottom);
								~WindowArea();

			bool				Init(SATGroup* group);
			SATGroup*			Group() { return fGroup; }

			void				DoGroupLayout();
			void				UpdateSizeLimits();
			void				UpdateSizeConstaints(const BRect& frame);

	const	SATWindowList&		WindowList() { return fWindowList; }
	const	SATWindowList&		LayerOrder() { return fWindowLayerOrder; }
			bool				MoveWindowToPosition(SATWindow* window,
									int32 index);
			SATWindow*			TopWindow();

			Crossing*			LeftTopCrossing()
									{ return fLeftTopCrossing.Get(); }
			Crossing*			RightTopCrossing()
									{ return fRightTopCrossing.Get(); }
			Crossing*			LeftBottomCrossing()
									{ return fLeftBottomCrossing.Get(); }
			Crossing*			RightBottomCrossing()
									{ return fRightBottomCrossing.Get(); }

			Tab*				LeftTab();
			Tab*				RightTab();
			Tab*				TopTab();
			Tab*				BottomTab();

			Variable*			LeftVar() { return LeftTab()->Var(); }
			Variable*			RightVar() { return RightTab()->Var(); }
			Variable*			TopVar() { return TopTab()->Var(); }
			Variable*			BottomVar() { return BottomTab()->Var(); }

			BRect				Frame();

			bool				PropagateToGroup(SATGroup* group);

			bool				MoveToTopLayer(SATWindow* window);

private:
		friend class SATGroup;
			void				_UninitConstraints();
			void				_UpdateConstraintValues();

			/*! SATGroup adds new windows to the area. */
			bool				_AddWindow(SATWindow* window,
									SATWindow* after = NULL);
			/*! After the last window has been removed the WindowArea delete
			himself and clean up all crossings. */
			bool				_RemoveWindow(SATWindow* window);

	inline	void				_InitCorners();
	inline	void				_CleanupCorners();
	inline	void				_SetToWindowCorner(Corner* corner);
	inline	void				_SetToNeighbourCorner(Corner* neighbour);
	inline	void				_UnsetWindowCorner(Corner* corner);
		//! opponent is the other neighbour of the neighbour
	inline	void				_UnsetNeighbourCorner(Corner* neighbour,
									Corner* opponent);

			// Find crossing by tab position in group and if not exist create
			// it.
			BReference<Crossing>	_CrossingByPosition(Crossing* crossing,
										SATGroup* group);

			void				_MoveToSAT(SATWindow* topWindow);

			BReference<SATGroup>	fGroup;

			SATWindowList		fWindowList;

			SATWindowList		fWindowLayerOrder;

			BReference<Crossing>	fLeftTopCrossing;
			BReference<Crossing>	fRightTopCrossing;
			BReference<Crossing>	fLeftBottomCrossing;
			BReference<Crossing>	fRightBottomCrossing;

			Constraint*			fMinWidthConstraint;
			Constraint*			fMinHeightConstraint;
			Constraint*			fMaxWidthConstraint;
			Constraint*			fMaxHeightConstraint;
			Constraint*			fKeepMaxWidthConstraint;
			Constraint*			fKeepMaxHeightConstraint;
			Constraint*			fWidthConstraint;
			Constraint*			fHeightConstraint;
			
			MagneticBorder		fMagneticBorder;
};


typedef BObjectList<WindowArea> WindowAreaList;
typedef BObjectList<Tab> TabList;

class BMessage;
class StackAndTile;


class SATGroup : public BReferenceable {
public:
		friend class Tab;
		friend class WindowArea;
		friend class GroupCookie;

								SATGroup();
								~SATGroup();

			LinearSpec*			GetLinearSpec() { return &fLinearSpec; }

			/*! Create a new WindowArea from the crossing and add the window. */
			bool				AddWindow(SATWindow* window, Tab* left, Tab* top,
									Tab* right, Tab* bottom);
			/*! Add a window to an existing window area. */
			bool				AddWindow(SATWindow* window, WindowArea* area,
									SATWindow* after = NULL);
			/*! If stayBelowMouse is true move the removed window below the
			cursor if necessary. */
			bool				RemoveWindow(SATWindow* window,
									bool stayBelowMouse = true);
			int32				CountItems();
			SATWindow*			WindowAt(int32 index);

			const WindowAreaList&	GetAreaList() { return fWindowAreaList; }
			
			/*! \return a sorted tab list. */
			const TabList*		HorizontalTabs();
			const TabList*		VerticalTabs();

			Tab*				FindHorizontalTab(float position);
			Tab*				FindVerticalTab(float position);

			void				WindowAreaRemoved(WindowArea* area);

	static	status_t			RestoreGroup(const BMessage& archive,
									StackAndTile* sat);
			status_t			ArchiveGroup(BMessage& archive);

private:
			BReference<Tab>		_AddHorizontalTab(float position = 0);
			BReference<Tab>		_AddVerticalTab(float position = 0);

			bool				_RemoveHorizontalTab(Tab* tab);
			bool				_RemoveVerticalTab(Tab* tab);

			Tab*				_FindTab(const TabList& list, float position);

			void				_SplitGroupIfNecessary(
									WindowArea* removedArea);
			void				_FillNeighbourList(
									WindowAreaList& neighbourWindows,
									WindowArea* area);
			void				_LeftNeighbours(
									WindowAreaList& neighbourWindows,
									WindowArea* window);
			void				_TopNeighbours(
									WindowAreaList& neighbourWindows,
									WindowArea* window);
			void				_RightNeighbours(
									WindowAreaList& neighbourWindows,
									WindowArea* window);
			void				_BottomNeighbours(
									WindowAreaList& neighbourWindows,
									WindowArea* window);
			bool				_FindConnectedGroup(WindowAreaList& seedList,
									WindowArea* removedArea,
									WindowAreaList& newGroup);
			void				_FollowSeed(WindowArea* area, WindowArea* veto,
									WindowAreaList& seedList,
									WindowAreaList& newGroup);
			void				_SpawnNewGroup(const WindowAreaList& newGroup);

			void				_EnsureGroupIsOnScreen(SATGroup* group);
	inline	void				_CallculateXOffset(BPoint& offset, BRect& frame,
									BRect& screen);
	inline	void				_CallculateYOffset(BPoint& offset, BRect& frame,
									BRect& screen);

protected:
			WindowAreaList		fWindowAreaList;
			SATWindowList		fSATWindowList;

			LinearSpec			fLinearSpec;

private:
			TabList				fHorizontalTabs;
			bool				fHorizontalTabsSorted;
			TabList				fVerticalTabs;
			bool				fVerticalTabsSorted;
};


typedef BObjectList<SATGroup> SATGroupList;

#endif
