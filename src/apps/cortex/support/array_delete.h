/*******************************************************************************
/
/	File:			array_delete.h
/
/   Description:	Template for deleting a new[] array of something.
/
/	Copyright 1998-1999, Be Incorporated, All Rights Reserved
/
*******************************************************************************/


#if !defined( _array_delete_h )
#define _array_delete_h

//	Oooh! It's a template!
template<class C> class array_delete {
	C * & m_ptr;
public:
	//	auto_ptr<> uses delete, not delete[], so we have to write our own.
	//	I like hanging on to a reference, because if we manually delete the
	//	array and set the pointer to NULL (or otherwise change the pointer)
	//	it will still work. Others like the more elaborate implementation
	//	of auto_ptr<>. Your Mileage May Vary.
	array_delete(C * & ptr) : m_ptr(ptr) {}
	~array_delete() { delete[] m_ptr; }
};


#endif	/* array_delete_h */

