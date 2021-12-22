/*=--------------------------------------------------------------------------=*
 * NetDebug.h -- Interface definition for the BNetDebug class.
 *
 * Written by S.T. Mansfield (thephantom@mac.com)
 *=--------------------------------------------------------------------------=*
 * Copyright (c) 2002, The Haiku project.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *=--------------------------------------------------------------------------=*
 */

#ifndef _NETDEBUG_H
#define _NETDEBUG_H

/*
 * Net debugging class
 *
 * Use this class to print informative messages and dump data
 * to stderr and provide control over whether debug messages
 * get printed or not.
 */

class BNetDebug
{
public:
	// turn debugging message output on or off
    static void Enable(bool);

	// test debugging message output state
    static bool IsEnabled();

	// print a debugging message
    static void Print(const char *msg);

	// dump raw data in a nice hd-like format
    static void Dump(const char *data, size_t size, const char *title);
};

#endif // <-- #ifndef _NETDEBUG_H

/*=------------------------------------------------------------------- End -=*/

