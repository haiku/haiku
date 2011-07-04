/*
 * Copyright 2011, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */
#ifndef TYPE_LOOKUP_CONSTRAINTS_H
#define TYPE_LOOKUP_CONSTRAINTS_H


#include <String.h>

#include "Type.h"


class TypeLookupConstraints {
public:
								TypeLookupConstraints();
									// no constraints
								TypeLookupConstraints(type_kind typeKind);
									// constrain on type only
								TypeLookupConstraints(type_kind typeKind,
									int32 subtypeKind);

		bool					HasTypeKind() const;
		bool					HasSubtypeKind() const;
		bool					HasBaseTypeName() const;
		type_kind				TypeKind() const;
		int32					SubtypeKind() const;
		const BString&			BaseTypeName() const;

		void					SetTypeKind(type_kind typeKind);
		void					SetSubtypeKind(int32 subtypeKind);
		void					SetBaseTypeName(const BString& name);

private:
		type_kind				fTypeKind;
		int32					fSubtypeKind;
		bool					fTypeKindGiven;
		bool					fSubtypeKindGiven;
		BString					fBaseTypeName;
};

#endif // TYPE_LOOKUP_CONSTRAINTS_H
