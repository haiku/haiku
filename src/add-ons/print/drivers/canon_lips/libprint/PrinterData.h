/*
 * PrinterData.h
 * Copyright 1999-2000 Y.Takagi All Rights Reserved.
 */

#ifndef __PRINTERDATA_H
#define __PRINTERDATA_H

#include <string>
#include <SerialPort.h>

#if (!__MWERKS__ || defined(MSIPL_USING_NAMESPACE))
using namespace std;
#else 
#define std
#endif

class BNode;

class PrinterData {
public:
	PrinterData(BNode *node = NULL);
	~PrinterData();
/*
	PrinterData(const PrinterData &printer_data);
	PrinterData &operator = (const PrinterData &printer_data);
*/
	void load(BNode *node);
//	void save(BNode *node = NULL);

	const string &getDriverName() const;
	const string &getPrinterName() const;
	const string &getComments() const;
	const string &getTransport() const;

//	void  setDriverName(const char *s) { __driver_name = driver_name; }
	void  setPrinterName(const char *printer_name);
	void  setComments(const char *comments);

	bool getPath(char *path) const;

protected:
	PrinterData(const PrinterData &printer_data);
	PrinterData &operator = (const PrinterData &printer_data);

private:
	string __driver_name;
	string __printer_name;
	string __comments;
	string __transport;
	BNode  *__node;
};

inline const string &PrinterData::getDriverName() const
{
	return __driver_name;
}

inline const string &PrinterData::getPrinterName() const
{
	return __printer_name;
}

inline const string &PrinterData::getComments() const
{
	return __comments;
}

inline const string &PrinterData::getTransport() const
{
	return __transport;
}

inline void PrinterData::setPrinterName(const char *printer_name)
{
	__printer_name = printer_name;
}

inline void PrinterData::setComments(const char *comments)
{
	__comments = comments;
}

#endif	// __PRINTERDATA_H
