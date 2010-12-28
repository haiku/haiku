/*
* Copyright 2010, Haiku. All rights reserved.
* Distributed under the terms of the MIT License.
*
* Authors:
*		Michael Pfeiffer
*/
#ifndef GP_ARRAY_H
#define GP_ARRAY_H

template<typename TYPE>
class GPArray
{
public:
	typedef TYPE* PointerType;

					GPArray();
	virtual			~GPArray();

	void			SetSize(int size);
	int				Size() const;
	void			DecreaseSize();
	TYPE**			Array();
	TYPE**			Array() const;
	bool			IsEmpty() const;

private:
	TYPE**	fArray;
	int		fSize;
};

#include "GPArray.cpp"

#endif
