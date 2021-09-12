/*
 * Copyright 2009, Michael Lotz, mmlr@mlotz.ch.
 * Distributed under the terms of the MIT License.
 */

#ifndef USERLAND_HID
#include "Driver.h"
#else
#include "UserlandHID.h"
#endif

#include "HIDCollection.h"
#include "HIDDevice.h"
#include "HIDReport.h"
#include "HIDReportItem.h"

#include <new>
#include <stdlib.h>
#include <string.h>


HIDReport::HIDReport(HIDParser *parser, uint8 type, uint8 id)
	:	fParser(parser),
		fType(type),
		fReportID(id),
		fReportSize(0),
		fReportStatus(B_NO_INIT),
		fCurrentReport(NULL),
		fBusyCount(0)
{
#ifndef USERLAND_HID
	fConditionVariable.Init(this, "hid report");
#endif
}


HIDReport::~HIDReport()
{
}


void
HIDReport::AddMainItem(global_item_state &globalState,
	local_item_state &localState, main_item_data &mainData,
	HIDCollection *collection)
{
	TRACE("adding main item to report of type 0x%02x with id 0x%02x\n",
		fType, fReportID);
	TRACE("\tmain data:\n");
	TRACE("\t\t%s\n", mainData.data_constant ? "constant" : "data");
	TRACE("\t\t%s\n", mainData.array_variable ? "variable" : "array");
	TRACE("\t\t%s\n", mainData.relative ? "relative" : "absolute");
	TRACE("\t\t%swrap\n", mainData.wrap ? "" : "no-");
	TRACE("\t\t%slinear\n", mainData.non_linear ? "non-" : "");
	TRACE("\t\t%spreferred state\n", mainData.no_preferred ? "no " : "");
	TRACE("\t\t%s null\n", mainData.null_state ? "has" : "no");
	TRACE("\t\t%svolatile\n", mainData.is_volatile ? "" : "non-");
	TRACE("\t\t%s\n", mainData.bits_bytes ? "bit array" : "buffered bytes");

	uint32 logicalMinimum = globalState.logical_minimum;
	uint32 logicalMaximum = globalState.logical_maximum;
	if (logicalMinimum > logicalMaximum)
		_SignExtend(logicalMinimum, logicalMaximum);

	uint32 physicalMinimum = globalState.physical_minimum;
	uint32 physicalMaximum = globalState.physical_maximum;
	if (physicalMinimum > physicalMaximum)
		_SignExtend(physicalMinimum, physicalMaximum);

	TRACE("\tglobal state:\n");
	TRACE("\t\tusage_page: 0x%x\n", globalState.usage_page);
	TRACE("\t\tlogical_minimum: %" B_PRId32 "\n", logicalMinimum);
	TRACE("\t\tlogical_maximum: %" B_PRId32 "\n", logicalMaximum);
	TRACE("\t\tphysical_minimum: %" B_PRId32 "\n", physicalMinimum);
	TRACE("\t\tphysical_maximum: %" B_PRId32 "\n", physicalMaximum);
	TRACE("\t\tunit_exponent: %d\n", globalState.unit_exponent);
	TRACE("\t\tunit: %d\n", globalState.unit);
	TRACE("\t\treport_size: %" B_PRIu32 "\n", globalState.report_size);
	TRACE("\t\treport_count: %" B_PRIu32 "\n", globalState.report_count);
	TRACE("\t\treport_id: %u\n", globalState.report_id);

	TRACE("\tlocal state:\n");
	TRACE("\t\tusage stack (%" B_PRIu32 ")\n", localState.usage_stack_used);
	for (uint32 i = 0; i < localState.usage_stack_used; i++) {
		TRACE("\t\t\t0x%08" B_PRIx32 "\n",
			localState.usage_stack[i].u.extended);
	}

	TRACE("\t\tusage_minimum: 0x%08" B_PRIx32 "\n",
		localState.usage_minimum.u.extended);
	TRACE("\t\tusage_maximum: 0x%08" B_PRIu32 "\n",
		localState.usage_maximum.u.extended);
	TRACE("\t\tdesignator_index: %" B_PRIu32 "\n",
		localState.designator_index);
	TRACE("\t\tdesignator_minimum: %" B_PRIu32 "\n",
		localState.designator_minimum);
	TRACE("\t\tdesignator_maximum: %" B_PRIu32 "\n",
		localState.designator_maximum);
	TRACE("\t\tstring_index: %u\n", localState.string_index);
	TRACE("\t\tstring_minimum: %u\n", localState.string_minimum);
	TRACE("\t\tstring_maximum: %u\n", localState.string_maximum);

	for (uint32 n = 0; n <localState.usage_stack_used; n++) {
		if (fUsages.PushBack(localState.usage_stack[n].u.extended) != B_OK) {
			TRACE_ALWAYS("no memory allocating usages\n");
			break;
		}
	}

	usage_value page;

	if (localState.usage_stack_used > 0) {
		page = localState.usage_stack[0];
		page.u.s.usage_id = 0;
	}

	uint32 usage = page.u.extended;

	for (uint32 i = 0; i < globalState.report_count; i++) {
		if (mainData.array_variable == 1) {
			
			if (i < localState.usage_stack_used)
				usage = localState.usage_stack[i].u.extended;
		}

		HIDReportItem *item = new(std::nothrow) HIDReportItem(this,
			fReportSize, globalState.report_size, mainData.data_constant == 0,
			mainData.array_variable == 0, mainData.relative != 0,
			logicalMinimum, logicalMaximum, usage);
		if (item == NULL)
			TRACE_ALWAYS("no memory when creating report item\n");

		if (collection != NULL)
			collection->AddItem(item);
		else
			TRACE_ALWAYS("main item not part of a collection\n");

		if (fItems.PushBack(item) == B_NO_MEMORY) {
			TRACE_ALWAYS("no memory when growing report item list\n");
		}

		fReportSize += globalState.report_size;
	}

}


void
HIDReport::SetReport(status_t status, uint8 *report, size_t length)
{
	fReportStatus = status;
	fCurrentReport = report;
	if (status == B_OK && length * 8 < fReportSize) {
		TRACE_ALWAYS("report of %lu bits too small, expected %" B_PRIu32
			" bits\n", length * 8, fReportSize);
		fReportStatus = B_ERROR;
	}

#ifndef USERLAND_HID
	fConditionVariable.NotifyAll();
#endif
}


#ifndef USERLAND_HID
status_t
HIDReport::SendReport()
{
	size_t reportSize = ReportSize();
	uint8 *report = (uint8 *)malloc(reportSize);
	if (report == NULL)
		return B_NO_MEMORY;

	fCurrentReport = report;
	memset(fCurrentReport, 0, reportSize);

	for (int32 i = 0; i < fItems.Count(); i++) {
		HIDReportItem *item = fItems[i];
		if (item == NULL)
			continue;

		item->Insert();
	}

	status_t result = fParser->Device()->SendReport(this);

	fCurrentReport = NULL;
	free(report);
	return result;
}
#endif // !USERLAND_HID


HIDReportItem *
HIDReport::ItemAt(uint32 index)
{
	if (index >= fItems.Count())
		return NULL;
	return fItems[index];
}


HIDReportItem *
HIDReport::FindItem(uint16 usagePage, uint16 usageID)
{
	for (int32 i = 0; i < fItems.Count(); i++) {
		if (fItems[i]->UsagePage() == usagePage
			&& fItems[i]->UsageID() == usageID)
			return fItems[i];
	}

	return NULL;
}


uint32 *
HIDReport::Usages()
{
	if (fUsages.Count() > 0)
		return &fUsages[0];
	
	return NULL;
}


#ifndef USERLAND_HID
status_t
HIDReport::WaitForReport(bigtime_t timeout)
{
	while (atomic_get(&fBusyCount) != 0)
		snooze(1000);

	ConditionVariableEntry conditionVariableEntry;
	fConditionVariable.Add(&conditionVariableEntry);
	status_t result = fParser->Device()->MaybeScheduleTransfer(this);
	if (result != B_OK) {
		TRACE_ALWAYS("scheduling transfer failed\n");
		conditionVariableEntry.Wait(B_RELATIVE_TIMEOUT, 0);
		return result;
	}

	result = conditionVariableEntry.Wait(B_RELATIVE_TIMEOUT, timeout);
	TRACE("waiting for report returned with result: %s\n", strerror(result));
	if (result != B_OK)
		return result;

	if (fReportStatus != B_OK)
		return fReportStatus;

	atomic_add(&fBusyCount, 1);
	return B_OK;
}


void
HIDReport::DoneProcessing()
{
	atomic_add(&fBusyCount, -1);
}
#endif // !USERLAND_HID


void
HIDReport::PrintToStream()
{
	TRACE_ALWAYS("HIDReport %p\n", this);

	const char *typeName = "unknown";
	switch (fType) {
		case HID_REPORT_TYPE_INPUT:
			typeName = "input";
			break;
		case HID_REPORT_TYPE_OUTPUT:
			typeName = "output";
			break;
		case HID_REPORT_TYPE_FEATURE:
			typeName = "feature";
			break;
	}

	TRACE_ALWAYS("\ttype: %u %s\n", fType, typeName);
	TRACE_ALWAYS("\treport id: %u\n", fReportID);
	TRACE_ALWAYS("\treport size: %" B_PRIu32 " bits = %" B_PRIu32 " bytes\n",
		fReportSize, (fReportSize + 7) / 8);

	TRACE_ALWAYS("\titem count: %" B_PRIu32 "\n", fItems.Count());
	for (int32 i = 0; i < fItems.Count(); i++) {
		HIDReportItem *item = fItems[i];
		if (item != NULL)
			item->PrintToStream(1);
	}
}


void
HIDReport::_SignExtend(uint32 &minimum, uint32 &maximum)
{
	uint32 mask = 0x80000000;
	for (uint8 i = 0; i < 4; i++) {
		if (minimum & mask) {
			minimum |= mask;
			if (maximum & mask)
				maximum |= mask;
			return;
		}

		mask >>= 8;
		mask |= 0xff000000;
	}
}
