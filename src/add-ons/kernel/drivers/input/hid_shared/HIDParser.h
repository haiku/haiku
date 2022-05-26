/*
 * Copyright 2009, Michael Lotz, mmlr@mlotz.ch.
 * Distributed under the terms of the MIT License.
 */
#ifndef HID_PARSER_H
#define HID_PARSER_H

#include "HIDDataTypes.h"
#include "util/Vector.h"


class HIDCollection;
class HIDDevice;
class HIDReport;

class HIDParser {
public:
								HIDParser(HIDDevice *device);
								~HIDParser();

		HIDDevice *				Device() { return fDevice; };

		status_t				ParseReportDescriptor(
									const uint8 *reportDescriptor,
									size_t descriptorLength);

		bool					UsesReportIDs() { return fUsesReportIDs; };

		HIDReport *				FindReport(uint8 type, uint8 id);
		uint8					CountReports(uint8 type);
		HIDReport *				ReportAt(uint8 type, uint8 index);
		size_t					MaxReportSize();

		HIDCollection *			RootCollection() { return fRootCollection; };

		void					SetReport(status_t status, uint8 *report,
									size_t length);

		void					PrintToStream();

private:
		HIDReport *				_FindOrCreateReport(uint8 type, uint8 id);
		float					_CalculateResolution(global_item_state *state);
		void					_Reset();

		HIDDevice *				fDevice;
		bool					fUsesReportIDs;
		Vector<HIDReport *>		fReports;
		HIDCollection *			fRootCollection;
};

#endif // HID_PARSER_H
