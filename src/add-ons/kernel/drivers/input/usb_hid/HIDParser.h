/*
 * Copyright 2009, Michael Lotz, mmlr@mlotz.ch.
 * Distributed under the terms of the MIT License.
 */
#ifndef HID_PARSER_H
#define HID_PARSER_H

#include "HIDDataTypes.h"

class HIDReport;
class HIDCollection;

class HIDParser {
public:
								HIDParser();
								~HIDParser();

		status_t				ParseReportDescriptor(uint8 *reportDescriptor,
									size_t descriptorLength);

		bool					UsesReportIDs() { return fUsesReportIDs; };

		HIDReport *				FindReport(uint8 type, uint8 id);
		uint8					CountReports(uint8 type);
		HIDReport *				ReportAt(uint8 type, uint8 index);

		HIDCollection *			RootCollection() { return fRootCollection; };

		status_t				SetReport(uint8 *report, size_t length);

private:
		HIDReport *				_FindOrCreateReport(uint8 type, uint8 id);
		float					_CalculateResolution(global_item_state *state);
		void					_Reset();

		bool					fUsesReportIDs;
		uint8					fReportCount;
		HIDReport **			fReports;
		HIDCollection *			fRootCollection;
};

#endif // HID_PARSER_H
