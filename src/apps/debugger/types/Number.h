/*
 * Copyright 2014, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */
#ifndef NUMBER_H
#define NUMBER_H

#include <TypeConstants.h>

#include "Variant.h"


class BString;


class Number {
public:
								Number();
	virtual						~Number();
								Number(type_code type, const BString& value);
								Number(const BVariant& value);
								Number(const Number& other);

			void				SetTo(type_code type, const BString& value,
									int32 base = 10);
			void				SetTo(const BVariant& value);

			Number&				operator=(const Number& rhs);
			Number&				operator+=(const Number& rhs);
			Number&				operator-=(const Number& rhs);
			Number&				operator/=(const Number& rhs);
			Number&				operator*=(const Number& rhs);
			Number&				operator%=(const Number& rhs);

			Number&				operator&=(const Number& rhs);
			Number&				operator|=(const Number& rhs);
			Number&				operator^=(const Number& rhs);

			Number				operator-() const;
			Number				operator~() const;

			int					operator<(const Number& rhs) const;
			int					operator<=(const Number& rhs) const;
			int					operator>(const Number& rhs) const;
			int					operator>=(const Number& rhs) const;
			int					operator==(const Number& rhs) const;
			int					operator!=(const Number& rhs) const;

			BVariant			GetValue() const;

private:
	BVariant					fValue;
};


#endif	// NUMBER_H
