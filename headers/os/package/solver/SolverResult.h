/*
 * Copyright 2013, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _PACKAGE__SOLVER_RESULT_H_
#define _PACKAGE__SOLVER_RESULT_H_


#include <ObjectList.h>


namespace BPackageKit {


class BSolverPackage;


class BSolverResultElement {
public:
			enum BType {
				B_TYPE_INSTALL,
				B_TYPE_UNINSTALL
			};

public:
								BSolverResultElement(BType type,
									BSolverPackage* package);
								BSolverResultElement(
									const BSolverResultElement& other);
								~BSolverResultElement();

			BType				Type() const;
			BSolverPackage*		Package() const;

			BSolverResultElement& operator=(const BSolverResultElement& other);

private:
			BType				fType;
			BSolverPackage*		fPackage;
};


class BSolverResult {
public:
								BSolverResult();
								~BSolverResult();

			bool				IsEmpty() const;
			int32				CountElements() const;
			const BSolverResultElement* ElementAt(int32 index) const;

			void				MakeEmpty();
			bool				AppendElement(
									const BSolverResultElement& element);

private:
			typedef BObjectList<BSolverResultElement> ElementList;

private:
			ElementList			fElements;
};


}	// namespace BPackageKit


#endif // _PACKAGE__SOLVER_RESULT_H_
