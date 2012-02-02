/*
 * Copyright 2012, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _SHARED_SOLVER_H
#define _SHARED_SOLVER_H


#include <set>

#include <Archivable.h>
#include <LayoutContext.h>
#include <ObjectList.h>
#include <Referenceable.h>

#include "LinearSpec.h"


class BMessage;

namespace BALM {
	class BALMLayout;
	class Area;
};

using BALM::BALMLayout;
using BALM::Area;


namespace BPrivate {


class SharedSolver : BLayoutContextListener, public BReferenceable,
	   public BArchivable {
public:
								SharedSolver(BMessage* archive);
								SharedSolver();
								~SharedSolver();

			void				Invalidate(bool children);

			LinearSpec*			Solver() const;
			ResultType			Result();

			void				RegisterLayout(BALMLayout* layout);
			void				LayoutLeaving(const BALMLayout* layout);

			ResultType			ValidateMinSize();
			ResultType			ValidateMaxSize();
			ResultType			ValidatePreferredSize();
			ResultType			ValidateLayout(BLayoutContext* context);

			status_t			AddFriendReferences(const BALMLayout* layout,
									BMessage* archive, const char* field);

			status_t			Archive(BMessage* archive, bool deep) const;
			status_t			AllArchived(BMessage* archive) const;
			status_t			AllUnarchived(const BMessage* archive);

	static	BArchivable*		Instantiate(BMessage* archive);

private:
			struct MinSizeValidator;
			struct MaxSizeValidator;
			struct PreferredSizeValidator;

			friend struct MinSizeValidator;
			friend struct MaxSizeValidator;
			friend struct PreferredSizeValidator;


	static	void				_AddConstraintsToSet(Area* area,
									std::set<Constraint*>& constraints);
	static	status_t			_AddConstraintToArchive(Constraint* constraint,
									BMessage* archive);
			status_t			_InstantiateConstraint(const void* rawData,
									ssize_t numBytes, BUnarchiver& unarchiver);

			void				SetMaxSize(BALM::BALMLayout* layout,
									const BSize& max);
			void				SetMinSize(BALM::BALMLayout* layout,
									const BSize& min);
			void				SetPreferredSize(BALM::BALMLayout* layout,
									const BSize& preferred);

	virtual	void				LayoutContextLeft(BLayoutContext* context);

			void				_UpdateConstraints();
			void				_SetContext(BLayoutContext* context);
			bool				_IsMinSet();
			bool				_IsMaxSet();
			void				_ValidateConstraints();

			template <class Validator>
			void				_Validate(bool& isValid, ResultType& result);


			bool				fConstraintsValid;
			bool				fMinValid;
			bool				fMaxValid;
			bool				fPreferredValid;
			bool				fLayoutValid;

			BLayoutContext*		fLayoutContext;
			BObjectList<BALM::BALMLayout> fLayouts;
			LinearSpec			fLinearSpec;

			ResultType			fMinResult;
			ResultType			fMaxResult;
			ResultType			fPreferredResult;
			ResultType			fLayoutResult;
};


};


using BPrivate::SharedSolver;


#endif
