// Modified BeOS header. Just here to be able to compile and test BPath.
// To be replaced by the OpenBeOS version to be provided by the IK Team.

#ifndef	__sk_flattenable_h__
#define	__sk_flattenable_h__

#include <SupportDefs.h>

/*-------------------------------------------------------------*/
/*----- BFlattenable class ------------------------------------*/

class BFlattenable {
public:

virtual	bool		IsFixedSize() const = 0;
virtual	type_code	TypeCode() const = 0;
virtual	ssize_t		FlattenedSize() const = 0;
virtual	status_t	Flatten(void *buffer, ssize_t size) const = 0;
virtual	bool		AllowsTypeCode(type_code code) const;
virtual	status_t	Unflatten(type_code c, const void *buf, ssize_t size) = 0;

virtual				~BFlattenable();	//	was a reserved virtual in R4.0

/*----- Private or reserved ---------------*/

private:
		void		_ReservedFlattenable1();
virtual	void		_ReservedFlattenable2();
virtual	void		_ReservedFlattenable3();
};

/*-------------------------------------------------------------*/
/*-------------------------------------------------------------*/

#endif	// __sk_flattenable_h__

