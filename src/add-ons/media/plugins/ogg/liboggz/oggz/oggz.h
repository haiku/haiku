/*
   Copyright (C) 2003 Commonwealth Scientific and Industrial Research
   Organisation (CSIRO) Australia

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions
   are met:

   - Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.

   - Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.

   - Neither the name of CSIRO Australia nor the names of its
   contributors may be used to endorse or promote products derived from
   this software without specific prior written permission.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
   ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
   PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE ORGANISATION OR
   CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
   EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
   PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
   PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
   LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
   NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
   SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef __OGGZ_H__
#define __OGGZ_H__

#include <stdio.h>
#include <sys/types.h>

#include <ogg/ogg.h>
#include <oggz/oggz_constants.h>
#include <oggz/oggz_table.h>

/** \mainpage
 *
 * \section intro Oggz makes programming with Ogg easy!
 *
 * This is the documentation for the Oggz C API. Oggz provides a simple
 * programming interface for reading and writing Ogg files and streams.
 * Ogg is an interleaving data container developed by Monty
 * at <a href="http://www.xiph.org/">Xiph.Org</a>, originally to
 * support the Ogg Vorbis audio format.
 *
 * liboggz supports the flexibility afforded by the Ogg file format while
 * presenting the following API niceties:
 *
 * - Strict adherence to the formatting requirements of Ogg
 *   \link basics bitstreams \endlink, to ensure that only valid bitstreams
 *   are generated
 * - A simple, callback based open/read/close or open/write/close
 *   \link oggz.h interface \endlink to raw Ogg files
 * - A customisable \link seek_api seeking \endlink abstraction for
 *   seeking on multitrack Ogg data
 * - A packet queue for feeding incoming packets for writing, with callback
 *   based notification when this queue is empty
 * - A means of overriding the \link oggz_io.h IO functions \endlink used by
 *   Oggz, for easier integration with media frameworks and similar systems.
 * - A handy \link oggz_table.h table \endlink structure for storing
 *   information on each logical bitstream
 * 
 * \subsection contents Contents
 *
 * - \link basics Basics \endlink:
 * Information about Ogg required to understand liboggz
 *
 * - \link oggz.h oggz.h \endlink:
 * Documentation of the Oggz C API
 *
 * - \link configuration Configuration \endlink:
 * Customizing liboggz to only read or write.
 *
 * - \link building Building \endlink:
 * Information related to building software that uses liboggz.
 *
 * \section Licensing
 *
 * liboggz is provided under the following BSD-style open source license:
 *
 * \include COPYING
 *
 */

/** \defgroup basics Ogg basics
 *
 * \section Scope
 *
 * This section provides a minimal introduction to Ogg concepts, covering
 * only that which is required to use liboggz.
 *
 * For more detailed information, see the
 * <a href="http://www.xiph.org/ogg/">Ogg</a> homepage
 * or IETF <a href="http://www.ietf.org/rfc/rfc3533.txt">RFC 3533</a>
 * <i>The Ogg File Format version 0</i>.
 *
 * \section Terminology
 *
 * The monospace text below is quoted directly from RFC 3533.
 * For each concept introduced, tips related to liboggz are provided
 * in bullet points.
 *
 * \subsection bitstreams Physical and Logical Bitstreams
 *
 * The raw data of an Ogg stream, as read directly from a file or network
 * socket, is called a <i>physical bitstream</i>.
 * 
<pre>
   The result of an Ogg encapsulation is called the "Physical (Ogg)
   Bitstream".  It encapsulates one or several encoder-created
   bitstreams, which are called "Logical Bitstreams".  A logical
   bitstream, provided to the Ogg encapsulation process, has a
   structure, i.e., it is split up into a sequence of so-called
   "Packets".  The packets are created by the encoder of that logical
   bitstream and represent meaningful entities for that encoder only
   (e.g., an uncompressed stream may use video frames as packets).
</pre>
 *
 * \subsection pages Packets and Pages
 *
 * Within the Ogg format, packets are written into \a pages. You can think
 * of pages like pages in a book, and packets as items of the actual text.
 * Consider, for example, individual poems or short stories as the packets.
 * Pages are of course all the same size, and a few very short packets could
 * be written into a single page. On the other hand, a very long packet will
 * use many pages.
 *
 * - liboggz handles the details of writing packets into pages, and of
 *   reading packets from pages; your application deals only with
 *   <tt>ogg_packet</tt> structures.
 * - Each <tt>ogg_packet</tt> structure contains a block of data and its
 *   length in bytes, plus other information related to the stream structure
 *   as explained below.
 *
 * \subsection serialno
 *
 * Each logical bitstream is uniquely identified by a serial number or
 * \a serialno.
 *
 * - Packets are always associated with a \a serialno. This is not actually
 *   part of the <tt>ogg_packet</tt> structure, so wherever you see an
 *   <tt>ogg_packet</tt> in the liboggz API, you will see an accompanying
 *   \a serialno.
 *
<pre>
   This unique serial number is created randomly and does not have any
   connection to the content or encoder of the logical bitstream it
   represents.
</pre>
 *
 * - Use oggz_serialno_new() to generate a new serial number for each
 *   logical bitstream you write.
 * - Use an \link oggz_table.h OggzTable \endlink, keyed by \a serialno,
 *   to store and retrieve data related to each logical bitstream.
 *
 * \subsection boseos b_o_s and e_o_s
<pre>
   bos page: The initial page (beginning of stream) of a logical
      bitstream which contains information to identify the codec type
      and other decoding-relevant information.

   eos page: The final page (end of stream) of a logical bitstream.
</pre>
 *
 * - Every <tt>ogg_packet</tt> contains \a b_o_s and \a e_o_s flags.
 *   Of course each of these will be set only once per logical bitstream.
 *   See the Structuring section below for rules on setting \a b_o_s and
 *   \a e_o_s when interleaving logical bitstreams.
 * - This documentation will refer to \a bos and \a eos <i>packets</i>
 *   (not <i>pages</i>) as that is more closely represented by the API.
 *   The \a bos packet is the only packet on the \a bos page, and the
 *   \a eos packet is the last packet on the \a eos page.
 *
 * \subsection granulepos
<pre>
   granule position: An increasing position number for a specific
      logical bitstream stored in the page header.  Its meaning is
      dependent on the codec for that logical bitstream
</pre>
 *
 * - Every <tt>ogg_packet</tt> contains a \a granulepos. The \a granulepos
 *   of each packet is used mostly for \link seek_api seeking. \endlink
 *
 * \section Structuring
 *
 * The general structure of an Ogg stream is governed by various rules.
 *
 * \subsection secondaries Secondary header packets
 *
 * Some data sources require initial setup information such as comments
 * and codebooks to be present near the beginning of the stream (directly
 * following the b_o_s packets.
 *
<pre>
   Ogg also allows but does not require secondary header packets after
   the bos page for logical bitstreams and these must also precede any
   data packets in any logical bitstream.  These subsequent header
   packets are framed into an integral number of pages, which will not
   contain any data packets.  So, a physical bitstream begins with the
   bos pages of all logical bitstreams containing one initial header
   packet per page, followed by the subsidiary header packets of all
   streams, followed by pages containing data packets.
</pre>
 *
 * - liboggz handles the framing of \a packets into low-level \a pages. To
 *   ensure that the pages used by secondary headers contain no data packets,
 *   set the \a flush parameter of oggz_write_feed() to \a OGGZ_FLUSH_AFTER
 *   when queueing the last of the secondary headers.
 * - or, equivalently, set \a flush to \a OGGZ_FLUSH_BEFORE when queueing
 *   the first of the data packets.
 *
 * \subsection boseosseq Sequencing b_o_s and e_o_s packets
 *
 * The following rules apply for sequencing \a bos and \a eos packets in
 * a physical bitstream:
<pre>
   ... All bos pages of all logical bitstreams MUST appear together at
   the beginning of the Ogg bitstream.

   ... eos pages for the logical bitstreams need not all occur
   contiguously.  Eos pages may be 'nil' pages, that is, pages
   containing no content but simply a page header with position
   information and the eos flag set in the page header.
</pre>
 *
 * - oggz_write_feed() will fail with a return value of
 *   \a OGGZ_ERR_BOS if an attempt is made to queue a late \a bos packet
 *
 * \subsection interleaving Interleaving logical bitstreams
<pre>
   It is possible to consecutively chain groups of concurrently
   multiplexed bitstreams.  The groups, when unchained, MUST stand on
   their own as a valid concurrently multiplexed bitstream.  The
   following diagram shows a schematic example of such a physical
   bitstream that obeys all the rules of both grouped and chained
   multiplexed bitstreams.
   
               physical bitstream with pages of 
          different logical bitstreams grouped and chained
      -------------------------------------------------------------
      |*A*|*B*|*C*|A|A|C|B|A|B|#A#|C|...|B|C|#B#|#C#|*D*|D|...|#D#|
      -------------------------------------------------------------
       bos bos bos             eos           eos eos bos       eos
   
   In this example, there are two chained physical bitstreams, the first
   of which is a grouped stream of three logical bitstreams A, B, and C.
   The second physical bitstream is chained after the end of the grouped
   bitstream, which ends after the last eos page of all its grouped
   logical bitstreams.  As can be seen, grouped bitstreams begin
   together - all of the bos pages MUST appear before any data pages.
   It can also be seen that pages of concurrently multiplexed bitstreams
   need not conform to a regular order.  And it can be seen that a
   grouped bitstream can end long before the other bitstreams in the
   group end.
</pre>
 *
 * - oggz_write_feed() will fail, returning an explicit error value, if
 *   an attempt is made to queue a packet in violation of these rules.
 *
 * \section References
 *
 * This introduction to the Ogg format is derived from
 * IETF <a href="http://www.ietf.org/rfc/rfc3533.txt">RFC 3533</a>
 * <i>The Ogg File Format version 0</i> in accordance with the
 * following copyright statement pertaining to the text of RFC 3533:
 *
<pre>
   Copyright (C) The Internet Society (2003).  All Rights Reserved.

   This document and translations of it may be copied and furnished to
   others, and derivative works that comment on or otherwise explain it
   or assist in its implementation may be prepared, copied, published
   and distributed, in whole or in part, without restriction of any
   kind, provided that the above copyright notice and this paragraph are
   included on all such copies and derivative works.  However, this
   document itself may not be modified in any way, such as by removing
   the copyright notice or references to the Internet Society or other
   Internet organizations, except as needed for the purpose of
   developing Internet standards in which case the procedures for
   copyrights defined in the Internet Standards process must be
   followed, or as required to translate it into languages other than
   English.

   The limited permissions granted above are perpetual and will not be
   revoked by the Internet Society or its successors or assigns.

   This document and the information contained herein is provided on an
   "AS IS" basis and THE INTERNET SOCIETY AND THE INTERNET ENGINEERING
   TASK FORCE DISCLAIMS ALL WARRANTIES, EXPRESS OR IMPLIED, INCLUDING
   BUT NOT LIMITED TO ANY WARRANTY THAT THE USE OF THE INFORMATION
   HEREIN WILL NOT INFRINGE ANY RIGHTS OR ANY IMPLIED WARRANTIES OF
   MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE.
</pre>
 *
 */

/** \defgroup configuration Configuration
 * \section ./configure ./configure
 *
 * It is possible to customize the functionality of liboggz
 * by using various ./configure flags when
 * building it from source. You can build a smaller
 * version of liboggz to only read or write.
 * By default, both reading and writing support is built.
 *
 * For general information about using ./configure, see the file
 * \link install INSTALL \endlink
 *
 * \subsection no_encode Removing writing support
 *
 * Configuring with \a --disable-write will remove all support for writing:
 * - All internal write related functions will not be built
 * - Any attempt to call oggz_new(), oggz_open() or oggz_open_stdio()
 *   with \a flags == OGGZ_WRITE will fail, returning NULL
 * - Any attempt to call oggz_write(), oggz_write_output(), oggz_write_feed(),
 *   oggz_write_set_hungry_callback(), or oggz_write_get_next_page_size()
 *   will return OGGZ_ERR_DISABLED
 *
 * \subsection no_decode Removing reading support
 *
 * Configuring with \a --disable-read will remove all support for reading:
 * - All internal reading related functions will not be built
 * - Any attempt to call oggz_new(), oggz_open() or oggz_open_stdio()
 *    with \a flags == OGGZ_READ will fail, returning NULL
 * - Any attempt to call oggz_read(), oggz_read_input(),
 *   oggz_set_read_callback(), oggz_seek(), or oggz_seek_units() will return 
 *   OGGZ_ERR_DISABLED
 *
 */

/** \defgroup install Installation
 * \section install INSTALL
 *
 * \include INSTALL
 */

/** \defgroup building Building against liboggz
 *
 *
 * \section autoconf Using GNU autoconf
 *
 * If you are using GNU autoconf, you do not need to call pkg-config
 * directly. Use the following macro to determine if liboggz is
 * available:
 *
 <pre>
 PKG_CHECK_MODULES(OGGZ, oggz >= 0.6.0,
                   HAVE_OGGZ="yes", HAVE_OGGZ="no")
 if test "x$HAVE_OGGZ" = "xyes" ; then
   AC_SUBST(OGGZ_CFLAGS)
   AC_SUBST(OGGZ_LIBS)
 fi
 </pre>
 *
 * If liboggz is found, HAVE_OGGZ will be set to "yes", and
 * the autoconf variables OGGZ_CFLAGS and OGGZ_LIBS will
 * be set appropriately.
 *
 * \section pkg-config Determining compiler options with pkg-config
 *
 * If you are not using GNU autoconf in your project, you can use the
 * pkg-config tool directly to determine the correct compiler options.
 *
 <pre>
 OGGZ_CFLAGS=`pkg-config --cflags oggz`

 OGGZ_LIBS=`pkg-config --libs oggz`
 </pre>
 *
 */

/** \file
 * The liboggz C API.
 * 
 * \section general Generic semantics
 *
 * All access is managed via an OGGZ handle. This can be instantiated
 * in one of three ways:
 *
 * - oggz_open() - Open a full pathname
 * - oggz_open_stdio() - Use an already opened FILE *
 * - oggz_new() - Create an anonymous OGGZ object, which you can later
 *   handle via memory buffers
 *
 * To finish using an OGGZ handle, it should be closed with oggz_close().
 *
 * \section reading Reading Ogg data
 *
 * To read from Ogg files or streams you must instantiate an OGGZ handle
 * with flags set to OGGZ_READ, and provide an OggzReadPacket
 * callback with oggz_set_read_callback().
 * See the \ref read_api section for details.
 *
 * \section writing Writing Ogg data
 *
 * To write to Ogg files or streams you must instantiate an OGGZ handle
 * with flags set to OGGZ_WRITE, and provide an OggzWritePacket
 * callback with oggz_set_write_callback().
 * See the \ref write_api section for details.
 *
 * \section seeking Seeking on Ogg data
 *
 * To seek while reading Ogg files or streams you must instantiate an OGGZ
 * handle for reading, and ensure that an \link metric OggzMetric \endlink
 * function is defined to translate packet positions into units such as time.
 * See the \ref seek_api section for details.
 *
 * \section io Overriding the IO methods
 *
 * When an OGGZ handle is instantiated by oggz_open() or oggz_open_stdio(),
 * Oggz uses stdio functions internally to access the raw data. However for
 * some applications, the raw data cannot be accessed via stdio -- this
 * commonly occurs when integrating with media frameworks. For such
 * applications, you can provide Oggz with custom IO methods that it should
 * use to access the raw data. Oggz will then use these custom methods,
 * rather than using stdio methods, to access the raw data internally.
 *
 * For details, see \link oggz_io.h <oggz/oggz_io.h> \endlink.
 *
 * \section headers Headers
 *
 * oggz.h provides direct access to libogg types such as ogg_packet, defined
 * in <ogg/ogg.h>.
 */

/**
 * An opaque handle to an Ogg file. This is returned by oggz_open() or
 * oggz_new(), and is passed to all other oggz_* functions.
 */
typedef void OGGZ;

/**
 * Create a new OGGZ object
 * \param flags OGGZ_READ or OGGZ_WRITE
 * \returns A new OGGZ object
 * \retval NULL on system error; check errno for details
 */
OGGZ * oggz_new (int flags);

/**
 * Open an Ogg file, creating an OGGZ handle for it
 * \param filename The file to open
 * \param flags OGGZ_READ or OGGZ_WRITE
 * \return A new OGGZ handle
 * \retval NULL System error; check errno for details
 */
OGGZ * oggz_open (char * filename, int flags);

/**
 * Create an OGGZ handle associated with a stdio stream
 * \param file An open FILE handle
 * \param flags OGGZ_READ or OGGZ_WRITE
 * \returns A new OGGZ handle
 * \retval NULL System error; check errno for details
 */
OGGZ * oggz_open_stdio (FILE * file, int flags);

/**
 * Ensure any associated io streams are flushed.
 * \param oggz An OGGZ handle
 * \retval 0 Success
 * \retval OGGZ_ERR_BAD_OGGZ \a oggz does not refer to an existing OGGZ
 * \retval OGGZ_ERR_INVALID Operation not suitable for this OGGZ
 * \retval OGGZ_ERR_SYSTEM System error; check errno for details
 */
int oggz_flush (OGGZ * oggz);

/**
 * Close an OGGZ handle
 * \param oggz An OGGZ handle
 * \retval 0 Success
 * \retval OGGZ_ERR_BAD_OGGZ \a oggz does not refer to an existing OGGZ
 * \retval OGGZ_ERR_SYSTEM System error; check errno for details
 */
int oggz_close (OGGZ * oggz);

/**
 * Determine if a given logical bitstream is at bos (beginning of stream).
 * \param oggz An OGGZ handle
 * \param serialno Identify a logical bitstream within \a oggz, or -1 to
 * query if all logical bitstreams in \a oggz are at bos
 * \retval 1 The given stream is at bos
 * \retval 0 The given stream is not at bos
 * \retval OGGZ_ERR_BAD_SERIALNO \a serialno does not identify an existing
 * logical bitstream in \a oggz.
 */
int oggz_get_bos (OGGZ * oggz, long serialno);

/**
 * Determine if a given logical bitstream is at eos (end of stream).
 * \param oggz An OGGZ handle
 * \param serialno Identify a logical bitstream within \a oggz, or -1 to
 * query if all logical bitstreams in \a oggz are at eos
 * \retval 1 The given stream is at eos
 * \retval 0 The given stream is not at eos
 * \retval OGGZ_ERR_BAD_SERIALNO \a serialno does not identify an existing
 * logical bitstream in \a oggz.
 */
int oggz_get_eos (OGGZ * oggz, long serialno);

/** \defgroup read_api OGGZ Read API
 *
 * OGGZ parses Ogg bitstreams, forming ogg_packet structures, and calling
 * your OggzReadPacket callback(s).
 *
 * You provide Ogg data to OGGZ with oggz_read() or oggz_read_input(), and
 * independently process it in OggzReadPacket callbacks.
 * It is possible to set a different callback per \a serialno (ie. for each
 * logical bitstream in the Ogg bitstream - see the \ref basics section for
 * more detail).
 *
 * See \ref seek_api for information on seeking on interleaved Ogg data.
 * 
 * \{
 */

/**
 * This is the signature of a callback which you must provide for Oggz
 * to call whenever it finds a new packet in the Ogg stream associated
 * with \a oggz.
 *
 * \param oggz The OGGZ handle
 * \param op The full ogg_packet (see <ogg/ogg.h>)
 * \param serialno Identify the logical bistream in \a oggz that contains
 *                 \a op
 * \param user_data A generic pointer you have provided earlier
 * \returns 0 to continue, non-zero to instruct OGGZ to stop.
 *
 * \note It is possible to provide different callbacks per logical
 * bitstream -- see oggz_set_read_callback() for more information.
 */
typedef int (*OggzReadPacket) (OGGZ * oggz, ogg_packet * op, long serialno,
			       void * user_data);

/**
 * Set a callback for Oggz to call when a new Ogg packet is found in the
 * stream.
 *
 * \param oggz An OGGZ handle previously opened for reading
 * \param serialno Identify the logical bitstream in \a oggz to attach
 * this callback to, or -1 to attach this callback to all unattached
 * logical bitstreams in \a oggz.
 * \param read_packet Your callback function
 * \param user_data Arbitrary data you wish to pass to your callback
 * \retval 0 Success
 * \retval OGGZ_ERR_BAD_SERIALNO \a serialno does not identify an existing
 * logical bitstream in \a oggz.
 * \retval OGGZ_ERR_BAD_OGGZ \a oggz does not refer to an existing OGGZ
 * \retval OGGZ_ERR_INVALID Operation not suitable for this OGGZ
 *
 * \note Values of \a serialno other than -1 allows you to specify different
 * callback functions for each logical bitstream.
 *
 * \note It is safe to call this callback from within an OggzReadPacket
 * function, in order to specify that subsequent packets should be handled
 * by a different OggzReadPacket function.
 */
int oggz_set_read_callback (OGGZ * oggz, long serialno,
			    OggzReadPacket read_packet, void * user_data);

/**
 * Read n bytes into \a oggz, calling any read callbacks on the fly.
 * \param oggz An OGGZ handle previously opened for reading
 * \param n A count of bytes to ingest
 * \retval ">  0" The number of bytes successfully ingested.
 * \retval 0 End of file
 * \retval OGGZ_ERR_BAD_OGGZ \a oggz does not refer to an existing OGGZ
 * \retval OGGZ_ERR_INVALID Operation not suitable for this OGGZ
 * \retval OGGZ_ERR_SYSTEM System error; check errno for details
 */
long oggz_read (OGGZ * oggz, long n);

/**
 * Input data into \a oggz.
 * \param oggz An OGGZ handle previously opened for reading
 * \param buf A memory buffer
 * \param n A count of bytes to input
 * \retval ">  0" The number of bytes successfully ingested.
 * \retval OGGZ_ERR_BAD_OGGZ \a oggz does not refer to an existing OGGZ
 * \retval OGGZ_ERR_INVALID Operation not suitable for this OGGZ
 */
long oggz_read_input (OGGZ * oggz, unsigned char * buf, long n);

/** \}
 */

/** \defgroup force_feed Writing by force feeding OGGZ
 *
 * Force feeding involves synchronously:
 * - Creating an \a ogg_packet structure
 * - Adding it to the packet queue with oggz_write_feed()
 * - Calling oggz_write() or oggz_write_output(), repeatedly as necessary,
 *   to generate the Ogg bitstream.
 *
 * This process is illustrated in the following diagram:
 *
 * \image html forcefeed.png
 * \image latex forcefeed.eps "Force Feeding OGGZ" width=10cm
 *
 * The following example code generates a stream of ten packets, each
 * containing a single byte ('A', 'B', ... , 'J'):
 *
 * \include write-feed.c
 */

/** \defgroup hungry Writing with OggzHungry callbacks
 *
 * You can add packets to the OGGZ packet queue only when it is "hungry"
 * by providing an OggzHungry callback.
 *
 * An OggzHungry callback will:
 * - Create an \a ogg_packet structure
 * - Add it to the packet queue with oggz_write_feed()
 *
 * Once you have set such a callback with oggz_write_set_hungry_callback(),
 * simply call oggz_write() or oggz_write_output() repeatedly, and OGGZ
 * will call your callback to provide packets when it is hungry.
 *
 * This process is illustrated in the following diagram:
 *
 * \image html hungry.png
 * \image latex hungry.eps "Using OggzHungry" width=10cm
 *
 * The following example code generates a stream of ten packets, each
 * containing a single byte ('A', 'B', ... , 'J'):
 *
 * \include write-hungry.c
 */

/** \defgroup write_api OGGZ Write API
 *
 * OGGZ maintains a packet queue, such that you can independently add
 * packets to the queue and write an Ogg bitstream.
 * There are two complementary methods for adding packets to the
 * packet queue.
 *
 * - by \link force_feed force feeding OGGZ \endlink
 * - by using \link hungry OggzHungry \endlink callbacks
 *
 * As each packet is enqueued, its validity is checked against the framing
 * constraints outlined in the \link basics Ogg basics \endlink section.
 * If it does not pass these constraints, oggz_write_feed() will fail with
 * an appropriate error code.
 *
 * \note
 * - When writing, you can ensure that a packet starts on a new page
 *   by setting the \a flush parameter of oggz_write_feed() to
 *   \a OGGZ_FLUSH_BEFORE when enqueuing it.
 *   Similarly you can ensure that the last page a packet is written into
 *   won't contain any following packets by setting the \a flush parameter
 *   of oggz_write_feed() to \a OGGZ_FLUSH_AFTER.
 * - The \a OGGZ_FLUSH_BEFORE and \a OGGZ_FLUSH_AFTER flags can be bitwise
 *   OR'd together to ensure that the packet will not share any pages with
 *   any other packets, either before or after.
 *
 * \{
 */

/**
 * This is the signature of a callback which Oggz will call when \a oggz
 * is \link hungry hungry \endlink.
 *
 * \param oggz The OGGZ handle
 * \param empty A value of 1 indicates that the packet queue is currently
 *        empty. A value of 0 indicates that the packet queue is not empty.
 * \param user_data A generic pointer you have provided earlier
 * \retval 0 Continue
 * \retval non-zero Instruct OGGZ to stop.
 */
typedef int (*OggzWriteHungry) (OGGZ * oggz, int empty, void * user_data);

/**
 * Set a callback for Oggz to call when \a oggz
 * is \link hungry hungry \endlink.
 *
 * \param oggz An OGGZ handle previously opened for writing
 * \param hungry Your callback function
 * \param only_when_empty When to call: a value of 0 indicates that
 * OGGZ should call \a hungry() after each and every packet is written;
 * a value of 1 indicates that OGGZ should call \a hungry() only when
 * its packet queue is empty
 * \param user_data Arbitrary data you wish to pass to your callback
 * \retval 0 Success
 * \retval OGGZ_ERR_BAD_OGGZ \a oggz does not refer to an existing OGGZ
 * \retval OGGZ_ERR_INVALID Operation not suitable for this OGGZ
 * \note Passing a value of 0 for \a only_when_empty allows you to feed
 * new packets into \a oggz's packet queue on the fly.
 */
int oggz_write_set_hungry_callback (OGGZ * oggz,
				    OggzWriteHungry hungry,
				    int only_when_empty,
				    void * user_data);
/**
 * Add a packet to \a oggz's packet queue.
 * \param oggz An OGGZ handle previously opened for writing
 * \param op An ogg_packet with all fields filled in
 * \param serialno Identify the logical bitstream in \a oggz to add the
 * packet to
 * \param flush Bitmask of OGGZ_FLUSH_BEFORE, OGGZ_FLUSH_AFTER
 * \param guard A guard for nocopy, NULL otherwise
 * \retval 0 Success
 * \retval OGGZ_ERR_BAD_GUARD \a guard specified has non-zero initialization
 * \retval OGGZ_ERR_BOS Packet would be bos packet of a new logical bitstream,
 *         but oggz has already written one or more non-bos packets in
 *         other logical bitstreams,
 *         and \a oggz is not flagged OGGZ_NONSTRICT
 * \retval OGGZ_ERR_EOS The logical bitstream identified by \a serialno is
 *         already at eos,
 *         and \a oggz is not flagged OGGZ_NONSTRICT
 * \retval OGGZ_ERR_BAD_BYTES \a op->bytes is invalid,
 *         and \a oggz is not flagged OGGZ_NONSTRICT
 * \retval OGGZ_ERR_BAD_B_O_S \a op->b_o_s is invalid,
 *         and \a oggz is not flagged OGGZ_NONSTRICT
 * \retval OGGZ_ERR_BAD_GRANULEPOS \a op->granulepos is less than that of
 *         an earlier packet within this logical bitstream,
 *         and \a oggz is not flagged OGGZ_NONSTRICT
 * \retval OGGZ_ERR_BAD_PACKETNO \a op->packetno is less than that of an
 *         earlier packet within this logical bitstream,
 *         and \a oggz is not flagged OGGZ_NONSTRICT
 * \retval OGGZ_ERR_BAD_SERIALNO \a serialno does not identify an existing
 *         logical bitstream in \a oggz,
 *         and \a oggz is not flagged OGGZ_NONSTRICT
 * \retval OGGZ_ERR_BAD_OGGZ \a oggz does not refer to an existing OGGZ
 * \retval OGGZ_ERR_INVALID Operation not suitable for this OGGZ
 *
 * \note If \a op->b_o_s is initialized to \a -1 before calling
 *       oggz_write_feed(), Oggz will fill it in with the appropriate
 *       value; ie. 1 for the first packet of a new stream, and 0 otherwise.
 */
int oggz_write_feed (OGGZ * oggz, ogg_packet * op, long serialno, int flush,
		     int * guard);

/**
 * Output data from an OGGZ handle. Oggz will call your write callback
 * as needed.
 *
 * \param oggz An OGGZ handle previously opened for writing
 * \param buf A memory buffer
 * \param n A count of bytes to output
 * \retval "> 0" The number of bytes successfully output
 * \retval 0 End of stream
 * \retval OGGZ_ERR_RECURSIVE_WRITE Attempt to initiate writing from
 * within an OggzHungry callback
 * \retval OGGZ_ERR_BAD_OGGZ \a oggz does not refer to an existing OGGZ
 * \retval OGGZ_ERR_INVALID Operation not suitable for this OGGZ
 */
long oggz_write_output (OGGZ * oggz, unsigned char * buf, long n);

/**
 * Write n bytes from an OGGZ handle. Oggz will call your write callback
 * as needed.
 *
 * \param oggz An OGGZ handle previously opened for writing
 * \param n A count of bytes to be written
 * \retval "> 0" The number of bytes successfully output
 * \retval 0 End of stream
 * \retval OGGZ_ERR_RECURSIVE_WRITE Attempt to initiate writing from
 * within an OggzHungry callback
 * \retval OGGZ_ERR_BAD_OGGZ \a oggz does not refer to an existing OGGZ
 * \retval OGGZ_ERR_INVALID Operation not suitable for this OGGZ
 */
long oggz_write (OGGZ * oggz, long n);

/**
 * Query the number of bytes in the next page to be written.
 *
 * \param oggz An OGGZ handle previously opened for writing
 * \retval ">= 0" The number of bytes in the next page
 * \retval OGGZ_ERR_BAD_OGGZ \a oggz does not refer to an existing OGGZ
 * \retval OGGZ_ERR_INVALID Operation not suitable for this OGGZ
 */
long oggz_write_get_next_page_size (OGGZ * oggz);

/** \}
 */

/** \defgroup auto OGGZ_AUTO
 *
 * \section auto Automatic Metrics
 *
 * Oggz can automatically provide \link metric metrics \endlink for the
 * common media codecs
 * <a href="http://www.speex.org/">Speex</a>,
 * <a href="http://www.vorbis.com/">Vorbis</a>,
 * and <a href="http://www.theora.org/">Theora</a>,
 * as well as all <a href="http://www.annodex.net/">Annodex</a> streams.
 * You need do nothing more than open the file with the OGGZ_AUTO flag set.
 *
 * - Create an OGGZ handle for reading with \a flags = OGGZ_READ | OGGZ_AUTO
 * - Read data, ensuring that you have received all b_o_s pages before
 *   attempting to seek.
 *
 * Oggz will silently parse known codec headers and associate metrics
 * appropriately; if you attempt to seek before you have received all
 * b_o_s pages, Oggz will not have had a chance to parse the codec headers
 * and associate metrics.
 * It is safe to seek once you have received a packet with \a b_o_s == 0;
 * see the \link basics Ogg basics \endlink section for more details.
 *
 * \note This functionality is provided for convenience. Oggz parses
 * these codec headers internally, and so liboggz is \b not linked to
 * libspeex, libvorbis, libtheora or libannodex.
 */

/** \defgroup metric Using OggzMetric
 *
 * \section metric_intro Introduction
 *
 * An OggzMetric is a helper function for Oggz's seeking mechanism.
 *
 * If every position in an Ogg stream can be described by a metric such as
 * time, then it is possible to define a function that, given a serialno and
 * granulepos, returns a measurement in units such as milliseconds. Oggz
 * will use this function repeatedly while seeking in order to navigate
 * through the Ogg stream.
 *
 * The meaning of the units is arbitrary, but must be consistent across all
 * logical bitstreams. This allows Oggz to seek accurately through Ogg
 * bitstreams containing multiple logical bitstreams such as tracks of media.
 *
 * \section setting How to set metrics
 *
 * You don't need to set metrics for Vorbis, Speex, Theora or Annodex.
 * These can be handled \link auto automatically \endlink by Oggz.
 *
 * For most others it is simply a matter of providing a linear multiplication
 * factor (such as a sampling rate, if each packet's granulepos represents a
 * sample number). For non-linear data streams, you will need to explicitly
 * provide your own OggzMetric function.
 *
 * \subsection linear Linear Metrics
 *
 * - Set the \a granule_rate_numerator and \a granule_rate_denominator
 *   appropriately using oggz_set_metric_linear()
 *
 * \subsection custom Custom Metrics
 *
 * For streams with non-linear granulepos, you need to set a custom metric:
 *
 * - Implement an OggzMetric callback
 * - Set the OggzMetric callback using oggz_set_metric()
 *
 * \section using Seeking with OggzMetrics
 *
 * To seek, use oggz_seek_units(). Oggz will perform a ratio search
 * through the Ogg bitstream, using the OggzMetric callback to determine
 * its position relative to the desired unit.
 *
 * \note
 *
 * Many data streams begin with headers describing such things as codec
 * setup parameters. One of the assumptions Monty makes is:
 *
 * - Given pre-cached decode headers, a player may seek into a stream at
 *   any point and begin decode.
 *
 * Thus, the first action taken by applications dealing with such data is
 * to read in and cache the decode headers; thereafter the application can
 * safely seek to arbitrary points in the data.
 *
 * This impacts seeking because the portion of the bitstream containing
 * decode headers should not be considered part of the metric space. To
 * inform Oggz not to seek earlier than the end of the decode headers,
 * use oggz_set_data_start().
 *
 */

/** \defgroup seek_semantics Semantics of seeking in Ogg bitstreams
 *
 * \section seek_semantics_intro Introduction
 *
 * The seeking semantics of the Ogg file format were outlined by Monty in
 * <a href="http://www.xiph.org/archives/theora-dev/200209/0040.html">a
 * post to theora-dev</a> in September 2002. Quoting from that post, we
 * have the following assumptions:
 *
 * - Ogg is not a non-linear format. ... It is a media transport format
 *   designed to do nothing more than deliver content, in a stream, and
 *   have all the pieces arrive on time and in sync.
 * - The Ogg layer does not know the specifics of the codec data it's
 *   multiplexing into a stream. It knows nothing beyond 'Oooo, packets!',
 *   that the packets belong to different buckets, that the packets go in
 *   order, and that packets have position markers. Ogg does not even have
 *   a concept of 'time'; it only knows about the sequentially increasing,
 *   unitless position markers. It is up to higher layers which have
 *   access to the codec APIs to assign and convert units of framing or
 *   time.
 *
 * (For more details on the structure of Ogg streams, see the
 * \link basics Ogg Basics \endlink section).
 *
 * For data such as media, for which it is possible to provide a mapping
 * such as 'time', OGGZ can efficiently navigate through an Ogg stream
 * by use of an OggzMetric callback, thus allowing automatic seeking to
 * points in 'time'.
 *
 * For common codecs you can ask Oggz to set this for you automatically by
 * instantiating the OGGZ handle with the OGGZ_AUTO flag set. For others
 * you can specify a multiplier with oggz_set_metric_linear(), or a generic
 * non-linear metric with oggz_set_metric().
 *
 */

/** \defgroup seek_api OGGZ Seek API
 *
 * Oggz can seek on multitrack, multicodec bitstreams.
 *
 * - If you are expecting to only handle
 *   <a href="http://www.annodex.net/">Annodex</a> bitstreams,
 *   or any combination of <a href="http://www.vorbis.com/">Vorbis</a>,
 *   <a href="http://www.speex.org/">Speex</a> and
 *   <a href="http://www.theora.org/">Theora</a> logical bitstreams, seeking
 *   is built-in to Oggz. See \link auto OGGZ_AUTO \endlink for details.
 *
 * - For other data streams, you will need to provide a metric function;
 *   see the section on \link metric Using OggzMetrics \endlink for details
 *   of setting up and seeking with metrics.
 *
 * - It is always possible to seek to exact byte positions using oggz_seek().
 *
 * - A mechanism to aid seeking across non-metric spaces for which a partial
 *   order exists (ie. data that is not synchronised by a measure such as time,
 *   but is nevertheless somehow seekably structured), is also planned.
 *
 * For a full description of the seeking methods possible in Ogg, see
 * \link seek_semantics Semantics of seeking in Ogg bitstreams \endlink.
 *
 * \{
 */

/**
 * Specify that a logical bitstream has a linear metric
 * \param oggz An OGGZ handle
 * \param serialno Identify the logical bitstream in \a oggz to attach
 * this linear metric to. A value of -1 indicates that the metric should
 * be attached to all unattached logical bitstreams in \a oggz.
 * \param granule_rate_numerator The numerator of the granule rate
 * \param granule_rate_denominator The denominator of the granule rate
 * \returns 0 Success
 * \retval OGGZ_ERR_BAD_SERIALNO \a serialno does not identify an existing
 * logical bitstream in \a oggz.
 * \retval OGGZ_ERR_BAD_OGGZ \a oggz does not refer to an existing OGGZ
 */
int oggz_set_metric_linear (OGGZ * oggz, long serialno,
			    ogg_int64_t granule_rate_numerator,
			    ogg_int64_t granule_rate_denominator);

/**
 * This is the signature of a function to correlate Ogg streams.
 * If every position in an Ogg stream can be described by a metric (eg. time)
 * then define this function that returns some arbitrary unit value.
 * This is the normal use of OGGZ for media streams. The meaning of units is
 * arbitrary, but must be consistent across all logical bitstreams; for
 * example a conversion of the time offset of a given packet into nanoseconds
 * or a similar stream-specific subdivision may be appropriate.
 *
 * \param oggz An OGGZ handle
 * \param serialno Identifies a logical bitstream within \a oggz
 * \param granulepos A granulepos within the logical bitstream identified
 *                   by \a serialno
 * \param user_data Arbitrary data you wish to pass to your callback
 * \returns A conversion of the (serialno, granulepos) pair into a measure
 * in units which is consistent across all logical bitstreams within \a oggz
 */
typedef ogg_int64_t (*OggzMetric) (OGGZ * oggz, long serialno,
				   ogg_int64_t granulepos, void * user_data);

/**
 * Set the OggzMetric to use for an OGGZ handle
 *
 * \param oggz An OGGZ handle
 * \param serialno Identify the logical bitstream in \a oggz to attach
 *                 this metric to. A value of -1 indicates that this metric
 *                 should be attached to all unattached logical bitstreams
 *                 in \a oggz.
 * \param metric An OggzMetric callback
 * \param user_data arbitrary data to pass to the metric callback
 *
 * \returns 0 Success
 * \retval OGGZ_ERR_BAD_SERIALNO \a serialno does not identify an existing
 *                               logical bitstream in \a oggz, and is not -1
 * \retval OGGZ_ERR_BAD_OGGZ \a oggz does not refer to an existing OGGZ
 *
 * \note Specifying values of \a serialno other than -1 allows you to pass
 *       logical bitstream specific user_data to the same metric.
 * \note Alternatively, you may use a different \a metric for each
 *       \a serialno, but all metrics used must return mutually consistent
 *       unit measurements.
 */
int oggz_set_metric (OGGZ * oggz, long serialno, OggzMetric metric,
		     void * user_data);

/**
 * Query the current offset in units corresponding to the Metric function
 * \param oggz An OGGZ handle
 * \returns the offset in units
 * \retval OGGZ_ERR_BAD_OGGZ \a oggz does not refer to an existing OGGZ
 * \retval OGGZ_ERR_INVALID Operation not suitable for this OGGZ
 */
ogg_int64_t oggz_tell_units (OGGZ * oggz);

/**
 * Seek to a number of units corresponding to the Metric function
 * \param oggz An OGGZ handle
 * \param units A number of units
 * \param whence As defined in <stdio.h>: SEEK_SET, SEEK_CUR or SEEK_END
 * \returns the new file offset, or -1 on failure.
 */
ogg_int64_t oggz_seek_units (OGGZ * oggz, ogg_int64_t units, int whence);

#ifdef _UNIMPLEMENTED
/** \defgroup order OggzOrder
 *
 * \subsection OggzOrder
 *
 * Suppose there is a partial order < and a corresponding equivalence
 * relation = defined on the space of packets in the Ogg stream of 'OGGZ'.
 * An OggzOrder simply provides a comparison in terms of '<' and '=' for
 * ogg_packets against a target.
 *
 * To use OggzOrder:
 *
 * - Implement an OggzOrder callback
 * - Set the OggzOrder callback for an OGGZ handle with oggz_set_order()
 * - To seek, use oggz_seek_byorder(). Oggz will use a combination bisection
 *   search and scan of the Ogg bitstream, using the OggzOrder callback to
 *   match against the desired 'target'.
 *
 * Otherwise, for more general ogg streams for which a partial order can be
 * defined, define a function matching this specification.
 *
 * Parameters:
 *
 *     OGGZ: the OGGZ object
 *     op:  an ogg packet in the stream
 *     target: a user defined object
 *
 * Return values:
 *
 *    -1 , if 'op' would occur before the position represented by 'target'
 *     0 , if the position of 'op' is equivalent to that of 'target'
 *     1 , if 'op' would occur after the position represented by 'target'
 *     2 , if the relationship between 'op' and 'target' is undefined.
 *
 * Symbolically:
 *
 * Suppose there is a partial order < and a corresponding equivalence
 * relation = defined on the space of packets in the Ogg stream of 'OGGZ'.
 * Let p represent the position of the packet 'op', and t be the position
 * represented by 'target'.
 *
 * Then a function implementing OggzPacketOrder should return as follows:
 *
 *    -1 , p < t
 *     0 , p = t
 *     1 , t < p
 *     2 , otherwise
 *
 * Hacker's hint: if there are no circumstances in which you would return
 * a value of 2, there is a linear order; it may be possible to define a
 * Metric rather than an Order.
 *
 */
typedef int (*OggzOrder) (OGGZ * oggz, ogg_packet * op, void * target,
			 void * user_data);
/**
 * \retval 0 Success
 * \retval OGGZ_ERR_BAD_OGGZ \a oggz does not refer to an existing OGGZ
 * \retval OGGZ_ERR_INVALID Operation not suitable for this OGGZ
 */
int oggz_set_order (OGGZ * oggz, long serialno, OggzOrder order,
		    void * user_data);

long oggz_seek_byorder (OGGZ * oggz, void * target);

#endif /* _UNIMPLEMENTED */

/**
 * Erase any input buffered in OGGZ. This discards any input read from the
 * underlying IO system but not yet delivered as ogg_packets.
 *
 * \param oggz An OGGZ handle
 * \retval 0 Success
 * \retval OGGZ_ERR_SYSTEM Error seeking on underlying IO.
 * \retval OGGZ_ERR_BAD_OGGZ \a oggz does not refer to an existing OGGZ
 * \retval OGGZ_ERR_INVALID Operation not suitable for this OGGZ
 */
int oggz_purge (OGGZ * oggz);

/**
 * Tell OGGZ to remember the given offset as the start of data.
 * This informs the seeking mechanism that when seeking back to unit 0,
 * go to the given offset, not to the start of the file, which is usually
 * codec headers.
 * The usual usage is:
<pre>
    oggz_set_data_start (oggz, oggz_tell (oggz));
</pre>
 * \param oggz An OGGZ handle previously opened for reading
 * \param offset The offset of the start of data
 * \returns 0 on success, -1 on failure.
 */
int oggz_set_data_start (OGGZ * oggz, off_t offset);

/**
 * Provide the file offset in bytes corresponding to the data read.
 * \param oggz An OGGZ handle
 * \returns The current offset of oggz.
 *
 * \note When reading, the value returned by oggz_tell() reflects the
 * data offset of the start of the most recent packet processed, so that
 * when called from an OggzReadPacket callback it reflects the byte
 * offset of the start of the packet. As OGGZ may have internally read
 * ahead, this may differ from the current offset of the associated file
 * descriptor.
 */
off_t oggz_tell (OGGZ * oggz);

/**
 * Seek to a specific byte offset
 * \param oggz An OGGZ handle
 * \param offset a byte offset
 * \param whence As defined in <stdio.h>: SEEK_SET, SEEK_CUR or SEEK_END
 * \returns the new file offset, or -1 on failure.
 */
off_t oggz_seek (OGGZ * oggz, off_t offset, int whence);

#ifdef _UNIMPLEMENTED
long oggz_seek_packets (OGGZ * oggz, long serialno, long packets, int whence);
#endif

/** \}
 */

/**
 * Request a new serialno, as required for a new stream, ensuring the serialno
 * is not yet used for any other streams managed by this OGGZ.
 * \param oggz An OGGZ handle
 * \returns A new serialno, not already occuring in any logical bitstreams
 * in \a oggz.
 */
long oggz_serialno_new (OGGZ * oggz);

#include <oggz/oggz_io.h>

#endif /* __OGGZ_H__ */
