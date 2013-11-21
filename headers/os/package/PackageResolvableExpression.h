/*
 * Copyright 2011, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 */
#ifndef _PACKAGE__PACKAGE_RESOLVABLE_EXPRESSION_H_
#define _PACKAGE__PACKAGE_RESOLVABLE_EXPRESSION_H_


#include <String.h>

#include <package/PackageResolvableOperator.h>
#include <package/PackageVersion.h>


namespace BPackageKit {


namespace BHPKG {
	class BPackageResolvableExpressionData;
}
using BHPKG::BPackageResolvableExpressionData;


class BPackageResolvable;


/*
 * Expresses a constraint on a specific resolvable, either just a name
 * or a name plus a relational operator and a version.
 * Instances of these will be matched against all the BPackageResolvable(s)
 * of individual packages in order to solve a package management request.
 *
 * 		resolvable-expression ::= <name>[<op><version>]
 * 		op                    ::= '<' | '<=' | '==' | '>=' | '>'
 *
 * String examples:
 * 		haiku==r1
 * 		lib:libssl==0.9.8
 * 		subversion>=1.5
 * 		cmd:svn
 */
class BPackageResolvableExpression {
public:
								BPackageResolvableExpression();
								BPackageResolvableExpression(
									const BPackageResolvableExpressionData& data
									);
	explicit					BPackageResolvableExpression(
									const BString& expressionString);
								BPackageResolvableExpression(
									const BString& name,
									BPackageResolvableOperator _operator,
									const BPackageVersion& version);

			status_t			InitCheck() const;

			const BString&		Name() const;
			BPackageResolvableOperator	Operator() const;
			const BPackageVersion& Version() const;

			BString				ToString() const;

			status_t			SetTo(const BString& expressionString);
			void				SetTo(const BString& name,
									BPackageResolvableOperator _operator,
									const BPackageVersion& version);
			void				Clear();

			bool				Matches(const BPackageVersion& version,
									const BPackageVersion& compatibleVersion)
									const;
			bool				Matches(const BPackageResolvable& provides)
									const;

public:
	static	const char*			kOperatorNames[];

private:
			BString				fName;
			BPackageResolvableOperator	fOperator;
			BPackageVersion		fVersion;
};


}	// namespace BPackageKit


#endif	// _PACKAGE__PACKAGE_RESOLVABLE_EXPRESSION_H_
