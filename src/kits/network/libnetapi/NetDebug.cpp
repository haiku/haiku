/*=--------------------------------------------------------------------------=*
 * NetDebug.cpp -- Implementation of the BNetDebug class.
 *
 * Written by S.T. Mansfield (thephantom@mac.com)
 *
 * Remarks:
 *   * Although this would more properly be implemented as a namespace...
 *   * Do not burn the candle at both ends as it leads to the life of a
 *     hairdresser.
 *=--------------------------------------------------------------------------=*
 * Copyright (c) 2002, The OpenBeOS project.
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


#include <ctype.h>
#include <stdio.h>
#include <string.h>

#include <NetDebug.h>


// Off by default cuz the BeBook sez so.
static bool g_IsEnabled = false;


/* Enable
 *=--------------------------------------------------------------------------=*
 * Purpose:
 *     Enable/disable debug message capability for your app.
 *
 * Input parameter:
 *     Enable       : True/False to enable/disable debug message output.
 *
 * Remarks:
 *     This flag/setting is a per application basis, and not a per-instance
 *     occurrence as one would expect when instantiating an instance of
 *     this class.  This is by design as /everything/ is static.  Caveat
 *     Emptor.  Needs to be dealt with in G.E.
 */
void BNetDebug::Enable( bool Enable )
{
    g_IsEnabled = Enable;
}


/* IsEnabled
 *=--------------------------------------------------------------------------=*
 * Purpose:
 *     Quiz the enable/disable status.
 *
 * Returns:
 *     True/false if enabled/disabled.
 */
bool BNetDebug::IsEnabled( void )
{
    return g_IsEnabled;
}


/* Print
 *=--------------------------------------------------------------------------=*
 * Purpose:
 *     If enabled, spew forth a debug message.
 *
 * Input parameter:
 *     msg          : The message to print.
 *
 * Remarks:
 *     * Basically a no-op if not enabled.
 *     * We're inheriting R5 (and Nettle's) behavior, so...
 *       * The output is always "debug: msg\n"  Yes, kids, you read it right;
 *         you get a newline whether you want it or not!
 *       * If msg is empty, you get "debug: \n"
 *       * Message is always printed on stderr.  Redirect accordingly.
 */
void BNetDebug::Print( const char* msg )
{
    if ( g_IsEnabled )
    {
        if ( msg == NULL )
        {
            msg = "";
        }

        fprintf( stderr, "debug: %s\n", msg );
    }
}


/* Dump
 *=--------------------------------------------------------------------------=*
 * Purpose:
 *     If enabled, spew forth a combination hex/ASCII dump of raw data.
 *
 * Input parameters:
 *     data         : Data to dump.
 *     size         : How many bytes of data to dump.
 *     title        : Title to display in message header.
 *
 * Remarks:
 *     * Basically a no-op if not enabled.
 *     * We're inheriting R5 (and Nettle's) behavior, so...
 *       * The output is always "debug: msg\n"  Yes, kids, you read it right;
 *         you get a newline whether you want it or not!
 *       * If msg is empty, you get "debug: \n"
 *       * Behavior is undefined if data or title is NULL.  This is a
 *         possible design flaw in Nettle/R5.
 *       * Do not expect an output like this:
 *          00 11 22 33 44 55 66 77 88 99 AA BB CC DD EE FF | abcdefghijklmnop
 *          00 11 22 33 44 55 66 77 88 99 AA BB CC DD EE FF | abcdefghijklmnop
 *          00 11 22 33 44 55 66 77 88 99 AA BB CC DD EE FF | abcdefghijklmnop
 *          00 11 22 33 44 55 66 77 88 99 AA BB CC DD EE FF | abcdefghijklmnop
 *         because you ain't gettin' it.  You will get a complete hex dump,
 *         /followed/ by an ASCII dump.  More Glass Elevator stuff.
 *     * Nettle dumps to stdOUT, the BeBook says all output goes to stdERR, so
 *       the author chose to...
 *     * always print to stdERR.  Redirect accordingly.
 *     * stderr is flushed after the dump is complete to keep things
 *       reasonably cohesive in appearance.  This might be an expensive
 *       operation so use judiciously.
 */
void BNetDebug::Dump( const char* data, size_t size, const char* title )
{
    int     i;
    char    line[17];

    if ( g_IsEnabled == false )
    {
        return;
    }

    fprintf( stderr, "----------\n%s\n(dumping %d bytes)\n", title, size );

    memset( line, 0, sizeof( line ) );

    while ( i < size )
    {
        fprintf( stderr, "%02x ", ( unsigned char )data[i] );

        line[i % 16] = ( isprint( data[i] ) ) ? data[i] : '.';

        if ( ( i + 1 ) % 16 == 0 )
        {
            fprintf( stderr, "  |  %s\n", line );
            memset( line, 0, sizeof( line ) );
        }

        i++;
    }

    if ( ( i + 1 ) % 16 != 0 )
    {
        while ( i % 16 != 0 )
        {
            printf( "   " );
            i++;
        }

        fprintf( stderr, "  |  %s", line );
    }

    fprintf( stderr, "\n----------\n" );
    fflush( stderr );
}


/*=------------------------------------------------------------------- End -=*/

