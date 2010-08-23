/*
 * Copyright 2010, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Clemens Zeidler <haiku@clemens-zeidler.de>
 */
#ifndef SAT_WINDOW_H
#define SAT_WINDOW_H


#include <Region.h>
#include "SATDecorator.h"
#include "Stacking.h"
#include "Tiling.h"


class Desktop;
class SATGroup;
class StackAndTile;
class Window;


class GroupCookie
{
public:
							GroupCookie(SATWindow* satWindow);
							~GroupCookie();

		bool				Init(SATGroup* group, WindowArea* area);
		void				Uninit();

		void				DoGroupLayout(SATWindow* triggerWindow);
		void				MoveWindow(int32 workspace);
		void				UpdateWindowSize();

		SATGroup*			GetGroup() { return fSATGroup.Get(); }

		WindowArea*			GetWindowArea() { return windowArea; }

		bool				PropagateToGroup(SATGroup* group, WindowArea* area);

private:
		SATWindow*			fSATWindow;

		BReference<SATGroup>	fSATGroup;

		WindowArea*			windowArea;

		Variable*			leftBorder;
		Variable*			topBorder;
		Variable*			rightBorder;
		Variable*			bottomBorder;

		Constraint*			leftBorderConstraint;
		Constraint*			topBorderConstraint;
		Constraint*			rightBorderConstraint;
		Constraint*			bottomBorderConstraint;

		Constraint*			leftConstraint;
		Constraint*			topConstraint;
		Constraint*			minWidthConstraint;
		Constraint*			minHeightConstraint;
		Constraint*			widthConstraint;
		Constraint*			heightConstraint;
};


class SATWindow {
public:
							SATWindow(StackAndTile* sat, Window* window);
							~SATWindow();

		Window*				GetWindow() { return fWindow; }
		SATDecorator*		GetDecorator() { return fDecorator; }
		StackAndTile*		GetStackAndTile() { return fStackAndTile; }
		Desktop*			GetDesktop() { return fDesktop; }
		//! Can be NULL if memory allocation failed!
		SATGroup*			GetGroup();
		WindowArea*			GetWindowArea() {
								return fGroupCookie->GetWindowArea(); }

		bool				HandleMessage(SATWindow* sender,
								BPrivate::ServerLink& link);

		bool				PropagateToGroup(SATGroup* group, WindowArea* area);

		void				UpdateGroupWindowsSize();
		void				UpdateWindowSize();

		//! Move the window to the tab's position. 
		void				MoveWindowToSAT(int32 workspace);

		// hook function called from SATGroup
		bool				AddedToGroup(SATGroup* group, WindowArea* area);
		bool				RemovedFromGroup(SATGroup* group);
		void				RemovedFromArea(WindowArea* area);

		bool				StackWindow(SATWindow* child);

		void				FindSnappingCandidates();
		bool				JoinCandidates();
		void				DoGroupLayout();

		//! \return the complete window frame including the Decorator
		BRect				CompleteWindowFrame();

		//! \return true if window is in a group with a least another window
		bool				PositionManagedBySAT();

		bool				HighlightTab(bool active);
		bool				HighlightBorders(bool active);
		bool				IsTabHighlighted();
		bool				IsBordersHighlighted();

		bool				SetStackedMode(bool stacked = true);
		bool				SetStackedTabLength(float length, bool drawZoom);
		bool				SetStackedTabMoving(bool moving = true);
		void				TabLocationMoved(float location, bool shifting);

private:
		void						_InitGroup();

		Window*						fWindow;
		SATDecorator*				fDecorator;
		StackAndTile*				fStackAndTile;
		Desktop*					fDesktop;

		//! Current group.
		GroupCookie*				fGroupCookie;
		/*! If the window is added to another group the own group is cached
		here. */
		GroupCookie					fOwnGroupCookie;
		GroupCookie					fForeignGroupCookie;

		SATSnappingBehaviour*		fOngoingSnapping;
		SATStacking					fSATStacking;
		SATTiling					fSATTiling;

		SATSnappingBehaviourList	fSATSnappingBehaviourList;

		bool						fShutdown;
};


#endif
