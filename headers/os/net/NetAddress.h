//------------------------------------------------------------------------------
//	Copyright (c) 2001-2002, OpenBeOS
//
//	Permission is hereby granted, free of charge, to any person obtaining a
//	copy of this software and associated documentation files (the "Software"),
//	to deal in the Software without restriction, including without limitation
//	the rights to use, copy, modify, merge, publish, distribute, sublicense,
//	and/or sell copies of the Software, and to permit persons to whom the
//	Software is furnished to do so, subject to the following conditions:
//
//	The above copyright notice and this permission notice shall be included in
//	all copies or substantial portions of the Software.
//
//	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
//	FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
//	DEALINGS IN THE SOFTWARE.
//------------------------------------------------------------------------------

#ifndef _NETADDRESS_H
#define _NETADDRESS_H

#include <BeBuild.h>
#include <SupportDefs.h>
#include <Archivable.h>

#include <sys/socket.h>
#include <netinet/in.h>

/*
 * BNetAddress
 *
 * This class is used to represent network addresses, and provide access
 * to a network address in a variety of formats. BNetAddress provides various ways
 * to get and set a network address, converting to or from the chosen representation
 * into a generic internal one.
 */

class NLAddress;

class BNetAddress : public BArchivable
{
public:

	BNetAddress(BMessage *archive);
	virtual ~BNetAddress();
	
	virtual	status_t Archive(BMessage *into, bool deep = true) const;
	static BArchivable *Instantiate(BMessage *archive);
	 
	BNetAddress(const char *hostname = 0, unsigned short port = 0);
	BNetAddress(const struct sockaddr_in &sa);
	BNetAddress(in_addr addr, int port = 0);
	BNetAddress(uint32 addr, int port = 0 );
	BNetAddress(const BNetAddress &);
	BNetAddress(const char *hostname, const char *protocol, const char *service);
	
	BNetAddress &operator=(const BNetAddress &);

	/*
	 * It is important to call InitCheck() after creating an instance of this class.
	 */	
	status_t InitCheck();
	
	/*
	 * BNetAddress::set() sets the internal address representation to refer
	 * to the internet address specified by the passed-in data.
	 * Returns true if successful, false otherwise.
	 */
	status_t SetTo(const char *hostname, const char *protocol, const char *service);
	status_t SetTo(const char *hostname = 0, unsigned short port = 0);
	status_t SetTo(const struct sockaddr_in &sa);
	status_t SetTo(in_addr addr, int port = 0);
	status_t SetTo(uint32 addr=INADDR_ANY, int port = 0);

	/*
	 * BNetAddress::get() converts the internal internet address representation
	 * into the specified form and returns it by filling in the passed-in parameters.
	 * Returns true if successful, false otherwise.
	 */
	status_t GetAddr(char *hostname = 0, unsigned short *port = 0) const;
	status_t GetAddr(struct sockaddr_in &sa) const;
	status_t GetAddr(in_addr &addr, unsigned short *port = 0) const;

	inline NLAddress *GetImpl() const;

protected:
	status_t m_init;
	
private:
	virtual	void		_ReservedBNetAddressFBCCruft1();
	virtual	void		_ReservedBNetAddressFBCCruft2();
	virtual	void		_ReservedBNetAddressFBCCruft3();
	virtual	void		_ReservedBNetAddressFBCCruft4();
	virtual	void		_ReservedBNetAddressFBCCruft5();
	virtual	void		_ReservedBNetAddressFBCCruft6();

	int32				__ReservedBNetAddressFBCCruftData[9];
};




inline NLAddress *BNetAddress::GetImpl() const
{
	return NULL;	// No Nettle backward compatibility
}


#endif	// _NETADDRESS_H