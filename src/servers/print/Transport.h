/*
 * Copyright 2008, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Ithamar R. Adema
 */
#ifndef TRANSPORT_H
#define TRANSPORT_H

class Transport;

#include <FindDirectory.h>
#include <Handler.h>
#include <String.h>
#include <Path.h>

#include <ObjectList.h>

class Transport : public BHandler
{
	typedef BHandler Inherited;
public:
	Transport(const BPath& path);
	~Transport();

	BString Name() const { return fPath.Leaf(); }

	status_t ListAvailablePorts(BMessage* msg);

	static status_t Scan(directory_which which);

	static Transport* Find(const BString& name);
	static void Remove(Transport* transport);
	static Transport* At(int32 idx);
	static int32 CountTransports();

	void MessageReceived(BMessage* msg);

		// Scripting support, see Printer.Scripting.cpp
	status_t GetSupportedSuites(BMessage* msg);
	void HandleScriptingCommand(BMessage* msg);
	BHandler* ResolveSpecifier(BMessage* msg, int32 index, BMessage* spec,
								int32 form, const char* prop);
	
private:
	BPath fPath;
	long fImageID;
	int fFeatures;

	static BObjectList<Transport> sTransports;
};

#endif
