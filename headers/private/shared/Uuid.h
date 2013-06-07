/*
 * Copyright 2013, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 */
#ifndef _UUID_H_
#define _UUID_H_


#include <String.h>


namespace BPrivate {


class BUuid {
public:
								BUuid();
								BUuid(const BUuid& other);
								~BUuid();

			bool				IsNil() const;

			BUuid&				SetToRandom();

			BString				ToString() const;

			int					Compare(const BUuid& other) const;

	inline	bool				operator==(const BUuid& other) const;
	inline	bool				operator!=(const BUuid& other) const;

	inline	bool				operator<(const BUuid& other) const;
	inline	bool				operator>(const BUuid& other) const;
	inline	bool				operator<=(const BUuid& other) const;
	inline	bool				operator>=(const BUuid& other) const;

			BUuid&				operator=(const BUuid& other);

private:
			bool				_SetToDevRandom();
			void				_SetToRandomFallback();

private:
			uint8				fValue[16];
};


inline bool
BUuid::operator==(const BUuid& other) const
{
	return Compare(other) == 0;
}


inline bool
BUuid::operator!=(const BUuid& other) const
{
	return Compare(other) != 0;
}


inline bool
BUuid::operator<(const BUuid& other) const
{
	return Compare(other) < 0;
}


inline bool
BUuid::operator>(const BUuid& other) const
{
	return Compare(other) > 0;
}


inline bool
BUuid::operator<=(const BUuid& other) const
{
	return Compare(other) <= 0;
}


inline bool
BUuid::operator>=(const BUuid& other) const
{
	return Compare(other) >= 0;
}


}	// namespace BPrivate


using BPrivate::BUuid;


#endif	// _UUID_H_
