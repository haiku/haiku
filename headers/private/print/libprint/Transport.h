/*
 * Transport.h
 * Copyright 1999-2000 Y.Takagi. All Rights Reserved.
 */

#ifndef __TRANSPORT_H
#define __TRANSPORT_H

#include <image.h>
#include <string>

class BDataIO;
class PrinterData;

#if (!__MWERKS__ || defined(MSIPL_USING_NAMESPACE))
using namespace std;
#else 
#define std
#endif

extern "C" {
	typedef BDataIO *(*PFN_init_transport)(BMessage *);
	typedef void (*PFN_exit_transport)(void);
}

class TransportException {
private:
	string __str;
public:
	TransportException(const string &what_arg) : __str(what_arg) {}
	const char *what() const { return __str.c_str(); }
};

class Transport {
public:
	Transport(const PrinterData *printer_data);
	~Transport();
	void write(const void *buffer, size_t size) throw(TransportException);
	bool check_abort() const;
	const string &last_error() const;

protected:
	void set_last_error(const char *e);
	Transport(const Transport &);
	Transport &operator = (const Transport &);

private:
	image_id           __image;
	PFN_init_transport __init_transport;
	PFN_exit_transport __exit_transport;
	BDataIO            *__data_stream;
	bool               __abort;
	string             __last_error_string;
};

#endif	// __TRANSPORT_H
