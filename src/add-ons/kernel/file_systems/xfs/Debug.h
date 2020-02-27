/*
 * Copyright 2020, Shubham Bhagat, shubhambhagat111@yahoo.com
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef _DEBUG_H_
#define _DEBUG_H_

#define TRACE_XFS
#ifdef TRACE_XFS
#define TRACE(x...) dprintf("\n\33[34mxfs:\33[0m " x)
#else
#define TRACE(x...) ;
#endif
#define ERROR(x...) dprintf("\n\33[34mxfs:\33[0m " x)

#endif