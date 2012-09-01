/*
 * Copyright 2006 - 2010, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef	ALM_LAYOUT_H
#define	ALM_LAYOUT_H


#include <AbstractLayout.h>
#include <Size.h>

#include "Area.h"
#include "LinearSpec.h"
#include "Tab.h"


class BView;
class BLayoutItem;
class Constraint;


namespace BPrivate {
	class SharedSolver;
};


namespace BALM {


class Column;
class ALMGroup;
class Row;
class RowColumnManager;


/*!
 * A GUI layout engine using the Auckland Layout Model (ALM).
 */
class BALMLayout : public BAbstractLayout {
public:
								BALMLayout(float hSpacing = 0.0f,
									float vSpacing = 0.0f,
									BALMLayout* friendLayout = NULL);
								BALMLayout(BMessage* archive);
	virtual						~BALMLayout();

			BReference<XTab>	AddXTab();
			void				AddXTabs(BReference<XTab>* tabs, uint32 count);
			BReference<YTab>	AddYTab();
			void				AddYTabs(BReference<YTab>* tabs, uint32 count);

			int32				CountXTabs() const;
			int32				CountYTabs() const;
			XTab*				XTabAt(int32 index, bool ordered = false);
			XTab*				XTabAt(int32 index) const;
			YTab*				YTabAt(int32 index, bool ordered = false);
			YTab*				YTabAt(int32 index) const;
	const	XTabList			GetXTabs() const;
	const	YTabList			GetYTabs() const;

			int32				IndexOf(XTab* tab, bool ordered = false);
			int32				IndexOf(YTab* tab, bool ordered = false);

			int32				CountConstraints() const;
			Constraint*			ConstraintAt(int32 index) const;
			bool				AddConstraint(Constraint* constraint);
			bool				RemoveConstraint(Constraint* constraint,
									bool deleteConstraint = true);

			Constraint*			AddConstraint(double coeff1, Variable* var1,
									OperatorType op, double rightSide,
									double penaltyNeg = -1,
									double penaltyPos = -1);
			Constraint*			AddConstraint(double coeff1, Variable* var1,
									double coeff2, Variable* var2,
									OperatorType op, double rightSide,
									double penaltyNeg = -1,
									double penaltyPos = -1);
			Constraint*			AddConstraint(double coeff1, Variable* var1,
									double coeff2, Variable* var2,
									double coeff3, Variable* var3,
									OperatorType op, double rightSide,
									double penaltyNeg = -1,
									double penaltyPos = -1);
			Constraint*			AddConstraint(double coeff1, Variable* var1,
									double coeff2, Variable* var2,
									double coeff3, Variable* var3,
									double coeff4, Variable* var4,
									OperatorType op, double rightSide,
									double penaltyNeg = -1,
									double penaltyPos = -1);

			Row*				AddRow(YTab* top, YTab* bottom);
			Column*				AddColumn(XTab* left, XTab* right);

			XTab*				Left() const;
			XTab*				Right() const;
			YTab*				Top() const;
			YTab*				Bottom() const;

			LinearProgramming::LinearSpec* Solver() const;
			LinearProgramming::ResultType ValidateLayout();

			void				SetInsets(float insets);
			void				SetInsets(float x, float y);
			void				SetInsets(float left, float top, float right,
									float bottom);
			void				GetInsets(float* left, float* top, float* right,
									float* bottom) const;

			void				SetSpacing(float hSpacing, float vSpacing);
			void				GetSpacing(float* _hSpacing,
									float* _vSpacing) const;

			Area*				AreaFor(int32 id) const;
			Area*				AreaFor(const BView* view) const;
			Area*				AreaFor(const BLayoutItem* item) const;
			int32				CountAreas() const;
			Area*				AreaAt(int32 index) const;
	
			XTab*				LeftOf(const BView* view) const;
			XTab*				LeftOf(const BLayoutItem* item) const;
			XTab*				RightOf(const BView* view) const;
			XTab*				RightOf(const BLayoutItem* item) const;
			YTab*				TopOf(const BView* view) const;
			YTab*				TopOf(const BLayoutItem* item) const;
			YTab*				BottomOf(const BView* view) const;
			YTab*				BottomOf(const BLayoutItem* item) const;

	virtual	BLayoutItem*		AddView(BView* child);
	virtual	BLayoutItem*		AddView(int32 index, BView* child);
	virtual	Area*				AddView(BView* view, XTab* left, YTab* top,
									XTab* right = NULL, YTab* bottom = NULL);
	virtual	Area*				AddView(BView* view, Row* row, Column* column);

	virtual	bool				AddItem(BLayoutItem* item);
	virtual	bool				AddItem(int32 index, BLayoutItem* item);
	virtual	Area*				AddItem(BLayoutItem* item, XTab* left,
									YTab* top, XTab* right = NULL,
									YTab* bottom = NULL);
	virtual	Area*				AddItem(BLayoutItem* item, Row* row,
									Column* column);

	struct BadLayoutPolicy;

			void				SetBadLayoutPolicy(BadLayoutPolicy* policy);
			BadLayoutPolicy*	GetBadLayoutPolicy() const;

	virtual	BSize				BaseMinSize();
	virtual	BSize				BaseMaxSize();
	virtual	BSize				BasePreferredSize();
	virtual	BAlignment			BaseAlignment();

	virtual	status_t 			Archive(BMessage* into, bool deep = true) const;
	static 	BArchivable*		Instantiate(BMessage* archive);

	virtual	status_t			Perform(perform_code d, void* arg);

protected:
	virtual	bool				ItemAdded(BLayoutItem* item, int32 atIndex);
	virtual	void				ItemRemoved(BLayoutItem* item, int32 fromIndex);

	virtual status_t			ItemArchived(BMessage* into, BLayoutItem* item,
									int32 index) const;
	virtual	status_t			ItemUnarchived(const BMessage* from,
									BLayoutItem* item, int32 index);

	virtual	status_t 			AllUnarchived(const BMessage* archive);
	virtual	status_t 			AllArchived(BMessage* archive) const;

	virtual	void				LayoutInvalidated(bool children);
	virtual	void				DoLayout();

public:
	struct BadLayoutPolicy : public BArchivable {
								BadLayoutPolicy();
								BadLayoutPolicy(BMessage* archive);
		virtual					~BadLayoutPolicy();
		/* return false to abandon layout, true to use layout */
		virtual bool			OnBadLayout(BALMLayout* layout,
									LinearProgramming::ResultType result,
									BLayoutContext* context) = 0;
	};

	struct DefaultPolicy : public BadLayoutPolicy {
								DefaultPolicy();
								DefaultPolicy(BMessage* archive);
		virtual					~DefaultPolicy();
		virtual bool			OnBadLayout(BALMLayout* layout,
									LinearProgramming::ResultType result,
									BLayoutContext* context);
		virtual status_t		Archive(BMessage* message,
									bool deep = false) const;
		static BArchivable*		Instantiate(BMessage* message);
	};

private:

	// FBC padding
	virtual	void				_ReservedALMLayout1();
	virtual	void				_ReservedALMLayout2();
	virtual	void				_ReservedALMLayout3();
	virtual	void				_ReservedALMLayout4();
	virtual	void				_ReservedALMLayout5();
	virtual	void				_ReservedALMLayout6();
	virtual	void				_ReservedALMLayout7();
	virtual	void				_ReservedALMLayout8();
	virtual	void				_ReservedALMLayout9();
	virtual	void				_ReservedALMLayout10();

	// forbidden methods
								BALMLayout(const BALMLayout&);
			void				operator =(const BALMLayout&);

private:
	template <class T>
	struct TabAddTransaction;

	template <class T>
	friend class TabAddTransaction;
	friend class BPrivate::SharedSolver;

	friend class XTab;
	friend class YTab;
	friend class Area;

			class BALMLayoutSpecListener;
	friend	class BALMLayoutSpecListener;
			
			float				InsetForTab(XTab* tab) const;
			float				InsetForTab(YTab* tab) const;

			void				UpdateConstraints(BLayoutContext* context);

			void				_RemoveSelfFromTab(XTab* tab);
			void				_RemoveSelfFromTab(YTab* tab);
			bool				_HasTabInLayout(XTab* tab);
			bool				_HasTabInLayout(YTab* tab);
			bool				_AddedTab(XTab* tab);
			bool				_AddedTab(YTab* tab);

			BLayoutItem*		_LayoutItemToAdd(BView* view);
			void				_SetSolver(BPrivate::SharedSolver* solver);

			BPrivate::SharedSolver* fSolver;
			BALMLayoutSpecListener* fSpecListener;

			BReference<XTab>	fLeft;
			BReference<XTab>	fRight;
			BReference<YTab>	fTop;
			BReference<YTab>	fBottom;
			BSize				fMinSize;
			BSize				fMaxSize;
			BSize				fPreferredSize;

			float				fLeftInset;
			float				fRightInset;
			float				fTopInset;
			float				fBottomInset;

			float				fHSpacing;
			float				fVSpacing;

			XTabList			fXTabList;
			bool				fXTabsSorted;

			YTabList			fYTabList;
			bool				fYTabsSorted;

			BObjectList<Constraint>	fConstraints;

			RowColumnManager*	fRowColumnManager;

			BadLayoutPolicy*	fBadLayoutPolicy;

			uint32				_reserved[5];
};

}	// namespace BALM


using BALM::BALMLayout;


#endif	// ALM_LAYOUT_H
