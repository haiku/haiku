/*
 * Copyright 2012, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _SHARED_SOLVER_H
#define _SHARED_SOLVER_H


#include <LayoutContext.h>
#include <ObjectList.h>
#include <Referenceable.h>

#include "LinearSpec.h"


namespace BALM {
	class BALMLayout;
};



namespace BPrivate {


class SharedSolver : BLayoutContextListener, public BReferenceable {
public:
								SharedSolver();
								~SharedSolver();

			void				Invalidate(bool children);

			LinearSpec*			Solver() const;
			ResultType			Result();

			void				RegisterLayout(BALM::BALMLayout* layout);
			void				LayoutLeaving(BALM::BALMLayout* layout);

			ResultType			ValidateMinSize();
			ResultType			ValidateMaxSize();
			ResultType			ValidateLayout(BLayoutContext* context);
private:
			struct MinSizeValidator;
			struct MaxSizeValidator;

			friend struct MinSizeValidator;
			friend struct MaxSizeValidator;

			void				SetMaxSize(BALM::BALMLayout* layout,
									const BSize& max);
			void				SetMinSize(BALM::BALMLayout* layout,
									const BSize& min);

	virtual	void				LayoutContextLeft(BLayoutContext* context);

			void				_UpdateConstraints();
			void				_SetContext(BLayoutContext* context);
			bool				_IsMinSet();
			bool				_IsMaxSet();
			void				_ValidateConstraints();

			bool				fConstraintsValid;
			bool				fMinValid;
			bool				fMaxValid;
			bool				fLayoutValid;

			BLayoutContext*		fLayoutContext;
			BObjectList<BALM::BALMLayout> fLayouts;
			LinearSpec			fLinearSpec;

			ResultType			fMinResult;
			ResultType			fMaxResult;
			ResultType			fLayoutResult;
};


};


using BPrivate::SharedSolver;


#endif
