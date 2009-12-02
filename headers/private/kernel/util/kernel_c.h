/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef _KERNEL_UTIL_KERNEL_C_H
#define _KERNEL_UTIL_KERNEL_C_H


/*!	Defines a structure that has the size of a certain C++ structure.
	\param name The name of the C++ structure.
	\param flatName The name of the structure to be defined.
*/
#define DEFINE_FLAT_KERNEL_CPP_STRUCT(name, flatName)	\
	struct flatName {									\
		char	bytes[_KERNEL_CPP_STRUCT_SIZE_##name];	\
	};


/*!	In C mode DEFINE_KERNEL_CPP_STRUCT() defines a struct \a name with the
	size of the C++ structure of the same name. In C++ it is a no-op.
*/
#ifdef __cplusplus
#	define DEFINE_KERNEL_CPP_STRUCT(name)
#else
#	define DEFINE_KERNEL_CPP_STRUCT(name)			\
		DEFINE_FLAT_KERNEL_CPP_STRUCT(name, name)	\
		typedef struct name name;
#endif


#endif	/* _KERNEL_UTIL_KERNEL_C_H */
